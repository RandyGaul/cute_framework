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

#include <internal/cute_net_internal.h>
#include <internal/cute_app_internal.h>
#include <internal/cute_protocol_internal.h>
#include <internal/cute_transport_internal.h>

#include <time.h>

#define CUTE_CLIENT_MAX_RECONNECT_TRIES 3

namespace cute
{

struct client_t
{
	protocol::client_t* p_client = NULL;
	transport_t* transport = NULL;
	void* mem_ctx = NULL;
};

static error_t s_send(int client_index, void* packet, int size, void* udata)
{
	client_t* client = (client_t*)udata;
	return protocol::client_send(client->p_client, packet, size);
}

client_t* client_make(uint16_t port, uint64_t application_id, void* user_allocator_context)
{
	protocol::client_t* p_client = protocol::client_make(port, application_id, user_allocator_context);
	if (!p_client) return NULL;

	client_t* client = CUTE_NEW(client_t, user_allocator_context);
	client->mem_ctx = user_allocator_context;
	client->p_client = p_client;

	transport_config_t config;
	config.send_packet_fn = s_send;
	config.user_allocator_context = user_allocator_context;
	client->transport = transport_make(&config);

	return client;
}

void client_destroy(client_t* client)
{
	protocol::client_destroy(client->p_client);
	transport_destroy(client->transport);
	void* mem_ctx = client->mem_ctx;
	client->~client_t();
	CUTE_FREE(client, mem_ctx);
}

error_t client_connect(client_t* client, const uint8_t* connect_token)
{
	return protocol::client_connect(client->p_client, connect_token);
}

void client_disconnect(client_t* client)
{
	protocol::client_disconnect(client->p_client);
}

void client_update(client_t* client, float dt)
{
	protocol::client_update(client->p_client, dt, 0);

	if (protocol::client_get_state(client->p_client) == protocol::CLIENT_STATE_CONNECTED) {
		transport_process_acks(client->transport);
		transport_resend_unacked_fragments(client->transport);
	}
}

bool client_pop_packet(client_t* client, void** packet, int* size)
{
	if (protocol::client_get_state(client->p_client) != protocol::CLIENT_STATE_CONNECTED) {
		return false;
	}

	bool got = !transport_receive_reliably_and_in_order(client->transport, packet, size).is_error();
	if (!got) {
		got = transport_receive_fire_and_forget(client->transport, packet, size).is_error();
	}
	return got;
}

error_t client_send(client_t* client, const void* packet, int size, bool send_reliably)
{
	if (protocol::client_get_state(client->p_client) != protocol::CLIENT_STATE_CONNECTED) {
		return error_failure("Client is not connected.");
	}

	return transport_send(client->transport, packet, size, send_reliably);
}

client_state_t client_state_get(const client_t* client)
{
	return (client_state_t)protocol::client_get_state(client->p_client);
}

float client_time_of_last_packet_recieved(const client_t* client)
{
	return 0;
}

}
