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
	float connect_time;
	float keep_alive_time;
	uint64_t sequence_offset;
	uint64_t sequence;
	socket_t socket;
	crypto_key_t server_public_key;
	crypto_key_t session_key;
	serialize_t* io;
	nonce_buffer_t nonce_buffer;
	packet_queue_t packets;
	uint8_t buffer[CUTE_PACKET_SIZE_MAX];
	void* mem_ctx;
};

// -------------------------------------------------------------------------------------------------

client_t* client_make(void* user_allocator_context)
{
	client_t* client = (client_t*)CUTE_ALLOC(sizeof(client_t), app->mem_ctx);
	CUTE_CHECK_POINTER(client);
	CUTE_MEMSET(client, 0, sizeof(client_t));
	client->state = CLIENT_STATE_DISCONNECTED;
	client->mem_ctx = user_allocator_context;
	return client;

cute_error:
	if (client) packet_queue_free(&client->packets);
	CUTE_FREE(client, app->mem_ctx);
	return NULL;
}

void client_destroy(client_t* client)
{
	// TODO: Disconnect.
	socket_cleanup(&client->socket);
	packet_queue_free(&client->packets);
	CUTE_FREE(client, client->app->mem_ctx);
}

int client_connect(client_t* client, endpoint_t endpoint, const crypto_key_t* server_public_key)
{
	client->state = CLIENT_STATE_CONNECTING;
	client->connect_time = 0;
	client->keep_alive_time = 0;
	client->sequence_offset = 0;
	client->sequence = 0;
	CUTE_CHECK(socket_init(&client->socket, endpoint, CUTE_CLIENT_SEND_BUFFER_SIZE, CUTE_CLIENT_RECEIVE_BUFFER_SIZE));
	client->session_key = crypto_generate_symmetric_key();
	client->io = serialize_buffer_create(SERIALIZE_READ, NULL, 0, NULL);
	CUTE_CHECK(packet_queue_init(&client->packets, CUTE_CLIENT_RECEIVE_BUFFER_SIZE, client->mem_ctx));
	client->server_public_key = *server_public_key;
	return 0;

cute_error:
	return -1;
}

void client_disconnect(client_t* client)
{
}

enum packet_type_t : int
{
	PACKET_TYPE_HELLO,
	PACKET_TYPE_CONNECTION_ACCEPTED,
	PACKET_TYPE_CONNECTION_DENIED,
	PACKET_TYPE_KEEP_ALIVE,
	PACKET_TYPE_DISCONNECT,
	PACKET_TYPE_USERDATA,

	PACKET_TYPE_MAX,
};

client_state_t client_state_get(client_t* client)
{
	return client->state;
}

struct replay_protection_buffer_t;
int replay_protection(uint64_t sequence, replay_protection_buffer_t* buffer)
{
	return -1;
}

static uint8_t* s_client_open_packet(client_t* client, uint8_t* packet, int size, uint64_t sequence)
{
	crypto_nonce_t nonce;
	memset(&nonce, 0, sizeof(nonce));
	CUTE_ASSERT(sizeof(nonce) >= sizeof(uint64_t));
	*(uint64_t*)((uint8_t*)&nonce + sizeof(nonce) - sizeof(uint64_t)) = sequence;

	if (replay_protection(sequence, NULL) < 0) {
		// Duplicate packet detected.
		return NULL;
	}

	if (crypto_decrypt(&client->session_key, packet, size, &nonce) < 0) {
		// Forged packet!
		return NULL;
	}

	return packet;
}

/*
	Packet Format - WIP
	[sequence]         8 bytes
	[packet type]      req_bits(0, PACKET_TYPE_MAX) bits
	[payload data]     variable
	[optional padding] padding up to CUTE_PACKET_SIZE_MAX
*/

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

		if (!endpoint_equals(from, client->socket.endpoint)) {
			// Only accept communications if the address match's the server's address.
			// This is mostly just a "sanity" check.
			break;
		}

		serialize_t* io = client->io;
		serialize_reset_buffer(io, SERIALIZE_READ, buffer, bytes_read);

		uint64_t sequence;
		CUTE_SERIALIZE_CHECK(serialize_uint64_full(io, &sequence));

		uint8_t* packet = s_client_open_packet(client, buffer, bytes_read, sequence);
		if (!packet) {
			// A forged or otherwise corrupt/unknown type of packet has appeared.
			continue;
		}

		uint64_t packet_typeu64;
		CUTE_SERIALIZE_CHECK(serialize_uint64(io, &packet_typeu64, 0, PACKET_TYPE_MAX));
		packet_type_t type = (packet_type_t)packet_typeu64;
		int skip_bytes = crypto_secretbox_MACBYTES + serialize_serialized_bytes(io);
		int packet_size = bytes_read - skip_bytes;
		packet += skip_bytes;
		if (packet_size <= 0) break;

		// Packet has been verified to have come from the server -- safe to process.
		switch (type)
		{
		case PACKET_TYPE_HELLO:
			// Clients should not ever receive hello, since it's intended to be sent to servers
			// to initiate a connection.
			break;

		case PACKET_TYPE_CONNECTION_ACCEPTED:
			if (client->state == CLIENT_STATE_CONNECTING) {
				// The first sequence from the server is used as the initialization vector for all
				// subsequent nonces.
				client->sequence_offset = sequence;
				client->state = CLIENT_STATE_CONNECTED;
			}
			break;

		case PACKET_TYPE_CONNECTION_DENIED:
			client->state = CLIENT_STATE_CONNECTION_DENIED;
			break;

		case PACKET_TYPE_KEEP_ALIVE:
			if (client->state == CLIENT_STATE_CONNECTED) {
			}
			break;

		case PACKET_TYPE_DISCONNECT:
			client->state = CLIENT_STATE_DISCONNECTED;
			break;

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

static void s_client_send_packets(client_t* client)
{
	uint8_t* buffer = client->buffer;

	switch (client->state)
	{
	case CLIENT_STATE_CONNECTING:
	{
		if (client->connect_time >= CUTE_CLIENT_RECONNECT_SECONDS) {
			client->connect_time = 0;
		}

		if (client->connect_time) {
			break;
		}

		serialize_t* io = client->io;
		serialize_reset_buffer(io, SERIALIZE_WRITE, buffer, CUTE_PACKET_SIZE_MAX);

		// Write version string.
		const char* version_string = CUTE_PROTOCOL_VERSION;
		int version_len = (int)CUTE_STRLEN(CUTE_PROTOCOL_VERSION);
		CUTE_CHECK(serialize_bytes(io, (char*)version_string, version_len + 1));

		// Write packet type.
		uint64_t packet_typeu64;
		CUTE_SERIALIZE_CHECK(serialize_uint64(io, &packet_typeu64, 0, PACKET_TYPE_MAX));
		packet_type_t type = (packet_type_t)packet_typeu64;

		// Write symmetric key.
		serialize_bytes(io, (char*)&client->session_key, sizeof(client->session_key));

		// Pad zero-bytes to end.
		CUTE_SERIALIZE_CHECK(serialize_flush(io));
		int bytes_so_far = serialize_serialized_bytes(io);
		int pad_bytes = CUTE_PACKET_SIZE_MAX - bytes_so_far;
		uint32_t bits = 0;
		for (int i = 0; i < pad_bytes; ++i) serialize_bits(io, &bits, 8);
		CUTE_ASSERT(serialize_serialized_bytes(io) == CUTE_PACKET_SIZE_MAX);

		// Send the packet off.
		socket_send(&client->socket, buffer, CUTE_PACKET_SIZE_MAX);
	}	break;

		// Keep-alive packet.
	case CLIENT_STATE_CONNECTED:
		break;

		// No-op.
	case CLIENT_STATE_CONNECTION_DENIED:
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
	s_client_receive_packets(client);
	s_client_send_packets(client);
	client->connect_time += dt;
	client->keep_alive_time += dt;
}

void client_get_packet(client_t* client, void* data, int* size)
{
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
