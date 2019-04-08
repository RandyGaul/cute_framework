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

CUTE_TEST_CASE(test_encryption_map_basic, "Create map, make entry, lookup entry, remove, and cleanup.");
int test_encryption_map_basic()
{
	using namespace protocol;
	encryption_map_t map;

	CUTE_TEST_CHECK(encryption_map_init(&map, NULL));

	encryption_state_t state;
	state.sequence = 0;
	state.expiration_timestamp = 10;
	state.handshake_timeout = 5;
	state.last_handshake_access_time = 0;
	state.client_to_server_key = crypto_generate_key();
	state.server_to_client_key = crypto_generate_key();

	endpoint_t endpoint;
	CUTE_TEST_CHECK(endpoint_init(&endpoint, "[::]:5000"));

	encryption_map_insert(&map, endpoint, &state);

	encryption_state_t state_looked_up;
	CUTE_TEST_CHECK(encryption_map_find(&map, endpoint, &state_looked_up));

	CUTE_TEST_ASSERT(!CUTE_MEMCMP(&state, &state_looked_up, sizeof(state)));

	encryption_map_cleanup(&map);

	return 0;
}
