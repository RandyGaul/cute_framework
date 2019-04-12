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

#include <internal/cute_protocol_internal.h>
using namespace cute;

CUTE_TEST_CASE(test_protocol_client_server, "Create client and server, perform connection handshake, then disconnect.");
int test_protocol_client_server()
{
	using namespace protocol;
	
	crypto_key_t client_to_server_key = crypto_generate_key();
	crypto_key_t server_to_client_key = crypto_generate_key();
	crypto_key_t secret_key = crypto_generate_key();

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
	CUTE_TEST_CHECK(generate_connect_token(
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
		&secret_key,
		connect_token
	));

	server_t* server = server_make(application_id, &secret_key, NULL);
	CUTE_TEST_CHECK_POINTER(server);

	client_t* client = client_make(5001, NULL, application_id, NULL);
	CUTE_TEST_CHECK_POINTER(client);

	CUTE_TEST_CHECK(server_start(server, "[::1]:5000", 5));
	CUTE_TEST_CHECK(client_connect(client, connect_token));

	int iters = 0;
	while (iters++ < 100)
	{
		client_update(client, 0, 0);
		server_update(server, 0, 0);

		if (client_get_state(client) <= 0) break;
		if (client_get_state(client) == CLIENT_STATE_CONNECTED) break;
	}
	CUTE_TEST_ASSERT(server_running(server));
	CUTE_TEST_ASSERT(iters < 100);
	CUTE_TEST_ASSERT(client_get_state(client) == CLIENT_STATE_CONNECTED);

	client_disconnect(client);
	client_destroy(client);

	server_stop(server);
	server_destroy(server);

	return 0;
}

CUTE_TEST_CASE(test_protocol_client_no_server_responses, "Client tries to connect to servers, but none respond at all.");
int test_protocol_client_no_server_responses()
{
	using namespace protocol;
	
	crypto_key_t client_to_server_key = crypto_generate_key();
	crypto_key_t server_to_client_key = crypto_generate_key();
	crypto_key_t secret_key = crypto_generate_key();

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
	CUTE_TEST_CHECK(generate_connect_token(
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
		&secret_key,
		connect_token
	));

	client_t* client = client_make(5001, NULL, application_id, NULL);
	CUTE_TEST_CHECK_POINTER(client);
	CUTE_TEST_CHECK(client_connect(client, connect_token));

	int iters = 0;
	while (iters++ < 100)
	{
		client_update(client, 10, 0);

		if (client_get_state(client) <= 0) break;
	}
	CUTE_TEST_ASSERT(iters < 100);
	CUTE_TEST_ASSERT(client_get_state(client) == CLIENT_STATE_CONNECTION_REQUEST_TIMED_OUT);

	client_disconnect(client);
	client_destroy(client);

	return 0;
}

CUTE_TEST_CASE(test_protocol_client_server_list, "Client tries to connect to servers, but only third responds.");
int test_protocol_client_server_list()
{
	using namespace protocol;
	
	crypto_key_t client_to_server_key = crypto_generate_key();
	crypto_key_t server_to_client_key = crypto_generate_key();
	crypto_key_t secret_key = crypto_generate_key();

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
	CUTE_TEST_CHECK(generate_connect_token(
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
		&secret_key,
		connect_token
	));

	server_t* server = server_make(application_id, &secret_key, NULL);
	CUTE_TEST_CHECK_POINTER(server);

	client_t* client = client_make(5001, NULL, application_id, NULL);
	CUTE_TEST_CHECK_POINTER(client);

	CUTE_TEST_CHECK(server_start(server, "[::1]:5002", 5));
	CUTE_TEST_CHECK(client_connect(client, connect_token));

	int iters = 0;
	while (iters++ < 100)
	{
		client_update(client, 1, 0);
		server_update(server, 0, 0);

		if (client_get_state(client) <= 0) break;
		if (client_get_state(client) == CLIENT_STATE_CONNECTED) break;
	}
	CUTE_TEST_ASSERT(server_running(server));
	CUTE_TEST_ASSERT(iters < 100);
	CUTE_TEST_ASSERT(client_get_state(client) == CLIENT_STATE_CONNECTED);

	client_disconnect(client);
	client_destroy(client);

	server_stop(server);
	server_destroy(server);

	return 0;
}

CUTE_TEST_CASE(test_protocol_server_challenge_response_timeout, "Client times out when sending challenge response.");
int test_protocol_server_challenge_response_timeout()
{
	using namespace protocol;
	
	crypto_key_t client_to_server_key = crypto_generate_key();
	crypto_key_t server_to_client_key = crypto_generate_key();
	crypto_key_t secret_key = crypto_generate_key();

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
	CUTE_TEST_CHECK(generate_connect_token(
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
		&secret_key,
		connect_token
	));

	server_t* server = server_make(application_id, &secret_key, NULL);
	CUTE_TEST_CHECK_POINTER(server);

	client_t* client = client_make(5001, NULL, application_id, NULL);
	CUTE_TEST_CHECK_POINTER(client);

	CUTE_TEST_CHECK(server_start(server, "[::1]:5000", 5));
	CUTE_TEST_CHECK(client_connect(client, connect_token));

	int iters = 0;
	while (iters++ < 100)
	{
		client_update(client, 1, 0);

		if (client_get_state(client) != CLIENT_STATE_SENDING_CHALLENGE_RESPONSE) {
			server_update(server, 0, 0);
		}

		if (client_get_state(client) <= 0) break;
		if (client_get_state(client) == CLIENT_STATE_CONNECTED) break;
	}
	CUTE_TEST_ASSERT(server_running(server));
	CUTE_TEST_ASSERT(iters < 100);
	CUTE_TEST_ASSERT(client_get_state(client) == CLIENT_STATE_CHALLENGED_RESPONSE_TIMED_OUT);

	client_disconnect(client);
	client_destroy(client);

	server_stop(server);
	server_destroy(server);

	return 0;
}

CUTE_TEST_CASE(test_protocol_client_expired_token, "Client gets an expired token before connecting.");
int test_protocol_client_expired_token()
{
	using namespace protocol;
	
	crypto_key_t client_to_server_key = crypto_generate_key();
	crypto_key_t server_to_client_key = crypto_generate_key();
	crypto_key_t secret_key = crypto_generate_key();

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
	CUTE_TEST_CHECK(generate_connect_token(
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
		&secret_key,
		connect_token
	));

	client_t* client = client_make(5001, NULL, application_id, NULL);
	CUTE_TEST_CHECK_POINTER(client);
	CUTE_TEST_CHECK(client_connect(client, connect_token));
	client_update(client, 0, 1);
	CUTE_TEST_ASSERT(client_get_state(client) == CLIENT_STATE_CONNECT_TOKEN_EXPIRED);

	client_disconnect(client);
	client_destroy(client);

	return 0;
}

CUTE_TEST_CASE(test_protocol_client_connect_expired_token, "Client detects its own token expires in the middle of a handshake.");
int test_protocol_client_connect_expired_token()
{
	using namespace protocol;
	
	crypto_key_t client_to_server_key = crypto_generate_key();
	crypto_key_t server_to_client_key = crypto_generate_key();
	crypto_key_t secret_key = crypto_generate_key();

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
	CUTE_TEST_CHECK(generate_connect_token(
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
		&secret_key,
		connect_token
	));

	server_t* server = server_make(application_id, &secret_key, NULL);
	CUTE_TEST_CHECK_POINTER(server);

	client_t* client = client_make(5001, NULL, application_id, NULL);
	CUTE_TEST_CHECK_POINTER(client);

	CUTE_TEST_CHECK(server_start(server, "[::1]:5000", 5));
	CUTE_TEST_CHECK(client_connect(client, connect_token));

	int iters = 0;
	uint64_t time = 0;
	while (iters++ < 100)
	{
		client_update(client, 0, time++);
		server_update(server, 0, 0);

		if (client_get_state(client) <= 0) break;
		if (client_get_state(client) == CLIENT_STATE_CONNECTED) break;
	}
	CUTE_TEST_ASSERT(server_running(server));
	CUTE_TEST_ASSERT(iters < 100);
	CUTE_TEST_ASSERT(client_get_state(client) == CLIENT_STATE_CONNECT_TOKEN_EXPIRED);

	client_disconnect(client);
	client_destroy(client);

	server_stop(server);
	server_destroy(server);

	return 0;
}

CUTE_TEST_CASE(test_protocol_server_connect_expired_token, "Server detects token expires in the middle of a handshake.");
int test_protocol_server_connect_expired_token()
{
	using namespace protocol;
	
	crypto_key_t client_to_server_key = crypto_generate_key();
	crypto_key_t server_to_client_key = crypto_generate_key();
	crypto_key_t secret_key = crypto_generate_key();

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
	CUTE_TEST_CHECK(generate_connect_token(
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
		&secret_key,
		connect_token
	));

	server_t* server = server_make(application_id, &secret_key, NULL);
	CUTE_TEST_CHECK_POINTER(server);

	client_t* client = client_make(5001, NULL, application_id, NULL);
	CUTE_TEST_CHECK_POINTER(client);

	CUTE_TEST_CHECK(server_start(server, "[::1]:5000", 5));
	CUTE_TEST_CHECK(client_connect(client, connect_token));

	int iters = 0;
	uint64_t time = 0;
	while (iters++ < 100)
	{
		++time;
		client_update(client, 0, time - 1);
		server_update(server, 0, time);

		if (client_get_state(client) <= 0) break;
		if (client_get_state(client) == CLIENT_STATE_CONNECTED) break;
	}
	CUTE_TEST_ASSERT(server_running(server));
	CUTE_TEST_ASSERT(iters < 100);
	CUTE_TEST_ASSERT(client_get_state(client) == CLIENT_STATE_CONNECT_TOKEN_EXPIRED);

	client_disconnect(client);
	client_destroy(client);

	server_stop(server);
	server_destroy(server);

	return 0;
}

CUTE_TEST_CASE(test_protocol_client_bad_keys, "Client attempts to connect without keys from REST SECTION of connect token.");
int test_protocol_client_bad_keys()
{
	using namespace protocol;
	
	crypto_key_t client_to_server_key = crypto_generate_key();
	crypto_key_t server_to_client_key = crypto_generate_key();
	crypto_key_t secret_key = crypto_generate_key();

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
	CUTE_TEST_CHECK(generate_connect_token(
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
		&secret_key,
		connect_token
	));

	server_t* server = server_make(application_id, &secret_key, NULL);
	CUTE_TEST_CHECK_POINTER(server);

	client_t* client = client_make(5001, NULL, application_id, NULL);
	CUTE_TEST_CHECK_POINTER(client);

	CUTE_TEST_CHECK(server_start(server, "[::1]:5000", 5));
	CUTE_TEST_CHECK(client_connect(client, connect_token));

	// Invalidate client keys.
	client->connect_token.client_to_server_key = crypto_generate_key();
	client->connect_token.server_to_client_key = crypto_generate_key();

	int iters = 0;
	while (iters++ < 100)
	{
		client_update(client, 1, 0);
		server_update(server, 1, 0);

		if (client_get_state(client) <= 0) break;
		if (client_get_state(client) == CLIENT_STATE_CONNECTED) break;
	}
	CUTE_TEST_ASSERT(server_running(server));
	CUTE_TEST_ASSERT(iters < 100);
	CUTE_TEST_ASSERT(client_get_state(client) == CLIENT_STATE_CONNECTION_REQUEST_TIMED_OUT);

	client_disconnect(client);
	client_destroy(client);

	server_stop(server);
	server_destroy(server);

	return 0;
}

CUTE_TEST_CASE(test_protocol_server_not_in_list_but_gets_request, "Client tries to connect to server, but token does not contain server endpoint.");
int test_protocol_server_not_in_list_but_gets_request()
{
	using namespace protocol;
	
	crypto_key_t client_to_server_key = crypto_generate_key();
	crypto_key_t server_to_client_key = crypto_generate_key();
	crypto_key_t secret_key = crypto_generate_key();

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
	CUTE_TEST_CHECK(generate_connect_token(
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
		&secret_key,
		connect_token
	));

	server_t* server = server_make(application_id, &secret_key, NULL);
	CUTE_TEST_CHECK_POINTER(server);

	client_t* client = client_make(5001, NULL, application_id, NULL);
	CUTE_TEST_CHECK_POINTER(client);

	CUTE_TEST_CHECK(server_start(server, "[::1]:5000", 5));
	CUTE_TEST_CHECK(client_connect(client, connect_token));

	// This will make packets arrive to correct server address, but connect token has the wrong address.
	CUTE_TEST_CHECK(endpoint_init(client->connect_token.endpoints, "[::1]:5000"));

	int iters = 0;
	while (iters++ < 100)
	{
		client_update(client, 1, 0);
		server_update(server, 1, 0);

		if (client_get_state(client) <= 0) break;
		if (client_get_state(client) == CLIENT_STATE_CONNECTED) break;
	}
	CUTE_TEST_ASSERT(server_running(server));
	CUTE_TEST_ASSERT(iters < 100);
	CUTE_TEST_ASSERT(client_get_state(client) == CLIENT_STATE_CONNECTION_REQUEST_TIMED_OUT);

	client_disconnect(client);
	client_destroy(client);

	server_stop(server);
	server_destroy(server);

	return 0;
}

CUTE_TEST_CASE(test_protocol_connect_a_few_clients, "Multiple clients connecting to one server.");
int test_protocol_connect_a_few_clients()
{
	using namespace protocol;
	
	crypto_key_t client_to_server_key = crypto_generate_key();
	crypto_key_t server_to_client_key = crypto_generate_key();
	crypto_key_t secret_key = crypto_generate_key();

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

	CUTE_TEST_CHECK(generate_connect_token(
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
		&secret_key,
		connect_token
	));
	client_t* client0 = client_make(5001, NULL, application_id, NULL);
	CUTE_TEST_CHECK_POINTER(client0);
	CUTE_TEST_CHECK(client_connect(client0, connect_token));

	client_to_server_key = crypto_generate_key();
	server_to_client_key = crypto_generate_key();
	client_id = 2;
	CUTE_TEST_CHECK(generate_connect_token(
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
		&secret_key,
		connect_token
	));
	client_t* client1 = client_make(5002, NULL, application_id, NULL);
	CUTE_TEST_CHECK_POINTER(client1);
	CUTE_TEST_CHECK(client_connect(client1, connect_token));

	client_to_server_key = crypto_generate_key();
	server_to_client_key = crypto_generate_key();
	client_id = 3;
	CUTE_TEST_CHECK(generate_connect_token(
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
		&secret_key,
		connect_token
	));
	client_t* client2 = client_make(5003, NULL, application_id, NULL);
	CUTE_TEST_CHECK_POINTER(client2);
	CUTE_TEST_CHECK(client_connect(client2, connect_token));

	server_t* server = server_make(application_id, &secret_key, NULL);
	CUTE_TEST_CHECK_POINTER(server);
	CUTE_TEST_CHECK(server_start(server, "[::1]:5000", 5));

	int iters = 0;
	float dt = 1.0f / 60.0f;
	while (iters++ < 100)
	{
		client_update(client0, dt, 0);
		client_update(client1, dt, 0);
		client_update(client2, dt, 0);
		server_update(server, dt, 0);

		if (client_get_state(client0) <= 0) break;
		if (client_get_state(client1) <= 0) break;
		if (client_get_state(client2) <= 0) break;
		if (client_get_state(client0) == CLIENT_STATE_CONNECTED &&
		    client_get_state(client1) == CLIENT_STATE_CONNECTED &&
		    client_get_state(client2) == CLIENT_STATE_CONNECTED) break;
	}
	CUTE_TEST_ASSERT(server_running(server));
	CUTE_TEST_ASSERT(iters < 100);
	CUTE_TEST_ASSERT(client_get_state(client0) == CLIENT_STATE_CONNECTED);
	CUTE_TEST_ASSERT(client_get_state(client1) == CLIENT_STATE_CONNECTED);
	CUTE_TEST_ASSERT(client_get_state(client2) == CLIENT_STATE_CONNECTED);

	client_disconnect(client0);
	client_disconnect(client1);
	client_disconnect(client2);
	client_destroy(client0);
	client_destroy(client1);
	client_destroy(client2);

	server_stop(server);
	server_destroy(server);

	return 0;
}

CUTE_TEST_CASE(test_protocol_keepalive, "Client and server setup connection and maintain it through keepalive packets.");
int test_protocol_keepalive()
{
	using namespace protocol;
	
	crypto_key_t client_to_server_key = crypto_generate_key();
	crypto_key_t server_to_client_key = crypto_generate_key();
	crypto_key_t secret_key = crypto_generate_key();

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

	CUTE_TEST_CHECK(generate_connect_token(
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
		&secret_key,
		connect_token
	));
	client_t* client = client_make(5001, NULL, application_id, NULL);
	CUTE_TEST_CHECK_POINTER(client);
	CUTE_TEST_CHECK(client_connect(client, connect_token));

	server_t* server = server_make(application_id, &secret_key, NULL);
	CUTE_TEST_CHECK_POINTER(server);
	CUTE_TEST_CHECK(server_start(server, "[::1]:5000", 5));

	int iters = 0;
	float dt = 1.0f / 60.0f;
	while (iters++ < 1000)
	{
		client_update(client, dt, 0);
		server_update(server, dt, 0);

		if (client_get_state(client) <= 0) break;
	}
	CUTE_TEST_ASSERT(server_running(server));
	CUTE_TEST_ASSERT(iters == 1001);
	CUTE_TEST_ASSERT(client_get_state(client) == CLIENT_STATE_CONNECTED);

	client_disconnect(client);
	client_destroy(client);

	server_stop(server);
	server_destroy(server);

	return 0;
}

// WORKING HERE
// Gotta test disconnects
