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
#include <cute_server.h>
#include <cute_alloc.h>
#include <cute_crypto.h>

#include <internal/cute_defines_internal.h>
#include <internal/cute_net_internal.h>

#include <cute/cute_serialize.h>

#define CUTE_SERVER_SEND_BUFFER_SIZE (20 * CUTE_MB)
#define CUTE_SERVER_RECEIVE_BUFFER_SIZE (20 * CUTE_MB)

namespace cute
{

struct server_t
{
	int running = 0;
	endpoint_t endpoint;
	crypto_key_t public_key;
	crypto_key_t private_key;
	server_config_t config;
	socket_t socket;
	serialize_t* io;
	packet_queue_t packets;
	uint8_t buffer[CUTE_PACKET_SIZE_MAX];
	void* mem_ctx = NULL;

	handle_table_t client_handles;
	int client_count = 0;
	int client_is_connected[CUTE_SERVER_MAX_CLIENTS];
	int client_is_loopback[CUTE_SERVER_MAX_CLIENTS];
	float client_last_packet_recieved_time[CUTE_SERVER_MAX_CLIENTS];
	endpoint_t client_endpoint[CUTE_SERVER_MAX_CLIENTS];
	uint64_t client_sequence_offset[CUTE_SERVER_MAX_CLIENTS];
	uint64_t client_sequence[CUTE_SERVER_MAX_CLIENTS];
	nonce_buffer_t client_nonce_buffer[CUTE_SERVER_MAX_CLIENTS];
	handle_t client_id[CUTE_SERVER_MAX_CLIENTS];
	crypto_key_t client_session_key[CUTE_SERVER_MAX_CLIENTS];
	packet_queue_t client_packets[CUTE_SERVER_MAX_CLIENTS];
};

server_t* server_alloc(void* user_allocator_context)
{
	server_t* server = (server_t*)CUTE_ALLOC(sizeof(server_t), user_allocator_context);
	CUTE_CHECK_POINTER(server);
	CUTE_PLACEMENT_NEW(server) server_t;
	server->mem_ctx = user_allocator_context;

cute_error:
	CUTE_FREE(server, user_allocator_context);
	return NULL;
}

void server_destroy(server_t* server)
{
	CUTE_FREE(server, server->mem_ctx);
}

int server_start(server_t* server, endpoint_t endpoint, const crypto_key_t* public_key, const crypto_key_t* private_key, const server_config_t* config)
{
	server->endpoint = endpoint;
	server->public_key = *public_key;
	server->private_key = *private_key;
	CUTE_CHECK(socket_init(&server->socket, endpoint, CUTE_SERVER_SEND_BUFFER_SIZE, CUTE_SERVER_RECEIVE_BUFFER_SIZE));
	server->config = config ? *config : server_config_t();
	CUTE_CHECK(handle_table_init(&server->client_handles, 256, server->mem_ctx));
	server->client_count = 0;
	server->running = 1;

	server->client_count = 0;
	return 0;

cute_error:
	return -1;
}

void server_stop(server_t* server)
{
	server->running = 0;
	socket_cleanup(&server->socket);
}

static uint32_t s_client_index_from_endpoint(server_t* server, endpoint_t endpoint)
{
	endpoint_t* endpoints = server->client_endpoint;
	int count = server->client_count;
	for (int i = 0; i < count; ++i)
	{
		if (endpoint_equals(endpoints[i], endpoint)) {
			return i;
		}
	}
	return UINT32_MAX;
}

static void s_server_recieve_packets(server_t* server)
{
	uint8_t* buffer = server->buffer;

	while (1)
	{
		endpoint_t from;
		int bytes_read = socket_receive(&server->socket, &from, buffer, CUTE_PACKET_SIZE_MAX);
		if (bytes_read <= 0) {
			// No more packets to receive for now.
			break;
		}

		// Find client by address.
		uint32_t client_index = s_client_index_from_endpoint(server, from);
		if (client_index == UINT32_MAX) {
			// Client address not found -- potential new connection.
			// Lookup symmetric key.
			// Decrypt packet.
			int is_new_connection_packet = 0;
			if (!is_new_connection_packet) {
				// Do not know who this is, and they are not requesting a new connection.
				// Ignore this nonsense.
				continue;
			}
		}

		// Otherwise use server private key.
		// Decrypt packet.
		// Read sequence number, do replay protection.
		// Switch on packet type.

		// TODO: Make sure on client replay protection happens *after* decrypting.
	}
}

static void s_server_send_packets(server_t* server, float dt)
{
	// Look for connection responses to send.
	// Look for keep-alives to send.
}

void server_update(server_t* server, float dt)
{
	s_server_recieve_packets(server);
	s_server_send_packets(server, dt);
}

int server_poll_event(server_t* server, server_event_t* event)
{
	return -1;
}

void server_disconnect_client(server_t* server, handle_t id)
{
}

void server_look_for_and_disconnected_timed_out_clients(server_t* server)
{
}

void server_broadcast_to_all_clients(server_t* server, const void* packet, int size, int reliable)
{
}

void server_broadcast_to_all_but_one_client(server_t* server, const void* packet, int size, handle_t id, int reliable)
{
}

void server_send_to_client(server_t* server, const void* packet, int size, handle_t id, int reliable)
{
}

}
