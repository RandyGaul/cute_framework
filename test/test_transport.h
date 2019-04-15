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

#include <internal/cute_transport_internal.h>
using namespace cute;

struct test_transport_data_t
{
	int drop_packet = 0;
	int id;
	transport_t* transport_a;
	transport_t* transport_b;
};

int test_send_packet_fn(uint16_t sequence, void* packet, int size, void* udata)
{
	test_transport_data_t* data = (test_transport_data_t*)udata;
	if (data->drop_packet) return 0;
	if (data->id) {
		return transport_receive_packet(data->transport_a, packet, size);
	} else {
		return transport_receive_packet(data->transport_b, packet, size);
	}
}

int test_open_packet_fn(uint16_t sequence, void* packet, int size, void* udata)
{
	uint64_t* val_ptr = (uint64_t*)packet;
	if (*val_ptr != 100) {
		return -1;
	} else {
		return 0;
	}
}

CUTE_TEST_CASE(test_transport_basic, "Create transport, send a few packets, and receive them. Make sure some drop.");
int test_transport_basic()
{
	test_transport_data_t data_a;
	test_transport_data_t data_b;
	data_a.id = 0;
	data_b.id = 1;

	transport_config_t config;
	config.send_packet_fn = test_send_packet_fn;
	config.open_packet_fn = test_open_packet_fn;
	config.udata = &data_a;
	transport_t* transport_a = transport_make(&config);
	config.udata = &data_b;
	transport_t* transport_b = transport_make(&config);
	data_a.transport_a = transport_a;
	data_a.transport_b = transport_b;
	data_b.transport_a = transport_a;
	data_b.transport_b = transport_b;

	uint64_t packet_data = 100;

	for (int i = 0; i < 10; ++i)
	{
		if ((i % 3) == 0) {
			data_a.drop_packet = 1;
			data_b.drop_packet = 1;
		} else {
			data_a.drop_packet = 0;
			data_b.drop_packet = 0;
		}
		uint16_t sequence_a, sequence_b;
		CUTE_TEST_CHECK(transport_send_packet(transport_a, &packet_data, sizeof(packet_data), &sequence_a));
		CUTE_TEST_CHECK(transport_send_packet(transport_b, &packet_data, sizeof(packet_data), &sequence_b));
	}

	uint64_t a_sent = transport_get_counter(transport_a, TRANSPORT_COUNTERS_PACKETS_SENT);
	uint64_t b_sent = transport_get_counter(transport_b, TRANSPORT_COUNTERS_PACKETS_SENT);
	CUTE_TEST_ASSERT(a_sent == b_sent);

	uint64_t a_received = transport_get_counter(transport_a, TRANSPORT_COUNTERS_PACKETS_RECEIVED);
	uint64_t b_received = transport_get_counter(transport_b, TRANSPORT_COUNTERS_PACKETS_RECEIVED);
	CUTE_TEST_ASSERT(a_received == b_received);

	CUTE_TEST_ASSERT(a_sent > a_received);
	CUTE_TEST_ASSERT(b_sent > b_received);

	uint16_t* acks_a = transport_get_acks(transport_a);
	uint16_t* acks_b = transport_get_acks(transport_b);
	int count_a = transport_get_acks_count(transport_a);
	int count_b = transport_get_acks_count(transport_b);
	CUTE_TEST_ASSERT(count_a - 1 == count_b);
	for (int i = 0; i < count_b; ++i)
	{
		CUTE_TEST_ASSERT(acks_a[i] == acks_b[i]);
		CUTE_TEST_ASSERT(acks_a[i] != 0);
		CUTE_TEST_ASSERT(acks_a[i] != 3);
		CUTE_TEST_ASSERT(acks_a[i] != 6);
		CUTE_TEST_ASSERT(acks_a[i] != 9);
	}

	transport_destroy(transport_a);
	transport_destroy(transport_b);

	return 0;
}
