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

struct test_ack_system_data_t
{
	int drop_packet = 0;
	int id = ~0;
	ack_system_t* ack_system_a = NULL;
	ack_system_t* ack_system_b = NULL;
	transport_t* transport_a = NULL;
	transport_t* transport_b = NULL;
};

int test_send_packet_fn(uint16_t sequence, void* packet, int size, void* udata)
{
	test_ack_system_data_t* data = (test_ack_system_data_t*)udata;
	if (data->drop_packet) return 0;
	if (data->id) {
		return ack_system_receive_packet(data->ack_system_a, packet, size);
	} else {
		return ack_system_receive_packet(data->ack_system_b, packet, size);
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

CUTE_TEST_CASE(test_ack_system_basic, "Create ack system, send a few packets, and receive them. Make sure some drop. Assert acks.");
int test_ack_system_basic()
{
	test_ack_system_data_t data_a;
	test_ack_system_data_t data_b;
	data_a.id = 0;
	data_b.id = 1;

	ack_system_config_t config;
	config.send_packet_fn = test_send_packet_fn;
	config.open_packet_fn = test_open_packet_fn;
	config.udata = &data_a;
	ack_system_t* ack_system_a = ack_system_make(&config);
	config.udata = &data_b;
	ack_system_t* ack_system_b = ack_system_make(&config);
	data_a.ack_system_a = ack_system_a;
	data_a.ack_system_b = ack_system_b;
	data_b.ack_system_a = ack_system_a;
	data_b.ack_system_b = ack_system_b;

	CUTE_TEST_CHECK_POINTER(ack_system_a);
	CUTE_TEST_CHECK_POINTER(ack_system_b);

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
		CUTE_TEST_CHECK(ack_system_send_packet(ack_system_a, &packet_data, sizeof(packet_data), &sequence_a));
		CUTE_TEST_CHECK(ack_system_send_packet(ack_system_b, &packet_data, sizeof(packet_data), &sequence_b));
	}

	uint64_t a_sent = ack_system_get_counter(ack_system_a, ACK_SYSTEM_COUNTERS_PACKETS_SENT);
	uint64_t b_sent = ack_system_get_counter(ack_system_b, ACK_SYSTEM_COUNTERS_PACKETS_SENT);
	CUTE_TEST_ASSERT(a_sent == b_sent);

	uint64_t a_received = ack_system_get_counter(ack_system_a, ACK_SYSTEM_COUNTERS_PACKETS_RECEIVED);
	uint64_t b_received = ack_system_get_counter(ack_system_b, ACK_SYSTEM_COUNTERS_PACKETS_RECEIVED);
	CUTE_TEST_ASSERT(a_received == b_received);

	CUTE_TEST_ASSERT(a_sent > a_received);
	CUTE_TEST_ASSERT(b_sent > b_received);

	uint16_t* acks_a = ack_system_get_acks(ack_system_a);
	uint16_t* acks_b = ack_system_get_acks(ack_system_b);
	int count_a = ack_system_get_acks_count(ack_system_a);
	int count_b = ack_system_get_acks_count(ack_system_b);
	CUTE_TEST_ASSERT(count_a - 1 == count_b);
	for (int i = 0; i < count_b; ++i)
	{
		CUTE_TEST_ASSERT(acks_a[i] == acks_b[i]);
		CUTE_TEST_ASSERT(acks_a[i] != 0);
		CUTE_TEST_ASSERT(acks_a[i] != 3);
		CUTE_TEST_ASSERT(acks_a[i] != 6);
		CUTE_TEST_ASSERT(acks_a[i] != 9);
	}

	ack_system_destroy(ack_system_a);
	ack_system_destroy(ack_system_b);

	return 0;
}

int test_transport_open_packet_fn(uint16_t sequence, void* packet, int size, void* udata)
{
	test_ack_system_data_t* data = (test_ack_system_data_t*)udata;
	if (data->id) {
		return transport_process_packet(data->transport_b, packet, size);
	} else {
		return transport_process_packet(data->transport_a, packet, size);
	}
}

CUTE_TEST_CASE(test_transport_basic, "Create transport, send a couple packets, receive them.");
int test_transport_basic()
{
	test_ack_system_data_t data_a;
	test_ack_system_data_t data_b;
	data_a.id = 0;
	data_b.id = 1;

	ack_system_config_t config;
	config.send_packet_fn = test_send_packet_fn;
	config.open_packet_fn = test_transport_open_packet_fn;
	config.udata = &data_a;
	ack_system_t* ack_system_a = ack_system_make(&config);
	config.udata = &data_b;
	ack_system_t* ack_system_b = ack_system_make(&config);
	data_a.ack_system_a = ack_system_a;
	data_a.ack_system_b = ack_system_b;
	data_b.ack_system_a = ack_system_a;
	data_b.ack_system_b = ack_system_b;

	transport_config_t transport_config;
	transport_config.ack_system = ack_system_a;
	transport_t* transport_a = transport_make(&transport_config);
	transport_config.ack_system = ack_system_b;
	transport_t* transport_b = transport_make(&transport_config);
	data_a.transport_a = transport_a;
	data_a.transport_b = transport_b;
	data_b.transport_a = transport_a;
	data_b.transport_b = transport_b;

	int packet_size = 4000;
	uint8_t* packet = (uint8_t*)CUTE_ALLOC(packet_size, NULL);
	CUTE_MEMSET(packet, 0xFF, packet_size);

	CUTE_TEST_CHECK(transport_send_reliably_and_in_order(transport_a, packet, packet_size));
	CUTE_TEST_CHECK(transport_send_reliably_and_in_order(transport_b, packet, packet_size));

	transport_process_acks(transport_a);
	transport_process_acks(transport_b);

	void* packet_received;
	int packet_received_size;

	CUTE_TEST_CHECK(transport_recieve_reliably_and_in_order(transport_a, &packet_received, &packet_received_size));
	CUTE_TEST_ASSERT(packet_size == packet_received_size);
	CUTE_TEST_ASSERT(!CUTE_MEMCMP(packet, packet_received, packet_size));
	transport_free(transport_a, packet_received);

	CUTE_TEST_CHECK(transport_recieve_reliably_and_in_order(transport_b, &packet_received, &packet_received_size));
	CUTE_TEST_ASSERT(packet_size == packet_received_size);
	CUTE_TEST_ASSERT(!CUTE_MEMCMP(packet, packet_received, packet_size));
	transport_free(transport_b, packet_received);

	CUTE_TEST_CHECK(transport_send_fire_and_forget(transport_a, packet, packet_size));
	CUTE_TEST_CHECK(transport_send_fire_and_forget(transport_b, packet, packet_size));

	transport_process_acks(transport_a);
	transport_process_acks(transport_b);

	CUTE_TEST_CHECK(transport_recieve_fire_and_forget(transport_a, &packet_received, &packet_received_size));
	CUTE_TEST_ASSERT(packet_size == packet_received_size);
	CUTE_TEST_ASSERT(!CUTE_MEMCMP(packet, packet_received, packet_size));
	transport_free(transport_a, packet_received);

	CUTE_TEST_CHECK(transport_recieve_fire_and_forget(transport_b, &packet_received, &packet_received_size));
	CUTE_TEST_ASSERT(packet_size == packet_received_size);
	CUTE_TEST_ASSERT(!CUTE_MEMCMP(packet, packet_received, packet_size));
	transport_free(transport_b, packet_received);

	CUTE_FREE(packet, NULL);

	transport_destroy(transport_a);
	transport_destroy(transport_b);

	ack_system_destroy(ack_system_a);
	ack_system_destroy(ack_system_b);

	return 0;
}

CUTE_TEST_CASE(test_transport_drop_fragments, "Create transport, send a couple packets, receive them under packet loss.");
int test_transport_drop_fragments()
{
	test_ack_system_data_t data_a;
	test_ack_system_data_t data_b;
	data_a.id = 0;
	data_b.id = 1;

	ack_system_config_t config;
	config.send_packet_fn = test_send_packet_fn;
	config.open_packet_fn = test_transport_open_packet_fn;
	config.udata = &data_a;
	ack_system_t* ack_system_a = ack_system_make(&config);
	config.udata = &data_b;
	ack_system_t* ack_system_b = ack_system_make(&config);
	data_a.ack_system_a = ack_system_a;
	data_a.ack_system_b = ack_system_b;
	data_b.ack_system_a = ack_system_a;
	data_b.ack_system_b = ack_system_b;

	transport_config_t transport_config;
	transport_config.ack_system = ack_system_a;
	transport_t* transport_a = transport_make(&transport_config);
	transport_config.ack_system = ack_system_b;
	transport_t* transport_b = transport_make(&transport_config);
	data_a.transport_a = transport_a;
	data_a.transport_b = transport_b;
	data_b.transport_a = transport_a;
	data_b.transport_b = transport_b;

	int packet_size = 4000;
	uint8_t* packet = (uint8_t*)CUTE_ALLOC(packet_size, NULL);
	CUTE_MEMSET(packet, 0xFF, packet_size);

	data_b.drop_packet = 1;

	CUTE_TEST_CHECK(transport_send_reliably_and_in_order(transport_a, packet, packet_size));
	CUTE_TEST_CHECK(transport_send_reliably_and_in_order(transport_b, packet, packet_size));

	transport_process_acks(transport_a);
	transport_process_acks(transport_b);

	void* packet_received;
	int packet_received_size;

	CUTE_TEST_ASSERT(transport_recieve_reliably_and_in_order(transport_a, &packet_received, &packet_received_size) < 0);
	CUTE_TEST_ASSERT(0 == packet_received_size);
	CUTE_TEST_ASSERT(packet_received == NULL);

	CUTE_TEST_CHECK(transport_recieve_reliably_and_in_order(transport_b, &packet_received, &packet_received_size));
	CUTE_TEST_ASSERT(packet_size == packet_received_size);
	CUTE_TEST_ASSERT(!CUTE_MEMCMP(packet, packet_received, packet_size));
	transport_free(transport_b, packet_received);

	data_b.drop_packet = 0;

	transport_resend_unacked_fragments(transport_b);

	CUTE_TEST_CHECK(transport_recieve_reliably_and_in_order(transport_a, &packet_received, &packet_received_size));
	CUTE_TEST_ASSERT(packet_size == packet_received_size);
	CUTE_TEST_ASSERT(!CUTE_MEMCMP(packet, packet_received, packet_size));
	transport_free(transport_a, packet_received);

	data_a.drop_packet = 1;

	CUTE_TEST_CHECK(transport_send_fire_and_forget(transport_a, packet, packet_size));
	CUTE_TEST_CHECK(transport_send_fire_and_forget(transport_b, packet, packet_size));

	transport_process_acks(transport_a);
	transport_process_acks(transport_b);

	CUTE_TEST_CHECK(transport_recieve_fire_and_forget(transport_a, &packet_received, &packet_received_size));
	CUTE_TEST_ASSERT(packet_size == packet_received_size);
	CUTE_TEST_ASSERT(!CUTE_MEMCMP(packet, packet_received, packet_size));
	transport_free(transport_a, packet_received);

	CUTE_TEST_ASSERT(transport_recieve_reliably_and_in_order(transport_b, &packet_received, &packet_received_size) < 0);
	CUTE_TEST_ASSERT(0 == packet_received_size);
	CUTE_TEST_ASSERT(packet_received == NULL);

	CUTE_FREE(packet, NULL);

	transport_destroy(transport_a);
	transport_destroy(transport_b);

	ack_system_destroy(ack_system_a);
	ack_system_destroy(ack_system_b);

	return 0;
}
int test_send_packet_many_drops_fn(uint16_t sequence, void* packet, int size, void* udata)
{
	test_ack_system_data_t* data = (test_ack_system_data_t*)udata;
	static int drop = 0;
	if (drop++ % 100 != 0) return 0;
	if (data->id) {
		return ack_system_receive_packet(data->ack_system_a, packet, size);
	} else {
		return ack_system_receive_packet(data->ack_system_b, packet, size);
	}
}

CUTE_TEST_CASE(test_transport_drop_fragments_reliable_hammer, "Create and send many fragments under packet loss.");
int test_transport_drop_fragments_reliable_hammer()
{
	test_ack_system_data_t data_a;
	test_ack_system_data_t data_b;
	data_a.id = 0;
	data_b.id = 1;

	ack_system_config_t config;
	config.send_packet_fn = test_send_packet_many_drops_fn;
	config.open_packet_fn = test_transport_open_packet_fn;
	config.udata = &data_a;
	ack_system_t* ack_system_a = ack_system_make(&config);
	config.udata = &data_b;
	ack_system_t* ack_system_b = ack_system_make(&config);
	data_a.ack_system_a = ack_system_a;
	data_a.ack_system_b = ack_system_b;
	data_b.ack_system_a = ack_system_a;
	data_b.ack_system_b = ack_system_b;

	transport_config_t transport_config;
	transport_config.ack_system = ack_system_a;
	transport_t* transport_a = transport_make(&transport_config);
	transport_config.ack_system = ack_system_b;
	transport_t* transport_b = transport_make(&transport_config);
	data_a.transport_a = transport_a;
	data_a.transport_b = transport_b;
	data_b.transport_a = transport_a;
	data_b.transport_b = transport_b;

	int packet_size = CUTE_MB * 4;
	uint8_t* packet = (uint8_t*)CUTE_ALLOC(packet_size, NULL);
	CUTE_MEMSET(packet, 0xFF, packet_size);

	int fire_and_forget_packet_size = 64;
	uint8_t fire_and_forget_packet[64] = { 0 };

	CUTE_TEST_CHECK(transport_send_reliably_and_in_order(transport_a, packet, packet_size));

	void* packet_received;
	int packet_received_size;

	int iters = 0;
	int received = 0;
	while (iters++ < 10000)
	{
		CUTE_TEST_CHECK(transport_send_fire_and_forget(transport_a, fire_and_forget_packet, fire_and_forget_packet_size));
		CUTE_TEST_CHECK(transport_send_fire_and_forget(transport_b, fire_and_forget_packet, fire_and_forget_packet_size));

		if (iters == 46) __debugbreak();

		transport_process_acks(transport_a);
		transport_process_acks(transport_b);

		transport_resend_unacked_fragments(transport_a);

		if (transport_recieve_reliably_and_in_order(transport_b, &packet_received, &packet_received_size) == 0) {
			CUTE_TEST_ASSERT(packet_size == packet_received_size);
			CUTE_TEST_ASSERT(!CUTE_MEMCMP(packet, packet_received, packet_size));
			received = 1;
			transport_free(transport_b, packet_received);
			break;
		}
	}

	CUTE_TEST_ASSERT(received);

	CUTE_FREE(packet, NULL);

	transport_destroy(transport_a);
	transport_destroy(transport_b);

	ack_system_destroy(ack_system_a);
	ack_system_destroy(ack_system_b);

	return 0;
}
