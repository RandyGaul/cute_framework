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

CUTE_TEST_CASE(test_generate_connect_token, "Basic test to generate a connect token and assert the expected token.");
int test_generate_connect_token()
{
	crypto_key_t client_to_server_key = crypto_generate_key();
	crypto_key_t server_to_client_key = crypto_generate_key();
	crypto_sign_public_t pk;
	crypto_sign_secret_t sk;
	crypto_sign_keygen(&pk, &sk);

	const char* endpoints[] = {
		"[::1]:5000",
		"[::1]:5001",
		"[::1]:5002"
	};

	uint64_t application_id = ~0ULL;
	uint64_t current_timestamp = 0;
	uint64_t expiration_timestamp = 1;
	uint32_t handshake_timeout = 10;
	uint64_t client_id = 17;

	uint8_t user_data[CUTE_CONNECT_TOKEN_USER_DATA_SIZE];
	crypto_random_bytes(user_data, sizeof(user_data));

	uint8_t token_buffer[CUTE_CONNECT_TOKEN_SIZE];
	CUTE_TEST_CHECK(protocol::generate_connect_token(
		application_id,
		current_timestamp,
		&client_to_server_key,
		&server_to_client_key,
		expiration_timestamp,
		handshake_timeout,
		3,
		endpoints,
		client_id,
		user_data,
		&sk,
		token_buffer
	).is_error());

	// Assert reading token from web service as a client.
	protocol::connect_token_t token;
	uint8_t* connect_token_packet = protocol::client_read_connect_token_from_web_service(token_buffer, application_id, current_timestamp, &token);
	CUTE_TEST_CHECK_POINTER(connect_token_packet);

	CUTE_TEST_ASSERT(token.creation_timestamp == current_timestamp);
	CUTE_TEST_ASSERT(!CUTE_MEMCMP(&client_to_server_key, &token.client_to_server_key, sizeof(crypto_key_t)));
	CUTE_TEST_ASSERT(!CUTE_MEMCMP(&server_to_client_key, &token.server_to_client_key, sizeof(crypto_key_t)));
	CUTE_TEST_ASSERT(token.expiration_timestamp == expiration_timestamp);
	CUTE_TEST_ASSERT(token.handshake_timeout == handshake_timeout);
	CUTE_TEST_ASSERT(token.endpoint_count == 3);
	for (int i = 0; i < token.endpoint_count; ++i)
	{
		endpoint_t endpoint;
		CUTE_TEST_CHECK(endpoint_init(&endpoint, endpoints[i]));
		CUTE_TEST_ASSERT(endpoint_equals(token.endpoints[i], endpoint));
	}

	// Assert reading *connect token packet* as server, and decrypting it successfully.
	protocol::connect_token_decrypted_t decrypted_token;
	CUTE_TEST_CHECK(protocol::server_decrypt_connect_token_packet(connect_token_packet, &pk, &sk, application_id, current_timestamp, &decrypted_token).is_error());
	CUTE_TEST_ASSERT(decrypted_token.expiration_timestamp == expiration_timestamp);
	CUTE_TEST_ASSERT(decrypted_token.handshake_timeout == handshake_timeout);
	CUTE_TEST_ASSERT(decrypted_token.endpoint_count == 3);
	for (int i = 0; i < token.endpoint_count; ++i)
	{
		endpoint_t endpoint;
		CUTE_TEST_CHECK(endpoint_init(&endpoint, endpoints[i]));
		CUTE_TEST_ASSERT(endpoint_equals(decrypted_token.endpoints[i], endpoint));
	}
	CUTE_TEST_ASSERT(decrypted_token.client_id == client_id);
	CUTE_TEST_ASSERT(!CUTE_MEMCMP(&client_to_server_key, &decrypted_token.client_to_server_key, sizeof(crypto_key_t)));
	CUTE_TEST_ASSERT(!CUTE_MEMCMP(&server_to_client_key, &decrypted_token.server_to_client_key, sizeof(crypto_key_t)));

	return 0;
}
