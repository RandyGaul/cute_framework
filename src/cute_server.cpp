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
#include <internal/cute_transport_internal.h>

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
	int client_count = 0;
	transport_t* client_transports[CUTE_SERVER_MAX_CLIENTS];

	protocol::server_t* p_server = NULL;
};

static int s_send_packet_fn(int client_index, uint16_t sequence, void* packet, int size, void* udata)
{
	server_t* server = (server_t*)udata;
	return protocol::server_send_to_client(server->p_server, packet, size, client_index);
}

static int s_open_packet_fn(int index, uint16_t sequence, void* packet, int size, void* udata)
{
	server_t* server = (server_t*)udata;
	return 0;
}

server_t* server_create(server_config_t* config, void* user_allocator_context)
{
	server_t* server = CUTE_NEW(server_t, user_allocator_context);
	server->config = *config;

	server->p_server = protocol::server_make(config->application_id, &server->config.public_key, &server->config.secret_key, server->mem_ctx);

	for (int i = 0; i < CUTE_SERVER_MAX_CLIENTS; ++i) {
		ack_system_config_t ack_config;
		ack_config.index = i;
		ack_config.send_packet_fn = s_send_packet_fn;
		ack_config.open_packet_fn = s_open_packet_fn;
		ack_config.udata = server;
		ack_config.user_allocator_context = user_allocator_context;

		transport_config_t transport_config;
		transport_config.ack_system = ack_system_make(&ack_config);
		transport_config.user_allocator_context = user_allocator_context;
		server->client_transports[i] = transport_make(&transport_config);
	}

	return server;
}

void server_destroy(server_t* server)
{
}

error_t server_start(server_t* server, const char* address_and_port)
{
	error_t err = protocol::server_start(server->p_server, address_and_port, server->config.connection_timeout);
	if (err.is_error()) return err;
	return error_success();
}

void server_stop(server_t* server)
{
	//while(server->client_count)
	//{
	//	server_disconnect_client(server, server->client_handle[0]);
	//}
}

void server_update(server_t* server, float dt)
{
	server->connection_denied_count = 0;
	//s_server_recieve_packets(server);
	//s_server_send_packets(server, dt);
}

bool server_poll_event(server_t* server, server_event_t* event)
{
	return false;
}

void server_disconnect_client(server_t* server, handle_t client_id, bool send_notification_to_client)
{
}

void server_look_for_and_disconnected_timed_out_clients(server_t* server)
{
	//int client_count = server->client_count;
	//float* last_recieved_times = server->client_last_packet_recieved_time;
	//for (int i = 0; i < client_count;)
	//{
	//	const float keepalive_rate = 3.0f;
	//	if (last_recieved_times[i] >= keepalive_rate) {
	//		--client_count;
	//		server_disconnect_client(server, server->client_handle[i], 1);
	//	} else {
	//		 ++i;
	//	}
	//}
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
	//uint32_t index = s_client_index_from_handle(server, client_id);
	//if (index == UINT32_MAX) return -1.0f;
	//CUTE_ASSERT(server->client_is_connected[index]);
	//return server->client_last_packet_recieved_time[index];
	return 0;
}

}
