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

#ifndef CUTE_NETWORKING_H
#define CUTE_NETWORKING_H

#include <cute_result.h>
#include <cute/cute_net.h>

//--------------------------------------------------------------------------------------------------
// C API

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

typedef struct cn_client_t cf_client_t;
typedef struct cn_server_t cf_server_t;
typedef struct cn_crypto_key_t cf_crypto_key_t;
typedef struct cn_crypto_sign_public_t cf_crypto_sign_public_t;
typedef struct cn_crypto_sign_secret_t cf_crypto_sign_secret_t;
typedef struct cn_crypto_signature_t cf_crypto_signature_t;

//--------------------------------------------------------------------------------------------------
// ENDPOINT

typedef struct cn_endpoint_t cf_endpoint_t;
typedef enum cn_address_type_t cf_address_type_t;
#define CUTE_ADDRESS_TYPE_NONE CN_ADDRESS_TYPE_NONE
#define CUTE_ADDRESS_TYPE_IPV4 CN_ADDRESS_TYPE_IPV4
#define CUTE_ADDRESS_TYPE_IPV6 CN_ADDRESS_TYPE_IPV6

CUTE_API int CUTE_CALL cf_endpoint_init(cf_endpoint_t* endpoint, const char* address_and_port_string);
CUTE_API void CUTE_CALL cf_endpoint_to_string(cf_endpoint_t endpoint, char* buffer, int buffer_size);
CUTE_API int CUTE_CALL cf_endpoint_equals(cf_endpoint_t a, cf_endpoint_t b);

//--------------------------------------------------------------------------------------------------
// CONNECT TOKEN

#define CUTE_CONNECT_TOKEN_SIZE 1114
#define CUTE_CONNECT_TOKEN_USER_DATA_SIZE 256

/**
 * Generates a cryptography key in a cryptographically secure way.
 */
CUTE_API cf_crypto_key_t CUTE_CALL cf_crypto_generate_key();

/**
 * Fills a buffer in a cryptographically secure way (i.e. a slow way).
 */
CUTE_API void CUTE_CALL cf_crypto_random_bytes(void* data, int byte_count);

/**
 * Generates a cryptographically secure keypair, used for facilitating connect tokens.
 */
CUTE_API void CUTE_CALL cf_crypto_sign_keygen(cf_crypto_sign_public_t* public_key, cf_crypto_sign_secret_t* secret_key);

/**
 * Generates a connect token, useable by clients to authenticate and securely connect to
 * a server. You can use this function whenever a validated client wants to join your game servers.
 *
 * It's recommended to setup a web service specifically for allowing players to authenticate
 * themselves (login). Once authenticated, the webservice can call this function and hand
 * the connect token to the client. The client can then read the public section of the
 * connect token and see the `address_list` of servers to try and connect to. The client then
 * sends the connect token to one of these servers to start the connection handshake. If the
 * handshake completes successfully, the client will connect to the server.
 *
 * The connect token is protected by an AEAD primitive (https://en.wikipedia.org/wiki/Authenticated_encryption),
 * which means the token cannot be modified or forged as long as the `shared_secret_key` is
 * not leaked. In the event your secret key is accidentally leaked, you can always roll a
 * new one and distribute it to your webservice and game servers.
 */
CUTE_API cf_result_t CUTE_CALL cf_generate_connect_token(
	uint64_t application_id,							// A unique number to identify your game, can be whatever value you like.
														// This must be the same number as in `client_create` and `server_create`.
	uint64_t creation_timestamp,						// A unix timestamp of the current time.
	const cf_crypto_key_t* client_to_server_key,		// A unique key for this connect token for the client to encrypt packets, and server to
														// decrypt packets. This can be generated with `crypto_generate_key` on your web service.
	const cf_crypto_key_t* server_to_client_key,		// A unique key for this connect token for the server to encrypt packets, and the client to
														// decrypt packets. This can be generated with `crypto_generate_key` on your web service.
	uint64_t expiration_timestamp,						// A unix timestamp for when this connect token expires and becomes invalid.
	uint32_t handshake_timeout,							// The number of seconds the connection will stay alive during the handshake process before
														// the client and server reject the handshake process as failed.
	int address_count,									// Must be from 1 to 32 (inclusive). The number of addresses in `address_list`.
	const char** address_list,							// A list of game servers the client can try connecting to, of length `address_count`.
	uint64_t client_id,									// The unique client identifier.
	const uint8_t* user_data,							// Optional buffer of data of `CUTE_PROTOCOL_CONNECT_TOKEN_USER_DATA_SIZE` (256) bytes. Can be NULL.
	const cf_crypto_sign_secret_t* shared_secret_key,	// Only your webservice and game servers know this key.
	uint8_t* token_ptr_out								// Pointer to your buffer, should be `CUTE_CONNECT_TOKEN_SIZE` bytes large.
);

//--------------------------------------------------------------------------------------------------
// CLIENT

CUTE_API cf_client_t* CUTE_CALL cf_client_create(
	uint16_t port,							// Port for opening a UDP socket.
	uint64_t application_id,				// A unique number to identify your game, can be whatever value you like.
											// This must be the same number as in `server_create`.
	bool use_ipv6 /*= false*/,				// Whether or not the socket should turn on ipv6. Some users will not have
											// ipv6 enabled, so this defaults to false.
	void* user_allocator_context /*= NULL*/ // Used for custom allocators, this can be set to NULL.
);
CUTE_API void CUTE_CALL cf_client_destroy(cf_client_t* client);

/**
 * The client will make an attempt to connect to all servers listed in the connect token, one after
 * another. If no server can be connected to the client's state will be set to an error state. Call
 * `client_state_get` to get the client's state. Once `client_connect` is called then successive calls to
 * `client_update` is expected, where `client_update` will perform the connection handshake and make
 * connection attempts to your servers.
 */
CUTE_API cf_result_t CUTE_CALL cf_client_connect(cf_client_t* client, const uint8_t* connect_token);
CUTE_API void CUTE_CALL cf_client_disconnect(cf_client_t* client);

/**
 * You should call this one per game loop after calling `client_connect`.
 */
CUTE_API void CUTE_CALL cf_client_update(cf_client_t* client, double dt, uint64_t current_time);

/**
 * Returns a packet from the server if one is available. You must free this packet when you're done by
 * calling `client_free_packet`.
 */
CUTE_API bool CUTE_CALL cf_client_pop_packet(cf_client_t* client, void** packet, int* size, bool* was_sent_reliably /*= NULL*/);
CUTE_API void CUTE_CALL cf_client_free_packet(cf_client_t* client, void* packet);

/**
 * Sends a packet to the server. If the packet size is too large (over 1k bytes) it will be split up
 * and sent in smaller chunks.
 *
 * `send_reliably` as true means the packet will be sent reliably an in-order relative to other
 * reliable packets. Under packet loss the packet will continually be sent until an acknowledgement
 * from the server is received. False means to send a typical UDP packet, with no special mechanisms
 * regarding packet loss.
 *
 * Reliable packets are significantly more expensive than unreliable packets, so try to send any data
 * that can be lost due to packet loss as an unreliable packet. Of course, some packets are required
 * to be sent, and so reliable is appropriate. As an optimization some kinds of data, such as frequent
 * transform updates, can be sent unreliably.
 */
CUTE_API cf_result_t CUTE_CALL cf_client_send(cf_client_t* client, const void* packet, int size, bool send_reliably);

typedef enum cf_client_state_t
{
	CF_CLIENT_STATE_CONNECT_TOKEN_EXPIRED         = -6,
	CF_CLIENT_STATE_INVALID_CONNECT_TOKEN         = -5,
	CF_CLIENT_STATE_CONNECTION_TIMED_OUT          = -4,
	CF_CLIENT_STATE_CHALLENGE_RESPONSE_TIMED_OUT  = -3,
	CF_CLIENT_STATE_CONNECTION_REQUEST_TIMED_OUT  = -2,
	CF_CLIENT_STATE_CONNECTION_DENIED             = -1,
	CF_CLIENT_STATE_DISCONNECTED                  = 0,
	CF_CLIENT_STATE_SENDING_CONNECTION_REQUEST    = 1,
	CF_CLIENT_STATE_SENDING_CHALLENGE_RESPONSE    = 2,
	CF_CLIENT_STATE_CONNECTED                     = 3,
} cf_client_state_t;

CUTE_API cf_client_state_t CUTE_CALL cf_client_state_get(const cf_client_t* client);
CUTE_API const char* CUTE_CALL cf_client_state_string(cf_client_state_t state);
CUTE_API float CUTE_CALL cf_client_time_of_last_packet_recieved(const cf_client_t* client);
CUTE_API void CUTE_CALL cf_client_enable_network_simulator(cf_client_t* client, double latency, double jitter, double drop_chance, double duplicate_chance);

//--------------------------------------------------------------------------------------------------
// SERVER

// Modify this value as seen fit.
#define CUTE_SERVER_MAX_CLIENTS 32

typedef struct cf_server_config_t
{
	uint64_t application_id;           // A unique number to identify your game, can be whatever value you like.
									   // This must be the same number as in `client_make`.
	int max_incoming_bytes_per_second;
	int max_outgoing_bytes_per_second;
	int connection_timeout;            // The number of seconds before consider a connection as timed out when not
									   // receiving any packets on the connection.
	double resend_rate;                // The number of seconds to wait before resending a packet that has not been
									   // acknowledge as received by a client.
	cf_crypto_sign_public_t public_key;   // The public part of your public key cryptography used for connect tokens.
									   // This can be safely shared with your players publicly.
	cf_crypto_sign_secret_t secret_key;   // The secret part of your public key cryptography used for connect tokens.
									   // This must never be shared publicly and remain a complete secret only know to your servers.
	void* user_allocator_context;
} cf_server_config_t;

CUTE_INLINE cf_server_config_t CUTE_CALL cf_server_config_defaults()
{
	cf_server_config_t config;
	config.application_id = 0;
	config.max_incoming_bytes_per_second = 0;
	config.max_outgoing_bytes_per_second = 0;
	config.connection_timeout = 10;
	config.resend_rate = 0.1f;
	return config;
}

CUTE_API cf_server_t* CUTE_CALL cf_server_create(cf_server_config_t config);
CUTE_API void CUTE_CALL cf_server_destroy(cf_server_t* server);

/**
 * Starts up the server, ready to receive new client connections.
 *
 * Please note that not all users will be able to access an ipv6 server address, so it might
 * be good to also provide a way to connect through ipv4.
 */
CUTE_API cf_result_t cf_server_start(cf_server_t* server, const char* address_and_port);
CUTE_API void cf_server_stop(cf_server_t* server);

typedef enum cf_server_event_type_t
{
	CF_SERVER_EVENT_TYPE_NEW_CONNECTION, // A new incoming connection.
	CF_SERVER_EVENT_TYPE_DISCONNECTED,   // A disconnecting client.
	CF_SERVER_EVENT_TYPE_PAYLOAD_PACKET, // An incoming packet from a client.
} cf_server_event_type_t;

typedef struct cf_server_event_t
{
	cf_server_event_type_t type;
	union
	{
		struct
		{
			int client_index;    // An index representing this particular client.
			uint64_t client_id;  // A unique identifier for this particular client, as read from the connect token.
			cf_endpoint_t endpoint; // The address and port of the incoming connection.
		} new_connection;

		struct
		{
			int client_index;    // An index representing this particular client.
		} disconnected;

		struct
		{
			int client_index;    // An index representing this particular client.
			void* data;          // Pointer to the packet's payload data. Send this back to `server_free_packet` when done.
			int size;            // Size of the packet at the data pointer.
		} payload_packet;
	} u;
} cf_server_event_t;

/**
 * Server events notify of when a client connects/disconnects, or has sent a payload packet.
 * You must free the payload packets with `server_free_packet` when done.
 */
CUTE_API bool CUTE_CALL cf_server_pop_event(cf_server_t* server, cf_server_event_t* event);
CUTE_API void CUTE_CALL cf_server_free_packet(cf_server_t* server, int client_index, void* data);

CUTE_API void CUTE_CALL cf_server_update(cf_server_t* server, double dt, uint64_t current_time);
CUTE_API void CUTE_CALL cf_server_disconnect_client(cf_server_t* server, int client_index, bool notify_client /* = true */);
CUTE_API void CUTE_CALL cf_server_send(cf_server_t* server, const void* packet, int size, int client_index, bool send_reliably);
CUTE_API void CUTE_CALL cf_server_send_to_all_clients(cf_server_t* server, const void* packet, int size, bool send_reliably);
CUTE_API void CUTE_CALL cf_server_send_to_all_but_one_client(cf_server_t* server, const void* packet, int size, int client_index, bool send_reliably);

CUTE_API bool CUTE_CALL cf_server_is_client_connected(cf_server_t* server, int client_index);
CUTE_API void CUTE_CALL cf_server_enable_network_simulator(cf_server_t* server, double latency, double jitter, double drop_chance, double duplicate_chance);

#ifdef __cplusplus
}
#endif // __cplusplus

//--------------------------------------------------------------------------------------------------
// C++ API

#ifdef CUTE_CPP

namespace cute
{
using client_t = cf_client_t;
using server_t = cf_server_t;
using crypto_key_t = cf_crypto_key_t;
using crypto_sign_public_t = cf_crypto_sign_public_t;
using crypto_sign_secret_t = cf_crypto_sign_secret_t;
using crypto_signature_t = cf_crypto_signature_t;

//--------------------------------------------------------------------------------------------------
// ENDPOINT

using endpoint_t = cf_endpoint_t;
using address_type_t = cf_address_type_t;

CUTE_INLINE int endpoint_init(endpoint_t* endpoint, const char* address_and_port_string) { return cf_endpoint_init(endpoint,address_and_port_string); }
CUTE_INLINE void endpoint_to_string(endpoint_t endpoint, char* buffer, int buffer_size) { cf_endpoint_to_string(endpoint,buffer,buffer_size); }
CUTE_INLINE int endpoint_equals(endpoint_t a, endpoint_t b) { return cf_endpoint_equals(a,b); }

//--------------------------------------------------------------------------------------------------
// CONNECT TOKEN

CUTE_INLINE crypto_key_t crypto_generate_key() { return cf_crypto_generate_key(); }
CUTE_INLINE void crypto_random_bytes(void* data, int byte_count) { cf_crypto_random_bytes(data,byte_count); }
CUTE_INLINE void crypto_sign_keygen(crypto_sign_public_t* public_key, crypto_sign_secret_t* secret_key) { cf_crypto_sign_keygen(public_key,secret_key); }
CUTE_INLINE result_t generate_connect_token(
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
	return cf_generate_connect_token(application_id,
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
}

//--------------------------------------------------------------------------------------------------
// CLIENT

using client_state_t = cf_client_state_t;

CUTE_INLINE client_t* client_create(uint16_t port, uint64_t application_id, bool use_ipv6 = false, void* user_allocator_context = NULL) { return cf_client_create(port,application_id,use_ipv6,user_allocator_context); }
CUTE_INLINE void client_destroy(client_t* client) { cf_client_destroy(client); }
CUTE_INLINE result_t client_connect(client_t* client, const uint8_t* connect_token) { return cf_client_connect(client,connect_token); }
CUTE_INLINE void client_disconnect(client_t* client) { cf_client_disconnect(client); }
CUTE_INLINE void client_update(client_t* client, double dt, uint64_t current_time) { cf_client_update(client,dt,current_time); }
CUTE_INLINE bool client_pop_packet(client_t* client, void** packet, int* size, bool* was_sent_reliably = NULL) { return cf_client_pop_packet(client,packet,size,was_sent_reliably); }
CUTE_INLINE void client_free_packet(client_t* client, void* packet) { cf_client_free_packet(client,packet); }
CUTE_INLINE result_t client_send(client_t* client, const void* packet, int size, bool send_reliably) { return cf_client_send(client,packet,size,send_reliably); }
CUTE_INLINE client_state_t client_state_get(const client_t* client) { return cf_client_state_get(client); }
CUTE_INLINE const char* client_state_string(client_state_t state) { return cf_client_state_string(state); }
CUTE_INLINE float client_time_of_last_packet_recieved(const client_t* client) { return cf_client_time_of_last_packet_recieved(client); }
CUTE_INLINE void client_enable_network_simulator(client_t* client, double latency, double jitter, double drop_chance, double duplicate_chance) { cf_client_enable_network_simulator(client,latency,jitter,drop_chance,duplicate_chance); }

//--------------------------------------------------------------------------------------------------
// SERVER

using server_config_t = cf_server_config_t;
using server_event_type_t = cf_server_event_type_t;
using server_event_t = cf_server_event_t;

CUTE_INLINE server_config_t server_config_defaults() { return cf_server_config_defaults(); }
CUTE_INLINE server_t* server_create(server_config_t config) { return cf_server_create(config); }
CUTE_INLINE void server_destroy(server_t* server) { cf_server_destroy(server); }
CUTE_INLINE result_t server_start(server_t* server, const char* address_and_port) { return cf_server_start(server,address_and_port); }
CUTE_INLINE void server_stop(server_t* server) { cf_server_stop(server); }
CUTE_INLINE bool server_pop_event(server_t* server, server_event_t* event) { return cf_server_pop_event(server,event); }
CUTE_INLINE void server_free_packet(server_t* server, int client_index, void* data) { cf_server_free_packet(server,client_index,data); }
CUTE_INLINE void server_update(server_t* server, double dt, uint64_t current_time) { cf_server_update(server,dt,current_time); }
CUTE_INLINE void server_disconnect_client(server_t* server, int client_index, bool notify_client = true) { cf_server_disconnect_client(server, client_index, notify_client); }
CUTE_INLINE void server_send(server_t* server, const void* packet, int size, int client_index, bool send_reliably) { cf_server_send(server,packet,size,client_index,send_reliably); }
CUTE_INLINE void server_send_to_all_clients(server_t* server, const void* packet, int size, bool send_reliably) { cf_server_send_to_all_clients(server,packet,size,send_reliably); }
CUTE_INLINE void server_send_to_all_but_one_client(server_t* server, const void* packet, int size, int client_index, bool send_reliably) { cf_server_send_to_all_but_one_client(server,packet,size,client_index,send_reliably); }
CUTE_INLINE bool server_is_client_connected(server_t* server, int client_index) { return cf_server_is_client_connected(server,client_index); }
CUTE_INLINE void server_enable_network_simulator(server_t* server, double latency, double jitter, double drop_chance, double duplicate_chance) { cf_server_enable_network_simulator(server,latency,jitter,drop_chance,duplicate_chance); }

}

#endif // CUTE_CPP

#endif // CUTE_NETWORKING_H
