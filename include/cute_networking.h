/*
	Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

#ifndef CF_NETWORKING_H
#define CF_NETWORKING_H

#include <cute_result.h>
#include <cute/cute_net.h>

//--------------------------------------------------------------------------------------------------
// C API

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

/**
 * @struct   CF_Client
 * @category net
 * @brief    An opaque pointer representing a single networked client.
 * @related  CF_Client CF_Server cf_make_client
 */
typedef struct cn_client_t CF_Client;
// @end

/**
 * @struct   CF_Server
 * @category net
 * @brief    An opaque pointer representing a single networked server.
 * @related  CF_Client CF_Server cf_make_client
 */
typedef struct cn_server_t CF_Server;
// @end

/**
 * @struct   CF_CryptoKey
 * @category net
 * @brief    A chunk of bytes representing a cryptographically secure key.
 * @related  CF_CryptoKey cf_crypto_generate_key cf_generate_connect_token
 */
typedef struct cn_crypto_key_t CF_CryptoKey;
// @end

/**
 * @struct   CF_CryptoSignPublic
 * @category net
 * @brief    One-half of a cryptographically secure keypair. This key can be freely shared to the public.
 * @related  CF_CryptoKey CF_CryptoSignPublic CF_CryptoSignSecret cf_crypto_sign_keygen CF_ServerConfig
 */
typedef struct cn_crypto_sign_public_t CF_CryptoSignPublic;
// @end

/**
 * @struct   CF_CryptoSignSecret
 * @category net
 * @brief    One-half of a cryptographically secure keypair. This key must be kept secret and hidden with your servers.
 * @related  CF_CryptoKey CF_CryptoSignPublic CF_CryptoSignSecret cf_crypto_sign_keygen CF_ServerConfig
 */
typedef struct cn_crypto_sign_secret_t CF_CryptoSignSecret;
// @end

//--------------------------------------------------------------------------------------------------
// ENDPOINT

/**
 * @struct   CF_Address
 * @category net
 * @brief    A network address.
 * @related  CF_Address CF_AddressType cf_address_init
 */
typedef struct cn_endpoint_t CF_Address;
// @end

typedef enum cn_address_type_t CF_AddressType;

/**
 * @enum     Address Type
 * @category net
 * @brief    Available types of endpoints.
 * @related  CF_Address
 */
#define CF_ADDRESS_TYPE_DEFS \
	/* @entry */ \
	CF_ENUM(ADDRESS_TYPE_NONE, 0) \
	/* @entry */ \
	CF_ENUM(ADDRESS_TYPE_IPV4, 1) \
	/* @entry */ \
	CF_ENUM(ADDRESS_TYPE_IPV6, 2) \
	/* @end */

enum
{
	#define CF_ENUM(K, V) CF_##K = V,
	CF_ADDRESS_TYPE_DEFS
	#undef CF_ENUM
};

/**
 * @function cf_address_init
 * @category net
 * @brief    Initialze a `CF_Address` from a C string.
 * @return   Returns 0 on success, -1 on failure.
 * @related  CF_Address cf_address_init cf_address_to_string cf_address_equals
 */
CF_API int CF_CALL cf_address_init(CF_Address* endpoint, const char* address_and_port_string);

/**
 * @function cf_address_to_string
 * @category net
 * @brief    Converts a `CF_Address` to a C string.
 * @related  CF_Address cf_address_init cf_address_to_string cf_address_equals
 */
CF_API void CF_CALL cf_address_to_string(CF_Address endpoint, char* buffer, int buffer_size);

/**
 * @function cf_address_equals
 * @category net
 * @brief    Tests two endpoints for equality.
 * @related  CF_Address cf_address_init cf_address_to_string cf_address_equals
 */
CF_API int CF_CALL cf_address_equals(CF_Address a, CF_Address b);

//--------------------------------------------------------------------------------------------------
// CONNECT TOKEN

/**
 * @function CF_CONNECT_TOKEN_SIZE
 * @category net
 * @brief    The size of a single connect token.
 * @related  CF_CONNECT_TOKEN_SIZE CF_CONNECT_TOKEN_USER_DATA_SIZE cf_generate_connect_token cf_client_connect
 */
#define CF_CONNECT_TOKEN_SIZE 1114

/**
 * @function CF_CONNECT_TOKEN_USER_DATA_SIZE
 * @category net
 * @brief    The size of the user data section of a connect token.
 * @related  CF_CONNECT_TOKEN_SIZE CF_CONNECT_TOKEN_USER_DATA_SIZE cf_generate_connect_token cf_client_connect
 */
#define CF_CONNECT_TOKEN_USER_DATA_SIZE 256

/**
 * @function cf_crypto_generate_key
 * @category net
 * @brief    Returns a cryptography key in a cryptographically secure way.
 * @related  CF_CryptoKey cf_crypto_generate_key cf_generate_connect_token
 */
CF_API CF_CryptoKey CF_CALL cf_crypto_generate_key();

/**
 * @function cf_crypto_random_bytes
 * @category net
 * @brief    Fills a buffer in a cryptographically secure way (i.e. a slow way).
 */
CF_API void CF_CALL cf_crypto_random_bytes(void* data, int byte_count);

/**
 * @function cf_crypto_sign_keygen
 * @category net
 * @brief    Generates a cryptographically secure keypair, used for facilitating connect tokens.
 * @param    public_key     The public key of the keypair. Freely share this publicy.
 * @param    secret_key     The secret key of the keypair. Keep this safe and hidden within your servers.
 * @related  CF_CryptoKey cf_crypto_generate_key cf_generate_connect_token
 */
CF_API void CF_CALL cf_crypto_sign_keygen(CF_CryptoSignPublic* public_key, CF_CryptoSignSecret* secret_key);

/**
 * @function cf_generate_connect_token
 * @category net
 * @brief    Generates a connect token, useable by clients to authenticate and securely connect to a server.
 * @param    application_id        A unique number to identify your game, can be whatever value you like.
 *                                 This must be the same number as in `cf_make_client` and `cf_make_server`.
 * @param    creation_timestamp    A unix timestamp of the current time.
 * @param    client_to_server_key  A unique key for this connect token for the client to encrypt packets, and server to
 *                                 decrypt packets. This can be generated with `cf_crypto_generate_key` on your web service.
 * @param    server_to_client_key  A unique key for this connect token for the server to encrypt packets, and the client to
 *                                 decrypt packets. This can be generated with `cf_crypto_generate_key` on your web service.
 * @param    expiration_timestamp  A unix timestamp for when this connect token expires and becomes invalid.
 * @param    handshake_timeout     The number of seconds the connection will stay alive during the handshake process before
 *                                 the client and server reject the handshake process as failed.
 * @param    address_count         Must be from 1 to 32 (inclusive). The number of addresses in `address_list`.
 * @param    address_list          A list of game servers the client can try connecting to, of length `address_count`.
 * @param    client_id             The unique client identifier (you pick this).
 * @param    user_data             Can be `NULL`. Optional buffer of data of `CF_PROTOCOL_CONNECT_TOKEN_USER_DATA_SIZE` (256) bytes.
 * @param    shared_secret_key     Only your webservice and game servers know this key.
 * @param    token_ptr_out         Pointer to your buffer, should be `CF_CONNECT_TOKEN_SIZE` bytes large.
 * @return   Returns any errors as `CF_Result`.
 * @remarks  You can use this function whenever a validated client wants to join your game servers.
 *           
 *           It's recommended to setup a web service specifically for allowing players to authenticate
 *           themselves (login). Once authenticated, the webservice can call this function and hand
 *           the connect token to the client. The client can then read the public section of the
 *           connect token and see the `address_list` of servers to try and connect to. The client then
 *           sends the connect token to one of these servers to start the connection handshake. If the
 *           handshake completes successfully, the client will connect to the server.
 *           
 *           The connect token is protected by an AEAD primitive (https://en.wikipedia.org/wiki/Authenticated_encryption),
 *           which means the token cannot be modified or forged as long as the `cf_shared_secret_key` is
 *           not leaked. In the event your secret key is accidentally leaked, you can always roll a
 *           new one and distribute it to your webservice and game servers.
 * @related  CF_CryptoKey cf_crypto_generate_key cf_generate_connect_token cf_client_connect
 */
CF_API CF_Result CF_CALL cf_generate_connect_token(uint64_t application_id, uint64_t creation_timestamp, const CF_CryptoKey* client_to_server_key, const CF_CryptoKey* server_to_client_key, uint64_t expiration_timestamp, uint32_t handshake_timeout, int address_count, const char** address_list, uint64_t client_id, const uint8_t* user_data, const CF_CryptoSignSecret* shared_secret_key, uint8_t* token_ptr_out);

//--------------------------------------------------------------------------------------------------
// CLIENT

/**
 * @function cf_make_client
 * @category net
 * @brief    Returns a new client.
 * @param    port            Port for opening a UDP socket.
 * @param    application_id  A unique number to identify your game, can be whatever value you like. This must be the same number as in `cf_server_create`.
 * @param    use_ipv6        Whether or not the socket should turn on ipv6. Some users will not have ipv6 enabled, so consider setting to `false`.
 * @related  CF_Client cf_make_client cf_destroy_client cf_client_connect cf_generate_connect_token
 */
CF_API CF_Client* CF_CALL cf_make_client(uint16_t port, uint64_t application_id, bool use_ipv6);

/**
 * @function cf_destroy_client
 * @category net
 * @brief    Destroys a client created by `cf_make_client`.
 * @remarks  Does not send out any disconnect packets. Call `cf_client_disconnect` first.
 * @related  CF_Client cf_make_client cf_destroy_client cf_client_connect cf_client_disconnect
 */
CF_API void CF_CALL cf_destroy_client(CF_Client* client);

/**
 * @function cf_client_connect
 * @category net
 * @brief    Attempts to connect the `CF_Client` to a `CF_Server`.
 * @return   Returns any errors as a `CF_Result`.
 * @remarks  The client will make an attempt to connect to all servers listed in the connect token, one after
 *           another. If no server can be connected to the client's state will be set to an error state. Call
 *           `cf_client_state_get` to get the client's state. Once `cf_client_connect` is called then successive calls to
 *           `cf_client_update` is expected, where `cf_client_update` will perform the connection handshake and make
 *           connection attempts to your servers.
 * @related  CF_Client cf_make_client cf_destroy_client cf_client_connect cf_client_disconnect cf_client_update
 */
CF_API CF_Result CF_CALL cf_client_connect(CF_Client* client, const uint8_t* connect_token);

/**
 * @function cf_client_disconnect
 * @category net
 * @brief    Attempts to gracefully disconnect a `CF_Client` from a `CF_Server`.
 * @related  CF_Client cf_make_client cf_destroy_client cf_client_connect cf_client_disconnect cf_client_update
 */
CF_API void CF_CALL cf_client_disconnect(CF_Client* client);

/**
 * @function cf_client_update
 * @category net
 * @brief    Updates the client.
 * @remarks  You should call this one per game loop after calling `cf_client_connect`.
 * @related  CF_Client cf_make_client cf_destroy_client cf_client_connect cf_client_disconnect cf_client_update
 */
CF_API void CF_CALL cf_client_update(CF_Client* client, double dt, uint64_t current_time);

/**
 * @function cf_client_pop_packet
 * @category net
 * @brief    Returns a packet from the server, if available.
 * @param    client             The client.
 * @param    packet             A pointer to the packet will be written here.
 * @param    size               The size of `packet` will be written here, in bytes.
 * @param    was_sent_reliably  `true` if the packet was a reliable packet.
 * @return   Returns `true` if a packet was popped.
 * @remarks  You must free this packet when you're done by calling `cf_client_free_packet`.
 * @related  CF_Client cf_client_pop_packet cf_client_free_packet cf_client_send
 */
CF_API bool CF_CALL cf_client_pop_packet(CF_Client* client, void** packet, int* size, bool* was_sent_reliably);

/**
 * @function cf_client_free_packet
 * @category net
 * @brief    Free's a packet created by `cf_client_pop_packet`.
 * @related  CF_Client cf_client_pop_packet cf_client_free_packet cf_client_send
 */
CF_API void CF_CALL cf_client_free_packet(CF_Client* client, void* packet);

/**
 * @function cf_client_send
 * @category net
 * @brief    Sends a packet to the server.
 * @param    client             The client.
 * @param    packet             The packet.
 * @param    size               The size of `packet` in bytes.
 * @param    send_reliably      If `true` the packet will be sent reliably and in order. If false, the packet will be sent just once, and it
 *                              may arrive out of order, or not at all.
 * @return   Returns any errors as a `CF_Result`.
 * @remarks  If the packet size is too large (over 1k bytes) it will be split up and sent in smaller chunks.
 *           
 *           `send_reliably` as true means the packet will be sent reliably an in-order relative to other
 *           reliable packets. Under packet loss the packet will continually be sent until an acknowledgement
 *           from the server is received. False means to send a typical UDP packet, with no special mechanisms
 *           regarding packet loss.
 *           
 *           Reliable packets are significantly more expensive than unreliable packets, so try to send any data
 *           that can be lost due to packet loss as an unreliable packet. Of course, some packets are required
 *           to be sent, and so reliable is appropriate. As an optimization some kinds of data, such as frequent
 *           transform updates, can be sent unreliably.
 * @related  CF_Client cf_client_pop_packet cf_client_free_packet cf_client_send
 */
CF_API CF_Result CF_CALL cf_client_send(CF_Client* client, const void* packet, int size, bool send_reliably);

/**
 * @enum     CF_ClientState
 * @category net
 * @brief    The various states of a `CF_Client`.
 * @remarks  Anything less than or equal to 0 is an error.
 * @related  CF_ClientState cf_client_state_to_string cf_client_state_get
 */
#define CF_CLIENT_STATE_DEFS \
	/* @entry */ \
	CF_ENUM(CLIENT_STATE_CONNECT_TOKEN_EXPIRED,        -6) \
	/* @entry */ \
	CF_ENUM(CLIENT_STATE_INVALID_CONNECT_TOKEN,        -5) \
	/* @entry */ \
	CF_ENUM(CLIENT_STATE_CONNECTION_TIMED_OUT,         -4) \
	/* @entry */ \
	CF_ENUM(CLIENT_STATE_CHALLENGE_RESPONSE_TIMED_OUT, -3) \
	/* @entry */ \
	CF_ENUM(CLIENT_STATE_CONNECTION_REQUEST_TIMED_OUT, -2) \
	/* @entry */ \
	CF_ENUM(CLIENT_STATE_CONNECTION_DENIED,            -1) \
	/* @entry */ \
	CF_ENUM(CLIENT_STATE_DISCONNECTED,                  0) \
	/* @entry */ \
	CF_ENUM(CLIENT_STATE_SENDING_CONNECTION_REQUEST,    1) \
	/* @entry */ \
	CF_ENUM(CLIENT_STATE_SENDING_CHALLENGE_RESPONSE,    2) \
	/* @entry */ \
	CF_ENUM(CLIENT_STATE_CONNECTED,                     3) \
	/* @end */

typedef enum CF_ClientState
{
	#define CF_ENUM(K, V) CF_##K = V,
	CF_CLIENT_STATE_DEFS
	#undef CF_ENUM
} CF_ClientState;

/**
 * @function cf_client_state_to_string
 * @category net
 * @brief    Convert an enum `CF_ClientState` to a c-style string.
 * @param    state        The state to convert to a string.
 * @related  CF_ClientState cf_client_state_to_string cf_client_state_get
 */
CF_INLINE const char* cf_client_state_to_string(CF_ClientState state)
{
	switch (state) {
	#define CF_ENUM(K, V) case CF_##K: return CF_STRINGIZE(CF_##K);
	CF_CLIENT_STATE_DEFS
	#undef CF_ENUM
	default: return NULL;
	}
}

/**
 * @function cf_client_state_get
 * @category net
 * @brief    Returns the `CF_ClientState` of a `CF_Client`.
 * @related  CF_ClientState cf_client_state_to_string cf_client_state_get
 */
CF_API CF_ClientState CF_CALL cf_client_state_get(const CF_Client* client);

/**
 * @function cf_client_enable_network_simulator
 * @category net
 * @brief    Turns on the network simulator for a client.
 * @param    client           The client.
 * @param    latency          A number of seconds of latency to add to the connection.
 * @param    jitter           The variability of latency.
 * @param    drop_chance      Number from [0,1]. 0 means drop no packets, 1 means drop all packets, 0.5f means 50% packet loss.
 * @param    duplicate_chance Number from [0,1] representing the chance to duplicate a packet, where 1 is 100% chance.
 * @related  CF_Client
 */
CF_API void CF_CALL cf_client_enable_network_simulator(CF_Client* client, double latency, double jitter, double drop_chance, double duplicate_chance);

//--------------------------------------------------------------------------------------------------
// SERVER

// Modify this value as seen fit.
#define CF_SERVER_MAX_CLIENTS 32

/**
 * @struct   CF_ServerConfig
 * @category net
 * @brief    Parameters for calling `cf_make_server`.
 * @remarks  Call `cf_server_config_defaults` to get a good set of default parameters.
 * @related  CF_ServerConfig cf_server_config_defaults cf_make_server
 */
typedef struct CF_ServerConfig
{
	/* @member A unique number to identify your game, can be whatever value you like. This must be the same number as in `client_make`. */
	uint64_t application_id;

	/* @member Not implemented yet. */
	int max_incoming_bytes_per_second;

	/* @member Not implemented yet. */
	int max_outgoing_bytes_per_second;

	/* @member The number of seconds before consider a connection as timed out when not receiving any packets on the connection. */
	int connection_timeout;

	/* @member The number of seconds to wait before resending a packet that has not been acknowledge as received by a client. */
	double resend_rate;

	/* @member The public part of your public key cryptography used for connect tokens. This can be safely shared with your players publicly. See `CF_CryptoSignPublic`. */
	CF_CryptoSignPublic public_key;

	/* @member The secret part of your public key cryptography used for connect tokens. This must never be shared publicly and remain a complete secret only know to your servers. See `CF_CryptoSignSecret`. */
	CF_CryptoSignSecret secret_key;
} CF_ServerConfig;
// @end

/**
 * @function cf_server_config_defaults
 * @category net
 * @brief    Returns a good set of default parameters for `cf_make_server`.
 * @related  CF_ServerConfig cf_server_config_defaults cf_make_server
 */
CF_INLINE CF_ServerConfig CF_CALL cf_server_config_defaults()
{
	CF_ServerConfig config;
	config.application_id = 0;
	config.max_incoming_bytes_per_second = 0;
	config.max_outgoing_bytes_per_second = 0;
	config.connection_timeout = 10;
	config.resend_rate = 0.1f;
	return config;
}

/**
 * @function cf_make_server
 * @category net
 * @brief    Returns a new `CF_Server`.
 * @param    config      The server settings `CF_ServerConfig`.
 * @related  CF_ServerConfig cf_server_config_defaults cf_make_server cf_destroy_server cf_server_start cf_server_update
 */
CF_API CF_Server* CF_CALL cf_make_server(CF_ServerConfig config);

/**
 * @function cf_destroy_server
 * @category net
 * @brief    Destroys a `CF_Server` created by `cf_make_server`.
 * @related  CF_ServerConfig cf_server_config_defaults cf_make_server cf_destroy_server cf_server_start cf_server_update
 */
CF_API void CF_CALL cf_destroy_server(CF_Server* server);

/**
 * @function cf_server_start
 * @category net
 * @brief    Starts up the server connection, ready to receive new client connections.
 * @param    address_and_port  The address and port combo to start the server upon.
 * @remarks  Please note that not all users will be able to access an ipv6 server address, so it might be good to also provide a way to connect through ipv4.
 * @related  CF_ServerConfig cf_server_config_defaults cf_make_server cf_destroy_server cf_server_start cf_server_update
 */
CF_API CF_Result cf_server_start(CF_Server* server, const char* address_and_port);

/**
 * @function cf_server_stop
 * @category net
 * @brief    Stops the server.
 * @related  CF_ServerConfig cf_server_config_defaults cf_make_server cf_destroy_server cf_server_start cf_server_update
 */
CF_API void cf_server_stop(CF_Server* server);

/**
 * @enum     CF_ServerEventType
 * @category net
 * @brief    The various possible `CF_ServerEvent` types.
 * @related  CF_ServerEventType cf_server_event_type_to_string CF_ServerEvent cf_server_pop_event
 */
#define CF_SERVER_EVENT_TYPE_DEFS \
	/* @entry A new incoming connection. */         \
	CF_ENUM(SERVER_EVENT_TYPE_NEW_CONNECTION, 0) \
	/* @entry A disconnecting client. */            \
	CF_ENUM(SERVER_EVENT_TYPE_DISCONNECTED,   1) \
	/* @entry An incoming packet from a client. */  \
	CF_ENUM(SERVER_EVENT_TYPE_PAYLOAD_PACKET, 2) \
	/* @end */

typedef enum CF_ServerEventType
{
	#define CF_ENUM(K, V) CF_##K = V,
	CF_SERVER_EVENT_TYPE_DEFS
	#undef CF_ENUM
} CF_ServerEventType;

/**
 * @function cf_server_event_type_to_string
 * @category net
 * @brief    Convert an enum `CF_ServerEventType` to a c-style string.
 * @param    state        The state to convert to a string.
 * @related  CF_ServerEventType cf_server_event_type_to_string CF_ServerEvent cf_server_pop_event
 */
CF_INLINE const char* cf_server_event_type_to_string(CF_ServerEventType type)
{
	switch (type) {
	#define CF_ENUM(K, V) case CF_##K: return CF_STRINGIZE(CF_##K);
	CF_SERVER_EVENT_TYPE_DEFS
	#undef CF_ENUM
	default: return NULL;
	}
}

/**
 * @struct   CF_ServerEvent
 * @category net
 * @brief    An event from the server, likely a client payload packet.
 * @related  CF_ServerEvent CF_ServerEvent cf_server_update cf_server_pop_event cf_server_free_packet
 */
typedef struct CF_ServerEvent
{
	/* @member The type of the server event. See `CF_ServerEventType`. */
	CF_ServerEventType type;
	union
	{
		struct
		{
			/* @member An index representing this particular client. */
			int client_index;

			/* @member A unique identifier for this particular client, as read from the connect token. */
			uint64_t client_id;

			/* @member The address and port of the incoming connection. */
			CF_Address endpoint;
		} new_connection;

		struct
		{
			/* @member An index representing this particular client. */
			int client_index;
		} disconnected;

		struct
		{
			/* @member An index representing this particular client. */
			int client_index;

			/* @member Pointer to the packet's payload data. Send this back to cf_`server_free_packet` when done. */
			void* data;

			/* @member Size of the packet at the data pointer. */
			int size;
		} payload_packet;
	} u;
} CF_ServerEvent;
// @end

/**
 * @function cf_server_pop_event
 * @category net
 * @brief    Pops a `CF_ServerEvent` off of the server, if available.
 * @return   Returns true if an event was popped.
 * @remarks  Server events notify of when a client connects/disconnects, or has sent a payload packet.
 *           You must free the payload packets with `cf_server_free_packet` when done.
 * @related  CF_ServerEventType cf_server_event_type_to_string CF_ServerEvent cf_server_pop_event cf_server_update cf_server_send
 */
CF_API bool CF_CALL cf_server_pop_event(CF_Server* server, CF_ServerEvent* event);

/**
 * @function cf_server_free_packet
 * @category net
 * @brief    Frees a payload packet from a `CF_ServerEvent`.
 * @related  CF_ServerEventType cf_server_event_type_to_string CF_ServerEvent cf_server_pop_event
 */
CF_API void CF_CALL cf_server_free_packet(CF_Server* server, int client_index, void* data);

/**
 * @function cf_server_update
 * @category net
 * @brief    Updates the server.
 * @remarks  Call this once per game tick.
 * @related  cf_server_update CF_ServerEvent cf_server_pop_event
 */
CF_API void CF_CALL cf_server_update(CF_Server* server, double dt, uint64_t current_time);

/**
 * @function cf_server_disconnect_client
 * @category net
 * @brief    Disconnects a client from the server.
 * @related  cf_server_update CF_ServerEvent cf_server_pop_event cf_server_send
 */
CF_API void CF_CALL cf_server_disconnect_client(CF_Server* server, int client_index, bool notify_client /* = true */);

/**
 * @function cf_server_send
 * @category net
 * @brief    Sends a packet to a client.
 * @param    server         The server.
 * @param    packet         Data to send.
 * @param    size           Size of `data` in bytes.
 * @param    client_index   An index representing a particular client, from `CF_ServerEvent`.
 * @param    send_reliably  If `true` the packet will be sent reliably and in order. If false the packet will be sent just once, and may
 *                          arrive out of order or not at all.
 * @related  cf_server_update CF_ServerEvent cf_server_pop_event cf_server_send
 */
CF_API void CF_CALL cf_server_send(CF_Server* server, const void* packet, int size, int client_index, bool send_reliably);

/**
 * @function cf_server_is_client_connected
 * @category net
 * @brief    Returns true if a client is still connected.
 * @related  cf_server_update CF_ServerEvent cf_server_pop_event cf_server_send
 */
CF_API bool CF_CALL cf_server_is_client_connected(CF_Server* server, int client_index);

/**
 * @function cf_server_enable_network_simulator
 * @category net
 * @brief    Turns on the network simulator for a client.
 * @param    server           The server.
 * @param    latency          A number of seconds of latency to add to the connection.
 * @param    jitter           The variability of latency.
 * @param    drop_chance      Number from [0,1]. 0 means drop no packets, 1 means drop all packets, 0.5f means 50% packet loss.
 * @param    duplicate_chance Number from [0,1] representing the chance to duplicate a packet, where 1 is 100% chance.
 * @related  CF_Server
 */
CF_API void CF_CALL cf_server_enable_network_simulator(CF_Server* server, double latency, double jitter, double drop_chance, double duplicate_chance);

#ifdef __cplusplus
}
#endif // __cplusplus

//--------------------------------------------------------------------------------------------------
// C++ API

#ifdef CF_CPP

namespace Cute
{

using Client = CF_Client;
using Server = CF_Server;
using CryptoKey = CF_CryptoKey;
using CryptoSignPublic = CF_CryptoSignPublic;
using CryptoSignSecret = CF_CryptoSignSecret;

//--------------------------------------------------------------------------------------------------
// ENDPOINT

using Address = CF_Address;
using AddressType = CF_AddressType;

enum : int
{
	#define CF_ENUM(K, V) K = V,
	CF_ADDRESS_TYPE_DEFS
	#undef CF_ENUM
};

CF_INLINE int address_init(Address* endpoint, const char* address_and_port_string) { return cf_address_init(endpoint,address_and_port_string); }
CF_INLINE void address_to_string(Address endpoint, char* buffer, int buffer_size) { cf_address_to_string(endpoint,buffer,buffer_size); }
CF_INLINE int address_equals(Address a, Address b) { return cf_address_equals(a,b); }

//--------------------------------------------------------------------------------------------------
// CONNECT TOKEN

CF_INLINE CryptoKey crypto_generate_key() { return cf_crypto_generate_key(); }
CF_INLINE void crypto_random_bytes(void* data, int byte_count) { cf_crypto_random_bytes(data,byte_count); }
CF_INLINE void crypto_sign_keygen(CryptoSignPublic* public_key, CryptoSignSecret* secret_key) { cf_crypto_sign_keygen(public_key,secret_key); }
CF_INLINE Result generate_connect_token(
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
#define CF_ENUM(K, V) CF_INLINE constexpr ClientState K = CF_##K;
CF_CLIENT_STATE_DEFS
#undef CF_ENUM

CF_INLINE const char* to_string(ClientState state)
{
	switch (state) {
	#define CF_ENUM(K, V) case CF_##K: return #K;
	CF_CLIENT_STATE_DEFS
	#undef CF_ENUM
	default: return NULL;
	}
}

CF_INLINE Client* make_client(uint16_t port, uint64_t application_id, bool use_ipv6 = false) { return cf_make_client(port,application_id,use_ipv6); }
CF_INLINE void destroy_client(Client* client) { cf_destroy_client(client); }
CF_INLINE Result client_connect(Client* client, const uint8_t* connect_token) { return cf_client_connect(client,connect_token); }
CF_INLINE void client_disconnect(Client* client) { cf_client_disconnect(client); }
CF_INLINE void client_update(Client* client, double dt, uint64_t current_time) { cf_client_update(client,dt,current_time); }
CF_INLINE bool client_pop_packet(Client* client, void** packet, int* size, bool* was_sent_reliably = NULL) { return cf_client_pop_packet(client,packet,size,was_sent_reliably); }
CF_INLINE void client_free_packet(Client* client, void* packet) { cf_client_free_packet(client,packet); }
CF_INLINE Result client_send(Client* client, const void* packet, int size, bool send_reliably) { return cf_client_send(client,packet,size,send_reliably); }
CF_INLINE ClientState client_state_get(const Client* client) { return cf_client_state_get(client); }
CF_INLINE void client_enable_network_simulator(Client* client, double latency, double jitter, double drop_chance, double duplicate_chance) { cf_client_enable_network_simulator(client,latency,jitter,drop_chance,duplicate_chance); }

//--------------------------------------------------------------------------------------------------
// SERVER

using ServerConfig = CF_ServerConfig;
using ServerEvent = CF_ServerEvent;

using ServerEventType = CF_ServerEventType;
#define CF_ENUM(K, V) CF_INLINE constexpr ServerEventType K = CF_##K;
CF_SERVER_EVENT_TYPE_DEFS
#undef CF_ENUM

CF_INLINE const char* to_string(ServerEventType type)
{
	switch (type) {
	#define CF_ENUM(K, V) case CF_##K: return #K;
	CF_SERVER_EVENT_TYPE_DEFS
	#undef CF_ENUM
	default: return NULL;
	}
}

CF_INLINE ServerConfig server_config_defaults() { return cf_server_config_defaults(); }
CF_INLINE Server* make_server(ServerConfig config) { return cf_make_server(config); }
CF_INLINE void destroy_server(Server* server) { cf_destroy_server(server); }
CF_INLINE Result server_start(Server* server, const char* address_and_port) { return cf_server_start(server,address_and_port); }
CF_INLINE void server_stop(Server* server) { cf_server_stop(server); }
CF_INLINE bool server_pop_event(Server* server, ServerEvent* event) { return cf_server_pop_event(server,event); }
CF_INLINE void server_free_packet(Server* server, int client_index, void* data) { cf_server_free_packet(server,client_index,data); }
CF_INLINE void server_update(Server* server, double dt, uint64_t current_time) { cf_server_update(server,dt,current_time); }
CF_INLINE void server_disconnect_client(Server* server, int client_index, bool notify_client = true) { cf_server_disconnect_client(server, client_index, notify_client); }
CF_INLINE void server_send(Server* server, const void* packet, int size, int client_index, bool send_reliably) { cf_server_send(server,packet,size,client_index,send_reliably); }
CF_INLINE bool server_is_client_connected(Server* server, int client_index) { return cf_server_is_client_connected(server,client_index); }
CF_INLINE void server_enable_network_simulator(Server* server, double latency, double jitter, double drop_chance, double duplicate_chance) { cf_server_enable_network_simulator(server,latency,jitter,drop_chance,duplicate_chance); }

}

#endif // CF_CPP

#endif // CF_NETWORKING_H
