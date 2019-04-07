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

#ifndef CUTE_PROTOCOL_H
#define CUTE_PROTOCOL_H

#include <cute_defines.h>
#include <cute_crypto.h>
#include <cute_net.h>

#define CUTE_PROTOCOL_VERSION_STRING ((const uint8_t*)"CUTE 1.00")
#define CUTE_PROTOCOL_VERSION_STRING_LEN (9 + 1)
#define CUTE_PACKET_SIZE_MAX (CUTE_KB + 256)
#define CUTE_PACKET_PAYLOAD_MAX (CUTE_PACKET_SIZE_MAX - 1 - sizeof(uint64_t) - CUTE_CRYPTO_HMAC_BYTES)

#define CUTE_CONNECT_TOKEN_SIZE 1114
#define CUTE_CONNECT_TOKEN_SERVER_COUNT_MAX 32
#define CUTE_CONNECT_TOKEN_USER_DATA_SIZE 256

#define CUTE_PACKET_QUEUE_MAX_ENTRIES (2 * 1024)
#define CUTE_REPLAY_BUFFER_SIZE 256
#define CUTE_KEEPALIVE_RATE (1.0f / 10.0f)
#define CUTE_DISCONNECT_REDUNDANT_PACKET_COUNT 10
#define CUTE_CHALLENGE_DATA_SIZE 256
#define CUTE_CONNECT_TOKEN_NONCE_SIZE CUTE_CRYPTO_NONCE_BYTES
#define CUTE_CONNECT_TOKEN_SECRET_SECTION_SIZE (8 + 32 + 32 + 256)

namespace cute
{

enum packet_type_t : int
{
	PACKET_TYPE_CONNECTION_REQUEST,
	PACKET_TYPE_CONNECTION_ACCEPTED,
	PACKET_TYPE_CONNECTION_DENIED,
	PACKET_TYPE_KEEPALIVE,
	PACKET_TYPE_DISCONNECT,
	PACKET_TYPE_CHALLENGE_REQUEST,
	PACKET_TYPE_CHALLENGE_RESPONSE,
	PACKET_TYPE_USERDATA,

	PACKET_TYPE_MAX,
};

extern CUTE_API int CUTE_CALL generate_connect_token(
	uint32_t application_id,
	uint64_t creation_timestamp,
	const crypto_key_t* client_to_server_key,
	const crypto_key_t* server_to_client_key,
	uint64_t expiration_timestamp,
	uint32_t handshake_timeout,
	int endpoint_count,
	const char** endpoint_list,
	uint64_t client_id,
	const uint8_t* user_data,
	const crypto_key_t* shared_secret_key,
	uint8_t* token_ptr_out
);

struct packet_decrypted_connect_token_t
{
	uint64_t expire_timestamp;
	uint64_t client_id;
	uint64_t sequence_offset;
	crypto_key_t key;
	uint16_t endpoint_count;
	endpoint_t endpoints[CUTE_CONNECT_TOKEN_SERVER_COUNT_MAX];
	uint8_t user_data[CUTE_CONNECT_TOKEN_USER_DATA_SIZE];
};

struct packet_encrypted_connect_token_t
{
	uint64_t expiration_timestamp;
	uint8_t nonce[CUTE_CONNECT_TOKEN_NONCE_SIZE];
	uint8_t secret_data[CUTE_CONNECT_TOKEN_SECRET_SECTION_SIZE];
};

struct packet_connection_accepted_t
{
	int client_number;
	int max_clients;
};

struct packet_connection_denied_t
{
	uint8_t packet_type;
};

struct packet_keepalive_t
{
	uint8_t packet_type;
};

struct packet_disconnect_t
{
	uint8_t packet_type;
};

struct packet_challenge_t
{
	uint64_t nonce;
	uint8_t challenge_data[CUTE_CHALLENGE_DATA_SIZE];
};

struct packet_userdata_t
{
	int size;
	uint8_t data[CUTE_PACKET_PAYLOAD_MAX];
};

struct connect_token_client_data_t
{
	uint64_t application_id;
	uint64_t expiration_timestamp;
	uint64_t creation_timestamp;
	crypto_key_t client_to_server_key;
	crypto_key_t server_to_client_key;
	uint32_t handshake_timeout;
	uint16_t endpoint_count;
	endpoint_t endpoints[CUTE_CONNECT_TOKEN_SERVER_COUNT_MAX];
};

extern CUTE_API uint8_t* CUTE_CALL connect_token_process_client_data(uint8_t* buffer, connect_token_client_data_t* token);

struct connect_token_t
{
	uint8_t nonce[CUTE_CONNECT_TOKEN_NONCE_SIZE];
	uint8_t secret_data_and_hmac[CUTE_CONNECT_TOKEN_SECRET_SECTION_SIZE + CUTE_CRYPTO_HMAC_BYTES];
};

}

#endif // CUTE_PROTOCOL_H
