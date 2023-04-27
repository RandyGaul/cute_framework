/*
	Cute Framework
	Copyright (C) 2023 Randy Gaul https://randygaul.github.io/

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

#define CF_NET_IMPLEMENTATION
#include <cute/cute_net.h>

static CF_INLINE CF_Result cf_wrap(cn_result_t cn_result)
{
	CF_Result result;
	result.code = cn_result.code;
	result.details = cn_result.details;
	return result;
}

CF_STATIC_ASSERT(CF_CONNECT_TOKEN_SIZE == CN_CONNECT_TOKEN_SIZE, "Must be equal.");
CF_STATIC_ASSERT(CF_CONNECT_TOKEN_USER_DATA_SIZE == CN_CONNECT_TOKEN_USER_DATA_SIZE, "Must be equal.");

int cf_adress_init(CF_Address* endpoint, const char* address_and_port_string)
{
	return cn_endpoint_init(endpoint, address_and_port_string);
}

void cf_adress_to_string(CF_Address endpoint, char* buffer, int buffer_size)
{
	cn_endpoint_to_string(endpoint, buffer, buffer_size);
}

int cf_adress_equals(CF_Address a, CF_Address b)
{
	return cn_endpoint_equals(a, b);
}

CF_CryptoKey cf_crypto_generate_key()
{
	return cn_crypto_generate_key();
}

void cf_crypto_random_bytes(void* data, int byte_count)
{
	cn_crypto_random_bytes(data, byte_count);
}

void cf_crypto_sign_keygen(CF_CryptoSignPublic* public_key, CF_CryptoSignSecret* secret_key)
{
	cn_crypto_sign_keygen(public_key, secret_key);
}

CF_Result cf_generate_connect_token(
	uint64_t application_id,
	uint64_t creation_timestamp,
	const CF_CryptoKey* client_to_server_key,
	const CF_CryptoKey* server_to_client_key,
	uint64_t expiration_timestamp,
	uint32_t handshake_timeout,
	int address_count,
	const char** address_list,
	uint64_t client_id,
	const uint8_t* user_data,
	const CF_CryptoSignSecret* shared_secret_key,
	uint8_t* token_ptr_out
)
{
	cn_result_t result = cn_generate_connect_token(
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
	return cf_wrap(result);
}

CF_Client* cf_make_client(
	uint16_t port,
	uint64_t application_id,
	bool use_ipv6 /* = false */
)
{
	return cn_client_create(port, application_id, use_ipv6, NULL);
}

void cf_destroy_client(CF_Client* client)
{
	cn_client_destroy(client);
}

CF_Result cf_client_connect(CF_Client* client, const uint8_t* connect_token)
{
	return cf_wrap(cn_client_connect(client, connect_token));
}

void cf_client_disconnect(CF_Client* client)
{
	cn_client_disconnect(client);
}

void cf_client_update(CF_Client* client, double dt, uint64_t current_time)
{
	cn_client_update(client, dt, current_time);
}

bool cf_client_pop_packet(CF_Client* client, void** packet, int* size, bool* was_sent_reliably /* = NULL */)
{
	return cn_client_pop_packet(client, packet, size, was_sent_reliably);
}

void cf_client_free_packet(CF_Client* client, void* packet)
{
	cn_client_free_packet(client, packet);
}

CF_Result cf_client_send(CF_Client* client, const void* packet, int size, bool send_reliably)
{
	return cf_wrap(cn_client_send(client, packet, size, send_reliably));
}

CF_ClientState cf_client_state_get(const CF_Client* client)
{
	return (CF_ClientState)cn_client_state_get(client);
}

void cf_client_enable_network_simulator(CF_Client* client, double latency, double jitter, double drop_chance, double duplicate_chance)
{
	cn_client_enable_network_simulator(client, latency, jitter, drop_chance, duplicate_chance);
}

//--------------------------------------------------------------------------------------------------
// SERVER

CF_STATIC_ASSERT(CF_SERVER_MAX_CLIENTS == CN_SERVER_MAX_CLIENTS, "Must be equal.");

CF_Server* cf_make_server(CF_ServerConfig config)
{
	cn_server_config_t cn_config;
	cn_config.application_id = config.application_id;
	cn_config.max_incoming_bytes_per_second = config.max_incoming_bytes_per_second;
	cn_config.max_outgoing_bytes_per_second = config.max_outgoing_bytes_per_second;
	cn_config.connection_timeout = config.connection_timeout;
	cn_config.resend_rate = config.resend_rate;
	cn_config.public_key = config.public_key;
	cn_config.secret_key = config.secret_key;
	cn_config.user_allocator_context = NULL;
	return cn_server_create(cn_config);
}

void cf_destroy_server(CF_Server* server)
{
	cn_server_destroy(server);
}

CF_Result cf_server_start(CF_Server* server, const char* address_and_port)
{
	return cf_wrap(cn_server_start(server, address_and_port));
}

void cf_server_stop(CF_Server* server)
{
	return cn_server_stop(server);
}

CF_STATIC_ASSERT(sizeof(CF_ServerEvent) == sizeof(cn_server_event_t), "Must be equal.");

bool cf_server_pop_event(CF_Server* server, CF_ServerEvent* event)
{
	return cn_server_pop_event(server, (cn_server_event_t*)event);
}

void cf_server_free_packet(CF_Server* server, int client_index, void* data)
{
	cn_server_free_packet(server, client_index, data);
}

void cf_server_update(CF_Server* server, double dt, uint64_t current_time)
{
	cn_server_update(server, dt, current_time);
}

void cf_server_disconnect_client(CF_Server* server, int client_index, bool notify_client /* = true */)
{
	cn_server_disconnect_client(server, client_index, notify_client);
}

void cf_server_send(CF_Server* server, const void* packet, int size, int client_index, bool send_reliably)
{
	cn_server_send(server, packet, size, client_index, send_reliably);
}
bool cf_server_is_client_connected(CF_Server* server, int client_index)
{
	return cn_server_is_client_connected(server, client_index);
}

void cf_server_enable_network_simulator(CF_Server* server, double latency, double jitter, double drop_chance, double duplicate_chance)
{
	cn_server_enable_network_simulator(server, latency, jitter, drop_chance, duplicate_chance);
}
