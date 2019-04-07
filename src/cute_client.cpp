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
#include <cute_protocol.h>

#include <internal/cute_defines_internal.h>
#include <internal/cute_net_internal.h>
#include <internal/cute_app_internal.h>

#include <cute/cute_serialize.h>

#include <time.h>

#define CUTE_CLIENT_SEND_BUFFER_SIZE (2 * CUTE_MB)
#define CUTE_CLIENT_RECEIVE_BUFFER_SIZE (2 * CUTE_MB)
#define CUTE_CLIENT_MAX_RECONNECT_TRIES 3

namespace cute
{

struct client_t
{
	client_state_t state;
	int loopback;
	float last_packet_recieved_time;
	float last_packet_sent_time;
	int client_number;
	int max_clients;
	int has_sent_disconnect_packets;
	connect_token_t connect_token;
	uint64_t challenge_sequence;
	uint8_t challenge_data[CUTE_CHALLENGE_DATA_SIZE];
	int server_endpoint_index;
	endpoint_t server_endpoint;
	socket_t socket;
	crypto_key_t key;
	uint64_t sequence;
	packet_allocator_t* packet_allocator;
	nonce_buffer_t nonce_buffer;
	packet_queue_t packet_queue;
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
	CUTE_FREE(client, app->mem_ctx);
	return NULL;
}

void client_destroy(client_t* client)
{
	socket_cleanup(&client->socket);
	CUTE_FREE(client, client->app->mem_ctx);
}

int client_connect(client_t* client, uint8_t* connect_token)
{
	//CUTE_CHECK(connect_token_open(&client->connect_token, connect_token));

	client->state = CLIENT_STATE_SENDING_CONNECTION_REQUEST;
	client->loopback = 0;
	client->last_packet_recieved_time = 0;
	client->last_packet_sent_time = CUTE_KEEPALIVE_RATE;
	CUTE_CHECK(socket_init(&client->socket, client->server_endpoint.type, client->server_endpoint.port, CUTE_CLIENT_SEND_BUFFER_SIZE, CUTE_CLIENT_RECEIVE_BUFFER_SIZE));
	client->sequence = 0;
	packet_queue_init(&client->packet_queue);
	nonce_buffer_init(&client->nonce_buffer);
	return 0;

cute_error:
	return -1;
}

void client_disconnect(client_t* client)
{
	socket_cleanup(&client->socket);
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
	uint64_t timestamp = (uint64_t)time(NULL);
	uint8_t* buffer = client->buffer;

	while (1)
	{
		endpoint_t from;
		int bytes_read = socket_receive(&client->socket, &from, buffer, CUTE_PACKET_SIZE_MAX);
		if (bytes_read <= 0) {
			// No more packets to receive for now.
			break;
		}

		if (!endpoint_equals(from, client->server_endpoint)) {
			// Only accept communications if the address match's the server's address.
			// This is mostly just a "sanity" check.
			break;
		}

		packet_type_t type;
		//void* packet_ptr = packet_open(
		//	client->packet_allocator,
		//	&client->nonce_buffer,
		//	client->connect_token.application_id,
		//	timestamp,
		//	client->buffer,
		//	bytes_read,
		//	&client->connect_token.key,
		//	0,
		//	&type
		//);
		void* packet_ptr = NULL;

		int push_packet = 0;
		switch (type)
		{
		case PACKET_TYPE_CONNECTION_ACCEPTED:
		{
			if (client->state == CLIENT_STATE_SENDING_CONNECTION_REQUEST) {
				client->last_packet_recieved_time = 0;
				packet_connection_accepted_t* packet = (packet_connection_accepted_t*)packet_ptr;
				client->client_number = packet->client_number;
				client->max_clients = packet->max_clients;
				client->state = CLIENT_STATE_CONNECTED;
			}
		}	break;
		
		case PACKET_TYPE_CONNECTION_DENIED:
		{
			if (client->state == CLIENT_STATE_SENDING_CONNECTION_REQUEST ||
			    client->state == CLIENT_STATE_SENDING_CONNECTION_REQUEST) {
				client->last_packet_recieved_time = 0;
				client->state = CLIENT_STATE_DISCONNECTED;
			}
		}	break;
		
		case PACKET_TYPE_KEEPALIVE:
		case PACKET_TYPE_DISCONNECT:
		case PACKET_TYPE_CHALLENGE_REQUEST:
		{
			client->last_packet_recieved_time = 0;
		}	break;
		
		case PACKET_TYPE_USERDATA:
		{
			client->last_packet_recieved_time = 0;
			push_packet = 1;
		}	break;
		}

		if (push_packet) {
			packet_queue_push(&client->packet_queue, packet_ptr, type);
		} else {
			packet_allocator_free(client->packet_allocator, type, packet_ptr);
		}
	}
}

static void s_client_send_packet(client_t* client, void* packet, packet_type_t type)
{
	uint8_t* buffer = client->buffer;
	const crypto_key_t* key = type == PACKET_TYPE_CONNECTION_REQUEST ? NULL : &client->key;
	//int size = packet_write(packet, type, buffer, client->connect_token.application_id, client->sequence + client->connect_token.sequence_offset, key);
	int size = 0;
	if (size <= 0) return;
	CUTE_ASSERT(size <= CUTE_PACKET_SIZE_MAX);
	int bytes_sent = socket_send(&client->socket, client->server_endpoint, buffer, size);
	(void)bytes_sent;
}

static void s_client_send_packets(client_t* client)
{
	switch (client->state)
	{
	case CLIENT_STATE_SENDING_CONNECTION_REQUEST:
	{
		if (client->last_packet_sent_time >= CUTE_KEEPALIVE_RATE) {
			client->last_packet_sent_time = 0;

			packet_encrypted_connect_token_t packet;
			//packet.expiration_timestamp = client->connect_token.expire_timestamp;
			//CUTE_ASSERT(sizeof(packet.nonce) == sizeof(client->connect_token.nonce));
			//CUTE_MEMCPY(packet.nonce, client->connect_token.nonce, sizeof(packet.nonce));
			//CUTE_ASSERT(sizeof(packet.secret_data) == sizeof(client->connect_token.secret_data));
			//CUTE_MEMCPY(packet.secret_data, client->connect_token.secret_data, sizeof(packet.secret_data));

			s_client_send_packet(client, &packet, PACKET_TYPE_CONNECTION_REQUEST);
		}
	}	break;

	case CLIENT_STATE_SENDING_CHALLENGE_RESPONSE:
	{
		if (client->last_packet_sent_time >= CUTE_KEEPALIVE_RATE) {
			client->last_packet_sent_time = 0;

			packet_challenge_t packet;
			packet.nonce = client->challenge_sequence;
			CUTE_MEMCPY(packet.challenge_data, client->challenge_data, CUTE_CHALLENGE_DATA_SIZE);

			s_client_send_packet(client, &packet, PACKET_TYPE_CHALLENGE_RESPONSE);
		}
	}	break;

	case CLIENT_STATE_CONNECTED:
		if (client->last_packet_sent_time >= CUTE_KEEPALIVE_RATE) {
			client->last_packet_sent_time = 0;

			packet_keepalive_t packet;
			packet.packet_type = PACKET_TYPE_KEEPALIVE;

			s_client_send_packet(client, &packet, PACKET_TYPE_KEEPALIVE);
		}
		break;
	}
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
