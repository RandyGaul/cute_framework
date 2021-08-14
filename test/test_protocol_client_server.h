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

#include <cute_protocol.h>
#include <cute_handle_table.h>

#include <internal/cute_protocol_internal.h>
using namespace cute;

CUTE_TEST_CASE(test_protocol_client_server, "Create client and server, perform connection handshake, then disconnect.");
int test_protocol_client_server()
{
	crypto_key_t client_to_server_key = crypto_generate_key();
	crypto_key_t server_to_client_key = crypto_generate_key();
	crypto_sign_public_t pk;
	crypto_sign_secret_t sk;
	crypto_sign_keygen(&pk, &sk);

	const char* endpoints[] = {
		"[::1]:5000",
	};

	uint64_t application_id = 100;
	uint64_t current_timestamp = 0;
	uint64_t expiration_timestamp = 1;
	uint32_t handshake_timeout = 5;
	uint64_t client_id = 17;

	uint8_t user_data[CUTE_CONNECT_TOKEN_USER_DATA_SIZE];
	crypto_random_bytes(user_data, sizeof(user_data));

	uint8_t connect_token[CUTE_CONNECT_TOKEN_SIZE];
	CUTE_TEST_CHECK(protocol::generate_connect_token(
		application_id,
		current_timestamp,
		&client_to_server_key,
		&server_to_client_key,
		expiration_timestamp,
		handshake_timeout,
		sizeof(endpoints) / sizeof(endpoints[0]),
		endpoints,
		client_id,
		user_data,
		&sk,
		connect_token
	).is_error());

	protocol::server_t* server = protocol::server_make(application_id, &pk, &sk);
	CUTE_TEST_CHECK_POINTER(server);

	protocol::client_t* client = protocol::client_make(5001, application_id, true);
	CUTE_TEST_CHECK_POINTER(client);

	CUTE_TEST_CHECK(protocol::server_start(server, "[::1]:5000", 5).is_error());
	CUTE_TEST_CHECK(protocol::client_connect(client, connect_token).is_error());

	int iters = 0;
	while (iters++ < 100)
	{
		protocol::client_update(client, 0, 0);
		protocol::server_update(server, 0, 0);

		if (protocol::client_get_state(client) <= 0) break;
		if (protocol::client_get_state(client) == protocol::CLIENT_STATE_CONNECTED) break;
	}
	CUTE_TEST_ASSERT(protocol::server_running(server));
	CUTE_TEST_ASSERT(iters < 100);
	CUTE_TEST_ASSERT(protocol::client_get_state(client) == protocol::CLIENT_STATE_CONNECTED);

	protocol::client_disconnect(client);
	protocol::client_destroy(client);

	protocol::server_stop(server);
	protocol::server_destroy(server);

	return 0;
}

CUTE_TEST_CASE(test_protocol_client_no_server_responses, "Client tries to connect to servers, but none respond at all.");
int test_protocol_client_no_server_responses()
{
	crypto_key_t client_to_server_key = crypto_generate_key();
	crypto_key_t server_to_client_key = crypto_generate_key();
	crypto_sign_public_t pk;
	crypto_sign_secret_t sk;
	crypto_sign_keygen(&pk, &sk);

	const char* endpoints[] = {
		"[::1]:5000",
		"[::1]:5001",
		"[::1]:5002",
	};

	uint64_t application_id = 100;
	uint64_t current_timestamp = 0;
	uint64_t expiration_timestamp = 1;
	uint32_t handshake_timeout = 5;
	uint64_t client_id = 17;

	uint8_t user_data[CUTE_CONNECT_TOKEN_USER_DATA_SIZE];
	crypto_random_bytes(user_data, sizeof(user_data));

	uint8_t connect_token[CUTE_CONNECT_TOKEN_SIZE];
	CUTE_TEST_CHECK(protocol::generate_connect_token(
		application_id,
		current_timestamp,
		&client_to_server_key,
		&server_to_client_key,
		expiration_timestamp,
		handshake_timeout,
		sizeof(endpoints) / sizeof(endpoints[0]),
		endpoints,
		client_id,
		user_data,
		&sk,
		connect_token
	).is_error());

	protocol::client_t* client = protocol::client_make(5001, application_id, true);
	CUTE_TEST_CHECK_POINTER(client);
	CUTE_TEST_CHECK(protocol::client_connect(client, connect_token).is_error());

	int iters = 0;
	while (iters++ < 100)
	{
		protocol::client_update(client, 10, 0);

		if (protocol::client_get_state(client) <= 0) break;
	}
	CUTE_TEST_ASSERT(iters < 100);
	CUTE_TEST_ASSERT(protocol::client_get_state(client) == protocol::CLIENT_STATE_CONNECTION_REQUEST_TIMED_OUT);

	protocol::client_disconnect(client);
	protocol::client_destroy(client);

	return 0;
}

CUTE_TEST_CASE(test_protocol_client_server_list, "Client tries to connect to servers, but only third responds.");
int test_protocol_client_server_list()
{
	crypto_key_t client_to_server_key = crypto_generate_key();
	crypto_key_t server_to_client_key = crypto_generate_key();
	crypto_sign_public_t pk;
	crypto_sign_secret_t sk;
	crypto_sign_keygen(&pk, &sk);

	const char* endpoints[] = {
		"[::1]:5000",
		"[::1]:5001",
		"[::1]:5002",
	};

	uint64_t application_id = 100;
	uint64_t current_timestamp = 0;
	uint64_t expiration_timestamp = 1;
	uint32_t handshake_timeout = 15;
	uint64_t client_id = 17;

	uint8_t user_data[CUTE_CONNECT_TOKEN_USER_DATA_SIZE];
	crypto_random_bytes(user_data, sizeof(user_data));

	uint8_t connect_token[CUTE_CONNECT_TOKEN_SIZE];
	CUTE_TEST_CHECK(protocol::generate_connect_token(
		application_id,
		current_timestamp,
		&client_to_server_key,
		&server_to_client_key,
		expiration_timestamp,
		handshake_timeout,
		sizeof(endpoints) / sizeof(endpoints[0]),
		endpoints,
		client_id,
		user_data,
		&sk,
		connect_token
	).is_error());

	protocol::server_t* server = protocol::server_make(application_id, &pk, &sk);
	CUTE_TEST_CHECK_POINTER(server);

	protocol::client_t* client = protocol::client_make(5001, application_id, true);
	CUTE_TEST_CHECK_POINTER(client);

	CUTE_TEST_CHECK(protocol::server_start(server, "[::1]:5002", 5).is_error());
	CUTE_TEST_CHECK(protocol::client_connect(client, connect_token).is_error());

	int iters = 0;
	while (iters++ < 100)
	{
		protocol::client_update(client, 1, 0);
		protocol::server_update(server, 0, 0);

		if (protocol::client_get_state(client) <= 0) break;
		if (protocol::client_get_state(client) == protocol::CLIENT_STATE_CONNECTED) break;
	}
	CUTE_TEST_ASSERT(protocol::server_running(server));
	CUTE_TEST_ASSERT(iters < 100);
	CUTE_TEST_ASSERT(protocol::client_get_state(client) == protocol::CLIENT_STATE_CONNECTED);

	protocol::client_disconnect(client);
	protocol::client_destroy(client);

	protocol::server_stop(server);
	protocol::server_destroy(server);

	return 0;
}

CUTE_TEST_CASE(test_protocol_server_challenge_response_timeout, "Client times out when sending challenge response.");
int test_protocol_server_challenge_response_timeout()
{
	crypto_key_t client_to_server_key = crypto_generate_key();
	crypto_key_t server_to_client_key = crypto_generate_key();
	crypto_sign_public_t pk;
	crypto_sign_secret_t sk;
	crypto_sign_keygen(&pk, &sk);

	const char* endpoints[] = {
		"[::1]:5000",
	};

	uint64_t application_id = 100;
	uint64_t current_timestamp = 0;
	uint64_t expiration_timestamp = 1;
	uint32_t handshake_timeout = 5;
	uint64_t client_id = 17;

	uint8_t user_data[CUTE_CONNECT_TOKEN_USER_DATA_SIZE];
	crypto_random_bytes(user_data, sizeof(user_data));

	uint8_t connect_token[CUTE_CONNECT_TOKEN_SIZE];
	CUTE_TEST_CHECK(protocol::generate_connect_token(
		application_id,
		current_timestamp,
		&client_to_server_key,
		&server_to_client_key,
		expiration_timestamp,
		handshake_timeout,
		sizeof(endpoints) / sizeof(endpoints[0]),
		endpoints,
		client_id,
		user_data,
		&sk,
		connect_token
	).is_error());

	protocol::server_t* server = protocol::server_make(application_id, &pk, &sk);
	CUTE_TEST_CHECK_POINTER(server);

	protocol::client_t* client = protocol::client_make(5001, application_id, true);
	CUTE_TEST_CHECK_POINTER(client);

	CUTE_TEST_CHECK(protocol::server_start(server, "[::1]:5000", 5).is_error());
	CUTE_TEST_CHECK(protocol::client_connect(client, connect_token).is_error());

	int iters = 0;
	while (iters++ < 100)
	{
		protocol::client_update(client, 0.1, 0);

		if (protocol::client_get_state(client) != protocol::CLIENT_STATE_SENDING_CHALLENGE_RESPONSE) {
			protocol::server_update(server, 0, 0);
		}

		if (protocol::client_get_state(client) <= 0) break;
		if (protocol::client_get_state(client) == protocol::CLIENT_STATE_CONNECTED) break;
	}
	CUTE_TEST_ASSERT(protocol::server_running(server));
	CUTE_TEST_ASSERT(iters < 100);
	CUTE_TEST_ASSERT(protocol::client_get_state(client) == protocol::CLIENT_STATE_CHALLENGED_RESPONSE_TIMED_OUT);

	protocol::client_disconnect(client);
	protocol::client_destroy(client);

	protocol::server_stop(server);
	protocol::server_destroy(server);

	return 0;
}

CUTE_TEST_CASE(test_protocol_client_expired_token, "Client gets an expired token before connecting.");
int test_protocol_client_expired_token()
{
	crypto_key_t client_to_server_key = crypto_generate_key();
	crypto_key_t server_to_client_key = crypto_generate_key();
	crypto_sign_public_t pk;
	crypto_sign_secret_t sk;
	crypto_sign_keygen(&pk, &sk);

	const char* endpoints[] = {
		"[::1]:5000",
	};

	uint64_t application_id = 100;
	uint64_t current_timestamp = 0;
	uint64_t expiration_timestamp = 1;
	uint32_t handshake_timeout = 5;
	uint64_t client_id = 17;

	uint8_t user_data[CUTE_CONNECT_TOKEN_USER_DATA_SIZE];
	crypto_random_bytes(user_data, sizeof(user_data));

	uint8_t connect_token[CUTE_CONNECT_TOKEN_SIZE];
	CUTE_TEST_CHECK(protocol::generate_connect_token(
		application_id,
		current_timestamp,
		&client_to_server_key,
		&server_to_client_key,
		expiration_timestamp,
		handshake_timeout,
		sizeof(endpoints) / sizeof(endpoints[0]),
		endpoints,
		client_id,
		user_data,
		&sk,
		connect_token
	).is_error());

	protocol::client_t* client = protocol::client_make(5001, application_id, true);
	CUTE_TEST_CHECK_POINTER(client);
	CUTE_TEST_CHECK(protocol::client_connect(client, connect_token).is_error());
	protocol::client_update(client, 0, 1);
	CUTE_TEST_ASSERT(protocol::client_get_state(client) == protocol::CLIENT_STATE_CONNECT_TOKEN_EXPIRED);

	protocol::client_disconnect(client);
	protocol::client_destroy(client);

	return 0;
}

CUTE_TEST_CASE(test_protocol_client_connect_expired_token, "Client detects its own token expires in the middle of a handshake.");
int test_protocol_client_connect_expired_token()
{
	using namespace protocol;
	
	crypto_key_t client_to_server_key = crypto_generate_key();
	crypto_key_t server_to_client_key = crypto_generate_key();
	crypto_sign_public_t pk;
	crypto_sign_secret_t sk;
	crypto_sign_keygen(&pk, &sk);

	const char* endpoints[] = {
		"[::1]:5000",
	};

	uint64_t application_id = 100;
	uint64_t current_timestamp = 0;
	uint64_t expiration_timestamp = 1;
	uint32_t handshake_timeout = 5;
	uint64_t client_id = 17;

	uint8_t user_data[CUTE_CONNECT_TOKEN_USER_DATA_SIZE];
	crypto_random_bytes(user_data, sizeof(user_data));

	uint8_t connect_token[CUTE_CONNECT_TOKEN_SIZE];
	CUTE_TEST_CHECK(protocol::generate_connect_token(
		application_id,
		current_timestamp,
		&client_to_server_key,
		&server_to_client_key,
		expiration_timestamp,
		handshake_timeout,
		sizeof(endpoints) / sizeof(endpoints[0]),
		endpoints,
		client_id,
		user_data,
		&sk,
		connect_token
	).is_error());

	protocol::server_t* server = protocol::server_make(application_id, &pk, &sk);
	CUTE_TEST_CHECK_POINTER(server);

	protocol::client_t* client = protocol::client_make(5001, application_id, true);
	CUTE_TEST_CHECK_POINTER(client);

	CUTE_TEST_CHECK(protocol::server_start(server, "[::1]:5000", 5).is_error());
	CUTE_TEST_CHECK(protocol::client_connect(client, connect_token).is_error());

	int iters = 0;
	uint64_t time = 0;
	while (iters++ < 100)
	{
		protocol::client_update(client, 0, time++);
		protocol::server_update(server, 0, 0);

		if (protocol::client_get_state(client) <= 0) break;
		if (protocol::client_get_state(client) == protocol::CLIENT_STATE_CONNECTED) break;
	}
	CUTE_TEST_ASSERT(protocol::server_running(server));
	CUTE_TEST_ASSERT(iters < 100);
	CUTE_TEST_ASSERT(protocol::client_get_state(client) == protocol::CLIENT_STATE_CONNECT_TOKEN_EXPIRED);

	protocol::client_disconnect(client);
	protocol::client_destroy(client);

	protocol::server_stop(server);
	protocol::server_destroy(server);

	return 0;
}

CUTE_TEST_CASE(test_protocol_server_connect_expired_token, "Server detects token expires in the middle of a handshake.");
int test_protocol_server_connect_expired_token()
{
	crypto_key_t client_to_server_key = crypto_generate_key();
	crypto_key_t server_to_client_key = crypto_generate_key();
	crypto_sign_public_t pk;
	crypto_sign_secret_t sk;
	crypto_sign_keygen(&pk, &sk);

	const char* endpoints[] = {
		"[::1]:5000",
	};

	uint64_t application_id = 100;
	uint64_t current_timestamp = 0;
	uint64_t expiration_timestamp = 1;
	uint32_t handshake_timeout = 5;
	uint64_t client_id = 17;

	uint8_t user_data[CUTE_CONNECT_TOKEN_USER_DATA_SIZE];
	crypto_random_bytes(user_data, sizeof(user_data));

	uint8_t connect_token[CUTE_CONNECT_TOKEN_SIZE];
	CUTE_TEST_CHECK(protocol::generate_connect_token(
		application_id,
		current_timestamp,
		&client_to_server_key,
		&server_to_client_key,
		expiration_timestamp,
		handshake_timeout,
		sizeof(endpoints) / sizeof(endpoints[0]),
		endpoints,
		client_id,
		user_data,
		&sk,
		connect_token
	).is_error());

	protocol::server_t* server = protocol::server_make(application_id, &pk, &sk);
	CUTE_TEST_CHECK_POINTER(server);

	protocol::client_t* client = protocol::client_make(5001, application_id, true);
	CUTE_TEST_CHECK_POINTER(client);

	CUTE_TEST_CHECK(protocol::server_start(server, "[::1]:5000", 5).is_error());
	CUTE_TEST_CHECK(protocol::client_connect(client, connect_token).is_error());

	int iters = 0;
	uint64_t time = 0;
	while (iters++ < 100)
	{
		++time;
		protocol::client_update(client, 0, time - 1);
		protocol::server_update(server, 0, time);

		if (protocol::client_get_state(client) <= 0) break;
		if (protocol::client_get_state(client) == protocol::CLIENT_STATE_CONNECTED) break;
	}
	CUTE_TEST_ASSERT(protocol::server_running(server));
	CUTE_TEST_ASSERT(iters < 100);
	CUTE_TEST_ASSERT(protocol::client_get_state(client) == protocol::CLIENT_STATE_CONNECT_TOKEN_EXPIRED);

	protocol::client_disconnect(client);
	protocol::client_destroy(client);

	protocol::server_stop(server);
	protocol::server_destroy(server);

	return 0;
}

CUTE_TEST_CASE(test_protocol_client_bad_keys, "Client attempts to connect without keys from REST SECTION of connect token.");
int test_protocol_client_bad_keys()
{
	crypto_key_t client_to_server_key = crypto_generate_key();
	crypto_key_t server_to_client_key = crypto_generate_key();
	crypto_sign_public_t pk;
	crypto_sign_secret_t sk;
	crypto_sign_keygen(&pk, &sk);

	const char* endpoints[] = {
		"[::1]:5000",
	};

	uint64_t application_id = 100;
	uint64_t current_timestamp = 0;
	uint64_t expiration_timestamp = 1;
	uint32_t handshake_timeout = 5;
	uint64_t client_id = 17;

	uint8_t user_data[CUTE_CONNECT_TOKEN_USER_DATA_SIZE];
	crypto_random_bytes(user_data, sizeof(user_data));

	uint8_t connect_token[CUTE_CONNECT_TOKEN_SIZE];
	CUTE_TEST_CHECK(protocol::generate_connect_token(
		application_id,
		current_timestamp,
		&client_to_server_key,
		&server_to_client_key,
		expiration_timestamp,
		handshake_timeout,
		sizeof(endpoints) / sizeof(endpoints[0]),
		endpoints,
		client_id,
		user_data,
		&sk,
		connect_token
	).is_error());

	protocol::server_t* server = protocol::server_make(application_id, &pk, &sk);
	CUTE_TEST_CHECK_POINTER(server);

	protocol::client_t* client = protocol::client_make(5001, application_id, true);
	CUTE_TEST_CHECK_POINTER(client);

	CUTE_TEST_CHECK(protocol::server_start(server, "[::1]:5000", 5).is_error());
	CUTE_TEST_CHECK(protocol::client_connect(client, connect_token).is_error());

	// Invalidate client keys.
	client->connect_token.client_to_server_key = crypto_generate_key();
	client->connect_token.server_to_client_key = crypto_generate_key();

	int iters = 0;
	while (iters++ < 100)
	{
		protocol::client_update(client, 1, 0);
		protocol::server_update(server, 1, 0);

		if (protocol::client_get_state(client) <= 0) break;
		if (protocol::client_get_state(client) == protocol::CLIENT_STATE_CONNECTED) break;
	}
	CUTE_TEST_ASSERT(protocol::server_running(server));
	CUTE_TEST_ASSERT(iters < 100);
	CUTE_TEST_ASSERT(protocol::client_get_state(client) == protocol::CLIENT_STATE_CONNECTION_REQUEST_TIMED_OUT);

	protocol::client_disconnect(client);
	protocol::client_destroy(client);

	server_stop(server);
	server_destroy(server);

	return 0;
}

CUTE_TEST_CASE(test_protocol_server_not_in_list_but_gets_request, "Client tries to connect to server, but token does not contain server endpoint.");
int test_protocol_server_not_in_list_but_gets_request()
{
	crypto_key_t client_to_server_key = crypto_generate_key();
	crypto_key_t server_to_client_key = crypto_generate_key();
	crypto_sign_public_t pk;
	crypto_sign_secret_t sk;
	crypto_sign_keygen(&pk, &sk);

	const char* endpoints[] = {
		"[::1]:5001",
	};

	uint64_t application_id = 100;
	uint64_t current_timestamp = 0;
	uint64_t expiration_timestamp = 1;
	uint32_t handshake_timeout = 5;
	uint64_t client_id = 17;

	uint8_t user_data[CUTE_CONNECT_TOKEN_USER_DATA_SIZE];
	crypto_random_bytes(user_data, sizeof(user_data));

	uint8_t connect_token[CUTE_CONNECT_TOKEN_SIZE];
	CUTE_TEST_CHECK(protocol::generate_connect_token(
		application_id,
		current_timestamp,
		&client_to_server_key,
		&server_to_client_key,
		expiration_timestamp,
		handshake_timeout,
		sizeof(endpoints) / sizeof(endpoints[0]),
		endpoints,
		client_id,
		user_data,
		&sk,
		connect_token
	).is_error());

	protocol::server_t* server = protocol::server_make(application_id, &pk, &sk);
	CUTE_TEST_CHECK_POINTER(server);

	protocol::client_t* client = protocol::client_make(5001, application_id, true);
	CUTE_TEST_CHECK_POINTER(client);

	CUTE_TEST_CHECK(protocol::server_start(server, "[::1]:5000", 5).is_error());
	CUTE_TEST_CHECK(protocol::client_connect(client, connect_token).is_error());

	// This will make packets arrive to correct server address, but connect token has the wrong address.
	CUTE_TEST_CHECK(endpoint_init(client->connect_token.endpoints, "[::1]:5000"));

	int iters = 0;
	while (iters++ < 100)
	{
		protocol::client_update(client, 1, 0);
		protocol::server_update(server, 1, 0);

		if (protocol::client_get_state(client) <= 0) break;
		if (protocol::client_get_state(client) == protocol::CLIENT_STATE_CONNECTED) break;
	}
	CUTE_TEST_ASSERT(protocol::server_running(server));
	CUTE_TEST_ASSERT(iters < 100);
	CUTE_TEST_ASSERT(protocol::client_get_state(client) == protocol::CLIENT_STATE_CONNECTION_REQUEST_TIMED_OUT);

	protocol::client_disconnect(client);
	protocol::client_destroy(client);

	protocol::server_stop(server);
	protocol::server_destroy(server);

	return 0;
}

CUTE_TEST_CASE(test_protocol_connect_a_few_clients, "Multiple clients connecting to one server.");
int test_protocol_connect_a_few_clients()
{
	crypto_key_t client_to_server_key = crypto_generate_key();
	crypto_key_t server_to_client_key = crypto_generate_key();
	crypto_sign_public_t pk;
	crypto_sign_secret_t sk;
	crypto_sign_keygen(&pk, &sk);

	const char* endpoints[] = {
		"[::1]:5000",
	};

	uint64_t application_id = 100;
	uint64_t current_timestamp = 0;
	uint64_t expiration_timestamp = 1;
	uint32_t handshake_timeout = 5;
	uint64_t client_id = 1;

	uint8_t user_data[CUTE_CONNECT_TOKEN_USER_DATA_SIZE];
	crypto_random_bytes(user_data, sizeof(user_data));

	uint8_t connect_token[CUTE_CONNECT_TOKEN_SIZE];

	CUTE_TEST_CHECK(protocol::generate_connect_token(
		application_id,
		current_timestamp,
		&client_to_server_key,
		&server_to_client_key,
		expiration_timestamp,
		handshake_timeout,
		sizeof(endpoints) / sizeof(endpoints[0]),
		endpoints,
		client_id,
		user_data,
		&sk,
		connect_token
	).is_error());
	protocol::client_t* client0 = protocol::client_make(5001, application_id, true);
	CUTE_TEST_CHECK_POINTER(client0);
	CUTE_TEST_CHECK(protocol::client_connect(client0, connect_token).is_error());

	client_to_server_key = crypto_generate_key();
	server_to_client_key = crypto_generate_key();
	client_id = 2;
	CUTE_TEST_CHECK(protocol::generate_connect_token(
		application_id,
		current_timestamp,
		&client_to_server_key,
		&server_to_client_key,
		expiration_timestamp,
		handshake_timeout,
		sizeof(endpoints) / sizeof(endpoints[0]),
		endpoints,
		client_id,
		user_data,
		&sk,
		connect_token
	).is_error());
	protocol::client_t* client1 = protocol::client_make(5002, application_id, true);
	CUTE_TEST_CHECK_POINTER(client1);
	CUTE_TEST_CHECK(protocol::client_connect(client1, connect_token).is_error());

	client_to_server_key = crypto_generate_key();
	server_to_client_key = crypto_generate_key();
	client_id = 3;
	CUTE_TEST_CHECK(protocol::generate_connect_token(
		application_id,
		current_timestamp,
		&client_to_server_key,
		&server_to_client_key,
		expiration_timestamp,
		handshake_timeout,
		sizeof(endpoints) / sizeof(endpoints[0]),
		endpoints,
		client_id,
		user_data,
		&sk,
		connect_token
	).is_error());
	protocol::client_t* client2 = protocol::client_make(5003, application_id, true);
	CUTE_TEST_CHECK_POINTER(client2);
	CUTE_TEST_CHECK(protocol::client_connect(client2, connect_token).is_error());

	protocol::server_t* server = protocol::server_make(application_id, &pk, &sk);
	CUTE_TEST_CHECK_POINTER(server);
	CUTE_TEST_CHECK(protocol::server_start(server, "[::1]:5000", 5).is_error());

	int iters = 0;
	float dt = 1.0f / 60.0f;
	while (iters++ < 100)
	{
		protocol::client_update(client0, dt, 0);
		protocol::client_update(client1, dt, 0);
		protocol::client_update(client2, dt, 0);
		protocol::server_update(server, dt, 0);

		if (protocol::client_get_state(client0) <= 0) break;
		if (protocol::client_get_state(client1) <= 0) break;
		if (protocol::client_get_state(client2) <= 0) break;
		if (protocol::client_get_state(client0) == protocol::CLIENT_STATE_CONNECTED &&
		    protocol::client_get_state(client1) == protocol::CLIENT_STATE_CONNECTED &&
		    protocol::client_get_state(client2) == protocol::CLIENT_STATE_CONNECTED) break;
	}
	CUTE_TEST_ASSERT(protocol::server_running(server));
	CUTE_TEST_ASSERT(iters < 100);
	CUTE_TEST_ASSERT(protocol::client_get_state(client0) == protocol::CLIENT_STATE_CONNECTED);
	CUTE_TEST_ASSERT(protocol::client_get_state(client1) == protocol::CLIENT_STATE_CONNECTED);
	CUTE_TEST_ASSERT(protocol::client_get_state(client2) == protocol::CLIENT_STATE_CONNECTED);

	protocol::client_disconnect(client0);
	protocol::client_disconnect(client1);
	protocol::client_disconnect(client2);
	protocol::client_destroy(client0);
	protocol::client_destroy(client1);
	protocol::client_destroy(client2);

	protocol::server_stop(server);
	protocol::server_destroy(server);

	return 0;
}

CUTE_TEST_CASE(test_protocol_keepalive, "Client and server setup connection and maintain it through keepalive packets.");
int test_protocol_keepalive()
{
	crypto_key_t client_to_server_key = crypto_generate_key();
	crypto_key_t server_to_client_key = crypto_generate_key();
	crypto_sign_public_t pk;
	crypto_sign_secret_t sk;
	crypto_sign_keygen(&pk, &sk);

	const char* endpoints[] = {
		"[::1]:5000",
	};

	uint64_t application_id = 100;
	uint64_t current_timestamp = 0;
	uint64_t expiration_timestamp = 1;
	uint32_t handshake_timeout = 5;
	uint64_t client_id = 1;

	uint8_t user_data[CUTE_CONNECT_TOKEN_USER_DATA_SIZE];
	crypto_random_bytes(user_data, sizeof(user_data));

	uint8_t connect_token[CUTE_CONNECT_TOKEN_SIZE];

	CUTE_TEST_CHECK(protocol::generate_connect_token(
		application_id,
		current_timestamp,
		&client_to_server_key,
		&server_to_client_key,
		expiration_timestamp,
		handshake_timeout,
		sizeof(endpoints) / sizeof(endpoints[0]),
		endpoints,
		client_id,
		user_data,
		&sk,
		connect_token
	).is_error());
	protocol::client_t* client = protocol::client_make(5001, application_id, true);
	CUTE_TEST_CHECK_POINTER(client);
	CUTE_TEST_CHECK(protocol::client_connect(client, connect_token).is_error());

	protocol::server_t* server = protocol::server_make(application_id, &pk, &sk);
	CUTE_TEST_CHECK_POINTER(server);
	CUTE_TEST_CHECK(protocol::server_start(server, "[::1]:5000", 5).is_error());

	int iters = 0;
	float dt = 1.0f / 60.0f;
	while (iters++ < 1000)
	{
		protocol::client_update(client, dt, 0);
		protocol::server_update(server, dt, 0);

		if (protocol::client_get_state(client) <= 0) break;
	}
	CUTE_TEST_ASSERT(protocol::server_running(server));
	CUTE_TEST_ASSERT(iters == 1001);
	CUTE_TEST_ASSERT(protocol::client_get_state(client) == protocol::CLIENT_STATE_CONNECTED);

	protocol::client_disconnect(client);
	protocol::client_destroy(client);

	protocol::server_stop(server);
	protocol::server_destroy(server);

	return 0;
}

CUTE_TEST_CASE(test_protocol_client_initiated_disconnect, "Client initiates disconnect, assert disconnect occurs cleanly.");
int test_protocol_client_initiated_disconnect()
{
	crypto_key_t client_to_server_key = crypto_generate_key();
	crypto_key_t server_to_client_key = crypto_generate_key();
	crypto_sign_public_t pk;
	crypto_sign_secret_t sk;
	crypto_sign_keygen(&pk, &sk);

	const char* endpoints[] = {
		"[::1]:5000",
	};

	uint64_t application_id = 100;
	uint64_t current_timestamp = 0;
	uint64_t expiration_timestamp = 1;
	uint32_t handshake_timeout = 5;
	uint64_t client_id = 1;

	uint8_t user_data[CUTE_CONNECT_TOKEN_USER_DATA_SIZE];
	crypto_random_bytes(user_data, sizeof(user_data));

	uint8_t connect_token[CUTE_CONNECT_TOKEN_SIZE];

	CUTE_TEST_CHECK(protocol::generate_connect_token(
		application_id,
		current_timestamp,
		&client_to_server_key,
		&server_to_client_key,
		expiration_timestamp,
		handshake_timeout,
		sizeof(endpoints) / sizeof(endpoints[0]),
		endpoints,
		client_id,
		user_data,
		&sk,
		connect_token
	).is_error());
	protocol::client_t* client = protocol::client_make(5001, application_id, true);
	CUTE_TEST_CHECK_POINTER(client);
	CUTE_TEST_CHECK(protocol::client_connect(client, connect_token).is_error());

	protocol::server_t* server = protocol::server_make(application_id, &pk, &sk);
	CUTE_TEST_CHECK_POINTER(server);
	CUTE_TEST_CHECK(protocol::server_start(server, "[::1]:5000", 5).is_error());

	int iters = 0;
	float dt = 1.0f / 60.0f;
	while (iters++ < 1000)
	{
		if (protocol::client_get_state(client) > 0) {
			protocol::client_update(client, dt, 0);
		}
		protocol::server_update(server, dt, 0);

		if (iters == 100) {
			CUTE_TEST_ASSERT(protocol::server_client_count(server) == 1);
			protocol::client_disconnect(client);
		}

		if (iters == 110) break;
	}
	CUTE_TEST_ASSERT(protocol::server_running(server));
	CUTE_TEST_ASSERT(protocol::server_client_count(server) == 0);
	CUTE_TEST_ASSERT(iters == 110);
	CUTE_TEST_ASSERT(protocol::client_get_state(client) == protocol::CLIENT_STATE_DISCONNECTED);

	protocol::client_disconnect(client);
	protocol::client_destroy(client);

	protocol::server_stop(server);
	protocol::server_destroy(server);

	return 0;
}

CUTE_TEST_CASE(test_protocol_server_initiated_disconnect, "Server initiates disconnect, assert disconnect occurs cleanly.");
int test_protocol_server_initiated_disconnect()
{
	crypto_key_t client_to_server_key = crypto_generate_key();
	crypto_key_t server_to_client_key = crypto_generate_key();
	crypto_sign_public_t pk;
	crypto_sign_secret_t sk;
	crypto_sign_keygen(&pk, &sk);

	const char* endpoints[] = {
		"[::1]:5000",
	};

	uint64_t application_id = 100;
	uint64_t current_timestamp = 0;
	uint64_t expiration_timestamp = 1;
	uint32_t handshake_timeout = 5;
	uint64_t client_id = 1;

	uint8_t user_data[CUTE_CONNECT_TOKEN_USER_DATA_SIZE];
	crypto_random_bytes(user_data, sizeof(user_data));

	uint8_t connect_token[CUTE_CONNECT_TOKEN_SIZE];

	CUTE_TEST_CHECK(protocol::generate_connect_token(
		application_id,
		current_timestamp,
		&client_to_server_key,
		&server_to_client_key,
		expiration_timestamp,
		handshake_timeout,
		sizeof(endpoints) / sizeof(endpoints[0]),
		endpoints,
		client_id,
		user_data,
		&sk,
		connect_token
	).is_error());
	protocol::client_t* client = protocol::client_make(5001, application_id, true);
	CUTE_TEST_CHECK_POINTER(client);
	CUTE_TEST_CHECK(protocol::client_connect(client, connect_token).is_error());

	protocol::server_t* server = protocol::server_make(application_id, &pk, &sk);
	CUTE_TEST_CHECK_POINTER(server);
	CUTE_TEST_CHECK(protocol::server_start(server, "[::1]:5000", 5).is_error());

	int client_index;

	int iters = 0;
	float dt = 1.0f / 60.0f;
	while (iters++ < 1000)
	{
		protocol::client_update(client, dt, 0);
		protocol::server_update(server, dt, 0);

		if (iters == 100) {
			CUTE_TEST_ASSERT(protocol::server_client_count(server) == 1);
			protocol::server_event_t event;
			CUTE_TEST_ASSERT(protocol::server_pop_event(server, &event));
			CUTE_TEST_ASSERT(event.type == protocol::SERVER_EVENT_NEW_CONNECTION);
			client_index = event.u.new_connection.client_index;
			protocol::server_disconnect_client(server, client_index, true);
		}

		if (iters == 110) break;
	}
	CUTE_TEST_ASSERT(protocol::server_running(server));
	CUTE_TEST_ASSERT(protocol::server_client_count(server) == 0);
	CUTE_TEST_ASSERT(iters == 110);
	CUTE_TEST_ASSERT(protocol::client_get_state(client) == protocol::CLIENT_STATE_DISCONNECTED);
	protocol::server_event_t event;
	CUTE_TEST_ASSERT(protocol::server_pop_event(server, &event));
	CUTE_TEST_ASSERT(event.type == protocol::SERVER_EVENT_DISCONNECTED);
	CUTE_TEST_ASSERT(event.u.disconnected.client_index == client_index);

	protocol::client_disconnect(client);
	protocol::client_destroy(client);

	protocol::server_stop(server);
	protocol::server_destroy(server);

	return 0;
}

CUTE_TEST_CASE(test_protocol_client_server_payloads, "Client and server connect and send payload packets. Server should confirm client.");
int test_protocol_client_server_payloads()
{
	crypto_key_t client_to_server_key = crypto_generate_key();
	crypto_key_t server_to_client_key = crypto_generate_key();
	crypto_sign_public_t pk;
	crypto_sign_secret_t sk;
	crypto_sign_keygen(&pk, &sk);

	const char* endpoints[] = {
		"[::1]:5000",
	};

	uint64_t application_id = 100;
	uint64_t current_timestamp = 0;
	uint64_t expiration_timestamp = 1;
	uint32_t handshake_timeout = 5;
	uint64_t client_id = 17;

	uint8_t user_data[CUTE_CONNECT_TOKEN_USER_DATA_SIZE];
	crypto_random_bytes(user_data, sizeof(user_data));

	uint8_t connect_token[CUTE_CONNECT_TOKEN_SIZE];
	CUTE_TEST_CHECK(protocol::generate_connect_token(
		application_id,
		current_timestamp,
		&client_to_server_key,
		&server_to_client_key,
		expiration_timestamp,
		handshake_timeout,
		sizeof(endpoints) / sizeof(endpoints[0]),
		endpoints,
		client_id,
		user_data,
		&sk,
		connect_token
	).is_error());

	protocol::server_t* server = protocol::server_make(application_id, &pk, &sk);
	CUTE_TEST_CHECK_POINTER(server);

	protocol::client_t* client = protocol::client_make(5001, application_id, true);
	CUTE_TEST_CHECK_POINTER(client);

	CUTE_TEST_CHECK(protocol::server_start(server, "[::1]:5000", 5).is_error());
	CUTE_TEST_CHECK(protocol::client_connect(client, connect_token).is_error());

	int client_index;
	uint64_t to_server_data = 3;
	uint64_t to_client_data = 4;

	int iters = 0;
	float dt = 1.0f / 60.0f;
	int payloads_received_by_server = 0;
	int payloads_received_by_client = 0;
	while (iters++ < 1000)
	{
		protocol::client_update(client, dt, 0);
		protocol::server_update(server, dt, 0);

		protocol::server_event_t event;
		if (protocol::server_pop_event(server, &event)) {
			CUTE_TEST_ASSERT(event.type != protocol::SERVER_EVENT_DISCONNECTED);
			if (event.type == protocol::SERVER_EVENT_NEW_CONNECTION) {
				client_index = event.u.new_connection.client_index;
				CUTE_TEST_ASSERT(protocol::server_get_client_id(server, client_index) == client_id);
			} else {
				CUTE_TEST_ASSERT(client_index == event.u.payload_packet.client_index);
				CUTE_TEST_ASSERT(sizeof(uint64_t) == event.u.payload_packet.size);
				uint64_t* data = (uint64_t*)event.u.payload_packet.data;
				CUTE_TEST_ASSERT(*data == to_server_data);
				protocol::server_free_packet(server, data);
				++payloads_received_by_server;
			}
		}

		void* packet = NULL;
		uint64_t sequence = ~0ULL;
		int size;
		if (protocol::client_get_packet(client, &packet, &size, &sequence)) {
			CUTE_TEST_ASSERT(sizeof(uint64_t) == size);
			uint64_t* data = (uint64_t*)packet;
			CUTE_TEST_ASSERT(*data == to_client_data);
			protocol::client_free_packet(client, packet);
			++payloads_received_by_client;
		}

		if (protocol::client_get_state(client) <= 0) break;
		if (protocol::client_get_state(client) == protocol::CLIENT_STATE_CONNECTED) {
			CUTE_TEST_CHECK(protocol::client_send(client, &to_server_data, sizeof(uint64_t)).is_error());
			CUTE_TEST_CHECK(protocol::server_send_to_client(server, &to_client_data, sizeof(uint64_t), client_index).is_error());
		}

		if (payloads_received_by_server >= 10 && payloads_received_by_client >= 10) break;
	}
	CUTE_TEST_ASSERT(protocol::server_running(server));
	CUTE_TEST_ASSERT(iters < 1000);
	CUTE_TEST_ASSERT(protocol::client_get_state(client) == protocol::CLIENT_STATE_CONNECTED);

	protocol::client_disconnect(client);
	protocol::client_destroy(client);

	protocol::server_stop(server);
	protocol::server_destroy(server);

	return 0;
}

CUTE_TEST_CASE(test_protocol_multiple_connections_and_payloads, "A server hosts multiple simultaneous clients with payloads and random disconnects/connects.");
int test_protocol_multiple_connections_and_payloads()
{
	crypto_sign_public_t pk;
	crypto_sign_secret_t sk;
	crypto_sign_keygen(&pk, &sk);

	const char* endpoints[] = {
		"[::1]:5000",
	};

	const int max_clients = 5;
	uint64_t application_id = 100;

	protocol::server_t* server = protocol::server_make(application_id, &pk, &sk, NULL);
	CUTE_TEST_CHECK_POINTER(server);
	CUTE_TEST_CHECK(protocol::server_start(server, "[::1]:5000", 2).is_error());

	uint64_t current_timestamp = 0;
	uint64_t expiration_timestamp = 1;
	uint32_t handshake_timeout = 2;
	uint8_t user_data[CUTE_CONNECT_TOKEN_USER_DATA_SIZE];
	uint8_t connect_token[CUTE_CONNECT_TOKEN_SIZE];
	crypto_random_bytes(user_data, sizeof(user_data));

	protocol::client_t** clients = (protocol::client_t**)CUTE_ALLOC(sizeof(protocol::client_t*) * max_clients, NULL);

	for (int i = 0; i < max_clients; ++i)
	{
		crypto_key_t client_to_server_key = crypto_generate_key();
		crypto_key_t server_to_client_key = crypto_generate_key();
		uint64_t client_id = (uint64_t)i;

		CUTE_TEST_CHECK(protocol::generate_connect_token(
			application_id,
			current_timestamp,
			&client_to_server_key,
			&server_to_client_key,
			expiration_timestamp,
			handshake_timeout,
			sizeof(endpoints) / sizeof(endpoints[0]),
			endpoints,
			client_id,
			user_data,
			&sk,
			connect_token
		).is_error());
		protocol::client_t* client = protocol::client_make(5000 + i, application_id, true);
		CUTE_TEST_CHECK_POINTER(client);
		CUTE_TEST_CHECK(protocol::client_connect(client, connect_token).is_error());
		clients[i] = client;
	}

	uint64_t to_server_data = 3;
	uint64_t to_client_data = 4;

	int iters = 0;
	float dt = 1.0f / 20.0f;
	int payloads_received_by_server = 0;
	int* payloads_received_by_client = (int*)CUTE_ALLOC(sizeof(int) * max_clients, NULL);
	CUTE_MEMSET(payloads_received_by_client, 0, sizeof(int) * max_clients);
	int client_count = 2;
	while (iters++ < 100) {
		for (int i = 0; i < client_count; ++i)
			protocol::client_update(clients[i], dt, 0);

		protocol::server_update(server, dt, 0);

		for (int i = 0; i < client_count; ++i)
			if (protocol::client_get_state(clients[i]) <= 0)
				break;

		if (iters == 4) {
			client_count += 3;
		}

		if (iters == 8) {
			client_count -= 2;
		}

		protocol::server_event_t event;
		while (protocol::server_pop_event(server, &event)) {
			if (event.type == protocol::SERVER_EVENT_PAYLOAD_PACKET) {
				CUTE_TEST_ASSERT(sizeof(uint64_t) == event.u.payload_packet.size);
				uint64_t* data = (uint64_t*)event.u.payload_packet.data;
				CUTE_TEST_ASSERT(*data == to_server_data);
				protocol::server_free_packet(server, data);
				++payloads_received_by_server;
			}
		}

		for (int i = 0; i < client_count; ++i) {
			void* packet = NULL;
			uint64_t sequence = ~0ULL;
			int size;
			if (protocol::client_get_packet(clients[i], &packet, &size, &sequence)) {
				CUTE_TEST_ASSERT(sizeof(uint64_t) == size);
				uint64_t* data = (uint64_t*)packet;
				CUTE_TEST_ASSERT(*data == to_client_data);
				protocol::client_free_packet(clients[i], packet);
				payloads_received_by_client[i]++;
			}
		}

		for (int i = 0; i < client_count; ++i) {
			if (protocol::client_get_state(clients[i]) == protocol::CLIENT_STATE_CONNECTED) {
				CUTE_TEST_CHECK(protocol::client_send(clients[i], &to_server_data, sizeof(uint64_t)).is_error());
				CUTE_TEST_CHECK(protocol::server_send_to_client(server, &to_client_data, sizeof(uint64_t), i).is_error());
			}
		}
	}
	for (int i = 0; i < client_count; ++i)
	{
		CUTE_TEST_ASSERT(payloads_received_by_client[i] >= 1);
	}
	for (int i = 0; i < max_clients; ++i)
	{
		protocol::client_update(clients[i], 0, 0);
		if (i >= client_count) {
			CUTE_TEST_ASSERT(protocol::client_get_state(clients[i]) == protocol::CLIENT_STATE_DISCONNECTED);
		} else {
			CUTE_TEST_ASSERT(protocol::client_get_state(clients[i]) == protocol::CLIENT_STATE_CONNECTED);
		}
		protocol::client_disconnect(clients[i]);
		protocol::client_destroy(clients[i]);
	}

	protocol::server_update(server, dt, 0);
	protocol::server_stop(server);
	protocol::server_destroy(server);

	CUTE_FREE(clients, NULL);
	CUTE_FREE(payloads_received_by_client, NULL);

	return 0;
}

CUTE_TEST_CASE(test_protocol_client_reconnect, "Client connects to server, disconnects, and reconnects.");
int test_protocol_client_reconnect()
{
	crypto_key_t client_to_server_key = crypto_generate_key();
	crypto_key_t server_to_client_key = crypto_generate_key();
	crypto_sign_public_t pk;
	crypto_sign_secret_t sk;
	crypto_sign_keygen(&pk, &sk);

	const char* endpoints[] = {
		"[::1]:5000",
	};

	uint64_t application_id = 100;
	uint64_t current_timestamp = 0;
	uint64_t expiration_timestamp = 1;
	uint32_t handshake_timeout = 5;
	uint64_t client_id = 17;

	uint8_t user_data[CUTE_CONNECT_TOKEN_USER_DATA_SIZE];
	crypto_random_bytes(user_data, sizeof(user_data));

	uint8_t connect_token[CUTE_CONNECT_TOKEN_SIZE];
	CUTE_TEST_CHECK(protocol::generate_connect_token(
		application_id,
		current_timestamp,
		&client_to_server_key,
		&server_to_client_key,
		expiration_timestamp,
		handshake_timeout,
		sizeof(endpoints) / sizeof(endpoints[0]),
		endpoints,
		client_id,
		user_data,
		&sk,
		connect_token
	).is_error());

	protocol::server_t* server = protocol::server_make(application_id, &pk, &sk);
	CUTE_TEST_CHECK_POINTER(server);

	protocol::client_t* client = protocol::client_make(5001, application_id, true);
	CUTE_TEST_CHECK_POINTER(client);

	CUTE_TEST_CHECK(protocol::server_start(server, "[::1]:5000", 5).is_error());
	CUTE_TEST_CHECK(protocol::client_connect(client, connect_token).is_error());

	// Connect client.
	int iters = 0;
	while (iters++ < 100)
	{
		protocol::client_update(client, 0, 0);
		protocol::server_update(server, 0, 0);

		if (protocol::client_get_state(client) <= 0) break;
		if (protocol::client_get_state(client) == protocol::CLIENT_STATE_CONNECTED) break;
	}
	CUTE_TEST_ASSERT(protocol::server_running(server));
	CUTE_TEST_ASSERT(iters < 100);
	CUTE_TEST_ASSERT(protocol::client_get_state(client) == protocol::CLIENT_STATE_CONNECTED);

	// Disonnect client.
	protocol::client_disconnect(client);
	CUTE_TEST_ASSERT(protocol::client_get_state(client) == protocol::CLIENT_STATE_DISCONNECTED);

	iters = 0;
	while (iters++ < 100)
	{
		protocol::server_update(server, 0, 0);
		if (protocol::server_client_count(server) == 0) break;
	}
	CUTE_TEST_ASSERT(iters < 100);

	// Generate new connect token.
	CUTE_TEST_CHECK(protocol::generate_connect_token(
		application_id,
		current_timestamp,
		&client_to_server_key,
		&server_to_client_key,
		expiration_timestamp,
		handshake_timeout,
		sizeof(endpoints) / sizeof(endpoints[0]),
		endpoints,
		client_id,
		user_data,
		&sk,
		connect_token
	).is_error());

	// Reconnect client.
	CUTE_TEST_CHECK(protocol::client_connect(client, connect_token).is_error());
	iters = 0;
	while (iters++ < 100)
	{
		protocol::client_update(client, 0, 0);
		protocol::server_update(server, 0, 0);

		if (protocol::client_get_state(client) <= 0) break;
		if (protocol::client_get_state(client) == protocol::CLIENT_STATE_CONNECTED) break;
	}
	CUTE_TEST_ASSERT(protocol::server_running(server));
	CUTE_TEST_ASSERT(iters < 100);
	CUTE_TEST_ASSERT(protocol::client_get_state(client) == protocol::CLIENT_STATE_CONNECTED);

	protocol::client_disconnect(client);
	protocol::client_destroy(client);

	protocol::server_update(server, 0, 0);
	protocol::server_stop(server);
	protocol::server_destroy(server);

	return 0;
}
