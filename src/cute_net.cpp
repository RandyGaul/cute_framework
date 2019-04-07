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

#include <cute_defines.h>
#include <cute_alloc.h>
#include <cute_net.h>
#include <cute_circular_buffer.h>
#include <cute_c_runtime.h>
#include <cute_error.h>
#include <cute_memory_pool.h>

#include <internal/cute_net_internal.h>
#include <internal/cute_defines_internal.h>
#include <internal/cute_serialize_internal.h>

#include <cute/cute_serialize.h>

namespace cute
{

void socket_cleanup(socket_t* socket)
{
	CUTE_ASSERT(socket);

	if (socket->handle != 0)
	{
#if _MSC_VER
		closesocket(socket->handle);
#else
		close(socket->handle);
#endif
		socket->handle = 0;
	}
}

static int s_socket_init(socket_t* socket, address_type_t address_type, int send_buffer_size, int receive_buffer_size)
{
	socket->handle = ::socket(address_type == ADDRESS_TYPE_IPV6 ? AF_INET6 : AF_INET, SOCK_DGRAM, IPPROTO_UDP);

#ifdef _MSC_VER
	if (socket->handle == INVALID_SOCKET)
#else
	if (socket->handle <= 0)
#endif
	{
		error_set("Failed to create socket.");
		return -1;
	}

	// Allow users to enforce ipv6 only.
	// See: https://msdn.microsoft.com/en-us/library/windows/desktop/ms738574(v=vs.85).aspx
	if (address_type == ADDRESS_TYPE_IPV6)
	{
		int enable = 1;
		if (setsockopt(socket->handle, IPPROTO_IPV6, IPV6_V6ONLY, (char*)&enable, sizeof(enable)) != 0)
		{
			error_set("Failed to strictly set socket only ipv6.");
			socket_cleanup(socket);
			return -1;
		}
	}

	// Increase socket send buffer size.
	if (setsockopt(socket->handle, SOL_SOCKET, SO_SNDBUF, (char*)&send_buffer_size, sizeof(int)) != 0)
	{
		error_set("Failed to set socket send buffer size.");
		socket_cleanup(socket);
		return -1;
	}

	// Increase socket receive buffer size.
	if (setsockopt(socket->handle, SOL_SOCKET, SO_RCVBUF, (char*)&receive_buffer_size, sizeof(int)) != 0)
	{
		error_set("Failed to set socket receive buffer size.");
		socket_cleanup(socket);
		return -1;
	}

	return 0;
}

static int s_socket_bind_port_and_set_non_blocking(socket_t* socket, address_type_t address_type, uint16_t port)
{
	// Binding to port zero means "any port", so record which one was bound.
	if (port == 0)
	{
		if (address_type == ADDRESS_TYPE_IPV6)
		{
			sockaddr_in6 sin;
			socklen_t len = sizeof(sin);
			if (getsockname(socket->handle, (sockaddr*)&sin, &len) == -1)
			{
				error_set("Failed to get ipv6 socket's assigned port number when binding to port 0.");
				socket_cleanup(socket);
				return -1;
			}
			socket->endpoint.port = ntohs(sin.sin6_port);
		}
		else
		{
			struct sockaddr_in sin;
			socklen_t len = sizeof(sin);
			if (getsockname(socket->handle, (struct sockaddr*)&sin, &len) == -1)
			{
				error_set("Failed to get ipv4 socket's assigned port number when binding to port 0.");
				socket_cleanup(socket);
				return -1;
			}
			socket->endpoint.port = ntohs(sin.sin_port);
		}
	}

	// Set non-blocking io.
#ifdef _MSC_VER

	DWORD non_blocking = 1;
	if (ioctlsocket(socket->handle, FIONBIO, &non_blocking) != 0)
	{
		error_set("Failed to set socket to non blocking io.");
		socket_cleanup(socket);
		return -1;
	}

#else

	int non_blocking = 1;
	if (fcntl(socket->handle, F_SETFL, O_NONBLOCK, non_blocking) == -1)
	{
		error_set("Failed to set socket to non blocking io.");
		socket_cleanup(socket);
		return -1;
	}

#endif

	return 0;
}

int socket_init(socket_t* socket, address_type_t address_type, uint16_t port, int send_buffer_size, int receive_buffer_size)
{
	CUTE_MEMSET(&socket->endpoint, 0, sizeof(endpoint_t));
	socket->endpoint.type = address_type;
	socket->endpoint.port = port;

	if (s_socket_init(socket, address_type, send_buffer_size, receive_buffer_size)) {
		return -1;
	}

	// Bind port.
	if (address_type == ADDRESS_TYPE_IPV6)
	{
		sockaddr_in6 socket_endpoint;
		memset(&socket_endpoint, 0, sizeof(sockaddr_in6));
		socket_endpoint.sin6_family = AF_INET6;
		socket_endpoint.sin6_addr = in6addr_any;
		socket_endpoint.sin6_port = htons(port);

		if (bind(socket->handle, (sockaddr*)&socket_endpoint, sizeof(socket_endpoint)) < 0)
		{
			error_set("Failed to bind ipv6 socket.");
			socket_cleanup(socket);
			return -1;
		}
	}
	else
	{
		sockaddr_in socket_endpoint;
		memset(&socket_endpoint, 0, sizeof(socket_endpoint));
		socket_endpoint.sin_family = AF_INET;
		socket_endpoint.sin_addr.s_addr = INADDR_ANY;
		socket_endpoint.sin_port = htons(port);

		if (bind(socket->handle, (sockaddr*)&socket_endpoint, sizeof(socket_endpoint)) < 0)
		{
			error_set("Failed to bind ipv4 socket.");
			socket_cleanup(socket);
			return -1;
		}
	}

	if (s_socket_bind_port_and_set_non_blocking(socket, address_type, port)) {
		return -1;
	}

	return 0;
}

int socket_init(socket_t* socket, const char* address_and_port, int send_buffer_size, int receive_buffer_size)
{
	endpoint_t endpoint;
	if (endpoint_init(&endpoint, address_and_port)) {
		return -1;
	}

	socket->endpoint = endpoint;

	if (s_socket_init(socket, endpoint.type, send_buffer_size, receive_buffer_size)) {
		return -1;
	}

	// Bind port.
	if (endpoint.type == ADDRESS_TYPE_IPV6)
	{
		sockaddr_in6 socket_endpoint;
		memset(&socket_endpoint, 0, sizeof(sockaddr_in6));
		socket_endpoint.sin6_family = AF_INET6;
		for (int i = 0; i < 8; ++i) ((uint16_t*)&socket_endpoint.sin6_addr) [i] = htons(endpoint.u.ipv6[i]);
		socket_endpoint.sin6_port = htons(endpoint.port);

		if (bind(socket->handle, (sockaddr*)&socket_endpoint, sizeof(socket_endpoint)) < 0)
		{
			error_set("Failed to bind ipv6 socket.");
			socket_cleanup(socket);
			return -1;
		}
	}
	else
	{
		sockaddr_in socket_endpoint;
		memset(&socket_endpoint, 0, sizeof(socket_endpoint));
		socket_endpoint.sin_family = AF_INET;
		socket_endpoint.sin_addr.s_addr = (((uint32_t) endpoint.u.ipv4[0]))       |
		                                  (((uint32_t) endpoint.u.ipv4[1]) << 8)  |
		                                  (((uint32_t) endpoint.u.ipv4[2]) << 16) |
		                                  (((uint32_t) endpoint.u.ipv4[3]) << 24);
		socket_endpoint.sin_port = htons(endpoint.port);

		if (bind(socket->handle, (sockaddr*)&socket_endpoint, sizeof(socket_endpoint)) < 0)
		{
			error_set("Failed to bind ipv4 socket.");
			socket_cleanup(socket);
			return -1;
		}
	}

	if (s_socket_bind_port_and_set_non_blocking(socket, endpoint.type, endpoint.port)) {
		return -1;
	}

	return 0;
}

int socket_send(socket_t* socket, endpoint_t send_to, const void* data, int byte_count)
{
	CUTE_ASSERT(data);
	CUTE_ASSERT(byte_count >= 0);
	CUTE_ASSERT(socket->handle != 0);
	endpoint_t endpoint = send_to;
	CUTE_ASSERT(endpoint.type != ADDRESS_TYPE_NONE);

	if (endpoint.type == ADDRESS_TYPE_IPV6)
	{
		sockaddr_in6 socket_address;
		memset(&socket_address, 0, sizeof(socket_address));
		socket_address.sin6_family = AF_INET6;
		int i;
		for (i = 0; i < 8; ++i)
		{
			((uint16_t*) &socket_address.sin6_addr) [i] = htons(endpoint.u.ipv6[i]);
		}
		socket_address.sin6_port = htons(endpoint.port);
		int result = sendto(socket->handle, (const char*)data, byte_count, 0, (sockaddr*)&socket_address, sizeof(socket_address));
		return result;
	}
	else if (endpoint.type == ADDRESS_TYPE_IPV4)
	{
		sockaddr_in socket_address;
		memset(&socket_address, 0, sizeof(socket_address));
		socket_address.sin_family = AF_INET;
		socket_address.sin_addr.s_addr = (((uint32_t)endpoint.u.ipv4[0]))        |
		                                 (((uint32_t)endpoint.u.ipv4[1]) << 8)   |
		                                 (((uint32_t)endpoint.u.ipv4[2]) << 16)  |
		                                 (((uint32_t)endpoint.u.ipv4[3]) << 24);
		socket_address.sin_port = htons(endpoint.port);
		int result = sendto(socket->handle, (const char*)data, byte_count, 0, (sockaddr*)&socket_address, sizeof(socket_address));
		return result;
	}

	return -1;
}

int socket_receive(socket_t* socket, endpoint_t* from, void* data, int byte_count)
{
	CUTE_ASSERT(socket);
	CUTE_ASSERT(socket->handle != 0);
	CUTE_ASSERT(from);
	CUTE_ASSERT(data);
	CUTE_ASSERT(byte_count >= 0);

#ifdef _MSC_VER
	typedef int socklen_t;
#endif

	memset(from, 0, sizeof(*from));

	sockaddr_storage sockaddr_from;
	socklen_t from_length = sizeof(sockaddr_from);
	int result = recvfrom(socket->handle, (char*)data, byte_count, 0, (sockaddr*)&sockaddr_from, &from_length);
	
#ifdef _MSC_VER
	if (result == SOCKET_ERROR)
	{
		int error = WSAGetLastError();
		if (error == WSAEWOULDBLOCK || error == WSAECONNRESET) return 0;
		error_set("The function recvfrom failed.");
		return -1;
	}
#else
	if (result <= 0)
	{
		if (errno == EAGAIN) return 0;
		error_set("The function recvfrom failed.");
		return -1;
	}
#endif

	if (sockaddr_from.ss_family == AF_INET6)
	{
		sockaddr_in6* addr_ipv6 = (sockaddr_in6*) &sockaddr_from;
		from->type = ADDRESS_TYPE_IPV6;
		int i;
		for (i = 0; i < 8; ++i)
		{
			from->u.ipv6[i] = ntohs(((uint16_t*) &addr_ipv6->sin6_addr) [i]);
		}
		from->port = ntohs(addr_ipv6->sin6_port);
	}
	else if (sockaddr_from.ss_family == AF_INET)
	{
		sockaddr_in* addr_ipv4 = (sockaddr_in*) &sockaddr_from;
		from->type = ADDRESS_TYPE_IPV4;
		from->u.ipv4[0] = (uint8_t)((addr_ipv4->sin_addr.s_addr & 0x000000FF));
		from->u.ipv4[1] = (uint8_t)((addr_ipv4->sin_addr.s_addr & 0x0000FF00) >> 8);
		from->u.ipv4[2] = (uint8_t)((addr_ipv4->sin_addr.s_addr & 0x00FF0000) >> 16);
		from->u.ipv4[3] = (uint8_t)((addr_ipv4->sin_addr.s_addr & 0xFF000000) >> 24);
		from->port = ntohs(addr_ipv4->sin_port);
	}
	else
	{
		CUTE_ASSERT(0);
		error_set("The function recvfrom returned an invalid ip format.");
		return -1;
	}
  
	CUTE_ASSERT(result >= 0);
	int bytes_read = result;
	return bytes_read;
}

// -------------------------------------------------------------------------------------------------

static CUTE_INLINE char* s_parse_ipv6_for_port(endpoint_t* endpoint, char* str, int len)
{
	if (*str == '[') {
		int base_index = len - 1;
		for (int i = 0; i < 6; ++i)
		{
			int index = base_index - i;
			if (index < 3) return NULL;
			if (str[index] == ':') {
				endpoint->port = (uint16_t)atoi(str + index + 1);
				str[index - 1] = '\0';
				break;
			}
		}
		CUTE_ASSERT(*str == '[');
		++str;
	}
	return str;
}

static int s_parse_ipv4_for_port(endpoint_t* endpoint, char* str)
{
	int len = (int)CUTE_STRLEN(str);
	int base_index = len - 1;
	for (int i = 0; i < 6; ++i)
	{
		int index = base_index - i;
		if (index < 0) break;
		if (str[index] == ':') {
			endpoint->port = (uint16_t)atoi(str + index + 1);
			str[index] = '\0';
		}
	}
	return len;
}

#define CUTE_ENDPOINT_STRING_MAX_LENGTH INET6_ADDRSTRLEN

int endpoint_init(endpoint_t* endpoint, const char* address_and_port_string)
{
	CUTE_ASSERT(address_and_port_string);
	memset(endpoint, 0, sizeof(*endpoint));

	char buffer[CUTE_ENDPOINT_STRING_MAX_LENGTH];
	CUTE_STRNCPY(buffer, address_and_port_string, CUTE_ENDPOINT_STRING_MAX_LENGTH - 1);
	buffer[CUTE_ENDPOINT_STRING_MAX_LENGTH - 1] = '\0';

	char* str = buffer;
	int len = (int)CUTE_STRLEN(str);

	str = s_parse_ipv6_for_port(endpoint, str, len);

	struct in6_addr sockaddr6;
	if (inet_pton(AF_INET6, str, &sockaddr6) == 1)
	{
		endpoint->type = ADDRESS_TYPE_IPV6;
		int i;
		for (i = 0; i < 8; ++i)
		{
			endpoint->u.ipv6[i] = ntohs(((uint16_t*)&sockaddr6)[i]);
		}
		return 0;
	}

	len = s_parse_ipv4_for_port(endpoint, str);

	struct sockaddr_in sockaddr4;
	if (inet_pton(AF_INET, str, &sockaddr4.sin_addr) == 1)
	{
		endpoint->type = ADDRESS_TYPE_IPV4;
		endpoint->u.ipv4[3] = (uint8_t)((sockaddr4.sin_addr.s_addr & 0xFF000000) >> 24);
		endpoint->u.ipv4[2] = (uint8_t)((sockaddr4.sin_addr.s_addr & 0x00FF0000) >> 16);
		endpoint->u.ipv4[1] = (uint8_t)((sockaddr4.sin_addr.s_addr & 0x0000FF00) >> 8 );
		endpoint->u.ipv4[0] = (uint8_t)((sockaddr4.sin_addr.s_addr & 0x000000FF)      );
		return 0;
	}

	return -1;
}

void endpoint_to_string(endpoint_t endpoint, char* buffer, int buffer_size)
{
	CUTE_ASSERT(buffer);
	CUTE_ASSERT(buffer_size >= 0);

	if (endpoint.type == ADDRESS_TYPE_IPV6) {
		if (endpoint.port == 0) {
			uint16_t ipv6_network_order[8];
			for (int i = 0; i < 8; ++i) ipv6_network_order[i] = htons(endpoint.u.ipv6[i]);
			inet_ntop(AF_INET6, (void*)ipv6_network_order, buffer, CUTE_ENDPOINT_STRING_MAX_LENGTH);
		} else {
			char buffer[INET6_ADDRSTRLEN];
			uint16_t ipv6_network_order[8];
			for (int i = 0; i < 8; ++i) ipv6_network_order[i] = htons(endpoint.u.ipv6[i]);
			inet_ntop(AF_INET6, (void*)ipv6_network_order, buffer, INET6_ADDRSTRLEN);
			CUTE_SNPRINTF(buffer, CUTE_ENDPOINT_STRING_MAX_LENGTH, "[%s]:%d", buffer, endpoint.port);
		}
	} else if (endpoint.type == ADDRESS_TYPE_IPV4) {
		if (endpoint.port != 0) {
			CUTE_SNPRINTF(buffer, CUTE_ENDPOINT_STRING_MAX_LENGTH, "%d.%d.%d.%d:%d", 
				endpoint.u.ipv4[0], 
				endpoint.u.ipv4[1], 
				endpoint.u.ipv4[2], 
				endpoint.u.ipv4[3], 
				endpoint.port);
		} else {
			CUTE_SNPRINTF(buffer, CUTE_ENDPOINT_STRING_MAX_LENGTH, "%d.%d.%d.%d", 
				endpoint.u.ipv4[0], 
				endpoint.u.ipv4[1], 
				endpoint.u.ipv4[2], 
				endpoint.u.ipv4[3]);
		}
	} else {
		CUTE_SNPRINTF(buffer, CUTE_ENDPOINT_STRING_MAX_LENGTH, "%s", "INVALID ADDRESS");
	}
}

int endpoint_equals(endpoint_t a, endpoint_t b)
{
	if (a.type != b.type) return 0;
	if (a.port != b.port) return 0;

	if (a.type == ADDRESS_TYPE_IPV4) {
		for (int i = 0; i < 4; ++i) if (a.u.ipv4[i] != b.u.ipv4[i]) return 0;
	} else if (a.type == ADDRESS_TYPE_IPV6) {
		for (int i = 0; i < 8; ++i) if (a.u.ipv6[i] != b.u.ipv6[i]) return 0;
	} else {
		return 0;
	}

	return 1;
}

// -------------------------------------------------------------------------------------------------

namespace internal
{
	int net_init()
	{
#ifdef _MSC_VER
		WSADATA wsa_data;
		if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != NO_ERROR) {
			error_set("Unable to initialize WSA.");
			return -1;
		}
#else
#endif
		return 0;
	}

	void net_cleanup()
	{;
#ifdef _MSC_VER
		WSACleanup();
#endif
	}
}

}
