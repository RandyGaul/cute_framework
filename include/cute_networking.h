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

#include <cute_error.h>

#include <cute/cute_net.h>

namespace cute
{

using client_t = cn_client_t;
using server_t = cn_server_t;
using endpoint_t = cn_endpoint_t;
using crypto_key_t = cn_crypto_key_t;
using crypto_sign_public_t = cn_crypto_sign_public_t;
using crypto_sign_secret_t = cn_crypto_sign_secret_t;
using crypto_signature_t = cn_crypto_signature_t;

using address_type_t = cn_address_type_t;
#define CUTE_ADDRESS_TYPE_NONE CN_ADDRESS_TYPE_NONE
#define CUTE_ADDRESS_TYPE_IPV4 CN_ADDRESS_TYPE_IPV4
#define CUTE_ADDRESS_TYPE_IPV6 CN_ADDRESS_TYPE_IPV6

//--------------------------------------------------------------------------------------------------
// CONNECT TOKEN

#define CUTE_CONNECT_TOKEN_SIZE 1114

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
CUTE_API error_t CUTE_CALL generate_connect_token(
	uint64_t application_id,                          // A unique number to identify your game, can be whatever value you like.
	                                                  // This must be the same number as in `client_create` and `server_create`.
	uint64_t creation_timestamp,                      // A unix timestamp of the current time.
	const crypto_key_t* client_to_server_key,         // A unique key for this connect token for the client to encrypt packets, and server to
	                                                  // decrypt packets. This can be generated with `crypto_generate_key` on your web service.
	const crypto_key_t* server_to_client_key,         // A unique key for this connect token for the server to encrypt packets, and the client to
	                                                  // decrypt packets. This can be generated with `crypto_generate_key` on your web service.
	uint64_t expiration_timestamp,                    // A unix timestamp for when this connect token expires and becomes invalid.
	uint32_t handshake_timeout,                       // The number of seconds the connection will stay alive during the handshake process before
	                                                  // the client and server reject the handshake process as failed.
	int address_count,                                // Must be from 1 to 32 (inclusive). The number of addresses in `address_list`.
	const char** address_list,                        // A list of game servers the client can try connecting to, of length `address_count`.
	uint64_t client_id,                               // The unique client identifier.
	const uint8_t* user_data,                         // Optional buffer of data of `CUTE_PROTOCOL_CONNECT_TOKEN_USER_DATA_SIZE` (256) bytes. Can be NULL.
	const crypto_sign_secret_t* shared_secret_key,    // Only your webservice and game servers know this key.
	uint8_t* token_ptr_out                            // Pointer to your buffer, should be `CUTE_CONNECT_TOKEN_SIZE` bytes large.
);

//--------------------------------------------------------------------------------------------------
// CLIENT

CUTE_API client_t* CUTE_CALL client_create(
	uint16_t port,                      // Port for opening a UDP socket.
	uint64_t application_id,            // A unique number to identify your game, can be whatever value you like.
	                                    // This must be the same number as in `server_create`.
	bool use_ipv6 = false,              // Whether or not the socket should turn on ipv6. Some users will not have
	                                    // ipv6 enabled, so this defaults to false.
	void* user_allocator_context = NULL // Used for custom allocators, this can be set to NULL.
);
CUTE_API void CUTE_CALL client_destroy(client_t* client);

/**
 * The client will make an attempt to connect to all servers listed in the connect token, one after
 * another. If no server can be connected to the client's state will be set to an error state. Call
 * `client_state_get` to get the client's state. Once `client_connect` is called then successive calls to
 * `client_update` is expected, where `client_update` will perform the connection handshake and make
 * connection attempts to your servers.
 */
CUTE_API error_t CUTE_CALL client_connect(client_t* client, const uint8_t* connect_token);
CUTE_API void CUTE_CALL client_disconnect(client_t* client);

/**
 * You should call this one per game loop after calling `client_connect`.
 */
CUTE_API void CUTE_CALL client_update(client_t* client, double dt, uint64_t current_time);

/**
 * Returns a packet from the server if one is available. You must free this packet when you're done by
 * calling `client_free_packet`.
 */
CUTE_API bool CUTE_CALL client_pop_packet(client_t* client, void** packet, int* size, bool* was_sent_reliably = NULL);
CUTE_API void CUTE_CALL client_free_packet(client_t* client, void* packet);

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
CUTE_API error_t CUTE_CALL client_send(client_t* client, const void* packet, int size, bool send_reliably);

typedef enum client_state_t
{
	CLIENT_STATE_CONNECT_TOKEN_EXPIRED         = -6,
	CLIENT_STATE_INVALID_CONNECT_TOKEN         = -5,
	CLIENT_STATE_CONNECTION_TIMED_OUT          = -4,
	CLIENT_STATE_CHALLENGE_RESPONSE_TIMED_OUT  = -3,
	CLIENT_STATE_CONNECTION_REQUEST_TIMED_OUT  = -2,
	CLIENT_STATE_CONNECTION_DENIED             = -1,
	CLIENT_STATE_DISCONNECTED                  = 0,
	CLIENT_STATE_SENDING_CONNECTION_REQUEST    = 1,
	CLIENT_STATE_SENDING_CHALLENGE_RESPONSE    = 2,
	CLIENT_STATE_CONNECTED                     = 3,
} client_state_t;

CUTE_API client_state_t CUTE_CALL client_state_get(const client_t* client);
CUTE_API const char* CUTE_CALL client_state_string(client_state_t state); 
CUTE_API float CUTE_CALL client_time_of_last_packet_recieved(const client_t* client);
CUTE_API void CUTE_CALL client_enable_network_simulator(client_t* client, double latency, double jitter, double drop_chance, double duplicate_chance);

//--------------------------------------------------------------------------------------------------
// SERVER

// Modify this value as seen fit.
#define CUTE_SERVER_MAX_CLIENTS 32

typedef struct server_config_t
{
	uint64_t application_id;            // A unique number to identify your game, can be whatever value you like.
	                                    // This must be the same number as in `client_make`.
	int max_incoming_bytes_per_second;
	int max_outgoing_bytes_per_second;
	int connection_timeout;             // The number of seconds before consider a connection as timed out when not
	                                    // receiving any packets on the connection.
	double resend_rate;                 // The number of seconds to wait before resending a packet that has not been
	                                    // acknowledge as received by a client.
	crypto_sign_public_t public_key;    // The public part of your public key cryptography used for connect tokens.
	                                    // This can be safely shared with your players publicly.
	crypto_sign_secret_t secret_key;    // The secret part of your public key cryptography used for connect tokens.
	                                    // This must never be shared publicly and remain a complete secret only know to your servers.
	void* user_allocator_context;
} server_config_t;

CUTE_INLINE server_config_t CUTE_CALL server_config_defaults()
{
	server_config_t config;
	config.application_id = 0;
	config.max_incoming_bytes_per_second = 0;
	config.max_outgoing_bytes_per_second = 0;
	config.connection_timeout = 10;
	config.resend_rate = 0.1f;
	return config;
}

CUTE_API server_t* CUTE_CALL server_create(server_config_t config);
CUTE_API void CUTE_CALL server_destroy(server_t* server);

/**
 * Starts up the server, ready to receive new client connections.
 * 
 * Please note that not all users will be able to access an ipv6 server address, so it might
 * be good to also provide a way to connect through ipv4.
 */
CUTE_API error_t server_start(server_t* server, const char* address_and_port);
CUTE_API void server_stop(server_t* server);

typedef enum server_event_type_t
{
	SERVER_EVENT_TYPE_NEW_CONNECTION, // A new incoming connection.
	SERVER_EVENT_TYPE_DISCONNECTED,   // A disconnecting client.
	SERVER_EVENT_TYPE_PAYLOAD_PACKET, // An incoming packet from a client.
} server_event_type_t;

typedef struct server_event_t
{
	server_event_type_t type;
	union
	{
		struct
		{
			int client_index;       // An index representing this particular client.
			uint64_t client_id;     // A unique identifier for this particular client, as read from the connect token.
			endpoint_t endpoint; // The address and port of the incoming connection.
		} new_connection;

		struct
		{
			int client_index;       // An index representing this particular client.
		} disconnected;

		struct
		{
			int client_index;       // An index representing this particular client.
			void* data;             // Pointer to the packet's payload data. Send this back to `server_free_packet` when done.
			int size;               // Size of the packet at the data pointer.
		} payload_packet;
	} u;
} server_event_t;

/**
 * Server events notify of when a client connects/disconnects, or has sent a payload packet.
 * You must free the payload packets with `server_free_packet` when done.
 */
CUTE_API bool CUTE_CALL server_pop_event(server_t* server, server_event_t* event);
CUTE_API void CUTE_CALL server_free_packet(server_t* server, int client_index, void* data);

CUTE_API void CUTE_CALL server_update(server_t* server, double dt, uint64_t current_time);
CUTE_API void CUTE_CALL server_disconnect_client(server_t* server, int client_index, bool notify_client /* = true */);
CUTE_API void CUTE_CALL server_send(server_t* server, const void* packet, int size, int client_index, bool send_reliably);
CUTE_API void CUTE_CALL server_send_to_all_clients(server_t* server, const void* packet, int size, bool send_reliably);
CUTE_API void CUTE_CALL server_send_to_all_but_one_client(server_t* server, const void* packet, int size, int client_index, bool send_reliably);

CUTE_API bool CUTE_CALL server_is_client_connected(server_t* server, int client_index);
CUTE_API void CUTE_CALL server_enable_network_simulator(server_t* server, double latency, double jitter, double drop_chance, double duplicate_chance);

}

#endif // CUTE_NETWORKING_H
