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
#include <cute_c_runtime.h>
#include <cute_protocol.h>
#include <cute_handle_table.h>

#include <internal/cute_net_internal.h>
#include <internal/cute_protocol_internal.h>

#define CUTE_SERVER_SEND_BUFFER_SIZE (20 * CUTE_MB)
#define CUTE_SERVER_RECEIVE_BUFFER_SIZE (20 * CUTE_MB)

#define CUTE_SERVER_CONNECTION_DENIED_MAX_COUNT (1024)

namespace cute
{

struct server_t
{
	bool running = 0;
	endpoint_t endpoint;
	crypto_sign_public_t public_key;
	crypto_sign_secret_t secret_key;
	server_config_t config;
	socket_t socket;
	uint8_t buffer[CUTE_PROTOCOL_PACKET_SIZE_MAX];
	void* mem_ctx = NULL;

	int connection_denied_count = 0;
	handle_table_t client_handle_table;
	int client_count = 0;
	handle_t client_handle[CUTE_SERVER_MAX_CLIENTS];
	bool client_is_connected[CUTE_SERVER_MAX_CLIENTS];
	float client_last_packet_recieved_time[CUTE_SERVER_MAX_CLIENTS];
	float client_last_packet_sent_time[CUTE_SERVER_MAX_CLIENTS];
	endpoint_t client_endpoint[CUTE_SERVER_MAX_CLIENTS];
};

server_t* server_alloc(void* user_allocator_context)
{
	return NULL;
}

void server_destroy(server_t* server)
{
}

int server_start(server_t* server, const char* address_and_port, const crypto_key_t* public_key, const crypto_key_t* secret_key, const server_config_t* config)
{
	return 0;
}

void server_stop(server_t* server)
{
	while(server->client_count)
	{
		server_disconnect_client(server, server->client_handle[0]);
	}
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

static CUTE_INLINE uint32_t s_client_index_from_handle(server_t* server, handle_t h)
{
	return server->client_handle_table.get_index(h);
}

void server_update(server_t* server, float dt)
{
	server->connection_denied_count = 0;
	//s_server_recieve_packets(server);
	//s_server_send_packets(server, dt);

	int client_count = server->client_count;
	float* last_recieved_times = server->client_last_packet_recieved_time;
	float* last_sent_times = server->client_last_packet_sent_time;
	for (int i = 0; i < client_count; ++i)
	{
		last_recieved_times[i] += dt;
		last_sent_times[i] += dt;
	}
}

bool server_poll_event(server_t* server, server_event_t* event)
{
	return false;
}

void server_disconnect_client(server_t* server, handle_t client_id, bool send_notification_to_client)
{
	CUTE_ASSERT(server->client_count >= 1);
	uint32_t index = server->client_handle_table.get_index(client_id);
	if (send_notification_to_client) {
		//s_server_send_packet_no_payload(server, index, protocol::PACKET_TYPE_DISCONNECT);
	}

	// Free client resources.
	server->client_is_connected[index] = 0;
	server->client_handle_table.free_handle(client_id);

	// Move client in back to the empty slot.
	int last_index = --server->client_count;
	if (last_index) {
		handle_t h = server->client_handle[index];
		server->client_handle_table.update_index(h, index);

		server->client_handle[index]                    = server->client_handle[last_index];
		server->client_is_connected[index]              = server->client_is_connected[last_index];
		server->client_last_packet_recieved_time[index] = server->client_last_packet_recieved_time[last_index];
		server->client_last_packet_sent_time[index]     = server->client_last_packet_sent_time[last_index];
		server->client_endpoint[index]                  = server->client_endpoint[last_index];
	}
}

void server_look_for_and_disconnected_timed_out_clients(server_t* server)
{
	int client_count = server->client_count;
	float* last_recieved_times = server->client_last_packet_recieved_time;
	for (int i = 0; i < client_count;)
	{
		const float keepalive_rate = 3.0f;
		if (last_recieved_times[i] >= keepalive_rate) {
			--client_count;
			server_disconnect_client(server, server->client_handle[i], 1);
		} else {
			 ++i;
		}
	}
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

float server_get_last_packet_recieved_time_from_client(server_t* server, handle_t client_id)
{
	uint32_t index = s_client_index_from_handle(server, client_id);
	if (index == UINT32_MAX) return -1.0f;
	CUTE_ASSERT(server->client_is_connected[index]);
	return server->client_last_packet_recieved_time[index];
}

}
