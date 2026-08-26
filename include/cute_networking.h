/*
	Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

#ifndef CF_NETWORKING_H
#define CF_NETWORKING_H

#include "cute_defines.h"

// Web builds get this whole API too. Browsers have no UDP, so a web client's traffic rides a
// CF_ClientWire (see cf_client_web_wire) to a relay; servers, whose sockets cannot open in a
// browser, fail cleanly at cf_server_start.

#include "cute_result.h"

// CF owns its networking API surface outright: the underlying cute_net library is an
// implementation detail hidden inside cute_networking.cpp, never exposed here. The types below are
// CF-defined with layouts that match cute_net's, and the source file static-asserts that match so
// the two never drift. Keeping cute_net behind this wall is what lets CF add conveniences (stats,
// broadcast) and, later, a packet compression / delta-encoding layer without leaking a second API.

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
typedef struct CF_Client CF_Client;
// @end

/**
 * @struct   CF_Server
 * @category net
 * @brief    An opaque pointer representing a single networked server.
 * @related  CF_Client CF_Server cf_make_client
 */
typedef struct CF_Server CF_Server;
// @end

/**
 * @struct   CF_CryptoKey
 * @category net
 * @brief    A chunk of bytes representing a cryptographically secure key.
 * @related  CF_CryptoKey cf_crypto_generate_key cf_generate_connect_token
 */
typedef struct CF_CryptoKey { uint8_t bytes[32]; } CF_CryptoKey;
// @end

/**
 * @struct   CF_CryptoSignPublic
 * @category net
 * @brief    One-half of a cryptographically secure keypair. This key can be freely shared to the public.
 * @related  CF_CryptoKey CF_CryptoSignPublic CF_CryptoSignSecret cf_crypto_sign_keygen CF_ServerConfig
 */
typedef struct CF_CryptoSignPublic { uint8_t bytes[32]; } CF_CryptoSignPublic;
// @end

/**
 * @struct   CF_CryptoSignSecret
 * @category net
 * @brief    One-half of a cryptographically secure keypair. This key must be kept secret and hidden with your servers.
 * @related  CF_CryptoKey CF_CryptoSignPublic CF_CryptoSignSecret cf_crypto_sign_keygen CF_ServerConfig
 */
typedef struct CF_CryptoSignSecret { uint8_t bytes[64]; } CF_CryptoSignSecret;
// @end

//--------------------------------------------------------------------------------------------------
// ENDPOINT

/**
 * @enum     CF_AddressType
 * @category net
 * @brief    Available types of endpoints.
 * @related  CF_Address
 */
#define CF_ADDRESS_TYPE_DEFS \
	/* @entry No or invalid address type. */ \
	CF_ENUM(ADDRESS_TYPE_NONE, 0)          \
	/* @entry IPv4 address type. */        \
	CF_ENUM(ADDRESS_TYPE_IPV4, 1)          \
	/* @entry IPv6 address type. */        \
	CF_ENUM(ADDRESS_TYPE_IPV6, 2)          \
	/* @end */

typedef enum CF_AddressType
{
	#define CF_ENUM(K, V) CF_##K = V,
	CF_ADDRESS_TYPE_DEFS
	#undef CF_ENUM
} CF_AddressType;

/**
 * @function cf_address_type_to_string
 * @category net
 * @brief    Convert an enum `CF_AddressType` to a C string.
 * @related  CF_Address CF_AddressType
 */
CF_INLINE const char* cf_address_type_to_string(CF_AddressType type)
{
	switch (type) {
	#define CF_ENUM(K, V) case CF_##K: return CF_STRINGIZE(CF_##K);
	CF_ADDRESS_TYPE_DEFS
	#undef CF_ENUM
	default: return NULL;
	}
}

/**
 * @struct   CF_Address
 * @category net
 * @brief    A network address.
 * @remarks  Layout matches the underlying transport's endpoint type; treat it as a value you obtain
 *           from `cf_address_init` or a `CF_ServerEvent`, compare with `cf_address_equals`, and print
 *           with `cf_address_to_string`.
 * @related  CF_Address CF_AddressType cf_address_init cf_address_to_string cf_address_equals
 */
typedef struct CF_Address
{
	/* @member The address family. See `CF_AddressType`. */
	CF_AddressType type;

	/* @member The port number. */
	uint16_t port;

	union
	{
		/* @member The four octets of an IPv4 address. */
		uint8_t ipv4[4];

		/* @member The eight groups of an IPv6 address. */
		uint16_t ipv6[8];
	} u;
} CF_Address;
// @end

/**
 * @function cf_address_init
 * @category net
 * @brief    Initialize a `CF_Address` from a C string.
 * @return   Returns 0 on success, -1 on failure.
 * @related  CF_Address cf_address_init cf_address_to_string cf_address_equals
 */
CF_API int CF_CALL cf_address_init(CF_Address* address, const char* address_and_port_string);

/**
 * @function cf_address_to_string
 * @category net
 * @brief    Converts a `CF_Address` to a C string.
 * @related  CF_Address cf_address_init cf_address_to_string cf_address_equals
 */
CF_API void CF_CALL cf_address_to_string(CF_Address address, char* buffer, int buffer_size);

/**
 * @function cf_address_equals
 * @category net
 * @brief    Tests two addresses for equality.
 * @return   Returns true if the two addresses are equal.
 * @related  CF_Address cf_address_init cf_address_to_string cf_address_equals
 */
CF_API bool CF_CALL cf_address_equals(CF_Address a, CF_Address b);

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
CF_API CF_CryptoKey CF_CALL cf_crypto_generate_key(void);

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
 * @param    public_key     The public key of the keypair. Freely share this publicly.
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
 * @param    user_data             Can be `NULL`. Optional buffer of data of `CF_CONNECT_TOKEN_USER_DATA_SIZE` (256) bytes.
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
 *           which means the token cannot be modified or forged as long as the `shared_secret_key` is
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
 * @param    application_id  A unique number to identify your game, can be whatever value you like. This must be the same number as in `cf_make_server`.
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
 *           `cf_client_state` to get the client's state. Once `cf_client_connect` is called then successive calls to
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
 * @struct   CF_ClientWire
 * @category net
 * @brief    An optional datagram transport for a client, replacing its internal UDP socket.
 * @remarks  Each call moves one whole packet: `send` ships `size` bytes to the server as one
 *           datagram, and `recv` fills `data` with one pending datagram (up to `size` bytes),
 *           returning its length, 0 when nothing is pending, or negative on transport failure.
 *           Datagram semantics are assumed -- packets may arrive dropped, duplicated, or
 *           reordered, and the protocol handles all of that as usual; the wire only moves bytes.
 *           This is how web builds connect (see `cf_client_web_wire` and the Web topic page):
 *           browsers have no UDP, so datagrams ride a WebTransport or WebSocket bridge to a
 *           relay, and the server keeps its normal UDP socket. A wire also makes loopback or
 *           in-memory transports possible for tests.
 * @related  CF_Client cf_client_set_wire cf_client_web_wire cf_client_connect
 */
typedef struct CF_ClientWire
{
	/* @member Passed back to `send` and `recv`. */
	void* udata;

	/* @member Sends one whole datagram to the server. Returns bytes sent, or negative on failure. */
	int (CF_CALL* send)(void* udata, const void* data, int size);

	/* @member Receives one whole pending datagram into `data`, up to `size` bytes. Returns its
	   length, 0 when nothing is pending, or negative on transport failure. */
	int (CF_CALL* recv)(void* udata, void* data, int size);
} CF_ClientWire;
// @end

/**
 * @function cf_client_set_wire
 * @category net
 * @brief    Routes all of a client's traffic through a `CF_ClientWire` instead of a UDP socket.
 * @remarks  Call after `cf_make_client` and before `cf_client_connect`. The wire struct is copied;
 *           whatever `udata` points at must outlive the client.
 * @related  CF_ClientWire cf_client_web_wire cf_client_connect
 */
CF_API void CF_CALL cf_client_set_wire(CF_Client* client, CF_ClientWire wire);

/**
 * @function cf_client_web_wire
 * @category net
 * @brief    On web builds, wires a client to a datagram relay by URL -- the browser's road to a UDP server.
 * @param    url  The relay endpoint, e.g. `"https://mygame.example/wire/5601"`. WebTransport is tried
 *                at this URL first; on failure the same URL is retried as a WebSocket (`https` ->
 *                `wss`). Both carry one datagram per message.
 * @return   Returns an error on native builds (this is web machinery), or when the URL is malformed.
 * @remarks  Call after `cf_make_client` and before `cf_client_connect`, in place of `cf_client_set_wire`.
 *           The connection opens in the background; packets sent before it opens are dropped, which the
 *           protocol's redundant handshake absorbs. The relay is an untrusted dumb pipe (every datagram
 *           is already encrypted), typically a tiny daemon on the game server's host that forwards
 *           WebTransport/WebSocket messages to the server's UDP port 1:1. See the Web topic page for
 *           the full picture, including a reference relay.
 * @related  CF_ClientWire cf_client_set_wire cf_client_connect
 */
CF_API CF_Result CF_CALL cf_client_web_wire(CF_Client* client, const char* url);

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
 * @brief    Frees a packet created by `cf_client_pop_packet`.
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
 * @related  CF_ClientState cf_client_state_to_string cf_client_state
 */
#define CF_CLIENT_STATE_DEFS \
	/* @entry The connect token has expired. */ \
	CF_ENUM(CLIENT_STATE_CONNECT_TOKEN_EXPIRED,        -6) \
	/* @entry The connect token is invalid. */ \
	CF_ENUM(CLIENT_STATE_INVALID_CONNECT_TOKEN,        -5) \
	/* @entry The connection attempt timed out. */ \
	CF_ENUM(CLIENT_STATE_CONNECTION_TIMED_OUT,         -4) \
	/* @entry The challenge response timed out. */ \
	CF_ENUM(CLIENT_STATE_CHALLENGE_RESPONSE_TIMED_OUT, -3) \
	/* @entry The connection request timed out. */ \
	CF_ENUM(CLIENT_STATE_CONNECTION_REQUEST_TIMED_OUT, -2) \
	/* @entry The connection was denied by the server. */ \
	CF_ENUM(CLIENT_STATE_CONNECTION_DENIED,            -1) \
	/* @entry The client is disconnected. */ \
	CF_ENUM(CLIENT_STATE_DISCONNECTED,                  0) \
	/* @entry The client is sending a connection request. */ \
	CF_ENUM(CLIENT_STATE_SENDING_CONNECTION_REQUEST,    1) \
	/* @entry The client is sending a challenge response. */ \
	CF_ENUM(CLIENT_STATE_SENDING_CHALLENGE_RESPONSE,    2) \
	/* @entry The client is connected. */ \
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
 * @related  CF_ClientState cf_client_state_to_string cf_client_state
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
 * @function cf_client_state
 * @category net
 * @brief    Returns the `CF_ClientState` of a `CF_Client`.
 * @related  CF_ClientState cf_client_state_to_string cf_client_state
 */
CF_API CF_ClientState CF_CALL cf_client_state(const CF_Client* client);

/**
 * @function cf_client_state_get
 * @category net
 * @brief    Returns the `CF_ClientState` of a `CF_Client`.
 * @deprecated Use cf_client_state instead.
 * @related  CF_ClientState cf_client_state_to_string cf_client_state
 */
CF_INLINE CF_ClientState cf_client_state_get(const CF_Client* client) { return cf_client_state(client); }

/**
 * @function cf_client_rtt
 * @category net
 * @brief    Returns the client's estimated round-trip time to the server, in milliseconds.
 * @related  CF_Client cf_client_rtt cf_client_packet_loss cf_client_incoming_kbps cf_client_outgoing_kbps
 */
CF_API float CF_CALL cf_client_rtt(CF_Client* client);

/**
 * @function cf_client_packet_loss
 * @category net
 * @brief    Returns the client's estimated packet loss, from 0 (none) to 1 (all).
 * @related  CF_Client cf_client_rtt cf_client_packet_loss cf_client_incoming_kbps cf_client_outgoing_kbps
 */
CF_API float CF_CALL cf_client_packet_loss(CF_Client* client);

/**
 * @function cf_client_incoming_kbps
 * @category net
 * @brief    Returns the client's estimated incoming bandwidth in kilobits per second.
 * @related  CF_Client cf_client_rtt cf_client_packet_loss cf_client_incoming_kbps cf_client_outgoing_kbps
 */
CF_API float CF_CALL cf_client_incoming_kbps(CF_Client* client);

/**
 * @function cf_client_outgoing_kbps
 * @category net
 * @brief    Returns the client's estimated outgoing bandwidth in kilobits per second.
 * @related  CF_Client cf_client_rtt cf_client_packet_loss cf_client_incoming_kbps cf_client_outgoing_kbps
 */
CF_API float CF_CALL cf_client_outgoing_kbps(CF_Client* client);

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

/**
 * @function CF_SERVER_MAX_CLIENTS
 * @category net
 * @brief    The maximum number of clients a single `CF_Server` supports.
 * @remarks  This is fixed at compile time. Raising it requires rebuilding CF (it also sizes the
 *           underlying transport's per-client state), so it is not a value you can tune per-server.
 * @related  CF_Server cf_make_server
 */
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

	/* @member The number of seconds before consider a connection as timed out when not receiving any packets on the connection. */
	int connection_timeout;

	/* @member The number of seconds to wait before resending a packet that has not been acknowledge as received by a client. */
	double resend_rate;

	/* @member The public part of your public key cryptography used for connect tokens. This can be safely shared with your players publicly. See `CF_CryptoSignPublic`. */
	CF_CryptoSignPublic public_key;

	/* @member The secret part of your public key cryptography used for connect tokens. This must never be shared publicly and remain a complete secret only known to your servers. See `CF_CryptoSignSecret`. */
	CF_CryptoSignSecret secret_key;
} CF_ServerConfig;
// @end

/**
 * @function cf_server_config_defaults
 * @category net
 * @brief    Returns a good set of default parameters for `cf_make_server`.
 * @remarks  The keypair is zeroed; fill in `public_key`/`secret_key` (see `cf_crypto_sign_keygen`)
 *           and `application_id` before calling `cf_make_server`.
 * @related  CF_ServerConfig cf_server_config_defaults cf_make_server
 */
CF_INLINE CF_ServerConfig CF_CALL cf_server_config_defaults(void)
{
	CF_ServerConfig config = { 0 };
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
 * @return   Returns any errors as a `CF_Result`.
 * @remarks  Please note that not all users will be able to access an ipv6 server address, so it might be good to also provide a way to connect through ipv4.
 * @related  CF_ServerConfig cf_server_config_defaults cf_make_server cf_destroy_server cf_server_start cf_server_update
 */
CF_API CF_Result CF_CALL cf_server_start(CF_Server* server, const char* address_and_port);

/**
 * @function cf_server_stop
 * @category net
 * @brief    Stops the server.
 * @related  CF_ServerConfig cf_server_config_defaults cf_make_server cf_destroy_server cf_server_start cf_server_update
 */
CF_API void CF_CALL cf_server_stop(CF_Server* server);

/**
 * @function cf_server_set_public_ip
 * @category net
 * @brief    Overrides the public address clients are told to connect to (for NAT / port-forwarding).
 * @param    server            The server.
 * @param    address_and_port  The publicly reachable address and port clients should use.
 * @remarks  Only needed for local development behind a router, or any setup where the address the
 *           server binds differs from the address clients must reach. Leave unset in production.
 * @related  CF_Server cf_server_start
 */
CF_API void CF_CALL cf_server_set_public_ip(CF_Server* server, const char* address_and_port);

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
 * @param    type        The type to convert to a string.
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
 * @return   Returns any errors as a `CF_Result` (e.g. the send queue being full).
 * @related  cf_server_update cf_server_send cf_server_send_to_all CF_ServerEvent cf_server_pop_event
 */
CF_API CF_Result CF_CALL cf_server_send(CF_Server* server, const void* packet, int size, int client_index, bool send_reliably);

/**
 * @function cf_server_send_to_all
 * @category net
 * @brief    Sends a packet to every connected client.
 * @param    server         The server.
 * @param    packet         Data to send.
 * @param    size           Size of `data` in bytes.
 * @param    send_reliably  If `true` the packet is sent reliably and in order to each client.
 * @remarks  A convenience over looping every client index and checking `cf_server_is_client_connected`.
 * @related  cf_server_send cf_server_send_to_all cf_server_is_client_connected
 */
CF_API void CF_CALL cf_server_send_to_all(CF_Server* server, const void* packet, int size, bool send_reliably);

/**
 * @function cf_server_is_client_connected
 * @category net
 * @brief    Returns true if a client is still connected.
 * @related  cf_server_update CF_ServerEvent cf_server_pop_event cf_server_send
 */
CF_API bool CF_CALL cf_server_is_client_connected(CF_Server* server, int client_index);

/**
 * @function cf_server_rtt
 * @category net
 * @brief    Returns the estimated round-trip time to a client, in milliseconds.
 * @related  CF_Server cf_server_rtt cf_server_packet_loss cf_server_incoming_kbps cf_server_outgoing_kbps
 */
CF_API float CF_CALL cf_server_rtt(CF_Server* server, int client_index);

/**
 * @function cf_server_packet_loss
 * @category net
 * @brief    Returns the estimated packet loss to a client, from 0 (none) to 1 (all).
 * @related  CF_Server cf_server_rtt cf_server_packet_loss cf_server_incoming_kbps cf_server_outgoing_kbps
 */
CF_API float CF_CALL cf_server_packet_loss(CF_Server* server, int client_index);

/**
 * @function cf_server_incoming_kbps
 * @category net
 * @brief    Returns the estimated incoming bandwidth from a client in kilobits per second.
 * @related  CF_Server cf_server_rtt cf_server_packet_loss cf_server_incoming_kbps cf_server_outgoing_kbps
 */
CF_API float CF_CALL cf_server_incoming_kbps(CF_Server* server, int client_index);

/**
 * @function cf_server_outgoing_kbps
 * @category net
 * @brief    Returns the estimated outgoing bandwidth to a client in kilobits per second.
 * @related  CF_Server cf_server_rtt cf_server_packet_loss cf_server_incoming_kbps cf_server_outgoing_kbps
 */
CF_API float CF_CALL cf_server_outgoing_kbps(CF_Server* server, int client_index);

/**
 * @function cf_server_enable_network_simulator
 * @category net
 * @brief    Turns on the network simulator for a server.
 * @param    server           The server.
 * @param    latency          A number of seconds of latency to add to the connection.
 * @param    jitter           The variability of latency.
 * @param    drop_chance      Number from [0,1]. 0 means drop no packets, 1 means drop all packets, 0.5f means 50% packet loss.
 * @param    duplicate_chance Number from [0,1] representing the chance to duplicate a packet, where 1 is 100% chance.
 * @related  CF_Server
 */
CF_API void CF_CALL cf_server_enable_network_simulator(CF_Server* server, double latency, double jitter, double drop_chance, double duplicate_chance);

//--------------------------------------------------------------------------------------------------
// MESSAGES + CHANNELS
//
// Small user messages routed over prioritized channels, layered above raw packets. Each message
// carries a user id (a type tag, e.g. "chat" or "jump") and is queued on one of a handful of
// channels. Every update the queues are pumped into the transport highest-priority channel first,
// under an optional bytes-per-second budget -- so bulk low-priority traffic (asset blobs, big state
// dumps) waits its turn locally instead of flooding the send queue and delaying urgent messages.
// Reliability is per channel: a reliable channel delivers every message in order (per channel,
// FIFO); an unreliable one may drop messages entirely under loss, but never corrupts or reorders
// what arrives. Small messages queued together on a channel are coalesced into a single packet.
//
// Note the budget only orders and throttles LOCAL queueing. Everything still crosses the wire on
// the one underlying connection, so a reliable retransmit storm can still delay unreliable packets
// at the socket -- channels keep your own bulk sends from being the cause of that.

/**
 * @function CF_NET_MAX_CHANNELS
 * @category net
 * @brief    The number of message channels available, indexed 0 to `CF_NET_MAX_CHANNELS - 1`.
 * @related  cf_client_channel_options cf_client_send_msg cf_server_send_msg
 */
#define CF_NET_MAX_CHANNELS 8

/**
 * @function CF_NET_MAX_MSG_SIZE
 * @category net
 * @brief    The largest single message that may be sent on a channel, in bytes.
 * @remarks  Large messages are fragmented and reassembled by the transport automatically. For bulk
 *           data, prefer a low-priority channel with a send rate set, so the fragments trickle out
 *           without starving urgent traffic.
 * @related  cf_client_send_msg cf_server_send_msg cf_client_set_send_rate
 */
#define CF_NET_MAX_MSG_SIZE (1024 * 1024)

/**
 * @function cf_client_channel_options
 * @category net
 * @brief    Configures one of the client's outgoing message channels.
 * @param    client       The client.
 * @param    channel      Which channel, from 0 to `CF_NET_MAX_CHANNELS - 1`.
 * @param    priority     Channels with higher priority are pumped over the wire first each update. Default 0.
 * @param    reliable     If `true` every message on this channel arrives, in order. If `false` messages
 *                        may be lost under packet loss (but never corrupted or reordered). Default `false`.
 * @remarks  Configure channels once, right after creating the client. Changing reliability while
 *           messages are queued applies to messages not yet sent.
 * @related  cf_client_send_msg cf_client_set_send_rate cf_server_channel_options
 */
CF_API void CF_CALL cf_client_channel_options(CF_Client* client, int channel, int priority, bool reliable);

/**
 * @function cf_client_set_send_rate
 * @category net
 * @brief    Caps how many queued message bytes the client pumps into the transport per second.
 * @param    client            The client.
 * @param    bytes_per_second  The budget. 0 (the default) means unlimited.
 * @remarks  The budget is what makes priorities matter: when messages are queued faster than the
 *           budget drains them, high-priority channels keep flowing while bulk waits locally. A
 *           message larger than one second's budget still sends, going into "debt" that throttles
 *           later sends. Raw `cf_client_send` packets bypass the budget entirely.
 * @related  cf_client_channel_options cf_client_send_msg cf_server_set_send_rate
 */
CF_API void CF_CALL cf_client_set_send_rate(CF_Client* client, int bytes_per_second);

/**
 * @function cf_client_send_msg
 * @category net
 * @brief    Queues a message to the server on a channel.
 * @param    client   The client.
 * @param    channel  Which channel carries it, from 0 to `CF_NET_MAX_CHANNELS - 1`.
 * @param    id       A user-defined type tag delivered along with the bytes (e.g. an enum of your game's message kinds).
 * @param    data     The message bytes. May be `NULL` when `size` is 0 -- an id alone is a fine message.
 * @param    size     Size of `data` in bytes, up to `CF_NET_MAX_MSG_SIZE`.
 * @return   Returns an error if the message is oversized or the channel's local queue is full (the
 *           queue drains each `cf_client_update`, paced by `cf_client_set_send_rate`).
 * @remarks  The bytes are copied; `data` may be reused immediately. Messages go over the wire during
 *           `cf_client_update`, highest-priority channel first. The server receives them with
 *           `cf_server_pop_msg`.
 * @related  cf_client_pop_msg cf_client_channel_options cf_client_set_send_rate cf_server_pop_msg
 */
CF_API CF_Result CF_CALL cf_client_send_msg(CF_Client* client, int channel, uint32_t id, const void* data, int size);

/**
 * @function cf_client_pop_msg
 * @category net
 * @brief    Pops the next message received from the server, if any.
 * @param    client   The client.
 * @param    id       The message's user id.
 * @param    data     Pointer to the message bytes. Free it with `cf_client_free_msg`.
 * @param    size     Size of `data` in bytes.
 * @return   Returns `true` when a message was popped.
 * @remarks  Incoming messages surface here as `cf_client_pop_packet` drains arriving packets, so
 *           keep popping packets each update like normal (even if you only ever use messages).
 * @related  cf_client_free_msg cf_server_send_msg cf_client_send_msg
 */
CF_API bool CF_CALL cf_client_pop_msg(CF_Client* client, uint32_t* id, void** data, int* size);

/**
 * @function cf_client_free_msg
 * @category net
 * @brief    Frees a message from `cf_client_pop_msg`.
 * @related  cf_client_pop_msg cf_server_free_msg
 */
CF_API void CF_CALL cf_client_free_msg(CF_Client* client, void* data);

/**
 * @function cf_server_channel_options
 * @category net
 * @brief    Configures one of the server's outgoing message channels (applies to every client).
 * @param    server       The server.
 * @param    channel      Which channel, from 0 to `CF_NET_MAX_CHANNELS - 1`.
 * @param    priority     Channels with higher priority are pumped over the wire first each update. Default 0.
 * @param    reliable     If `true` every message on this channel arrives, in order. If `false` messages
 *                        may be lost under packet loss (but never corrupted or reordered). Default `false`.
 * @related  cf_server_send_msg cf_server_set_send_rate cf_client_channel_options
 */
CF_API void CF_CALL cf_server_channel_options(CF_Server* server, int channel, int priority, bool reliable);

/**
 * @function cf_server_set_send_rate
 * @category net
 * @brief    Caps how many queued message bytes the server pumps per second, per client.
 * @param    server            The server.
 * @param    bytes_per_second  The budget, applied to each client's queues independently. 0 (the default) means unlimited.
 * @related  cf_server_channel_options cf_server_send_msg cf_client_set_send_rate
 */
CF_API void CF_CALL cf_server_set_send_rate(CF_Server* server, int bytes_per_second);

/**
 * @function cf_server_send_msg
 * @category net
 * @brief    Queues a message to one client on a channel.
 * @param    server        The server.
 * @param    client_index  An index representing a particular client, from `CF_ServerEvent`.
 * @param    channel       Which channel carries it, from 0 to `CF_NET_MAX_CHANNELS - 1`.
 * @param    id            A user-defined type tag delivered along with the bytes.
 * @param    data          The message bytes. May be `NULL` when `size` is 0.
 * @param    size          Size of `data` in bytes, up to `CF_NET_MAX_MSG_SIZE`.
 * @return   Returns an error if the message is oversized, the client is not connected, or that
 *           client's channel queue is full (queues drain each `cf_server_update`).
 * @remarks  The bytes are copied; `data` may be reused immediately. The client receives them with
 *           `cf_client_pop_msg`.
 * @related  cf_server_pop_msg cf_server_channel_options cf_server_set_send_rate cf_client_pop_msg
 */
CF_API CF_Result CF_CALL cf_server_send_msg(CF_Server* server, int client_index, int channel, uint32_t id, const void* data, int size);

/**
 * @function cf_server_pop_msg
 * @category net
 * @brief    Pops the next message received from any client, if any.
 * @param    server        The server.
 * @param    client_index  Which client sent it.
 * @param    id            The message's user id.
 * @param    data          Pointer to the message bytes. Free it with `cf_server_free_msg`.
 * @param    size          Size of `data` in bytes.
 * @return   Returns `true` when a message was popped.
 * @remarks  Incoming messages surface here only after `cf_server_pop_event` has drained the events
 *           carrying them, so keep popping events each update like normal.
 * @related  cf_server_free_msg cf_client_send_msg cf_server_pop_event
 */
CF_API bool CF_CALL cf_server_pop_msg(CF_Server* server, int* client_index, uint32_t* id, void** data, int* size);

/**
 * @function cf_server_free_msg
 * @category net
 * @brief    Frees a message from `cf_server_pop_msg`.
 * @related  cf_server_pop_msg cf_client_free_msg
 */
CF_API void CF_CALL cf_server_free_msg(CF_Server* server, void* data);

//--------------------------------------------------------------------------------------------------
// COMPRESSION
//
// Delta + entropy compression for game snapshots, built on CF's adaptive range coder (cute_arith.h).
// The intended flow for server-authoritative state: keep the last snapshot each client has
// acknowledged as a baseline, `cf_snapshot_compress` the new snapshot against it, and send the
// (usually tiny) result with `cf_server_send`. The client decompresses against the same baseline.
// Unchanged fields cost almost nothing, so bandwidth scales with what actually moved, not with
// world size. Baselines and which one a peer holds are the caller's to track for now.

/**
 * @function cf_snapshot_compress
 * @category net
 * @brief    Delta-compresses a snapshot against a baseline of the same size.
 * @param    baseline        The previous snapshot to delta against, or `NULL` to compress against all-zero.
 * @param    current         The new snapshot to compress. Must be `size` bytes.
 * @param    size            The size of both snapshots in bytes.
 * @param    out             Destination buffer for the compressed bytes.
 * @param    out_capacity    Capacity of `out` in bytes.
 * @return   Returns the compressed size in bytes, or -1 if it did not fit in `out_capacity`.
 * @remarks  `baseline` and `current` must have identical layout and size; only their differences
 *           are encoded. Decompress with `cf_snapshot_decompress` and the same baseline.
 * @related  cf_snapshot_decompress cf_client_send cf_server_send
 */
CF_API int CF_CALL cf_snapshot_compress(const void* baseline, const void* current, int size, void* out, int out_capacity);

/**
 * @function cf_snapshot_decompress
 * @category net
 * @brief    Reconstructs a snapshot from a baseline and a compressed delta.
 * @param    baseline        The same baseline passed to `cf_snapshot_compress` (or `NULL`).
 * @param    size            The snapshot size in bytes (known to both sides).
 * @param    compressed      The compressed delta bytes.
 * @param    compressed_size The number of compressed bytes.
 * @param    out             Destination buffer for the reconstructed snapshot, `size` bytes.
 * @return   Returns 0 on success.
 * @related  cf_snapshot_compress cf_client_pop_packet cf_server_pop_event
 */
CF_API int CF_CALL cf_snapshot_decompress(const void* baseline, int size, const void* compressed, int compressed_size, void* out);

//--------------------------------------------------------------------------------------------------
// DEV CONVENIENCES
//
// Shortcuts for local development, singleplayer/listen-server, and tests. The "insecure" pair use a
// fixed keypair compiled into CF, so they need no key exchange -- and provide no security. Never run
// a real, exposed server with them; use cf_crypto_sign_keygen + the full flow for production.

/**
 * @function cf_make_server_insecure
 * @category net
 * @brief    Creates a server using CF's built-in development keypair. INSECURE -- local/testing only.
 * @param    application_id  Your game id; must match the client.
 * @remarks  Pairs with `cf_client_connect_insecure`. Anyone can forge tokens for this server, so it
 *           is only appropriate for a machine you control (local dev, singleplayer, unit tests).
 * @related  cf_client_connect_insecure cf_make_server cf_server_start
 */
CF_API CF_Server* CF_CALL cf_make_server_insecure(uint64_t application_id);

/**
 * @function cf_client_connect_insecure
 * @category net
 * @brief    Connects to a `cf_make_server_insecure` server in one call. INSECURE -- local/testing only.
 * @param    address_and_port  The server to connect to, e.g. "127.0.0.1:5000".
 * @param    application_id    Your game id; must match the server.
 * @return   Returns any errors as a `CF_Result`.
 * @remarks  Generates a connect token from CF's built-in development keypair (with a random client id
 *           so many clients can connect) and connects -- no web service or key handling required.
 * @related  cf_make_server_insecure cf_client_connect cf_make_client
 */
CF_API CF_Result CF_CALL cf_client_connect_insecure(CF_Client* client, const char* address_and_port, uint64_t application_id);

/**
 * @function cf_client_tick
 * @category net
 * @brief    Updates the client using CF's own clock (`CF_DELTA_TIME` and the system time).
 * @remarks  A convenience over `cf_client_update` for games that just run on the app clock, so you
 *           don't have to plumb `dt` and a unix timestamp by hand. Call once per frame.
 * @related  cf_client_update cf_client_pop_packet
 */
CF_API void CF_CALL cf_client_tick(CF_Client* client);

/**
 * @function cf_server_tick
 * @category net
 * @brief    Updates the server using CF's own clock (`CF_DELTA_TIME` and the system time).
 * @remarks  A convenience over `cf_server_update`. Call once per frame.
 * @related  cf_server_update cf_server_pop_event
 */
CF_API void CF_CALL cf_server_tick(CF_Server* server);

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

CF_INLINE int address_init(Address* address, const char* address_and_port_string) { return cf_address_init(address,address_and_port_string); }
CF_INLINE void address_to_string(Address address, char* buffer, int buffer_size) { cf_address_to_string(address,buffer,buffer_size); }
CF_INLINE bool address_equals(Address a, Address b) { return cf_address_equals(a,b); }

//--------------------------------------------------------------------------------------------------
// CONNECT TOKEN

CF_INLINE CryptoKey crypto_generate_key() { return cf_crypto_generate_key(); }
CF_INLINE void crypto_random_bytes(void* data, int byte_count) { cf_crypto_random_bytes(data,byte_count); }
CF_INLINE void crypto_sign_keygen(CryptoSignPublic* public_key, CryptoSignSecret* secret_key) { cf_crypto_sign_keygen(public_key,secret_key); }
CF_INLINE CF_Result generate_connect_token(
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
CF_INLINE CF_Result client_connect(Client* client, const uint8_t* connect_token) { return cf_client_connect(client,connect_token); }
CF_INLINE void client_disconnect(Client* client) { cf_client_disconnect(client); }
CF_INLINE void client_update(Client* client, double dt, uint64_t current_time) { cf_client_update(client,dt,current_time); }
CF_INLINE bool client_pop_packet(Client* client, void** packet, int* size, bool* was_sent_reliably = NULL) { return cf_client_pop_packet(client,packet,size,was_sent_reliably); }
CF_INLINE void client_free_packet(Client* client, void* packet) { cf_client_free_packet(client,packet); }
CF_INLINE CF_Result client_send(Client* client, const void* packet, int size, bool send_reliably) { return cf_client_send(client,packet,size,send_reliably); }
CF_INLINE ClientState client_state(const Client* client) { return cf_client_state(client); }
CF_INLINE float client_rtt(Client* client) { return cf_client_rtt(client); }
CF_INLINE float client_packet_loss(Client* client) { return cf_client_packet_loss(client); }
CF_INLINE float client_incoming_kbps(Client* client) { return cf_client_incoming_kbps(client); }
CF_INLINE float client_outgoing_kbps(Client* client) { return cf_client_outgoing_kbps(client); }
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
CF_INLINE CF_Result server_start(Server* server, const char* address_and_port) { return cf_server_start(server,address_and_port); }
CF_INLINE void server_stop(Server* server) { cf_server_stop(server); }
CF_INLINE void server_set_public_ip(Server* server, const char* address_and_port) { cf_server_set_public_ip(server,address_and_port); }
CF_INLINE bool server_pop_event(Server* server, ServerEvent* event) { return cf_server_pop_event(server,event); }
CF_INLINE void server_free_packet(Server* server, int client_index, void* data) { cf_server_free_packet(server,client_index,data); }
CF_INLINE void server_update(Server* server, double dt, uint64_t current_time) { cf_server_update(server,dt,current_time); }
CF_INLINE void server_disconnect_client(Server* server, int client_index, bool notify_client = true) { cf_server_disconnect_client(server, client_index, notify_client); }
CF_INLINE CF_Result server_send(Server* server, const void* packet, int size, int client_index, bool send_reliably) { return cf_server_send(server,packet,size,client_index,send_reliably); }
CF_INLINE void server_send_to_all(Server* server, const void* packet, int size, bool send_reliably) { cf_server_send_to_all(server,packet,size,send_reliably); }
CF_INLINE bool server_is_client_connected(Server* server, int client_index) { return cf_server_is_client_connected(server,client_index); }
CF_INLINE float server_rtt(Server* server, int client_index) { return cf_server_rtt(server,client_index); }
CF_INLINE float server_packet_loss(Server* server, int client_index) { return cf_server_packet_loss(server,client_index); }
CF_INLINE float server_incoming_kbps(Server* server, int client_index) { return cf_server_incoming_kbps(server,client_index); }
CF_INLINE float server_outgoing_kbps(Server* server, int client_index) { return cf_server_outgoing_kbps(server,client_index); }
CF_INLINE void server_enable_network_simulator(Server* server, double latency, double jitter, double drop_chance, double duplicate_chance) { cf_server_enable_network_simulator(server,latency,jitter,drop_chance,duplicate_chance); }

//--------------------------------------------------------------------------------------------------
// MESSAGES + CHANNELS

CF_INLINE void client_channel_options(Client* client, int channel, int priority, bool reliable) { cf_client_channel_options(client,channel,priority,reliable); }
CF_INLINE void client_set_send_rate(Client* client, int bytes_per_second) { cf_client_set_send_rate(client,bytes_per_second); }
CF_INLINE CF_Result client_send_msg(Client* client, int channel, uint32_t id, const void* data, int size) { return cf_client_send_msg(client,channel,id,data,size); }
CF_INLINE bool client_pop_msg(Client* client, uint32_t* id, void** data, int* size) { return cf_client_pop_msg(client,id,data,size); }
CF_INLINE void client_free_msg(Client* client, void* data) { cf_client_free_msg(client,data); }
CF_INLINE void server_channel_options(Server* server, int channel, int priority, bool reliable) { cf_server_channel_options(server,channel,priority,reliable); }
CF_INLINE void server_set_send_rate(Server* server, int bytes_per_second) { cf_server_set_send_rate(server,bytes_per_second); }
CF_INLINE CF_Result server_send_msg(Server* server, int client_index, int channel, uint32_t id, const void* data, int size) { return cf_server_send_msg(server,client_index,channel,id,data,size); }
CF_INLINE bool server_pop_msg(Server* server, int* client_index, uint32_t* id, void** data, int* size) { return cf_server_pop_msg(server,client_index,id,data,size); }
CF_INLINE void server_free_msg(Server* server, void* data) { cf_server_free_msg(server,data); }

//--------------------------------------------------------------------------------------------------
// COMPRESSION

CF_INLINE int snapshot_compress(const void* baseline, const void* current, int size, void* out, int out_capacity) { return cf_snapshot_compress(baseline,current,size,out,out_capacity); }
CF_INLINE int snapshot_decompress(const void* baseline, int size, const void* compressed, int compressed_size, void* out) { return cf_snapshot_decompress(baseline,size,compressed,compressed_size,out); }

//--------------------------------------------------------------------------------------------------
// DEV CONVENIENCES

CF_INLINE Server* make_server_insecure(uint64_t application_id) { return cf_make_server_insecure(application_id); }
CF_INLINE CF_Result client_connect_insecure(Client* client, const char* address_and_port, uint64_t application_id) { return cf_client_connect_insecure(client,address_and_port,application_id); }
CF_INLINE void client_tick(Client* client) { cf_client_tick(client); }
CF_INLINE void server_tick(Server* server) { cf_server_tick(server); }

}

#endif // CF_CPP

#endif // CF_NETWORKING_H
