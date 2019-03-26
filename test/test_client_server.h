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

CUTE_TEST_CASE(test_client_server_handshake, "Test out barebones connection setup between client and server instances.");
int test_client_server_handshake()
{
	client_t* client = client_alloc(NULL);
	CUTE_TEST_CHECK_POINTER(client);

	server_t* server = server_alloc(NULL);
	CUTE_TEST_CHECK_POINTER(server);

	crypto_key_t pk, sk;
	CUTE_TEST_CHECK(crypto_generate_keypair(&pk, &sk));
	CUTE_TEST_CHECK(server_start(server, "127.0.0.1:500", &pk, &sk, NULL));
	CUTE_TEST_CHECK(client_connect(client, 501, "127.0.0.1:500", &pk));

	float dt = 1.0f / 60.0f;
	client_update(client, dt);
	server_update(server, dt);

	client_disconnect(client);
	server_stop(server);

	client_destroy(client);
	server_destroy(server);

	return 0;
}
