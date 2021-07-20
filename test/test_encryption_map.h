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

	encryption_map_init(&map, NULL);

	encryption_state_t state;
	state.sequence = 0;
	state.expiration_timestamp = 10;
	state.handshake_timeout = 5;
	state.last_packet_recieved_time = 0;
	state.last_packet_sent_time = 0;
	state.client_to_server_key = crypto_generate_key();
	state.server_to_client_key = crypto_generate_key();
	state.client_id = 0;

	endpoint_t endpoint;
	CUTE_TEST_CHECK(endpoint_init(&endpoint, "[::]:5000"));

	encryption_map_insert(&map, endpoint, &state);

	encryption_state_t* state_looked_up = encryption_map_find(&map, endpoint);
	CUTE_TEST_CHECK_POINTER(state_looked_up);

	CUTE_TEST_ASSERT(!CUTE_MEMCMP(&state, state_looked_up, sizeof(state)));

	encryption_map_cleanup(&map);

	return 0;
}

CUTE_TEST_CASE(test_encryption_map_timeout_and_expiration, "Ensure timeouts and expirations remove entries properly.");
int test_encryption_map_timeout_and_expiration()
{
	using namespace protocol;
	encryption_map_t map;

	encryption_map_init(&map, NULL);

	encryption_state_t state0;
	state0.sequence = 0;
	state0.expiration_timestamp = 10;
	state0.handshake_timeout = 5;
	state0.last_packet_recieved_time = 0;
	state0.last_packet_sent_time = 0;
	state0.client_to_server_key = crypto_generate_key();
	state0.server_to_client_key = crypto_generate_key();
	state0.client_id = 0;

	encryption_state_t state1;
	state1.sequence = 0;
	state1.expiration_timestamp = 10;
	state1.handshake_timeout = 6;
	state1.last_packet_recieved_time = 0;
	state1.last_packet_sent_time = 0;
	state1.client_to_server_key = crypto_generate_key();
	state1.server_to_client_key = crypto_generate_key();
	state1.client_id = 0;

	endpoint_t endpoint0;
	CUTE_TEST_CHECK(endpoint_init(&endpoint0, "[::]:5000"));

	endpoint_t endpoint1;
	CUTE_TEST_CHECK(endpoint_init(&endpoint1, "[::]:5001"));

	encryption_map_insert(&map, endpoint0, &state0);
	encryption_map_insert(&map, endpoint1, &state1);

	encryption_state_t* state_looked_up = encryption_map_find(&map, endpoint0);
	CUTE_TEST_CHECK_POINTER(state_looked_up);
	CUTE_TEST_ASSERT(!CUTE_MEMCMP(&state0, state_looked_up, sizeof(state0)));

	state_looked_up = encryption_map_find(&map, endpoint1);
	CUTE_TEST_CHECK_POINTER(state_looked_up);
	CUTE_TEST_ASSERT(!CUTE_MEMCMP(&state1, state_looked_up, sizeof(state1)));

	// Nothing should timeout or expire just yet.
	encryption_map_look_for_timeouts_or_expirations(&map, 4.0f, 9ULL);

	state_looked_up = encryption_map_find(&map, endpoint0);
	CUTE_TEST_CHECK_POINTER(state_looked_up);
	CUTE_TEST_ASSERT(!CUTE_MEMCMP(&state0, state_looked_up, sizeof(state0)));

	state_looked_up = encryption_map_find(&map, endpoint1);
	CUTE_TEST_CHECK_POINTER(state_looked_up);
	CUTE_TEST_ASSERT(!CUTE_MEMCMP(&state1, state_looked_up, sizeof(state1)));

	// Now timeout state0.
	encryption_map_look_for_timeouts_or_expirations(&map, 6.0f, 9ULL);
	CUTE_TEST_CHECK_POINTER(!encryption_map_find(&map, endpoint0));

	// Now expire state1.
	encryption_map_look_for_timeouts_or_expirations(&map, 0, 10ULL);
	CUTE_TEST_CHECK_POINTER(!encryption_map_find(&map, endpoint1));

	// Assert that there are no present entries.
	CUTE_TEST_ASSERT(encryption_map_count(&map) == 0);

	encryption_map_cleanup(&map);

	return 0;
}
