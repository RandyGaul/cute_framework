/*
	Cute Framework
	Copyright (C) 2022 Randy Gaul https://randygaul.net

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

#include <cute_networking.h>

#define CUTE_NET_IMPLEMENTATION
#include <cute/cute_net.h>

namespace cute
{

static CUTE_INLINE cf_error_t wrap(cn_error_t cn_err)
{
	cf_error_t err;
	err.code = cn_err.code;
	err.details = cn_err.details;
	return err;
}

CUTE_STATIC_ASSERT(CUTE_CONNECT_TOKEN_SIZE == CN_CONNECT_TOKEN_SIZE, "Must be equal.");
CUTE_STATIC_ASSERT(CUTE_CONNECT_TOKEN_USER_DATA_SIZE == CN_CONNECT_TOKEN_USER_DATA_SIZE, "Must be equal.");

int endpoint_init(endpoint_t* endpoint, const char* address_and_port_string)
{
	return cn_endpoint_init(endpoint, address_and_port_string);
}

void endpoint_to_string(endpoint_t endpoint, char* buffer, int buffer_size)
{
	cn_endpoint_to_string(endpoint, buffer, buffer_size);
}

int endpoint_equals(endpoint_t a, endpoint_t b)
{
	return cn_endpoint_equals(a, b);
}

crypto_key_t crypto_generate_key()
{
	return cn_crypto_generate_key();
}

void crypto_random_bytes(void* data, int byte_count)
{
	cn_crypto_random_bytes(data, byte_count);
}

void crypto_sign_keygen(crypto_sign_public_t* public_key, crypto_sign_secret_t* secret_key)
{
	cn_crypto_sign_keygen(public_key, secret_key);
}

cf_error_t generate_connect_token(
	uint64_t application_id,
	uint64_t creation_timestamp,
	const crypto_key_t* client_to_server_key,
	const crypto_key_t* server_to_client_key,
	uint64_t expiration_timestamp,
	uint32_t handshake_timeout,
	int address_count,
	const char** address_list,
	uint64_t client_id,
	const uint8_t* user_data,
	const crypto_sign_secret_t* shared_secret_key,
	uint8_t* token_ptr_out
)
{
	cn_error_t err = cn_generate_connect_token(
		application_id,
		creation_timestamp,
		client_to_server_key,
		server_to_client_key,
		expiration_timestamp,
		handshake_timeout,
		address_count,
		address_list,
		client_id,
		user_data,
		shared_secret_key,
		token_ptr_out);
	return wrap(err);
}

client_t* client_create(
	uint16_t port,
	uint64_t application_id,
	bool use_ipv6 /* = false */,
	void* user_allocator_context /* = NULL */
)
{
	return cn_client_create(port, application_id, use_ipv6, user_allocator_context);
}

void client_destroy(client_t* client)
{
	cn_client_destroy(client);
}

cf_error_t client_connect(client_t* client, const uint8_t* connect_token)
{
	return wrap(cn_client_connect(client, connect_token));
}

void client_disconnect(client_t* client)
{
	cn_client_disconnect(client);
}

void client_update(client_t* client, double dt, uint64_t current_time)
{
	cn_client_update(client, dt, current_time);
}

bool client_pop_packet(client_t* client, void** packet, int* size, bool* was_sent_reliably /* = NULL */)
{
	return cn_client_pop_packet(client, packet, size, was_sent_reliably);
}

void client_free_packet(client_t* client, void* packet)
{
	cn_client_free_packet(client, packet);
}

cf_error_t client_send(client_t* client, const void* packet, int size, bool send_reliably)
{
	return wrap(cn_client_send(client, packet, size, send_reliably));
}

client_state_t client_state_get(const client_t* client)
{
	return (client_state_t)cn_client_state_get(client);
}

const char* client_state_string(client_state_t state)
{
	return cn_client_state_string((cn_client_state_t)state);
}

float client_time_of_last_packet_recieved(const client_t* client)
{
	return cn_client_time_of_last_packet_recieved(client);
}

void client_enable_network_simulator(client_t* client, double latency, double jitter, double drop_chance, double duplicate_chance)
{
	cn_client_enable_network_simulator(client, latency, jitter, drop_chance, duplicate_chance);
}

//--------------------------------------------------------------------------------------------------
// SERVER

CUTE_STATIC_ASSERT(CUTE_SERVER_MAX_CLIENTS == CN_SERVER_MAX_CLIENTS, "Must be equal.");

server_t* server_create(server_config_t config)
{
	cn_server_config_t cn_config;
	cn_config.application_id = config.application_id;
	cn_config.max_incoming_bytes_per_second = config.max_incoming_bytes_per_second;
	cn_config.max_outgoing_bytes_per_second = config.max_outgoing_bytes_per_second;
	cn_config.connection_timeout = config.connection_timeout;
	cn_config.resend_rate = config.resend_rate;
	cn_config.public_key = config.public_key;
	cn_config.secret_key = config.secret_key;
	cn_config.user_allocator_context = config.user_allocator_context;
	return cn_server_create(cn_config);
}

void server_destroy(server_t* server)
{
	cn_server_destroy(server);
}

cf_error_t server_start(server_t* server, const char* address_and_port)
{
	return wrap(cn_server_start(server, address_and_port));
}

void server_stop(server_t* server)
{
	return cn_server_stop(server);
}

CUTE_STATIC_ASSERT(sizeof(server_event_t) == sizeof(cn_server_event_t), "Must be equal.");

bool server_pop_event(server_t* server, server_event_t* event)
{
	return cn_server_pop_event(server, (cn_server_event_t*)event);
}

void server_free_packet(server_t* server, int client_index, void* data)
{
	cn_server_free_packet(server, client_index, data);
}

void server_update(server_t* server, double dt, uint64_t current_time)
{
	cn_server_update(server, dt, current_time);
}

void server_disconnect_client(server_t* server, int client_index, bool notify_client /* = true */)
{
	cn_server_disconnect_client(server, client_index, notify_client);
}

void server_send(server_t* server, const void* packet, int size, int client_index, bool send_reliably)
{
	cn_server_send(server, packet, size, client_index, send_reliably);
}

void server_send_to_all_clients(server_t* server, const void* packet, int size, bool send_reliably)
{
	cn_server_send_to_all_clients(server, packet, size, send_reliably);
}

void server_send_to_all_but_one_client(server_t* server, const void* packet, int size, int client_index, bool send_reliably)
{
	cn_server_send_to_all_but_one_client(server, packet, size, client_index, send_reliably);
}

bool server_is_client_connected(server_t* server, int client_index)
{
	return cn_server_is_client_connected(server, client_index);
}

void server_enable_network_simulator(server_t* server, double latency, double jitter, double drop_chance, double duplicate_chance)
{
	cn_server_enable_network_simulator(server, latency, jitter, drop_chance, duplicate_chance);
}

}
