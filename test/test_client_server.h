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

#include <cute_client.h>
#include <cute_server.h>

using namespace cute;

CUTE_TEST_CASE(test_client_server_handshake, "Test out barebones connection between client and server instances.");
int test_client_server_handshake()
{
	client_t* client = client_alloc(NULL);
	CUTE_TEST_CHECK_POINTER(client);

	server_t* server = server_alloc(NULL);
	CUTE_TEST_CHECK_POINTER(server);

	crypto_key_t pk, sk;
	CUTE_TEST_CHECK(crypto_generate_keypair(&pk, &sk));
	CUTE_TEST_CHECK(server_start(server, "127.0.0.1:5000", &pk, &sk, NULL));
	CUTE_TEST_CHECK(client_connect(client, 501, "127.0.0.1:5000", &pk));
	CUTE_TEST_ASSERT(client_state_get(client) == CLIENT_STATE_CONNECTING);

	float dt = 1.0f / 60.0f;

	// Send hello.
	client_update(client, dt);
	CUTE_TEST_ASSERT(client_state_get(client) == CLIENT_STATE_CONNECTING);

	// Accept connection, send connection response.
	server_update(server, dt);

	// Recieve connection accepted. Client should report as connected.
	client_update(client, dt);
	CUTE_TEST_ASSERT(client_state_get(client) == CLIENT_STATE_CONNECTED);

	client_disconnect(client);
	server_stop(server);

	client_destroy(client);
	server_destroy(server);

	return 0;
}

CUTE_TEST_CASE(test_keep_alive_packets, "Make sure keepalive packets fly between idle client and server, and properly update timestamps.");
int test_keep_alive_packets()
{
	client_t* client = client_alloc(NULL);
	CUTE_TEST_CHECK_POINTER(client);
	server_t* server = server_alloc(NULL);
	CUTE_TEST_CHECK_POINTER(server);

	crypto_key_t pk, sk;
	CUTE_TEST_CHECK(crypto_generate_keypair(&pk, &sk));
	CUTE_TEST_CHECK(server_start(server, "127.0.0.1:5000", &pk, &sk, NULL));
	CUTE_TEST_CHECK(client_connect(client, 501, "127.0.0.1:5000", &pk));

	// Assert connectivity.
	// Advance client/server time significantly to trigger the need for keepalive packets upon the subsequent updates.
	client_update(client, 0);
	CUTE_TEST_ASSERT(client_state_get(client) == CLIENT_STATE_CONNECTING);
	server_update(server, CUTE_KEEPALIVE_RATE);
	client_update(client, CUTE_KEEPALIVE_RATE);
	CUTE_TEST_ASSERT(client_state_get(client) == CLIENT_STATE_CONNECTED);

	// Assert new client connected event arrival.
	server_event_t event;
	CUTE_TEST_CHECK(server_poll_event(server, &event));
	CUTE_TEST_ASSERT(event.type == SERVER_EVENT_TYPE_NEW_CONNECTION);
	cute::handle_t client_id = event.u.new_connection.client_id;

	// Event queue should be empty now.
	CUTE_TEST_ASSERT(server_poll_event(server, &event) < 0);

	// Assert timestamps as needing keepalive packets from the opposing endpoints.
	CUTE_TEST_ASSERT(client_get_last_packet_recieved_time(client) == CUTE_KEEPALIVE_RATE);
	CUTE_TEST_ASSERT(server_get_last_packet_recieved_time_from_client(server, client_id) == CUTE_KEEPALIVE_RATE);

	// Update each endpoint to trigger keepalive packets.
	server_update(server, 0); // Send keepalive.
	client_update(client, 0); // Get a keepalive, send a keepalive. Timestamp cleared.
	server_update(server, 0); // Get a keepalive. Timestamp cleared.

	// Assert timestamps were cleared.
	CUTE_TEST_ASSERT(client_get_last_packet_recieved_time(client) == 0);
	CUTE_TEST_ASSERT(server_get_last_packet_recieved_time_from_client(server, client_id) == 0);

	client_disconnect(client);
	server_stop(server);

	client_destroy(client);
	server_destroy(server);

	return 0;
}

CUTE_TEST_CASE(test_no_server_response_on_client_connect, "Client should attempt to connect to non-existant server and retry 3 times before disconnecting.");
int test_no_server_response_on_client_connect()
{
	client_t* client = client_alloc(NULL);
	CUTE_TEST_CHECK_POINTER(client);

	crypto_key_t pk, sk;
	CUTE_TEST_CHECK(crypto_generate_keypair(&pk, &sk));
	CUTE_TEST_CHECK(client_connect(client, 501, "127.0.0.1:5000", &pk));

	client_update(client, CUTE_KEEPALIVE_RATE);
	CUTE_TEST_ASSERT(client_state_get(client) == CLIENT_STATE_CONNECTING);

	client_update(client, CUTE_KEEPALIVE_RATE);
	CUTE_TEST_ASSERT(client_state_get(client) == CLIENT_STATE_CONNECTING);

	client_update(client, CUTE_KEEPALIVE_RATE);
	CUTE_TEST_ASSERT(client_state_get(client) == CLIENT_STATE_CONNECTING);

	client_update(client, CUTE_KEEPALIVE_RATE);
	CUTE_TEST_ASSERT(client_state_get(client) == CLIENT_STATE_DISCONNECTED);

	client_disconnect(client);
	client_destroy(client);

	return 0;
}

CUTE_TEST_CASE(test_client_timeout, "Client connects to server, then times out. Server should detect and cull the connection.");
int test_client_timeout()
{
	return 0;
}

// WORKING HERE
// TODO
// [x] Keep alive packet
// [ ] client timeout
// [x] no server response on connect
// [ ] server timeout
// [ ] connection denied
// [ ] server disconnect client after connecting
// [ ] client disconnect after connecting.
