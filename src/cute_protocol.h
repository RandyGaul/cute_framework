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
#define CUTE_PROTOCOL_CLIENT_MAX 32
#define CUTE_PROTOCOL_PACKET_SIZE_MAX (CUTE_KB + 256)
#define CUTE_PROTOCOL_PACKET_PAYLOAD_MAX (1255 - 2)

#define CUTE_CONNECT_TOKEN_PACKET_SIZE 1024
#define CUTE_CONNECT_TOKEN_SIZE 1114
#define CUTE_CONNECT_TOKEN_USER_DATA_SIZE 256
#define CUTE_CONNECT_TOKEN_NONCE_SIZE CUTE_CRYPTO_NONCE_BYTES
#define CUTE_CONNECT_TOKEN_SECRET_SECTION_SIZE (8 + 32 + 32 + 256)
#define CUTE_CONNECT_TOKEN_ENDPOINT_MAX 32

#define CUTE_PACKET_QUEUE_MAX_ENTRIES (2 * 1024)
#define CUTE_REPLAY_BUFFER_SIZE 256
#define CUTE_KEEPALIVE_RATE (1.0f / 10.0f)
#define CUTE_DISCONNECT_REDUNDANT_PACKET_COUNT 10
#define CUTE_CHALLENGE_DATA_SIZE 256

namespace cute
{
namespace protocol
{

enum packet_type_t : uint8_t
{
	PACKET_TYPE_CONNECT_TOKEN,
	PACKET_TYPE_CONNECTION_ACCEPTED,
	PACKET_TYPE_CONNECTION_DENIED,
	PACKET_TYPE_KEEPALIVE,
	PACKET_TYPE_DISCONNECT,
	PACKET_TYPE_CHALLENGE_REQUEST,
	PACKET_TYPE_CHALLENGE_RESPONSE,
	PACKET_TYPE_PAYLOAD,

	PACKET_TYPE_MAX,
};

extern CUTE_API int CUTE_CALL generate_connect_token(
	uint64_t application_id,
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

}
}

#endif // CUTE_PROTOCOL_H
