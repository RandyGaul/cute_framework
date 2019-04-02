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
#define CUTE_CLIENT_MAX_RECONNECT_TRIES 3

namespace cute
{

enum client_state_internal_t : int
{
	CLIENT_STATE_INTERNAL_CONNECT_TOKEN_EXPIRED         = -6,
	CLIENT_STATE_INTERNAL_INVALID_CONNECT_TOKEN         = -5,
	CLIENT_STATE_INTERNAL_CONNECTION_TIMED_OUT          = -4,
	CLIENT_STATE_INTERNAL_CONNECTION_RESPONSE_TIMED_OUT = -3,
	CLIENT_STATE_INTERNAL_CONNECTION_REQUEST_TIMED_OUT  = -2,
	CLIENT_STATE_INTERNAL_CONNECTION_DENIED             = -1,
	CLIENT_STATE_INTERNAL_DISCONNECTED                  = 0,
	CLIENT_STATE_INTERNAL_SENDING_CONNECTION_REQUEST    = 1,
	CLIENT_STATE_INTERNAL_SENDING_CONNECTION_RESPONSE   = 2,
	CLIENT_STATE_INTERNAL_CONNECTED                     = 3,
};

struct client_t
{
	client_state_t state;
	client_state_internal_t state_internal;
	int reconnect_tries;
	int loopback;
	float last_packet_recieved_time;
	float last_packet_sent_time;
	connect_token_t connect_token;
	int server_endpoint_index;
	endpoint_t server_endpoint;
	socket_t socket;
	crypto_key_t key;
	uint64_t sequence;
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
	client->state_internal = CLIENT_STATE_INTERNAL_DISCONNECTED;
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

int client_connect(client_t* client, uint8_t* connect_token)
{
	CUTE_CHECK(connect_token_open(&client->connect_token, connect_token));

	client->state = CLIENT_STATE_CONNECTING;
	client->state_internal = CLIENT_STATE_INTERNAL_SENDING_CONNECTION_REQUEST;
	client->reconnect_tries = 0;
	client->loopback = 0;
	client->last_packet_recieved_time = 0;
	client->last_packet_sent_time = CUTE_KEEPALIVE_RATE;
	CUTE_CHECK(socket_init(&client->socket, client->server_endpoint.type, client->server_endpoint.port, CUTE_CLIENT_SEND_BUFFER_SIZE, CUTE_CLIENT_RECEIVE_BUFFER_SIZE));
	client->sequence = 0;
	nonce_buffer_init(&client->nonce_buffer);
	CUTE_CHECK(packet_queue_init(&client->packets, CUTE_CLIENT_RECEIVE_BUFFER_SIZE, client->mem_ctx));
	return 0;

cute_error:
	return -1;
}

void client_disconnect(client_t* client)
{
	if (client->state == CLIENT_STATE_CONNECTED) {
		s_client_send_packet_no_payload(client, PACKET_TYPE_DISCONNECT);
	}

	socket_cleanup(&client->socket);
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

		if (packet_validate_size(bytes_read)) {
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
		uint8_t* packet = packet_open(io, &client->key, &client->nonce_buffer, buffer, bytes_read, sequence, client->sequence_offset, &type, &packet_size);

		if (!packet) {
			// A forged or otherwise corrupt/unknown type of packet has appeared.
			continue;
		}

		// Packet has been verified to have come from the server -- safe to process.
		int valid_packet = 0;

		switch (type)
		{
		case PACKET_TYPE_CONNECTION_ACCEPTED:
			if (client->state == CLIENT_STATE_CONNECTING) {
				// This first sequence from the server is used as the initialization vector for all
				// subsequent nonces.
				client->sequence = 1;
				CUTE_SERIALIZE_CHECK(serialize_uint64_full(io, &client->sequence_offset));
				client->state = CLIENT_STATE_CONNECTED;
				valid_packet = 1;
			}
			break;

		case PACKET_TYPE_CONNECTION_DENIED:
			client->state = CLIENT_STATE_DISCONNECTED;
			valid_packet = 1;
			return;

		case PACKET_TYPE_KEEPALIVE:
			valid_packet = 1;
			break;

		case PACKET_TYPE_DISCONNECT:
			client->state = CLIENT_STATE_DISCONNECTED;
			valid_packet = 1;
			return;

		case PACKET_TYPE_USERDATA:
			if (client->state == CLIENT_STATE_CONNECTED) {
				CUTE_CHECK(packet_queue_push(&client->packets, packet, packet_size, sequence));
				valid_packet = 1;
			}
			break;
		}

		if (valid_packet) {
			client->last_packet_recieved_time = 0;
		}
	}

cute_error:
	// Skip packet upon any errors.
	return;
}

static void s_client_send_packet(client_t* client, void* packet, packet_type_t type)
{
	uint8_t* buffer = client->buffer;
	const crypto_key_t* key = type == PACKET_TYPE_CONNECTION_REQUEST ? NULL : &client->key;
	int size = packet_write(packet, type, buffer, client->connect_token.game_id, client->sequence + client->connect_token.sequence_offset, key);
	if (size <= 0) return;
	CUTE_ASSERT(size <= CUTE_PACKET_SIZE_MAX);
	int bytes_sent = socket_send(&client->socket, client->server_endpoint, buffer, size);
	(void)bytes_sent;
}

static void s_client_send_packets(client_t* client)
{
	switch (client->state)
	{
	case CLIENT_STATE_CONNECTING:
	{
		if (client->last_packet_sent_time >= CUTE_KEEPALIVE_RATE) {
			client->last_packet_sent_time = 0;
			if (client->reconnect_tries == 3) {
				client->state = CLIENT_STATE_DISCONNECTED;
				break;
			}
			client->reconnect_tries++;

			packet_encrypted_connect_token_t packet;
			packet.expire_timestamp = client->connect_token.expire_timestamp;
			CUTE_ASSERT(sizeof(packet.nonce) == sizeof(client->connect_token.nonce));
			CUTE_MEMCPY(packet.nonce, client->connect_token.nonce, sizeof(packet.nonce));
			CUTE_ASSERT(sizeof(packet.secret_data) == sizeof(client->connect_token.secret_data));
			CUTE_MEMCPY(packet.secret_data, client->connect_token.secret_data, sizeof(packet.secret_data));

			s_client_send_packet(client, &packet, PACKET_TYPE_CONNECTION_REQUEST);
		}
	}	break;

		// Keep-alive packet.
	case CLIENT_STATE_CONNECTED:
	{
		if (client->last_packet_sent_time >= CUTE_KEEPALIVE_RATE) {
			client->last_packet_sent_time = 0;
			CUTE_CHECK(s_client_send_packet_no_payload(client, PACKET_TYPE_KEEPALIVE));
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

	if (client->last_packet_recieved_time >= CUTE_KEEPALIVE_RATE * 3) {
		client->state = CLIENT_STATE_DISCONNECTED;
		s_client_send_packet_no_payload(client, PACKET_TYPE_DISCONNECT);
	}

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
