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

namespace cute
{

enum packet_type_t : int
{
	PACKET_TYPE_HELLO,
	PACKET_TYPE_CONNECTION_ACCEPTED,
	PACKET_TYPE_CONNECTION_DENIED,
	PACKET_TYPE_KEEP_ALIVE,
	PACKET_TYPE_DISCONNECT,
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

namespace internal
{
	extern CUTE_API int CUTE_CALL net_init();
	extern CUTE_API void CUTE_CALL net_cleanup();
}

}

#endif // CUTE_NET_INTERNAL_H
