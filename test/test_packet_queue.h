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

#include <internal/cute_net_internal.h>

CUTE_TEST_CASE(test_packet_queue_basic, "Basic use-case example, push and pull a few packets.");
int test_packet_queue_basic()
{
	packet_queue_t q;
	CUTE_TEST_CHECK(packet_queue_init(&q, 1024, NULL));

	uint64_t data = 0x1234567812345678;
	CUTE_TEST_CHECK(packet_queue_push(&q, &data, sizeof(data), 0));
	CUTE_TEST_CHECK(packet_queue_push(&q, &data, sizeof(data), 1));

	uint64_t sequence;
	uint64_t got_data;
	int size;

	CUTE_TEST_CHECK(packet_queue_peek(&q, &size));
	CUTE_TEST_CHECK(packet_queue_pull(&q, &got_data, size, &sequence));
	CUTE_TEST_ASSERT(sequence == 0);
	CUTE_TEST_ASSERT(got_data == data);

	CUTE_TEST_CHECK(packet_queue_peek(&q, &size));
	CUTE_TEST_CHECK(packet_queue_pull(&q, &got_data, size, &sequence));
	CUTE_TEST_ASSERT(sequence == 1);
	CUTE_TEST_ASSERT(got_data == data);

	pack_queue_clean_up(&q);

	return 0;
}
