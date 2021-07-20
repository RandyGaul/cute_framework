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

#include <cute_alloc.h>

#include <internal/cute_protocol_internal.h>
using namespace cute;

CUTE_TEST_CASE(test_packet_connection_accepted, "Write, encrypt, decrypt, and assert the *connection accepted packet*.");
int test_packet_connection_accepted()
{
	crypto_key_t key = crypto_generate_key();
	uint64_t sequence = 100;

	protocol::packet_connection_accepted_t packet;
	packet.packet_type = protocol::PACKET_TYPE_CONNECTION_ACCEPTED;
	packet.client_handle = 7;
	packet.max_clients = 32;
	packet.connection_timeout = 10;

	uint8_t buffer[CUTE_PROTOCOL_PACKET_SIZE_MAX];

	int bytes_written = protocol::packet_write(&packet, buffer, sequence, &key);
	CUTE_TEST_ASSERT(bytes_written > 0);

	void* packet_ptr = protocol::packet_open(buffer, bytes_written, &key, NULL, NULL);
	CUTE_TEST_CHECK_POINTER(packet_ptr);
	protocol::packet_connection_accepted_t* packet_val = (protocol::packet_connection_accepted_t*)packet_ptr;

	CUTE_TEST_ASSERT(packet_val->packet_type == packet.packet_type);
	CUTE_TEST_ASSERT(packet_val->client_handle == packet.client_handle);
	CUTE_TEST_ASSERT(packet_val->max_clients == packet.max_clients);
	CUTE_TEST_ASSERT(packet_val->connection_timeout == packet.connection_timeout);

	protocol::packet_allocator_free(NULL, (protocol::packet_type_t)packet.packet_type, packet_ptr);

	return 0;
}

CUTE_TEST_CASE(test_packet_connection_denied, "Write, encrypt, decrypt, and assert the *connection denied packet*.");
int test_packet_connection_denied()
{
	crypto_key_t key = crypto_generate_key();
	uint64_t sequence = 100;

	protocol::packet_connection_denied_t packet;
	packet.packet_type = protocol::PACKET_TYPE_CONNECTION_DENIED;

	uint8_t buffer[CUTE_PROTOCOL_PACKET_SIZE_MAX];

	int bytes_written = protocol::packet_write(&packet, buffer, sequence, &key);
	CUTE_TEST_ASSERT(bytes_written > 0);

	void* packet_ptr = protocol::packet_open(buffer, bytes_written, &key, NULL, NULL);
	CUTE_TEST_CHECK_POINTER(packet_ptr);
	protocol::packet_connection_denied_t* packet_val = (protocol::packet_connection_denied_t*)packet_ptr;

	CUTE_TEST_ASSERT(packet_val->packet_type == packet.packet_type);

	protocol::packet_allocator_free(NULL, (protocol::packet_type_t)packet.packet_type, packet_ptr);

	return 0;
}

CUTE_TEST_CASE(test_packet_keepalive, "Write, encrypt, decrypt, and assert the *keepalive packet*.");
int test_packet_keepalive()
{
	crypto_key_t key = crypto_generate_key();
	uint64_t sequence = 100;

	protocol::packet_keepalive_t packet;
	packet.packet_type = protocol::PACKET_TYPE_KEEPALIVE;

	uint8_t buffer[CUTE_PROTOCOL_PACKET_SIZE_MAX];

	int bytes_written = protocol::packet_write(&packet, buffer, sequence, &key);
	CUTE_TEST_ASSERT(bytes_written > 0);

	void* packet_ptr = protocol::packet_open(buffer, bytes_written, &key, NULL, NULL);
	CUTE_TEST_CHECK_POINTER(packet_ptr);
	protocol::packet_keepalive_t* packet_val = (protocol::packet_keepalive_t*)packet_ptr;

	CUTE_TEST_ASSERT(packet_val->packet_type == packet.packet_type);

	protocol::packet_allocator_free(NULL, (protocol::packet_type_t)packet.packet_type, packet_ptr);

	return 0;
}

CUTE_TEST_CASE(test_packet_disconnect, "Write, encrypt, decrypt, and assert the *disconnect packet*.");
int test_packet_disconnect()
{
	crypto_key_t key = crypto_generate_key();
	uint64_t sequence = 100;

	protocol::packet_disconnect_t packet;
	packet.packet_type = protocol::PACKET_TYPE_DISCONNECT;

	uint8_t buffer[CUTE_PROTOCOL_PACKET_SIZE_MAX];

	int bytes_written = protocol::packet_write(&packet, buffer, sequence, &key);
	CUTE_TEST_ASSERT(bytes_written > 0);

	void* packet_ptr = protocol::packet_open(buffer, bytes_written, &key, NULL, NULL);
	CUTE_TEST_CHECK_POINTER(packet_ptr);
	protocol::packet_disconnect_t* packet_val = (protocol::packet_disconnect_t*)packet_ptr;

	CUTE_TEST_ASSERT(packet_val->packet_type == packet.packet_type);

	protocol::packet_allocator_free(NULL, (protocol::packet_type_t)packet.packet_type, packet_ptr);

	return 0;
}

CUTE_TEST_CASE(test_packet_challenge, "Write, encrypt, decrypt, and assert the *challenge request packet* and *challenge response packet*.");
int test_packet_challenge()
{
	crypto_key_t key = crypto_generate_key();
	uint64_t sequence = 100;

	protocol::packet_challenge_t packet;
	packet.packet_type = protocol::PACKET_TYPE_CHALLENGE_REQUEST;
	packet.challenge_nonce = 30;
	crypto_random_bytes(packet.challenge_data, sizeof(packet.challenge_data));

	uint8_t buffer[CUTE_PROTOCOL_PACKET_SIZE_MAX];

	int bytes_written = protocol::packet_write(&packet, buffer, sequence, &key);
	CUTE_TEST_ASSERT(bytes_written > 0);

	void* packet_ptr = protocol::packet_open(buffer, bytes_written, &key, NULL, NULL);
	CUTE_TEST_CHECK_POINTER(packet_ptr);
	protocol::packet_challenge_t* packet_val = (protocol::packet_challenge_t*)packet_ptr;

	CUTE_TEST_ASSERT(packet_val->packet_type == packet.packet_type);
	CUTE_TEST_ASSERT(packet_val->challenge_nonce == packet.challenge_nonce);
	CUTE_TEST_ASSERT(!(CUTE_MEMCMP(packet_val->challenge_data, packet.challenge_data, sizeof(packet.challenge_data))));

	protocol::packet_allocator_free(NULL, (protocol::packet_type_t)packet.packet_type, packet_ptr);

	return 0;
}

CUTE_TEST_CASE(test_packet_payload, "Write, encrypt, decrypt, and assert the *payload packet*.");
int test_packet_payload()
{
	crypto_key_t key = crypto_generate_key();
	uint64_t sequence = 100;

	protocol::packet_payload_t packet;
	packet.packet_type = protocol::PACKET_TYPE_PAYLOAD;
	packet.payload_size = CUTE_PROTOCOL_PACKET_PAYLOAD_MAX;
	crypto_random_bytes(packet.payload, sizeof(packet.payload));

	uint8_t buffer[CUTE_PROTOCOL_PACKET_SIZE_MAX];

	int bytes_written = protocol::packet_write(&packet, buffer, sequence, &key);
	CUTE_TEST_ASSERT(bytes_written > 0);

	void* packet_ptr = protocol::packet_open(buffer, bytes_written, &key, NULL, NULL);
	CUTE_TEST_CHECK_POINTER(packet_ptr);
	protocol::packet_payload_t* packet_val = (protocol::packet_payload_t*)packet_ptr;

	CUTE_TEST_ASSERT(packet_val->packet_type == packet.packet_type);
	CUTE_TEST_ASSERT(packet_val->payload_size == packet.payload_size);
	CUTE_TEST_ASSERT(!(CUTE_MEMCMP(packet_val->payload, packet.payload, sizeof(packet.payload))));

	protocol::packet_allocator_free(NULL, (protocol::packet_type_t)packet.packet_type, packet_ptr);

	return 0;
}
