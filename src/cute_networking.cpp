/*
	Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

#include <cute_networking.h>
#include <cute_alloc.h>
#include <cute_c_runtime.h>

// This entire file makes no sense for web builds, since web doesn't allow UDP.
#ifndef CF_EMSCRIPTEN

#define CUTE_NET_IMPLEMENTATION
#define CN_ALLOC(size, ctx) cf_alloc(size)
#define CN_FREE(mem, ctx) cf_free(mem)
#include <cute/cute_net.h>
#include <cute/cute_arith.h>

// cute_net is walled off behind the public CF_* API (see cute_networking.h). CF owns its own
// networking types; the ones below are defined so their layouts match cute_net's, letting this file
// bridge the two by reinterpret_cast / field copy. These static asserts are the guardrail that
// keeps the two definitions from ever drifting apart.
CF_STATIC_ASSERT(CF_CONNECT_TOKEN_SIZE == CN_CONNECT_TOKEN_SIZE, "Must be equal.");
CF_STATIC_ASSERT(CF_CONNECT_TOKEN_USER_DATA_SIZE == CN_CONNECT_TOKEN_USER_DATA_SIZE, "Must be equal.");
CF_STATIC_ASSERT(CF_SERVER_MAX_CLIENTS == CN_SERVER_MAX_CLIENTS, "Must be equal.");
CF_STATIC_ASSERT(sizeof(CF_CryptoKey) == sizeof(cn_crypto_key_t), "Must be equal.");
CF_STATIC_ASSERT(sizeof(CF_CryptoSignPublic) == sizeof(cn_crypto_sign_public_t), "Must be equal.");
CF_STATIC_ASSERT(sizeof(CF_CryptoSignSecret) == sizeof(cn_crypto_sign_secret_t), "Must be equal.");
CF_STATIC_ASSERT(sizeof(CF_Address) == sizeof(cn_endpoint_t), "Must be equal.");
CF_STATIC_ASSERT(sizeof(CF_ServerEvent) == sizeof(cn_server_event_t), "Must be equal.");

static CF_INLINE CF_Result cf_wrap(cn_result_t cn_result)
{
	CF_Result result;
	result.code = cn_result.code;
	result.details = cn_result.details;
	return result;
}

// Layout-compatible reinterpretations across the wall.
static CF_INLINE cn_endpoint_t cf_to_cn_endpoint(CF_Address a) { cn_endpoint_t e; CF_MEMCPY(&e, &a, sizeof(e)); return e; }
static CF_INLINE CF_CryptoKey cf_from_cn_key(cn_crypto_key_t k) { CF_CryptoKey out; CF_MEMCPY(&out, &k, sizeof(out)); return out; }

int cf_address_init(CF_Address* address, const char* address_and_port_string)
{
	return cn_endpoint_init((cn_endpoint_t*)address, address_and_port_string);
}

void cf_address_to_string(CF_Address address, char* buffer, int buffer_size)
{
	cn_endpoint_to_string(cf_to_cn_endpoint(address), buffer, buffer_size);
}

bool cf_address_equals(CF_Address a, CF_Address b)
{
	return cn_endpoint_equals(cf_to_cn_endpoint(a), cf_to_cn_endpoint(b)) != 0;
}

CF_CryptoKey cf_crypto_generate_key()
{
	return cf_from_cn_key(cn_crypto_generate_key());
}

void cf_crypto_random_bytes(void* data, int byte_count)
{
	cn_crypto_random_bytes(data, byte_count);
}

void cf_crypto_sign_keygen(CF_CryptoSignPublic* public_key, CF_CryptoSignSecret* secret_key)
{
	cn_crypto_sign_keygen((cn_crypto_sign_public_t*)public_key, (cn_crypto_sign_secret_t*)secret_key);
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
		(const cn_crypto_key_t*)client_to_server_key,
		(const cn_crypto_key_t*)server_to_client_key,
		expiration_timestamp,
		handshake_timeout,
		address_count,
		address_list,
		client_id,
		user_data,
		(const cn_crypto_sign_secret_t*)shared_secret_key,
		token_ptr_out);
	return cf_wrap(result);
}

CF_Client* cf_make_client(
	uint16_t port,
	uint64_t application_id,
	bool use_ipv6 /* = false */
)
{
	return (CF_Client*)cn_client_create(port, application_id, use_ipv6, NULL);
}

void cf_destroy_client(CF_Client* client)
{
	cn_client_destroy((cn_client_t*)client);
}

CF_Result cf_client_connect(CF_Client* client, const uint8_t* connect_token)
{
	return cf_wrap(cn_client_connect((cn_client_t*)client, connect_token));
}

void cf_client_disconnect(CF_Client* client)
{
	cn_client_disconnect((cn_client_t*)client);
}

void cf_client_update(CF_Client* client, double dt, uint64_t current_time)
{
	cn_client_update((cn_client_t*)client, dt, current_time);
}

bool cf_client_pop_packet(CF_Client* client, void** packet, int* size, bool* was_sent_reliably /* = NULL */)
{
	return cn_client_pop_packet((cn_client_t*)client, packet, size, was_sent_reliably);
}

void cf_client_free_packet(CF_Client* client, void* packet)
{
	cn_client_free_packet((cn_client_t*)client, packet);
}

CF_Result cf_client_send(CF_Client* client, const void* packet, int size, bool send_reliably)
{
	return cf_wrap(cn_client_send((cn_client_t*)client, packet, size, send_reliably));
}

CF_ClientState cf_client_state(const CF_Client* client)
{
	return (CF_ClientState)cn_client_state_get((cn_client_t*)client);
}

float cf_client_rtt(CF_Client* client)
{
	return cn_client_get_rtt_estimate((cn_client_t*)client);
}

float cf_client_packet_loss(CF_Client* client)
{
	return cn_client_get_packet_loss_estimate((cn_client_t*)client);
}

float cf_client_incoming_kbps(CF_Client* client)
{
	return cn_client_get_incoming_kbps_estimate((cn_client_t*)client);
}

float cf_client_outgoing_kbps(CF_Client* client)
{
	return cn_client_get_outgoing_kbps_estimate((cn_client_t*)client);
}

void cf_client_enable_network_simulator(CF_Client* client, double latency, double jitter, double drop_chance, double duplicate_chance)
{
	cn_client_enable_network_simulator((cn_client_t*)client, latency, jitter, drop_chance, duplicate_chance);
}

//--------------------------------------------------------------------------------------------------
// SERVER

CF_Server* cf_make_server(CF_ServerConfig config)
{
	cn_server_config_t cn_config;
	cn_config.application_id = config.application_id;
	cn_config.max_incoming_bytes_per_second = 0;
	cn_config.max_outgoing_bytes_per_second = 0;
	cn_config.connection_timeout = config.connection_timeout;
	cn_config.resend_rate = config.resend_rate;
	CF_MEMCPY(&cn_config.public_key, &config.public_key, sizeof(cn_config.public_key));
	CF_MEMCPY(&cn_config.secret_key, &config.secret_key, sizeof(cn_config.secret_key));
	cn_config.user_allocator_context = NULL;
	return (CF_Server*)cn_server_create(cn_config);
}

void cf_destroy_server(CF_Server* server)
{
	cn_server_destroy((cn_server_t*)server);
}

CF_Result cf_server_start(CF_Server* server, const char* address_and_port)
{
	return cf_wrap(cn_server_start((cn_server_t*)server, address_and_port));
}

void cf_server_stop(CF_Server* server)
{
	cn_server_stop((cn_server_t*)server);
}

void cf_server_set_public_ip(CF_Server* server, const char* address_and_port)
{
	cn_server_set_public_ip((cn_server_t*)server, address_and_port);
}

bool cf_server_pop_event(CF_Server* server, CF_ServerEvent* event)
{
	return cn_server_pop_event((cn_server_t*)server, (cn_server_event_t*)event);
}

void cf_server_free_packet(CF_Server* server, int client_index, void* data)
{
	cn_server_free_packet((cn_server_t*)server, client_index, data);
}

void cf_server_update(CF_Server* server, double dt, uint64_t current_time)
{
	cn_server_update((cn_server_t*)server, dt, current_time);
}

void cf_server_disconnect_client(CF_Server* server, int client_index, bool notify_client /* = true */)
{
	cn_server_disconnect_client((cn_server_t*)server, client_index, notify_client);
}

CF_Result cf_server_send(CF_Server* server, const void* packet, int size, int client_index, bool send_reliably)
{
	return cf_wrap(cn_server_send((cn_server_t*)server, packet, size, client_index, send_reliably));
}

void cf_server_send_to_all(CF_Server* server, const void* packet, int size, bool send_reliably)
{
	cn_server_t* s = (cn_server_t*)server;
	for (int i = 0; i < CF_SERVER_MAX_CLIENTS; ++i) {
		if (cn_server_is_client_connected(s, i)) {
			cn_server_send(s, packet, size, i, send_reliably);
		}
	}
}

bool cf_server_is_client_connected(CF_Server* server, int client_index)
{
	return cn_server_is_client_connected((cn_server_t*)server, client_index);
}

float cf_server_rtt(CF_Server* server, int client_index)
{
	return cn_server_get_rtt_estimate((cn_server_t*)server, client_index);
}

float cf_server_packet_loss(CF_Server* server, int client_index)
{
	return cn_server_get_packet_loss_estimate((cn_server_t*)server, client_index);
}

float cf_server_incoming_kbps(CF_Server* server, int client_index)
{
	return cn_server_get_incoming_kbps_estimate((cn_server_t*)server, client_index);
}

float cf_server_outgoing_kbps(CF_Server* server, int client_index)
{
	return cn_server_get_outgoing_kbps_estimate((cn_server_t*)server, client_index);
}

void cf_server_enable_network_simulator(CF_Server* server, double latency, double jitter, double drop_chance, double duplicate_chance)
{
	cn_server_enable_network_simulator((cn_server_t*)server, latency, jitter, drop_chance, duplicate_chance);
}

//--------------------------------------------------------------------------------------------------
// COMPRESSION

int cf_snapshot_compress(const void* baseline, const void* current, int size, void* out, int out_capacity)
{
	return cf_arith_delta_compress((const uint8_t*)baseline, (const uint8_t*)current, size, (uint8_t*)out, out_capacity);
}

int cf_snapshot_decompress(const void* baseline, int size, const void* compressed, int compressed_size, void* out)
{
	return cf_arith_delta_decompress((const uint8_t*)baseline, size, (const uint8_t*)compressed, compressed_size, (uint8_t*)out);
}

#endif // CF_EMSCRIPTEN
