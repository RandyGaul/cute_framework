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

#include <internal/cute_net_internal.h>

CUTE_TEST_CASE(test_socket_init_send_recieve_shutdown, "Test sending one packet on an ipv4 socket, and then retrieve it.");
int test_socket_init_send_recieve_shutdown()
{
	socket_t socket;
	endpoint_t endpoint;
	CUTE_TEST_CHECK(endpoint_init(&endpoint, "127.0.0.1:5000"));
	CUTE_TEST_CHECK(socket_init(&socket, endpoint, CUTE_MB, CUTE_MB));

	const char* message_string = "The message.";
	int message_length = (int)CUTE_STRLEN(message_string) + 1;
	uint8_t* message_buffer = (uint8_t*)malloc(sizeof(uint8_t) * message_length + CUTE_CRYPTO_SYMMETRIC_KEY_MAC_BYTES);
	CUTE_MEMCPY(message_buffer, message_string, message_length);

	int bytes_sent = socket_send(&socket, message_buffer, message_length);
	CUTE_TEST_ASSERT(bytes_sent == message_length);
	CUTE_MEMSET(message_buffer, 0, message_length);
	CUTE_TEST_ASSERT(CUTE_MEMCMP(message_buffer, message_string, message_length));

	endpoint_t from;
	int bytes_recieved = socket_receive(&socket, &from, message_buffer, message_length);
	CUTE_TEST_ASSERT(bytes_recieved == message_length);
	CUTE_TEST_ASSERT(endpoint_equals(endpoint, from));
	CUTE_TEST_ASSERT(!CUTE_MEMCMP(message_buffer, message_string, message_length));

	free(message_buffer);

	socket_cleanup(&socket);

	return 0;
}
