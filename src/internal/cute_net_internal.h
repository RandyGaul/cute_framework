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

namespace cute
{

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

namespace internal
{
	extern CUTE_API int CUTE_CALL net_init();
	extern CUTE_API void CUTE_CALL net_cleanup();
}

}

#endif // CUTE_NET_INTERNAL_H
