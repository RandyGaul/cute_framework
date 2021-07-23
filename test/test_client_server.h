/*
	Cute Framework
	Copyright (C) 2021 Randy Gaul https://randygaul.net

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
#include <cute_client.h>
#include <cute_server.h>

using namespace cute;

CUTE_TEST_CASE(test_client_server, "Connect a client to server, then disconnect and shutdown both.");
int test_client_server()
{
	crypto_key_t client_to_server_key = crypto_generate_key();
	crypto_key_t server_to_client_key = crypto_generate_key();
	uint64_t application_id = 333;
	uint64_t current_timestamp = 0;
	uint64_t expiration_timestamp = 1;
	uint32_t handshake_timeout = 5;
	uint64_t client_id = 17;
	const char* endpoints[] = {
		"[::1]:5000",
	};
	crypto_sign_public_t pk;
	crypto_sign_secret_t sk;
	crypto_sign_keygen(&pk, &sk);

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

	server_config_t config;
	config.public_key = pk;
	config.secret_key = sk;
	config.application_id = application_id;
	server_t* server = server_create(&config);
	client_t* client = client_make(5000, application_id);
	CUTE_TEST_ASSERT(server);
	CUTE_TEST_ASSERT(client);

	CUTE_TEST_CHECK(server_start(server, "[::1]:5000").is_error());
	CUTE_TEST_CHECK(client_connect(client, connect_token).is_error());

	int iters = 0;
	while (1) {
		client_update(client, 0);
		server_update(server, 0);

		if (client_state_get(client) < 0 || ++iters == 100) {
			CUTE_TEST_ASSERT(false);
			break;
		}

		if (client_state_get(client) == CLIENT_STATE_CONNECTED) {
			break;
		}
	}

	client_disconnect(client);
	server_update(server, 0);
	CUTE_TEST_ASSERT(!server_is_client_connected(server, 0));

	client_destroy(client);
	server_stop(server);
	server_destroy(server);

	return 0;

	//uint64_t data = 1234567;
	//CUTE_TEST_CHECK(client_send(client, &data, sizeof(data), true).is_error());
}
