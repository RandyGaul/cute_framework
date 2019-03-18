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

#ifndef CUTE_NET_UTILS_H
#define CUTE_NET_UTILS_H

#include <cute_crypto.h>

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

enum address_type_t : int
{
	ADDRESS_TYPE_NONE,
	ADDRESS_TYPE_IPV4,
	ADDRESS_TYPE_IPV6
};

struct endpoint_t
{
	address_type_t type;
	uint16_t port;

	union
	{
		uint32_t ipv4;
		uint16_t ipv6[8];
	} u;

	crypto_key_t public_key;
};

endpoint_t endpoint_make(int16_t port);
endpoint_t endpoint_make(uint8_t a, uint8_t b, uint8_t c, uint8_t d, int16_t port);
endpoint_t endpoint_make(sockaddr_storage* sockaddr);
endpoint_t endpoint_make(const char* address_and_port_string);

void endpoint_to_string(endpoint_t endpoint, void* buffer, int buffer_byte_count);
int endpoint_equals(endpoint_t a, endpoint_t b);

}

#endif // CUTE_NET_UTILS_H
