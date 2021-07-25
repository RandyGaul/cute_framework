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

CUTE_STATIC_ASSERT(CUTE_SERVER_MAX_CLIENTS == CUTE_PROTOCOL_SERVER_MAX_CLIENTS, "Must be equal for a simple implementation.");

namespace cute
{

struct server_t
{
	bool running = false;
	endpoint_t endpoint;
	crypto_sign_public_t public_key;
	crypto_sign_secret_t secret_key;
	server_config_t config;
	socket_t socket;
	uint8_t buffer[CUTE_PROTOCOL_PACKET_SIZE_MAX];
	circular_buffer_t event_queue;
	transport_t* client_transports[CUTE_SERVER_MAX_CLIENTS];
	protocol::server_t* p_server = NULL;
	void* mem_ctx = NULL;
};

static error_t s_send_packet_fn(int client_index, void* packet, int size, void* udata)
{
	server_t* server = (server_t*)udata;
	return protocol::server_send_to_client(server->p_server, packet, size, client_index);
}

server_t* server_create(server_config_t* config, void* user_allocator_context)
{
	server_t* server = CUTE_NEW(server_t, user_allocator_context);
	server->config = *config;
	server->event_queue = circular_buffer_make(CUTE_MB * 10, user_allocator_context);
	server->p_server = protocol::server_make(config->application_id, &server->config.public_key, &server->config.secret_key, server->mem_ctx);

	return server;
}

void server_destroy(server_t* server)
{
	server_stop(server);
	protocol::server_destroy(server->p_server);
	void* mem_ctx = server->mem_ctx;
	circular_buffer_free(&server->event_queue);
	server->~server_t();
	CUTE_FREE(server, mem_ctx);
}

error_t server_start(server_t* server, const char* address_and_port)
{
	error_t err = protocol::server_start(server->p_server, address_and_port, (uint32_t)server->config.connection_timeout);
	if (err.is_error()) return err;

	for (int i = 0; i < CUTE_SERVER_MAX_CLIENTS; ++i) {
		transport_config_t transport_config;
		transport_config.index = i;
		transport_config.send_packet_fn = s_send_packet_fn;
		transport_config.udata = server;
		transport_config.user_allocator_context = server->mem_ctx;
		server->client_transports[i] = transport_make(&transport_config);
	}

	return error_success();
}

void server_stop(server_t* server)
{
	circular_buffer_reset(&server->event_queue);
	protocol::server_stop(server->p_server);
	for (int i = 0; i < CUTE_SERVER_MAX_CLIENTS; ++i) {
		transport_destroy(server->client_transports[i]);
		server->client_transports[i] = NULL;
	}
}

static CUTE_INLINE int s_server_event_pull(server_t* server, server_event_t* event)
{
	return circular_buffer_pull(&server->event_queue, event, sizeof(server_event_t));
}

static CUTE_INLINE int s_server_event_push(server_t* server, server_event_t* event)
{
	if (circular_buffer_push(&server->event_queue, event, sizeof(server_event_t)) < 0) {
		if (circular_buffer_grow(&server->event_queue, server->event_queue.capacity * 2) < 0) {
			return -1;
		}
		return circular_buffer_push(&server->event_queue, event, sizeof(server_event_t));
	} else {
		return 0;
	}
}

void server_update(server_t* server, double dt, uint64_t current_time)
{
	// Update the protocol server.
	protocol::server_update(server->p_server, dt, current_time);

	// Capture any events from the protocol server and process them.
	protocol::server_event_t p_event;
	while (protocol::server_pop_event(server->p_server, &p_event)) {
		switch (p_event.type) {
		case protocol::SERVER_EVENT_NEW_CONNECTION:
		{
			server_event_t e;
			e.type = SERVER_EVENT_TYPE_NEW_CONNECTION;
			e.u.new_connection.client_index = p_event.u.new_connection.client_index;
			e.u.new_connection.client_id = p_event.u.new_connection.client_id;
			e.u.new_connection.endpoint = p_event.u.new_connection.endpoint;
			s_server_event_push(server, &e);
		}	break;

		case protocol::SERVER_EVENT_DISCONNECTED:
		{
			server_event_t e;
			e.type = SERVER_EVENT_TYPE_DISCONNECTED;
			e.u.disconnected.client_index = p_event.u.disconnected.client_index;
			s_server_event_push(server, &e);
		}	break;

		// Protocol packets are processed by the reliability transport layer before they
		// are converted into user-facing server events.
		case protocol::SERVER_EVENT_PAYLOAD_PACKET:
		{
			int index = p_event.u.payload_packet.client_index;
			void* data = p_event.u.payload_packet.data;
			int size = p_event.u.payload_packet.size;
			transport_process_packet(server->client_transports[index], data, size);
			protocol::server_free_packet(server->p_server, data);
		}	break;
		}
	}

	// Update all client reliability transports.
	for (int i = 0; i < CUTE_SERVER_MAX_CLIENTS; ++i) {
		if (protocol::server_is_client_connected(server->p_server, i)) {
			transport_update(server->client_transports[i], dt);
		}
	}

	// Look for any packets to receive from the reliability layer.
	// Convert these into server payload events.
	for (int i = 0; i < CUTE_SERVER_MAX_CLIENTS; ++i) {
		if (protocol::server_is_client_connected(server->p_server, i)) {
			void* data;
			int size;
			while (!transport_receive_reliably_and_in_order(server->client_transports[i], &data, &size).is_error()) {
				server_event_t e;
				e.type = SERVER_EVENT_TYPE_PAYLOAD_PACKET;
				e.u.payload_packet.client_index = i;
				e.u.payload_packet.data = data;
				e.u.payload_packet.size = size;
				s_server_event_push(server, &e);
			}
			while (!transport_receive_fire_and_forget(server->client_transports[i], &data, &size).is_error()) {
				server_event_t e;
				e.type = SERVER_EVENT_TYPE_PAYLOAD_PACKET;
				e.u.payload_packet.client_index = i;
				e.u.payload_packet.data = data;
				e.u.payload_packet.size = size;
				s_server_event_push(server, &e);
			}
		}
	}
}

bool server_pop_event(server_t* server, server_event_t* event)
{
	return s_server_event_pull(server, event) ? false : true;
}

void server_free_packet(server_t* server, int client_index, void* data)
{
	CUTE_ASSERT(client_index >= 0 && client_index < CUTE_SERVER_MAX_CLIENTS);
	CUTE_ASSERT(protocol::server_is_client_connected(server->p_server, client_index));
	transport_free_packet(server->client_transports[client_index], data);
}

void server_disconnect_client(server_t* server, int client_index, bool notify_client)
{
	CUTE_ASSERT(client_index >= 0 && client_index < CUTE_SERVER_MAX_CLIENTS);
	CUTE_ASSERT(protocol::server_is_client_connected(server->p_server, client_index));
	protocol::server_disconnect_client(server->p_server, client_index, notify_client);
}

void server_find_and_disconnect_timed_out_clients(server_t* server, float timeout)
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

void server_send(server_t* server, const void* packet, int size, int client_index, bool send_reliably)
{
	CUTE_ASSERT(client_index >= 0 && client_index < CUTE_SERVER_MAX_CLIENTS);
	CUTE_ASSERT(protocol::server_is_client_connected(server->p_server, client_index));
	transport_send(server->client_transports[client_index], packet, size, send_reliably);
}

void server_send_to_all_clients(server_t* server, const void* packet, int size, bool send_reliably)
{
	for (int i = 0; i < CUTE_SERVER_MAX_CLIENTS; ++i) {
		if (server_is_client_connected(server, i)) {
			server_send(server, packet, size, i, send_reliably);
		}
	}
}

void server_send_to_all_but_one_client(server_t* server, const void* packet, int size, int client_index, bool send_reliably)
{
	CUTE_ASSERT(client_index >= 0 && client_index < CUTE_SERVER_MAX_CLIENTS);
	CUTE_ASSERT(protocol::server_is_client_connected(server->p_server, client_index));

	for (int i = 0; i < CUTE_SERVER_MAX_CLIENTS; ++i) {
		if (i == client_index) continue;
		if (server_is_client_connected(server, i)) {
			server_send(server, packet, size, i, send_reliably);
		}
	}
}

float server_time_of_last_packet_recieved_from_client(server_t* server, int client_index)
{
	CUTE_ASSERT(client_index >= 0 && client_index < CUTE_SERVER_MAX_CLIENTS);
	CUTE_ASSERT(protocol::server_is_client_connected(server->p_server, client_index));
	//uint32_t index = s_client_index_from_handle(server, client_id);
	//if (index == UINT32_MAX) return -1.0f;
	//CUTE_ASSERT(server->client_is_connected[index]);
	//return server->client_last_packet_recieved_time[index];
	return 0;
}

bool server_is_client_connected(server_t* server, int client_index)
{
	return protocol::server_is_client_connected(server->p_server, client_index);
}

void server_enable_network_simulator(server_t* server, double latency, double jitter, double drop_chance, double duplicate_chance)
{
	protocol::server_enable_network_simulator(server->p_server, latency, jitter, drop_chance, duplicate_chance);
}

}
