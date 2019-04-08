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
#include <internal/cute_net_internal.h>
#include <internal/cute_protocol_internal.h>
using namespace cute;

CUTE_TEST_CASE(test_replay_buffer_valid_packets, "Typical use-case example, should pass all sequences.");
int test_replay_buffer_valid_packets()
{
	protocol::replay_buffer_t buffer;
	protocol::replay_buffer_init(&buffer);

	CUTE_TEST_ASSERT(buffer.max == 0);

	for (int i = 0; i < CUTE_REPLAY_BUFFER_SIZE; ++i)
	{
		uint64_t sequence = buffer.entries[i];
		CUTE_TEST_ASSERT(sequence == ~0ULL);
	}

	for (int i = 0; i < CUTE_REPLAY_BUFFER_SIZE; ++i)
	{
		CUTE_TEST_CHECK(protocol::replay_buffer_cull_duplicate(&buffer, (uint64_t)i));
		protocol::replay_buffer_update(&buffer, (uint64_t)i);
	}

	return 0;
}

CUTE_TEST_CASE(test_replay_buffer_old_packet_out_of_range, "Replay buffer should cull packets of sequence older than `CUTE_REPLAY_BUFFER_SIZE`.");
int test_replay_buffer_old_packet_out_of_range()
{
	protocol::replay_buffer_t buffer;
	protocol::replay_buffer_init(&buffer);

	for (int i = 0; i < CUTE_REPLAY_BUFFER_SIZE * 2; ++i)
	{
		CUTE_TEST_CHECK(protocol::replay_buffer_cull_duplicate(&buffer, (uint64_t)i));
		protocol::replay_buffer_update(&buffer, (uint64_t)i);
	}

	CUTE_TEST_CHECK(!replay_buffer_cull_duplicate(&buffer, 0));

	return 0;
}

CUTE_TEST_CASE(test_replay_buffer_duplicate, "Pass in some valid nonces, and then assert the duplicate fails.");
int test_replay_buffer_duplicate()
{
	protocol::replay_buffer_t buffer;
	protocol::replay_buffer_init(&buffer);

	for (int i = 0; i < CUTE_REPLAY_BUFFER_SIZE; ++i)
	{
		CUTE_TEST_CHECK(protocol::replay_buffer_cull_duplicate(&buffer, (uint64_t)i));
		protocol::replay_buffer_update(&buffer, (uint64_t)i);
	}

	CUTE_TEST_CHECK(!protocol::replay_buffer_cull_duplicate(&buffer, 100));

	return 0;
}
