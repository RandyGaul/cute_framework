/*
	Cute Framework
	Copyright (C) 2019 Randy Gaul https://randygaul.net

	This software is provided 'as-is', without any express or implied
	warranty.  In no event will the authors be held liable for any damages
	arising from the use of this software.

	Permission is granted to anyone to use this software for any purpose,
	including commercial applications, and to alter it and redistribute it
	freely, subject to the following restrictions:

	1. The origin of this software must not be misrepresented; you must not
	   claim that you wrote the original software. If you use this software
	   in a product, an acknowledgment in the product documentation would be
	   appreciated but is not required.
	2. Altered source versions must be plainly marked as such, and must not be
	   misrepresented as being the original software.
	3. This notice may not be removed or altered from any source distribution.
*/

#include <cute_net.h>
#include <cute_client.h>
#include <cute_error.h>
#include <cute_c_runtime.h>
#include <cute_alloc.h>
#include <cute_crypto.h>
#include <cute_circular_buffer.h>

#include <internal/cute_defines_internal.h>
#include <internal/cute_net_internal.h>
#include <internal/cute_app_internal.h>

#include <cute/cute_serialize.h>

#define CUTE_CLIENT_SEND_BUFFER_SIZE (2 * CUTE_MB)
#define CUTE_CLIENT_RECEIVE_BUFFER_SIZE (2 * CUTE_MB)

namespace cute
{

struct client_t
{
	client_state_t state;
	int loopback;
	float last_packet_recieved_time;
	float last_packet_sent_time;
	uint64_t sequence_offset;
	uint64_t sequence;
	socket_t socket;
	endpoint_t server_endpoint;
	crypto_key_t server_public_key;
	crypto_key_t session_key;
	serialize_t* io;
	nonce_buffer_t nonce_buffer;
	packet_queue_t packets;
	uint8_t buffer[CUTE_PACKET_SIZE_MAX];
	void* mem_ctx;
};

// -------------------------------------------------------------------------------------------------

client_t* client_alloc(void* user_allocator_context)
{
	client_t* client = (client_t*)CUTE_ALLOC(sizeof(client_t), app->mem_ctx);
	CUTE_CHECK_POINTER(client);
	CUTE_MEMSET(client, 0, sizeof(client_t));
	client->state = CLIENT_STATE_DISCONNECTED;
	client->mem_ctx = user_allocator_context;
	return client;

cute_error:
	if (client) pack_queue_clean_up(&client->packets);
	CUTE_FREE(client, app->mem_ctx);
	return NULL;
}

void client_destroy(client_t* client)
{
	socket_cleanup(&client->socket);
	pack_queue_clean_up(&client->packets);
	CUTE_FREE(client, client->app->mem_ctx);
}

int client_connect(client_t* client, uint16_t port, const char* server_address_and_port, const crypto_key_t* server_public_key, int loopback)
{
	endpoint_t server_endpoint;
	CUTE_CHECK(endpoint_init(&server_endpoint, server_address_and_port));

	client->state = CLIENT_STATE_CONNECTING;
	client->loopback = loopback;
	client->last_packet_recieved_time = 0;
	client->last_packet_sent_time = 0;
	client->sequence_offset = 0;
	client->sequence = 0;
	CUTE_CHECK(socket_init(&client->socket, server_endpoint.type, port, CUTE_CLIENT_SEND_BUFFER_SIZE, CUTE_CLIENT_RECEIVE_BUFFER_SIZE));
	client->server_endpoint = server_endpoint;
	client->session_key = crypto_generate_symmetric_key();
	client->io = serialize_buffer_create(SERIALIZE_READ, NULL, 0, NULL);
	nonce_buffer_init(&client->nonce_buffer);
	CUTE_CHECK(packet_queue_init(&client->packets, CUTE_CLIENT_RECEIVE_BUFFER_SIZE, client->mem_ctx));
	client->server_public_key = *server_public_key;
	return 0;

cute_error:
	return -1;
}

void client_disconnect(client_t* client)
{
	if (client->state == CLIENT_STATE_CONNECTED) {
		// TODO : Notify server with disconnect packet.
	}

	socket_cleanup(&client->socket);
	serialize_destroy(client->io);
	client->io = NULL;
	pack_queue_clean_up(&client->packets);
}

client_state_t client_state_get(const client_t* client)
{
	return client->state;
}

float client_get_last_packet_recieved_time(const client_t* client)
{
	return client->last_packet_recieved_time;
}

int client_is_loopback(const client_t* client)
{
	return client->loopback;
}

static void s_client_receive_packets(client_t* client)
{
	uint8_t* buffer = client->buffer;

	while (1)
	{
		endpoint_t from;
		int bytes_read = socket_receive(&client->socket, &from, buffer, CUTE_PACKET_SIZE_MAX);
		if (bytes_read <= 0) {
			// No more packets to receive for now.
			break;
		}

		if (validate_packet_size(bytes_read)) {
			continue;
		}

		if (!endpoint_equals(from, client->server_endpoint)) {
			// Only accept communications if the address match's the server's address.
			// This is mostly just a "sanity" check.
			break;
		}

		serialize_t* io = client->io;
		serialize_reset_buffer(io, SERIALIZE_READ, buffer, bytes_read);

		uint64_t sequence;
		CUTE_SERIALIZE_CHECK(serialize_uint64_full(io, &sequence));
		packet_type_t type;
		int packet_size;
		uint8_t* packet = open_packet(io, &client->session_key, &client->nonce_buffer, buffer, bytes_read, client->sequence, client->sequence_offset, &type, &packet_size);

		if (!packet) {
			// A forged or otherwise corrupt/unknown type of packet has appeared.
			continue;
		}

		// Packet has been verified to have come from the server -- safe to process.
		client->last_packet_recieved_time = 0;

		switch (type)
		{
		case PACKET_TYPE_CONNECTION_ACCEPTED:
			if (client->state == CLIENT_STATE_CONNECTING) {
				// The first sequence from the server is used as the initialization vector for all
				// subsequent nonces.
				client->sequence = 1;
				CUTE_SERIALIZE_CHECK(serialize_uint64_full(io, &client->sequence_offset));
				client->state = CLIENT_STATE_CONNECTED;
			}
			break;

		case PACKET_TYPE_CONNECTION_DENIED:
			client->state = CLIENT_STATE_DISCONNECTED;
			return;

		case PACKET_TYPE_KEEP_ALIVE:
			// No-op.
			break;

		case PACKET_TYPE_DISCONNECT:
			client->state = CLIENT_STATE_DISCONNECTED;
			return;

		case PACKET_TYPE_USERDATA:
			if (client->state == CLIENT_STATE_CONNECTED) {
				CUTE_CHECK(packet_queue_push(&client->packets, packet, packet_size, sequence));
			}
			break;
		}
	}

cute_error:
	// Skip packet upon any errors.
	return;
}

static int s_write_packet_header(client_t* client, packet_type_t packet_type)
{
	serialize_t* io = client->io;
	uint8_t* buffer = client->buffer;

	serialize_reset_buffer(io, SERIALIZE_WRITE, buffer, CUTE_PACKET_SIZE_MAX);
	uint64_t sequence = client->sequence;
	CUTE_CHECK(serialize_uint64_full(io, &sequence));
	uint64_t packet_typeu64 = packet_type;
	CUTE_SERIALIZE_CHECK(serialize_uint64(io, &packet_typeu64, 0, PACKET_TYPE_MAX));
	CUTE_SERIALIZE_CHECK(serialize_flush(io));
	int packet_size = serialize_serialized_bytes(io);
	return packet_size;

cute_error:
	return -1;
}

static int s_encrypt_and_send(client_t* client, int packet_size)
{
	uint8_t* buffer = client->buffer;
	uint64_t sequence = client->sequence++;
	CUTE_CHECK(crypto_encrypt(&client->session_key, buffer + sizeof(uint64_t), packet_size - sizeof(uint64_t), CUTE_PACKET_SIZE_MAX, sequence + client->sequence_offset));
	packet_size += CUTE_CRYPTO_SYMMETRIC_BYTES;

	int bytes_sent = socket_send(&client->socket, client->server_endpoint, buffer, packet_size);
	(void)bytes_sent;
	return 0;

cute_error:
	return -1;
}

static void s_client_send_packets(client_t* client)
{
	uint8_t* buffer = client->buffer;

	switch (client->state)
	{
	case CLIENT_STATE_CONNECTING:
	{
		if (client->last_packet_sent_time >= CUTE_KEEPALIVE_RATE) {
			client->last_packet_sent_time = 0;
		}

		if (client->last_packet_sent_time) {
			break;
		}

		serialize_t* io = client->io;
		serialize_reset_buffer(io, SERIALIZE_WRITE, buffer, CUTE_PACKET_SIZE_MAX);

		// Write version string.
		const char* version_string = CUTE_PROTOCOL_VERSION;
		CUTE_CHECK(serialize_bytes(io, (unsigned char*)version_string, CUTE_PROTOCOL_VERSION_STRING_LEN));

		// Write symmetric key.
		CUTE_SERIALIZE_CHECK(serialize_bytes(io, client->session_key.key, sizeof(client->session_key)));

		// Pad zero-bytes to end.
		CUTE_SERIALIZE_CHECK(serialize_flush(io));
		int bytes_so_far = serialize_serialized_bytes(io);
		int pad_bytes = CUTE_PACKET_SIZE_MAX - bytes_so_far - CUTE_CRYPTO_ASYMMETRIC_BYTES;
		CUTE_ASSERT(pad_bytes >= 0);
		uint32_t bits = 0;
		for (int i = 0; i < pad_bytes; ++i) CUTE_CHECK(serialize_bits(io, &bits, 8));
		CUTE_ASSERT(serialize_serialized_bytes(io) == CUTE_PACKET_SIZE_MAX - CUTE_CRYPTO_ASYMMETRIC_BYTES);

		// Encrypt with server's public key.
		CUTE_CHECK(crypto_encrypt_asymmetric(&client->server_public_key, buffer, CUTE_PACKET_SIZE_MAX - CUTE_CRYPTO_ASYMMETRIC_BYTES, CUTE_PACKET_SIZE_MAX));

		// Send the packet off.
		int bytes_sent = socket_send(&client->socket, client->server_endpoint, buffer, CUTE_PACKET_SIZE_MAX);
		(void)bytes_sent;
	}	break;

		// Keep-alive packet.
	case CLIENT_STATE_CONNECTED:
	{
		if (client->last_packet_sent_time >= CUTE_KEEPALIVE_RATE) {
			client->last_packet_sent_time = 0;
			int packet_size = s_write_packet_header(client, PACKET_TYPE_KEEP_ALIVE);
			if (packet_size <= 0) goto cute_error;
			CUTE_CHECK(s_encrypt_and_send(client, packet_size));
		}

	}	break;

		// No-op.
	case CLIENT_STATE_DISCONNECTED:
		break;
	}

	return;

cute_error:
	// Immediately set state to disconnected in the event of any errors.
	client->state = CLIENT_STATE_DISCONNECTED;
	return;
}

void client_update(client_t* client, float dt)
{
	if (client->state == CLIENT_STATE_DISCONNECTED) {
		return;
	}

	s_client_receive_packets(client);
	s_client_send_packets(client);

	client->last_packet_recieved_time += dt;
	client->last_packet_sent_time += dt;
}

int client_get_packet(client_t* client, void* data, int* size)
{
	return -1;
}

int client_send_data(client_t* client, void* data, int size)
{
	return -1;
}

int client_send_data_unreliable(client_t* client, void* data, int size)
{
	return -1;
}

}
