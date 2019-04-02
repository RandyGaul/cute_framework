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

#ifndef CUTE_NET_INTERNAL_H
#define CUTE_NET_INTERNAL_H

#include <cute_defines.h>
#include <cute_net.h>
#include <cute_circular_buffer.h>

#ifdef _MSC_VER
#	include <winsock2.h>   // socket
#	include <ws2tcpip.h>   // WSA stuff
#	pragma comment(lib, "ws2_32.lib")
#else
#	include <sys/socket.h> // socket
#	include <fcntl.h>      // fcntl
#	include <arpa/inet.h>  // inet_pton
#	include <unistd.h>     // close
#	include <errno.h>
#endif

#define CUTE_PACKET_QUEUE_MAX_ENTRIES (2 * 1024)
#define CUTE_NONCE_BUFFER_SIZE 256
#define CUTE_KEEPALIVE_RATE 10.0f
#define CUTE_DISCONNECT_REDUNDANT_PACKET_COUNT 10
#define CUTE_CHALLENGE_DATA_SIZE 256
#define CUTE_CONNECT_TOKEN_NONCE_SIZE CUTE_CRYPTO_NONCE_BYTES
#define CUTE_CONNECT_TOKEN_HEADER_SIZE (1 + CUTE_PROTOCOL_VERSION_STRING_LEN + sizeof(uint64_t) + 24 + sizeof(uint64_t))
#define CUTE_CONNECT_TOKEN_SECRET_SIZE (CUTE_PACKET_SIZE_MAX - CUTE_CONNECT_TOKEN_HEADER_SIZE)

struct serialize_t;

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

#ifdef _MSC_VER
	using socket_handle_t = uint64_t;
#else
	using socket_handle_t = int;
#endif

struct socket_t
{
	socket_handle_t handle;
	endpoint_t endpoint;
};

extern CUTE_API int CUTE_CALL socket_init(socket_t* socket, const char* address_and_port, int send_buffer_size, int receive_buffer_size);
extern CUTE_API int CUTE_CALL socket_init(socket_t* socket, address_type_t address_type, uint16_t port, int send_buffer_size, int receive_buffer_size);
extern CUTE_API void CUTE_CALL socket_cleanup(socket_t* socket);
extern CUTE_API int CUTE_CALL socket_send(socket_t* socket, endpoint_t send_to, const void* data, int byte_count);
extern CUTE_API int CUTE_CALL socket_receive(socket_t* socket, endpoint_t* from, void* data, int byte_count);

struct packet_queue_t
{
	int count = 0;
	int index0 = 0;
	int index1 = 0;
	int sizes[CUTE_PACKET_QUEUE_MAX_ENTRIES];
	uint64_t sequences[CUTE_PACKET_QUEUE_MAX_ENTRIES];
	circular_buffer_t packets;
};

extern CUTE_API int CUTE_CALL packet_queue_init(packet_queue_t* q, int size, void* mem_ctx);
extern CUTE_API void CUTE_CALL pack_queue_clean_up(packet_queue_t* q);
extern CUTE_API int CUTE_CALL packet_queue_push(packet_queue_t* q, const void* packet, int size, uint64_t sequence);
extern CUTE_API int CUTE_CALL packet_queue_peek(packet_queue_t* q, int* size);
extern CUTE_API int CUTE_CALL packet_queue_pull(packet_queue_t* q, void* packet, int size, uint64_t* sequence);

struct nonce_buffer_t
{
	uint64_t max;
	uint64_t entries[CUTE_NONCE_BUFFER_SIZE];
};

extern CUTE_API void CUTE_CALL nonce_buffer_init(nonce_buffer_t* buffer);
extern CUTE_API int CUTE_CALL nonce_cull_duplicate(nonce_buffer_t* buffer, uint64_t sequence, uint64_t seed);

struct packet_allocator_t;

extern CUTE_API packet_allocator_t* CUTE_CALL packet_allocator_make(void* user_allocator_context = NULL);
extern CUTE_API void CUTE_CALL packet_allocator_destroy(packet_allocator_t* packet_allocator);
extern CUTE_API void* CUTE_CALL packet_allocator_alloc(packet_allocator_t* packet_allocator, packet_type_t type);
extern CUTE_API void CUTE_CALL packet_allocator_free(packet_allocator_t* packet_allocator, packet_type_t type, void* packet);

int packet_write(void* packet_ptr, packet_type_t packet_type, uint8_t* buffer, uint64_t game_id, uint64_t sequence, const crypto_key_t* key);
void* packet_open(packet_allocator_t* pa, nonce_buffer_t* nonce_buffer, uint64_t game_id, uint64_t timestamp, uint8_t* buffer, int size, uint64_t sequence_offset, const crypto_key_t* key, int is_server, packet_type_t* packet_type);

struct packet_decrypted_connect_token_t
{
	uint64_t expire_timestamp;
	uint64_t client_id;
	uint64_t sequence_offset;
	crypto_key_t session_key;
	uint16_t endpoint_count;
	endpoint_t endpoints[CUTE_CONNECT_TOKEN_SERVER_COUNT_MAX];
	uint8_t user_data[CUTE_CONNECT_TOKEN_USER_DATA_SIZE];
};

struct packet_encrypted_connect_token_t
{
	uint64_t expire_timestamp;
	uint8_t nonce[CUTE_CONNECT_TOKEN_NONCE_SIZE];
	uint8_t secret_data[CUTE_CONNECT_TOKEN_SECRET_SIZE];
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

struct connect_token_t
{
	uint64_t expire_timestamp;
	uint64_t game_id;
	uint64_t sequence_offset;
	crypto_key_t session_key;
	uint16_t endpoint_count;
	endpoint_t endpoints[CUTE_CONNECT_TOKEN_SERVER_COUNT_MAX];
	uint8_t nonce[CUTE_CONNECT_TOKEN_NONCE_SIZE];
	uint8_t secret_data[CUTE_CONNECT_TOKEN_SECRET_SIZE];
};

extern CUTE_API int CUTE_CALL connect_token_open(connect_token_t* token, uint8_t* buffer);

namespace internal
{
	extern CUTE_API int CUTE_CALL net_init();
	extern CUTE_API void CUTE_CALL net_cleanup();
}

}

#endif // CUTE_NET_INTERNAL_H
