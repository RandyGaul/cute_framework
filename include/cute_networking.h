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

#ifndef CUTE_NETWORKING_H
#define CUTE_NETWORKING_H

#include <cute_result.h>
#include <cute/cute_net.h>

//--------------------------------------------------------------------------------------------------
// C API

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

typedef struct cn_client_t CF_Client;
typedef struct cn_server_t CF_Server;
typedef struct cn_crypto_key_t CF_CryptoKey;
typedef struct cn_crypto_sign_public_t CF_CryptoSignPublic;
typedef struct cn_crypto_sign_secret_t CF_CryptoSignSecret;
typedef struct cn_crypto_signature_t CF_CryptoSignature;

//--------------------------------------------------------------------------------------------------
// ENDPOINT

typedef struct cn_endpoint_t CF_Endpoint;
typedef enum cn_address_type_t CF_AddressType;

#define CF_ADDRESS_TYPE_DEFS \
	CF_ENUM(ADDRESS_TYPE_NONE, 0) \
	CF_ENUM(ADDRESS_TYPE_IPV4, 1) \
	CF_ENUM(ADDRESS_TYPE_IPV6, 2) \

enum
{
	#define CF_ENUM(K, V) CF_##K = V,
	CF_ADDRESS_TYPE_DEFS
	#undef CF_ENUM
};

CUTE_API int CUTE_CALL cf_endpoint_init(CF_Endpoint* endpoint, const char* address_and_port_string);
CUTE_API void CUTE_CALL CF_Endpointo_string(CF_Endpoint endpoint, char* buffer, int buffer_size);
CUTE_API int CUTE_CALL cf_endpoint_equals(CF_Endpoint a, CF_Endpoint b);

//--------------------------------------------------------------------------------------------------
// CONNECT TOKEN

#define CUTE_CONNECT_TOKEN_SIZE 1114
#define CUTE_CONNECT_TOKEN_USER_DATA_SIZE 256

/**
 * Generates a cryptography key in a cryptographically secure way.
 */
CUTE_API CF_CryptoKey CUTE_CALL cf_crypto_generate_key();

/**
 * Fills a buffer in a cryptographically secure way (i.e. a slow way).
 */
CUTE_API void CUTE_CALL cf_crypto_random_bytes(void* data, int byte_count);

/**
 * Generates a cryptographically secure keypair, used for facilitating connect tokens.
 */
CUTE_API void CUTE_CALL cf_crypto_sign_keygen(CF_CryptoSignPublic* public_key, CF_CryptoSignSecret* secret_key);

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
CUTE_API CF_Result CUTE_CALL cf_generate_connect_token(
	uint64_t application_id,                           // A unique number to identify your game, can be whatever value you like.
	                                                   // This must be the same number as in `make_client` and `make_server`.
	uint64_t creation_timestamp,                       // A unix timestamp of the current time.
	const CF_CryptoKey* client_to_server_key,       // A unique key for this connect token for the client to encrypt packets, and server to
	                                                   // decrypt packets. This can be generated with `crypto_generate_key` on your web service.
	const CF_CryptoKey* server_to_client_key,       // A unique key for this connect token for the server to encrypt packets, and the client to
	                                                   // decrypt packets. This can be generated with `crypto_generate_key` on your web service.
	uint64_t expiration_timestamp,                     // A unix timestamp for when this connect token expires and becomes invalid.
	uint32_t handshake_timeout,                        // The number of seconds the connection will stay alive during the handshake process before
	                                                   // the client and server reject the handshake process as failed.
	int address_count,                                 // Must be from 1 to 32 (inclusive). The number of addresses in `address_list`.
	const char** address_list,                         // A list of game servers the client can try connecting to, of length `address_count`.
	uint64_t client_id,                                // The unique client identifier.
	const uint8_t* user_data,                          // Optional buffer of data of `CUTE_PROTOCOL_CONNECT_TOKEN_USER_DATA_SIZE` (256) bytes. Can be NULL.
	const CF_CryptoSignSecret* shared_secret_key,  // Only your webservice and game servers know this key.
	uint8_t* token_ptr_out                             // Pointer to your buffer, should be `CUTE_CONNECT_TOKEN_SIZE` bytes large.
);

//--------------------------------------------------------------------------------------------------
// CLIENT

CUTE_API CF_Client* CUTE_CALL cf_make_client(
	uint16_t port,             // Port for opening a UDP socket.
	uint64_t application_id,   // A unique number to identify your game, can be whatever value you like.
	                           // This must be the same number as in `server_create`.
	bool use_ipv6 /*= false*/  // Whether or not the socket should turn on ipv6. Some users will not have
	                           // ipv6 enabled, so this defaults to false.
);
CUTE_API void CUTE_CALL cf_destroy_client(CF_Client* client);

/**
 * The client will make an attempt to connect to all servers listed in the connect token, one after
 * another. If no server can be connected to the client's state will be set to an error state. Call
 * `client_state_get` to get the client's state. Once `client_connect` is called then successive calls to
 * `client_update` is expected, where `client_update` will perform the connection handshake and make
 * connection attempts to your servers.
 */
CUTE_API CF_Result CUTE_CALL cf_client_connect(CF_Client* client, const uint8_t* connect_token);
CUTE_API void CUTE_CALL cf_client_disconnect(CF_Client* client);

/**
 * You should call this one per game loop after calling `client_connect`.
 */
CUTE_API void CUTE_CALL cf_client_update(CF_Client* client, double dt, uint64_t current_time);

/**
 * Returns a packet from the server if one is available. You must free this packet when you're done by
 * calling `client_free_packet`.
 */
CUTE_API bool CUTE_CALL cf_client_pop_packet(CF_Client* client, void** packet, int* size, bool* was_sent_reliably /*= NULL*/);
CUTE_API void CUTE_CALL cf_client_free_packet(CF_Client* client, void* packet);

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
CUTE_API CF_Result CUTE_CALL cf_client_send(CF_Client* client, const void* packet, int size, bool send_reliably);

#define CF_CLIENT_STATE_DEFS \
	CF_ENUM(CLIENT_STATE_CONNECT_TOKEN_EXPIRED,        -6) \
	CF_ENUM(CLIENT_STATE_INVALID_CONNECT_TOKEN,        -5) \
	CF_ENUM(CLIENT_STATE_CONNECTION_TIMED_OUT,         -4) \
	CF_ENUM(CLIENT_STATE_CHALLENGE_RESPONSE_TIMED_OUT, -3) \
	CF_ENUM(CLIENT_STATE_CONNECTION_REQUEST_TIMED_OUT, -2) \
	CF_ENUM(CLIENT_STATE_CONNECTION_DENIED,            -1) \
	CF_ENUM(CLIENT_STATE_DISCONNECTED,                  0) \
	CF_ENUM(CLIENT_STATE_SENDING_CONNECTION_REQUEST,    1) \
	CF_ENUM(CLIENT_STATE_SENDING_CHALLENGE_RESPONSE,    2) \
	CF_ENUM(CLIENT_STATE_CONNECTED,                     3) \

typedef enum CF_ClientState
{
	#define CF_ENUM(K, V) CF_##K = V,
	CF_CLIENT_STATE_DEFS
	#undef CF_ENUM
} CF_ClientState;

CUTE_INLINE const char* cf_client_state_to_string(CF_ClientState state)
{
	switch (state) {
	#define CF_ENUM(K, V) case CF_##K: return CUTE_STRINGIZE(CF_##K);
	CF_CLIENT_STATE_DEFS
	#undef CF_ENUM
	default: return NULL;
	}
}

CUTE_API CF_ClientState CUTE_CALL cf_client_state_get(const CF_Client* client);
CUTE_API const char* CUTE_CALL cf_client_state_string(CF_ClientState state);
CUTE_API void CUTE_CALL cf_client_enable_network_simulator(CF_Client* client, double latency, double jitter, double drop_chance, double duplicate_chance);

//--------------------------------------------------------------------------------------------------
// SERVER

// Modify this value as seen fit.
#define CUTE_SERVER_MAX_CLIENTS 32

typedef struct CF_ServerConfig
{
	uint64_t application_id;            // A unique number to identify your game, can be whatever value you like.
	                                    // This must be the same number as in `client_make`.
	int max_incoming_bytes_per_second;
	int max_outgoing_bytes_per_second;
	int connection_timeout;             // The number of seconds before consider a connection as timed out when not
	                                    // receiving any packets on the connection.
	double resend_rate;                 // The number of seconds to wait before resending a packet that has not been
	                                    // acknowledge as received by a client.
	CF_CryptoSignPublic public_key;     // The public part of your public key cryptography used for connect tokens.
	                                    // This can be safely shared with your players publicly.
	CF_CryptoSignSecret secret_key;     // The secret part of your public key cryptography used for connect tokens.
	                                    // This must never be shared publicly and remain a complete secret only know to your servers.
	void* user_allocator_context;
} CF_ServerConfig;

CUTE_INLINE CF_ServerConfig CUTE_CALL cf_server_config_defaults()
{
	CF_ServerConfig config;
	config.application_id = 0;
	config.max_incoming_bytes_per_second = 0;
	config.max_outgoing_bytes_per_second = 0;
	config.connection_timeout = 10;
	config.resend_rate = 0.1f;
	return config;
}

CUTE_API CF_Server* CUTE_CALL cf_make_server(CF_ServerConfig config);
CUTE_API void CUTE_CALL cf_destroy_server(CF_Server* server);

/**
 * Starts up the server, ready to receive new client connections.
 *
 * Please note that not all users will be able to access an ipv6 server address, so it might
 * be good to also provide a way to connect through ipv4.
 */
CUTE_API CF_Result cf_server_start(CF_Server* server, const char* address_and_port);
CUTE_API void cf_server_stop(CF_Server* server);

#define CF_SERVER_EVENT_TYPE_DEFS \
	CF_ENUM(CF_SERVER_EVENT_TYPE_NEW_CONNECTION, 0) /* A new incoming connection. */ \
	CF_ENUM(CF_SERVER_EVENT_TYPE_DISCONNECTED,   1) /* A disconnecting client. */ \
	CF_ENUM(CF_SERVER_EVENT_TYPE_PAYLOAD_PACKET, 2) /* An incoming packet from a client. */ \

typedef enum CF_ServerEventType
{
	#define CF_ENUM(K, V) CF_##K = V,
	CF_SERVER_EVENT_TYPE_DEFS
	#undef CF_ENUM
} CF_ServerEventType;

CUTE_INLINE const char* cf_server_event_type_to_string(CF_ServerEventType type)
{
	switch (type) {
	#define CF_ENUM(K, V) case CF_##K: return CUTE_STRINGIZE(CF_##K);
	CF_SERVER_EVENT_TYPE_DEFS
	#undef CF_ENUM
	default: return NULL;
	}
}

typedef struct CF_ServerEvent
{
	CF_ServerEventType type;
	union
	{
		struct
		{
			int client_index;    // An index representing this particular client.
			uint64_t client_id;  // A unique identifier for this particular client, as read from the connect token.
			CF_Endpoint endpoint; // The address and port of the incoming connection.
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
} CF_ServerEvent;

/**
 * Server events notify of when a client connects/disconnects, or has sent a payload packet.
 * You must free the payload packets with `server_free_packet` when done.
 */
CUTE_API bool CUTE_CALL cf_server_pop_event(CF_Server* server, CF_ServerEvent* event);
CUTE_API void CUTE_CALL cf_server_free_packet(CF_Server* server, int client_index, void* data);

CUTE_API void CUTE_CALL cf_server_update(CF_Server* server, double dt, uint64_t current_time);
CUTE_API void CUTE_CALL cf_server_disconnect_client(CF_Server* server, int client_index, bool notify_client /* = true */);
CUTE_API void CUTE_CALL cf_server_send(CF_Server* server, const void* packet, int size, int client_index, bool send_reliably);

CUTE_API bool CUTE_CALL cf_server_is_client_connected(CF_Server* server, int client_index);
CUTE_API void CUTE_CALL cf_server_enable_network_simulator(CF_Server* server, double latency, double jitter, double drop_chance, double duplicate_chance);

#ifdef __cplusplus
}
#endif // __cplusplus

//--------------------------------------------------------------------------------------------------
// C++ API

#ifdef CUTE_CPP

namespace Cute
{

using Client = CF_Client;
using Server = CF_Server;
using CryptoKey = CF_CryptoKey;
using CryptoSignPublic = CF_CryptoSignPublic;
using CryptoSignSecret = CF_CryptoSignSecret;
using CryptoSignSignature = CF_CryptoSignature;

//--------------------------------------------------------------------------------------------------
// ENDPOINT

using endpoint_t = CF_Endpoint;
using address_type_t = CF_AddressType;

enum : int
{
	#define CF_ENUM(K, V) K = V,
	CF_ADDRESS_TYPE_DEFS
	#undef CF_ENUM
};

CUTE_INLINE int endpoint_init(endpoint_t* endpoint, const char* address_and_port_string) { return cf_endpoint_init(endpoint,address_and_port_string); }
CUTE_INLINE void endpoint_to_string(endpoint_t endpoint, char* buffer, int buffer_size) { CF_Endpointo_string(endpoint,buffer,buffer_size); }
CUTE_INLINE int endpoint_equals(endpoint_t a, endpoint_t b) { return cf_endpoint_equals(a,b); }

//--------------------------------------------------------------------------------------------------
// CONNECT TOKEN

CUTE_INLINE CryptoKey crypto_generate_key() { return cf_crypto_generate_key(); }
CUTE_INLINE void crypto_random_bytes(void* data, int byte_count) { cf_crypto_random_bytes(data,byte_count); }
CUTE_INLINE void crypto_sign_keygen(CryptoSignPublic* public_key, CryptoSignSecret* secret_key) { cf_crypto_sign_keygen(public_key,secret_key); }
CUTE_INLINE Result generate_connect_token(
	uint64_t application_id,
	uint64_t creation_timestamp,
	const CryptoKey* client_to_server_key,
	const CryptoKey* server_to_client_key,
	uint64_t expiration_timestamp,
	uint32_t handshake_timeout,
	int address_count,
	const char** address_list,
	uint64_t client_id,
	const uint8_t* user_data,
	const CryptoSignSecret* shared_secret_key,
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

using ClientState = CF_ClientState;
#define CF_ENUM(K, V) CUTE_INLINE constexpr ClientState K = CF_##K;
CF_CLIENT_STATE_DEFS
#undef CF_ENUM

CUTE_INLINE const char* to_string(ClientState state)
{
	switch (state) {
	#define CF_ENUM(K, V) case CF_##K: return #K;
	CF_CLIENT_STATE_DEFS
	#undef CF_ENUM
	default: return NULL;
	}
}

CUTE_INLINE Client* make_client(uint16_t port, uint64_t application_id, bool use_ipv6 = false) { return cf_make_client(port,application_id,use_ipv6); }
CUTE_INLINE void destroy_client(Client* client) { cf_destroy_client(client); }
CUTE_INLINE Result client_connect(Client* client, const uint8_t* connect_token) { return cf_client_connect(client,connect_token); }
CUTE_INLINE void client_disconnect(Client* client) { cf_client_disconnect(client); }
CUTE_INLINE void client_update(Client* client, double dt, uint64_t current_time) { cf_client_update(client,dt,current_time); }
CUTE_INLINE bool client_pop_packet(Client* client, void** packet, int* size, bool* was_sent_reliably = NULL) { return cf_client_pop_packet(client,packet,size,was_sent_reliably); }
CUTE_INLINE void client_free_packet(Client* client, void* packet) { cf_client_free_packet(client,packet); }
CUTE_INLINE Result client_send(Client* client, const void* packet, int size, bool send_reliably) { return cf_client_send(client,packet,size,send_reliably); }
CUTE_INLINE ClientState client_state_get(const Client* client) { return cf_client_state_get(client); }
CUTE_INLINE const char* client_state_string(ClientState state) { return cf_client_state_string(state); }
CUTE_INLINE void client_enable_network_simulator(Client* client, double latency, double jitter, double drop_chance, double duplicate_chance) { cf_client_enable_network_simulator(client,latency,jitter,drop_chance,duplicate_chance); }

//--------------------------------------------------------------------------------------------------
// SERVER

using ServerConfig = CF_ServerConfig;
using ServerEvent = CF_ServerEvent;

using ServerEventType = CF_ServerEventType;
#define CF_ENUM(K, V) CUTE_INLINE constexpr ServerEventType K = CF_##K;
CF_SERVER_EVENT_TYPE_DEFS
#undef CF_ENUM

CUTE_INLINE const char* to_string(ServerEventType type)
{
	switch (type) {
	#define CF_ENUM(K, V) case CF_##K: return #K;
	CF_SERVER_EVENT_TYPE_DEFS
	#undef CF_ENUM
	default: return NULL;
	}
}

CUTE_INLINE ServerConfig server_config_defaults() { return cf_server_config_defaults(); }
CUTE_INLINE Server* make_server(ServerConfig config) { return cf_make_server(config); }
CUTE_INLINE void destroy_server(Server* server) { cf_destroy_server(server); }
CUTE_INLINE Result server_start(Server* server, const char* address_and_port) { return cf_server_start(server,address_and_port); }
CUTE_INLINE void server_stop(Server* server) { cf_server_stop(server); }
CUTE_INLINE bool server_pop_event(Server* server, ServerEvent* event) { return cf_server_pop_event(server,event); }
CUTE_INLINE void server_free_packet(Server* server, int client_index, void* data) { cf_server_free_packet(server,client_index,data); }
CUTE_INLINE void server_update(Server* server, double dt, uint64_t current_time) { cf_server_update(server,dt,current_time); }
CUTE_INLINE void server_disconnect_client(Server* server, int client_index, bool notify_client = true) { cf_server_disconnect_client(server, client_index, notify_client); }
CUTE_INLINE void server_send(Server* server, const void* packet, int size, int client_index, bool send_reliably) { cf_server_send(server,packet,size,client_index,send_reliably); }
CUTE_INLINE bool server_is_client_connected(Server* server, int client_index) { return cf_server_is_client_connected(server,client_index); }
CUTE_INLINE void server_enable_network_simulator(Server* server, double latency, double jitter, double drop_chance, double duplicate_chance) { cf_server_enable_network_simulator(server,latency,jitter,drop_chance,duplicate_chance); }

}

#endif // CUTE_CPP

#endif // CUTE_NETWORKING_H
