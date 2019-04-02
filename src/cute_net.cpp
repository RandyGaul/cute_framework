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

int packet_queue_init(packet_queue_t* q, int size, void* mem_ctx)
{
	CUTE_PLACEMENT_NEW(q) packet_queue_t;
	q->packets = circular_buffer_make(size, mem_ctx);
	if (q->packets.data) return 0;
	else return -1;
}

void pack_queue_clean_up(packet_queue_t* q)
{
	circular_buffer_free(&q->packets);
	CUTE_MEMSET(q, 0, sizeof(*q));
}

int packet_queue_push(packet_queue_t* q, const void* packet, int size, uint64_t sequence)
{
	if (q->count >= CUTE_PACKET_QUEUE_MAX_ENTRIES) return -1;
	if (circular_buffer_push(&q->packets, packet, size)) {
		return -1;
	} else {
		q->count++;
		q->sizes[q->index1] = size;
		q->sequences[q->index1] = sequence;
		q->index1 = (q->index1 + 1) % CUTE_PACKET_QUEUE_MAX_ENTRIES;
		return 0;
	}
}

int packet_queue_peek(packet_queue_t* q, int* size)
{
	if (q->count <= 0) return -1;
	int sz = q->sizes[q->index0];
	*size = sz;
	return 0;
}

int packet_queue_pull(packet_queue_t* q, void* packet, int size, uint64_t* sequence)
{
	if (q->count <= 0) return -1;
	int sz = q->sizes[q->index0];
	if (size != sz) return -1;
	if (circular_buffer_pull(&q->packets, packet, sz)) {
		return -1;
	} else {
		q->count--;
		*sequence = q->sequences[q->index0];
		q->index0 = (q->index0 + 1) % CUTE_PACKET_QUEUE_MAX_ENTRIES;
		return 0;
	}
}

// -------------------------------------------------------------------------------------------------

void nonce_buffer_init(nonce_buffer_t* buffer)
{
	buffer->max = 0;
	CUTE_MEMSET(buffer->entries, ~0, sizeof(uint64_t) * CUTE_NONCE_BUFFER_SIZE);
}

int nonce_cull_duplicate(nonce_buffer_t* buffer, uint64_t sequence, uint64_t seed)
{
	if (sequence + CUTE_NONCE_BUFFER_SIZE < buffer->max) {
		// This is UDP - just drop old packets.
		return -1;
	}

	if (buffer->max < sequence) {
		buffer->max = sequence;
	}

	uint64_t h = sequence + seed;
	int index = (int)(h % CUTE_NONCE_BUFFER_SIZE);
	uint64_t val = buffer->entries[index];
	int empty_slot = val == ~0ULL;
	int outdated = val >= sequence;
	if (empty_slot | !outdated) {
		buffer->entries[index] = sequence;
		return 0;
	} else {
		// Duplicate or replay packet detected.
		return -1;
	}
}

// -------------------------------------------------------------------------------------------------

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
			}
		}
		CUTE_ASSERT(*str == ']');
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

int serialize_endpoint(serialize_t* io, endpoint_t endpoint)
{
	CUTE_CHECK(endpoint.type == ADDRESS_TYPE_NONE);
	int type = endpoint.type == ADDRESS_TYPE_IPV4;
	CUTE_CHECK(serialize_bytes(io, (uint8_t*)type, 1));
	if (endpoint.type == ADDRESS_TYPE_IPV4) {
		CUTE_CHECK(serialize_bytes(io, (uint8_t*)endpoint.u.ipv4, sizeof(uint8_t) * 4));
	} else if (endpoint.type == ADDRESS_TYPE_IPV6) {
		CUTE_CHECK(serialize_bytes(io, (uint8_t*)endpoint.u.ipv6, sizeof(uint16_t) * 8));
	}
	CUTE_CHECK(serialize_bytes(io, (uint8_t*)&endpoint.port, 2));
	return SERIALIZE_SUCCESS;

cute_error:
	return SERIALIZE_FAILURE;
}

int generate_connect_token(
	int address_count,
	const char** address_list_public,
	const char** address_list_secret,
	uint64_t expire_time,
	uint32_t game_id,
	uint64_t client_id,
	const crypto_key_t* key,
	uint8_t* token_ptr_out,
	const uint8_t* user_data,
	void* user_allocator_context)
{
	serialize_t* io = serialize_buffer_create(SERIALIZE_WRITE, token_ptr_out, CUTE_CONNECT_TOKEN_SIZE, NULL);
	CUTE_CHECK_POINTER(io);

	endpoint_t endpoints[CUTE_CONNECT_TOKEN_SERVER_COUNT_MAX];
	for (int i = 0; i < address_count; ++i)
	{
		endpoint_t endpoint;
		CUTE_CHECK(endpoint_init(&endpoint, address_list_public[i]));
		endpoints[i] = endpoint;
	}

	// Write public portion of the connect token.

	CUTE_CHECK(serialize_uint64_full(io, &expire_time));
	CUTE_CHECK(serialize_uint32_full(io, &game_id));
	CUTE_CHECK(serialize_bytes(io, (uint8_t*)key, sizeof(crypto_key_t)));
	CUTE_CHECK(address_count > CUTE_CONNECT_TOKEN_SERVER_COUNT_MAX);
	unsigned address_count_unsigned = (unsigned)address_count;
	CUTE_CHECK(serialize_uint32(io, &address_count_unsigned, 0, CUTE_CONNECT_TOKEN_SERVER_COUNT_MAX));

	for (int i = 0; i < address_count; ++i)
		CUTE_CHECK(serialize_endpoint(io, endpoints[i]));

	// Write the secret portion of the connect token.
	CUTE_CHECK(serialize_uint64_full(io, &client_id));

cute_error:
	if (io) serialize_destroy(io);
	return -1;
}

// -------------------------------------------------------------------------------------------------

int packet_validate_size(int size)
{
	int crypto_bytes = CUTE_CRYPTO_SYMMETRIC_BYTES < CUTE_CRYPTO_ASYMMETRIC_BYTES ? CUTE_CRYPTO_SYMMETRIC_BYTES : CUTE_CRYPTO_ASYMMETRIC_BYTES;
	if (size <= sizeof(uint64_t) + crypto_bytes) {
		// Packet too small to be valid.
		return -1;
	}

	return 0;
}

#define CUTE_ADDITIONAL_DATA_SIZE (CUTE_PROTOCOL_VERSION_STRING_LEN + sizeof(uint64_t) + 1)
static CUTE_INLINE void s_write_header(uint8_t* header, packet_type_t packet_type, uint64_t game_id)
{
	write_uint8(&header, (uint8_t)packet_type);
	write_bytes(&header, CUTE_PROTOCOL_VERSION, CUTE_PROTOCOL_VERSION_STRING_LEN);
	write_uint64(&header, game_id);
}

static uint64_t s_read_uint64(uint8_t** buffer)
{
	uint64_t val = *(uint64_t*)*buffer;
	*buffer += sizeof(uint64_t);
	return val;
}

void* packet_open(packet_allocator_t* pa, nonce_buffer_t* nonce_buffer, uint64_t game_id, uint64_t timestamp, uint8_t* buffer, int size, uint64_t sequence_offset, const crypto_key_t* key, int is_server, packet_type_t* packet_type)
{
	uint8_t* buffer_start = buffer;
	packet_type_t type = (packet_type_t)read_uint8(&buffer);
	*packet_type = type;

	// Filter out invalid packet types.
	if (is_server) {
		switch (type)
		{
		default: return NULL;
		case PACKET_TYPE_CONNECTION_REQUEST: break;
		case PACKET_TYPE_KEEPALIVE: break;
		case PACKET_TYPE_DISCONNECT: break;
		case PACKET_TYPE_CHALLENGE_RESPONSE: break;
		case PACKET_TYPE_USERDATA: break;
		}
	} else {
		switch (type)
		{
		default: return NULL;
		case PACKET_TYPE_CONNECTION_ACCEPTED: break;
		case PACKET_TYPE_CONNECTION_DENIED: break;
		case PACKET_TYPE_KEEPALIVE: break;
		case PACKET_TYPE_DISCONNECT: break;
		case PACKET_TYPE_CHALLENGE_REQUEST: break;
		case PACKET_TYPE_USERDATA: break;
		}
	}

	if (type == PACKET_TYPE_CONNECTION_REQUEST) {
		CUTE_CHECK(size != CUTE_PACKET_SIZE_MAX);

		// Read in and verify the header.
		CUTE_CHECK(read_uint8(&buffer) != 'C');
		CUTE_CHECK(read_uint8(&buffer) != 'U');
		CUTE_CHECK(read_uint8(&buffer) != 'T');
		CUTE_CHECK(read_uint8(&buffer) != 'E');
		CUTE_CHECK(read_uint8(&buffer) != ' ');
		CUTE_CHECK(read_uint8(&buffer) != '1');
		CUTE_CHECK(read_uint8(&buffer) != '.');
		CUTE_CHECK(read_uint8(&buffer) != '0');
		CUTE_CHECK(read_uint8(&buffer) != '\0');
		uint64_t game_id_from_packet = read_uint64(&buffer);
		CUTE_CHECK(game_id_from_packet != game_id);
		uint8_t nonce[CUTE_CRYPTO_NONCE_BYTES];
		read_bytes(&buffer, nonce, CUTE_CRYPTO_NONCE_BYTES);
		uint64_t expire_timestamp = read_uint64(&buffer);
		CUTE_CHECK(expire_timestamp > timestamp);

		// Decrypt secret section of the connect token.
		uint8_t* additional_data = buffer_start;
		int additional_data_size = CUTE_CONNECT_TOKEN_HEADER_SIZE;

		int secret_size = CUTE_PACKET_SIZE_MAX - additional_data_size;
		uint8_t* secret_data = buffer_start + additional_data_size;

		CUTE_CHECK(crypto_decrypt_bignonce(key, secret_data, secret_size, additional_data, additional_data_size, nonce));
		secret_size -= CUTE_CRYPTO_MAC_BYTES;

		// Read the token secret data.
		packet_decrypted_connect_token_t* token = (packet_decrypted_connect_token_t*)packet_allocator_alloc(pa, PACKET_TYPE_CONNECTION_REQUEST);
		token->client_id = read_uint64(&secret_data);
		token->expire_timestamp = expire_timestamp;
		token->sequence_offset = read_uint64(&secret_data);
		read_bytes(&secret_data, token->session_key.key, sizeof(crypto_key_t));
		read_bytes(&secret_data, token->user_data, CUTE_CONNECT_TOKEN_USER_DATA_SIZE);

		// Read the token public data.
		uint16_t endpoint_count = read_uint16(&additional_data);
		token->endpoint_count = endpoint_count;
		for (int i = 0; i < endpoint_count; ++i)
		{
			address_type_t type = (address_type_t)read_uint8(&additional_data);
			if(type == ADDRESS_TYPE_NONE) {
				packet_allocator_free(pa, PACKET_TYPE_CHALLENGE_REQUEST, token);
				return NULL;
			}
			token->endpoints[i].type = type;
			read_bytes(&additional_data, (uint8_t*)(&token->endpoints[i].u), type == ADDRESS_TYPE_IPV4 ? sizeof(endpoint_t::u.ipv4) : sizeof(endpoint_t::u.ipv6));
			token->endpoints[i].port = read_uint16(&additional_data);
		}

		return token;
	} else {
		uint8_t additional_data[CUTE_ADDITIONAL_DATA_SIZE];
		s_write_header(additional_data, type, game_id);
		uint64_t sequence = read_uint64(&buffer);
		if (crypto_decrypt(key, buffer, size - sizeof(uint64_t) - 1, additional_data, CUTE_ADDITIONAL_DATA_SIZE, sequence + sequence_offset) < 0) {
			// Forged packet!
			return NULL;
		}

		if (nonce_cull_duplicate(nonce_buffer, sequence, sequence_offset) < 0) {
			// Duplicate, or very old, packet detected.
			return NULL;
		}

		switch (type)
		{
		case PACKET_TYPE_CONNECTION_ACCEPTED:
		{
			packet_connection_accepted_t* packet = (packet_connection_accepted_t*)packet_allocator_alloc(pa, PACKET_TYPE_CONNECTION_ACCEPTED);
			packet->client_number = read_uint32(&buffer);
			packet->max_clients = read_uint32(&buffer);
			return packet;
		}	break;

		case PACKET_TYPE_CONNECTION_DENIED:
		{
			packet_connection_denied_t* packet = (packet_connection_denied_t*)packet_allocator_alloc(pa, PACKET_TYPE_CONNECTION_DENIED);
			packet->packet_type = (uint8_t)type;
			return packet;
		}	break;

		case PACKET_TYPE_KEEPALIVE:
		{
			packet_connection_denied_t* packet = (packet_connection_denied_t*)packet_allocator_alloc(pa, PACKET_TYPE_KEEPALIVE);
			packet->packet_type = (uint8_t)type;
			return packet;
		}	break;

		case PACKET_TYPE_DISCONNECT:
		{
			packet_connection_denied_t* packet = (packet_connection_denied_t*)packet_allocator_alloc(pa, PACKET_TYPE_DISCONNECT);
			packet->packet_type = (uint8_t)type;
			return packet;
		}	break;

		case PACKET_TYPE_CHALLENGE_REQUEST: // fall-thru
		case PACKET_TYPE_CHALLENGE_RESPONSE:
		{
			packet_challenge_t* packet = (packet_challenge_t*)packet_allocator_alloc(pa, PACKET_TYPE_CHALLENGE_REQUEST);
			packet->nonce = read_uint64(&buffer);
			CUTE_MEMCPY(packet->challenge_data, buffer, sizeof(packet->challenge_data));
			return packet;
		}	break;

		case PACKET_TYPE_USERDATA:
		{
			packet_userdata_t* packet = (packet_userdata_t*)packet_allocator_alloc(pa, PACKET_TYPE_USERDATA);
			packet->size = read_uint32(&buffer);
			CUTE_MEMSET(packet->data, 0, sizeof(packet->data));
			CUTE_MEMCPY(packet->data, buffer, packet->size);
			return packet;
		}	break;

		default: return NULL;
		}
	}

cute_error:
	return NULL;
}

int packet_write(void* packet_ptr, packet_type_t packet_type, uint8_t* buffer, uint64_t game_id, uint64_t sequence, const crypto_key_t* key)
{
	uint8_t* buffer_start = buffer;

	if (packet_type == PACKET_TYPE_CONNECTION_REQUEST) {
		packet_encrypted_connect_token_t* packet = (packet_encrypted_connect_token_t*)packet_ptr;
		write_uint8(&buffer, (uint8_t)PACKET_TYPE_CONNECTION_REQUEST);
		write_bytes(&buffer, CUTE_PROTOCOL_VERSION, CUTE_PROTOCOL_VERSION_STRING_LEN);
		write_uint64(&buffer, game_id);
		write_bytes(&buffer, packet->nonce, sizeof(packet->nonce));
		write_uint64(&buffer, packet->expire_timestamp);
		write_bytes(&buffer, packet->secret_data, sizeof(packet->secret_data));
		int packet_size = (int)(buffer - buffer_start);
		CUTE_ASSERT(packet_size == CUTE_PACKET_SIZE_MAX);
		return packet_size;
	} else {
		write_uint8(&buffer, (uint8_t)packet_type);
		write_uint64(&buffer, sequence);

		uint8_t* payload_start = buffer;
		int payload_size = 0;
		switch (packet_type)
		{
		case PACKET_TYPE_CONNECTION_ACCEPTED:
		{
			packet_connection_accepted_t* packet = (packet_connection_accepted_t*)packet_ptr;
			write_uint32(&buffer, packet->client_number);
			write_uint32(&buffer, packet->max_clients);
		}	break;

		case PACKET_TYPE_CONNECTION_DENIED: // fall-thru
		case PACKET_TYPE_KEEPALIVE: // fall-thru
		case PACKET_TYPE_DISCONNECT:
		{
			// No payload data.
		}	break;

		case PACKET_TYPE_CHALLENGE_REQUEST: // fall-thru
		case PACKET_TYPE_CHALLENGE_RESPONSE:
		{
			packet_challenge_t* packet = (packet_challenge_t*)packet_ptr;
			write_uint64(&buffer, packet->nonce);
			write_bytes(&buffer, packet->challenge_data, sizeof(packet->challenge_data));
		}	break;

		case PACKET_TYPE_USERDATA:
		{
			packet_userdata_t* packet = (packet_userdata_t*)packet_ptr;
			CUTE_ASSERT(packet->size <= CUTE_PACKET_PAYLOAD_MAX);
			CUTE_MEMCPY(buffer, packet->data, packet->size);
			buffer += packet->size;
		}	break;

		default:
			error_set("Attempted to write invalid packet type.");
			CUTE_ASSERT(0);
			return -1;
		}

		uint8_t additional_data[CUTE_ADDITIONAL_DATA_SIZE];
		s_write_header(additional_data, packet_type, game_id);

		if (crypto_encrypt(key, payload_start, payload_size, additional_data, CUTE_ADDITIONAL_DATA_SIZE, sequence) < 0) {
			return -1;
		}

		buffer += CUTE_CRYPTO_MAC_BYTES;

		int packet_size = (int)(buffer - buffer_start);
		CUTE_ASSERT(packet_size <= CUTE_PACKET_SIZE_MAX);
		return packet_size;
	}
}

// -------------------------------------------------------------------------------------------------


packet_allocator_t* packet_allocator_make(void* user_allocator_context)
{
}

void packet_allocator_destroy(packet_allocator_t* packet_allocator)
{
}

void* packet_allocator_alloc(packet_allocator_t* packet_allocator, packet_type_t type)
{
}

void packet_allocator_free(packet_allocator_t* packet_allocator, packet_type_t type, void* packet)
{
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
