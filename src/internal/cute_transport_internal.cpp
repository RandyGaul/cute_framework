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
#include <cute_c_runtime.h>
#include <cute_buffer.h>

#include <internal/cute_defines_internal.h>
#include <internal/cute_transport_internal.h>
#include <internal/cute_serialize_internal.h>

#include <math.h>
#include <float.h>

#define CUTE_ACK_SYSTEM_HEADER_SIZE (2 + 2 + 4)

namespace cute
{

// Sequence buffer implementation strategy comes from Glenn's online articles:
// https://gafferongames.com/post/reliable_ordered_messages/

int sequence_buffer_init(sequence_buffer_t* buffer, int capacity, int stride, void* mem_ctx)
{
	CUTE_MEMSET(buffer, 0, sizeof(sequence_buffer_t));
	buffer->capacity = capacity;
	buffer->stride = stride;
	buffer->entry_sequence = (uint32_t*)CUTE_ALLOC(sizeof(uint32_t) * capacity, mem_ctx);
	CUTE_CHECK_POINTER(buffer->entry_sequence);
	buffer->entry_data = (uint8_t*)CUTE_ALLOC(stride * capacity, mem_ctx);
	CUTE_CHECK_POINTER(buffer->entry_data);
	buffer->mem_ctx = mem_ctx;
	sequence_buffer_reset(buffer);
	return 0;

cute_error:
	CUTE_FREE(buffer->entry_sequence, mem_ctx);
	CUTE_FREE(buffer->entry_data, mem_ctx);
	return -1;
}

void sequence_buffer_cleanup(sequence_buffer_t* buffer, sequence_buffer_cleanup_entry_fn* cleanup_fn)
{
	for (int i = 0; i < buffer->capacity; ++i)
	{
		sequence_buffer_remove(buffer, i, cleanup_fn);
	}

	CUTE_FREE(buffer->entry_sequence, buffer->mem_ctx);
	CUTE_FREE(buffer->entry_data, buffer->mem_ctx);
	CUTE_MEMSET(buffer, 0, sizeof(sequence_buffer_t));
}

void sequence_buffer_reset(sequence_buffer_t* buffer, sequence_buffer_cleanup_entry_fn* cleanup_fn)
{
	for (int i = 0; i < buffer->capacity; ++i)
	{
		sequence_buffer_remove(buffer, i, cleanup_fn);
	}

	buffer->sequence = 0;
	CUTE_MEMSET(buffer->entry_sequence, ~0, sizeof(uint32_t) * buffer->capacity);
}

static void s_sequence_buffer_remove_entries(sequence_buffer_t* buffer, int sequence_a, int sequence_b, sequence_buffer_cleanup_entry_fn* cleanup_fn = NULL)
{
	if (sequence_b < sequence_a) sequence_b += 65536;
	if (sequence_b - sequence_a < buffer->capacity) {
		for (int sequence = sequence_a; sequence <= sequence_b; ++sequence)
		{
			int index = sequence % buffer->capacity;
			if (cleanup_fn) cleanup_fn(buffer->entry_data + buffer->stride * index, buffer->mem_ctx);
			buffer->entry_sequence[index] = 0xFFFFFFFF;
		}
	} else {
		for (int i = 0; i < buffer->capacity; ++i)
		{
			if (cleanup_fn) cleanup_fn(buffer->entry_data + buffer->stride * i, buffer->mem_ctx);
			buffer->entry_sequence[i] = 0xFFFFFFFF;
		}
	}
}

static CUTE_INLINE int s_sequence_greater_than(uint16_t a, uint16_t b)
{
	return ((a > b) && (a - b <= 32768)) |
	       ((a < b) && (b - a  > 32768));
}

static CUTE_INLINE int s_sequence_less_than(uint16_t a, uint16_t b)
{
	return s_sequence_greater_than(b, a);
}

void sequence_buffer_advance(sequence_buffer_t* buffer, uint16_t sequence)
{
	if (s_sequence_greater_than(sequence + 1, buffer->sequence)) {
		s_sequence_buffer_remove_entries(buffer, buffer->sequence, sequence, NULL);
		buffer->sequence = sequence + 1;
	}
}

static CUTE_INLINE int s_sequence_is_stale(sequence_buffer_t* buffer, uint16_t sequence)
{
	return s_sequence_less_than(sequence, buffer->sequence - ((uint16_t)buffer->capacity));
}

void* sequence_buffer_insert(sequence_buffer_t* buffer, uint16_t sequence, sequence_buffer_cleanup_entry_fn* cleanup_fn)
{
	if (s_sequence_greater_than(sequence + 1, buffer->sequence)) {
		s_sequence_buffer_remove_entries(buffer, buffer->sequence, sequence, cleanup_fn);
		buffer->sequence = sequence + 1;
	} else if (s_sequence_is_stale(buffer, sequence)) {
		return NULL;
	}
	int index = sequence % buffer->capacity;
	if (cleanup_fn && buffer->entry_sequence[index] != 0xFFFFFFFF) {
		cleanup_fn(buffer->entry_data + buffer->stride * (sequence % buffer->capacity), buffer->mem_ctx);
	}
	buffer->entry_sequence[index] = sequence;
	return buffer->entry_data + index * buffer->stride;
}

void sequence_buffer_remove(sequence_buffer_t* buffer, uint16_t sequence, sequence_buffer_cleanup_entry_fn* cleanup_fn)
{
	int index = sequence % buffer->capacity;
	if (buffer->entry_sequence[index] != 0xFFFFFFFF)
	{
		buffer->entry_sequence[index] = 0xFFFFFFFF;
		if (cleanup_fn) cleanup_fn(buffer->entry_data + buffer->stride * index, buffer->mem_ctx);
	}
}

int sequence_buffer_is_empty(sequence_buffer_t* sequence_buffer, uint16_t sequence)
{
	return sequence_buffer->entry_sequence[sequence % sequence_buffer->capacity] == 0xFFFFFFFF;
}

void* sequence_buffer_find(sequence_buffer_t* sequence_buffer, uint16_t sequence)
{
	int index = sequence % sequence_buffer->capacity;
	return ((sequence_buffer->entry_sequence[index] == (uint32_t)sequence)) ? (sequence_buffer->entry_data + index * sequence_buffer->stride) : NULL;
}

void* sequence_buffer_at_index(sequence_buffer_t* sequence_buffer, int index)
{
	CUTE_ASSERT(index >= 0);
	CUTE_ASSERT(index < sequence_buffer->capacity);
	return sequence_buffer->entry_sequence[index] != 0xFFFFFFFF ? (sequence_buffer->entry_data + index * sequence_buffer->stride) : NULL;
}

void sequence_buffer_generate_ack_bits(sequence_buffer_t* sequence_buffer, uint16_t* ack, uint32_t* ack_bits)
{
	*ack = sequence_buffer->sequence - 1;
	*ack_bits = 0;
	uint32_t mask = 1;
	for (int i = 0; i < 32; ++i)
	{
		uint16_t sequence = *ack - ((uint16_t)i);
		if (sequence_buffer_find(sequence_buffer, sequence)) {
			*ack_bits |= mask;
		}
		mask <<= 1;
	}
}

// -------------------------------------------------------------------------------------------------

struct ack_system_t
{
	double time;
	int max_packet_size;
	int initial_ack_capacity;

	int (*send_packet_fn)(uint16_t sequence, void* packet, int size, void* udata);
	int (*open_packet_fn)(uint16_t sequence, void* packet, int size, void* udata);

	void* udata;
	void* mem_ctx;

	uint16_t sequence;
	buffer_t acks;
	sequence_buffer_t sent_packets;
	sequence_buffer_t received_packets;

	float rtt;
	float packet_loss;
	float outgoing_bandwidth_kbps;
	float incoming_bandwidth_kbps;

	uint64_t counters[ACK_SYSTEM_COUNTERS_MAX];
};

struct sent_packet_t
{
	double timestamp;
	int acked;
	int size;
};

struct received_packet_t
{
	double timestamp;
	int size;
};

ack_system_t* ack_system_make(const ack_system_config_t* config)
{
	int sent_packets_init = 0;
	int received_packets_init = 0;
	void* mem_ctx = config->user_allocator_context;

	if (!config->send_packet_fn || !config->open_packet_fn) return NULL;
	if (config->max_packet_size > CUTE_TRANSPORT_PACKET_PAYLOAD_MAX) return NULL;

	ack_system_t* transport = (ack_system_t*)CUTE_ALLOC(sizeof(ack_system_t), mem_ctx);
	if (!transport) return NULL;

	transport->time = 0;
	transport->max_packet_size = config->max_packet_size;
	transport->initial_ack_capacity = config->initial_ack_capacity;
	transport->send_packet_fn = config->send_packet_fn;
	transport->open_packet_fn = config->open_packet_fn;
	transport->udata = config->udata;
	transport->mem_ctx = config->user_allocator_context;

	transport->sequence = 0;
	transport->acks = buffer_make(sizeof(uint16_t));
	CUTE_CHECK(sequence_buffer_init(&transport->sent_packets, config->sent_packets_sequence_buffer_size, sizeof(sent_packet_t), mem_ctx));
	sent_packets_init = 1;
	CUTE_CHECK(sequence_buffer_init(&transport->received_packets, config->received_packets_sequence_buffer_size, sizeof(received_packet_t), mem_ctx));
	received_packets_init = 1;

	transport->rtt = 0;
	transport->packet_loss = 0;
	transport->outgoing_bandwidth_kbps = 0;
	transport->incoming_bandwidth_kbps = 0;

	for (int i = 0; i < ACK_SYSTEM_COUNTERS_MAX; ++i) {
		transport->counters[i] = 0;
	}

	return transport;

cute_error:
	if (transport)
	{
		if (sent_packets_init) sequence_buffer_cleanup(&transport->sent_packets);
		if (received_packets_init) sequence_buffer_cleanup(&transport->received_packets);
	}
	CUTE_FREE(transport, config->user_allocator_context);
	return NULL;
}

void ack_system_destroy(ack_system_t* transport)
{
	buffer_free(&transport->acks);
	sequence_buffer_cleanup(&transport->sent_packets);
	sequence_buffer_cleanup(&transport->received_packets);
	CUTE_FREE(transport, transport->mem_ctx);
}

void ack_system_reset(ack_system_t* transport)
{
	transport->sequence = 0;

	buffer_clear(&transport->acks);
	sequence_buffer_reset(&transport->sent_packets);
	sequence_buffer_reset(&transport->received_packets);

	transport->rtt = 0;
	transport->packet_loss = 0;
	transport->outgoing_bandwidth_kbps = 0;
	transport->incoming_bandwidth_kbps = 0;

	for (int i = 0; i < ACK_SYSTEM_COUNTERS_MAX; ++i) {
		transport->counters[i] = 0;
	}
}

static int s_write_ack_system_header(uint8_t* buffer, uint16_t sequence, uint16_t ack, uint32_t ack_bits)
{
	uint8_t* buffer_start = buffer;
	write_uint16(&buffer, sequence);
	write_uint16(&buffer, ack);
	write_uint32(&buffer, ack_bits);
	return (int)(buffer - buffer_start);
}

int ack_system_send_packet(ack_system_t* transport, void* data, int size, uint16_t* sequence_out)
{
	if (size > transport->max_packet_size) {
		transport->counters[ACK_SYSTEM_COUNTERS_PACKETS_TOO_LARGE_TO_SEND]++;
		return -1;
	}

	uint16_t sequence = transport->sequence++;
	uint16_t ack;
	uint32_t ack_bits;

	sequence_buffer_generate_ack_bits(&transport->received_packets, &ack, &ack_bits);
	sent_packet_t* packet = (sent_packet_t*)sequence_buffer_insert(&transport->sent_packets, sequence);

	packet->timestamp = transport->time;
	packet->acked = 0;
	packet->size = size + CUTE_ACK_SYSTEM_HEADER_SIZE;

	uint8_t buffer[CUTE_TRANSPORT_PACKET_PAYLOAD_MAX];
	int header_size = s_write_ack_system_header(buffer, sequence, ack, ack_bits);
	CUTE_ASSERT(header_size == CUTE_ACK_SYSTEM_HEADER_SIZE);
	CUTE_ASSERT(size + header_size < CUTE_TRANSPORT_PACKET_PAYLOAD_MAX);
	CUTE_MEMCPY(buffer + header_size, data, size);
	if (transport->send_packet_fn(sequence, buffer, size + header_size, transport->udata) < 0) {
		transport->counters[ACK_SYSTEM_COUNTERS_PACKETS_INVALID]++;
		return -1;
	}

	transport->counters[ACK_SYSTEM_COUNTERS_PACKETS_SENT]++;

	if (sequence_out) *sequence_out = sequence;
	return 0;
}

static int s_read_ack_system_header(uint8_t* buffer, int size, uint16_t* sequence, uint16_t* ack, uint32_t* ack_bits)
{
	if (size < CUTE_ACK_SYSTEM_HEADER_SIZE) return -1;
	uint8_t* buffer_start = buffer;
	*sequence = read_uint16(&buffer);
	*ack = read_uint16(&buffer);
	*ack_bits = read_uint32(&buffer);
	return (int)(buffer - buffer_start);
}

int ack_system_receive_packet(ack_system_t* transport, void* data, int size)
{
	if (size > transport->max_packet_size) {
		transport->counters[ACK_SYSTEM_COUNTERS_PACKETS_TOO_LARGE_TO_RECEIVE]++;
		return -1;
	}

	transport->counters[ACK_SYSTEM_COUNTERS_PACKETS_RECEIVED]++;

	uint16_t sequence;
	uint16_t ack;
	uint32_t ack_bits;
	uint8_t* buffer = (uint8_t*)data;

	int header_size = s_read_ack_system_header(buffer, size, &sequence, &ack, &ack_bits);
	if (header_size < 0) {
		transport->counters[ACK_SYSTEM_COUNTERS_PACKETS_INVALID]++;
		return -1;
	}
	CUTE_ASSERT(header_size == CUTE_ACK_SYSTEM_HEADER_SIZE);

	if (s_sequence_is_stale(&transport->received_packets, sequence)) {
		transport->counters[ACK_SYSTEM_COUNTERS_PACKETS_STALE]++;
		return -1;
	}

	if (transport->open_packet_fn(sequence, buffer + header_size, size - header_size, transport->udata) < 0) {
		return -1;
	}

	received_packet_t* packet = (received_packet_t*)sequence_buffer_insert(&transport->received_packets, sequence);
	packet->timestamp = transport->time;
	packet->size = size;
	
	for (int i = 0; i < 32; ++i)
	{
		int bit_was_set = ack_bits & 1;
		ack_bits >>= 1;

		if (bit_was_set) {
			uint16_t ack_sequence = ack - ((uint16_t)i);
			sent_packet_t* sent_packet = (sent_packet_t*)sequence_buffer_find(&transport->sent_packets, ack_sequence);

			if (sent_packet && !sent_packet->acked) {
				buffer_check_grow(&transport->acks, transport->initial_ack_capacity);
				buffer_push(&transport->acks, &ack_sequence);
				transport->counters[ACK_SYSTEM_COUNTERS_PACKETS_ACKED]++;
				sent_packet->acked = 1;

				float rtt = (float)(transport->time - sent_packet->timestamp);
				transport->rtt += (rtt - transport->rtt) * 0.001f;
				if (transport->rtt < 0) transport->rtt = 0;
			}
		}

	}

	return 0;
}

uint16_t* ack_system_get_acks(ack_system_t* transport)
{
	return (uint16_t*)transport->acks.data;
}

int ack_system_get_acks_count(ack_system_t* transport)
{
	return transport->acks.count;
}

void ack_system_clear_acks(ack_system_t* transport)
{
	buffer_clear(&transport->acks);
}

static CUTE_INLINE float s_calc_packet_loss(float packet_loss, sequence_buffer_t* sent_packets)
{
	int packet_count = 0;
	int packet_drop_count = 0;

	for (int i = 0; i < sent_packets->capacity; ++i)
	{
		sent_packet_t* packet = (sent_packet_t*)sequence_buffer_at_index(sent_packets, i);
		if (packet) {
			packet_count++;
			if (!packet->acked) packet_drop_count++;
		}
	}

	float loss = (float)packet_drop_count / (float)packet_count;
	packet_loss += (loss - packet_loss) * 0.1f;
	if (packet_loss < 0) packet_loss = 0;
	return packet_loss;
}

static CUTE_INLINE float s_calc_bandwidth(float bandwidth, sequence_buffer_t* sent_packets)
{
	int bytes_sent = 0;
	double start_timestamp = DBL_MAX;
	double end_timestamp = 0;

	for (int i = 0; i < sent_packets->capacity; ++i)
	{
		sent_packet_t* packet = (sent_packet_t*)sequence_buffer_at_index(sent_packets, i);
		if (packet) {
			bytes_sent += packet->size;
			if (packet->timestamp < start_timestamp) start_timestamp = packet->timestamp;
			if (packet->timestamp > end_timestamp) end_timestamp = packet->timestamp;
		}
	}

	if (start_timestamp != DBL_MAX) {
		float sent_bandwidth = (float)(((double)bytes_sent / 1024.0) / (end_timestamp - start_timestamp));
		bandwidth += (sent_bandwidth - bandwidth) * 0.1f;
		if (bandwidth < 0) bandwidth = 0;
	}

	return bandwidth;
}

void ack_system_update(ack_system_t* transport, float dt)
{
	transport->time += dt;
	transport->packet_loss = s_calc_packet_loss(transport->packet_loss, &transport->sent_packets);
	transport->incoming_bandwidth_kbps = s_calc_bandwidth(transport->incoming_bandwidth_kbps, &transport->sent_packets);
	transport->outgoing_bandwidth_kbps = s_calc_bandwidth(transport->outgoing_bandwidth_kbps, &transport->received_packets);
}

float ack_system_rtt(ack_system_t* transport)
{
	return transport->rtt;
}

float ack_system_packet_loss(ack_system_t* transport)
{
	return transport->packet_loss;
}

float ack_system_bandwidth_outgoing_kbps(ack_system_t* transport)
{
	return transport->outgoing_bandwidth_kbps;
}

float ack_system_bandwidth_incoming_kbps(ack_system_t* transport)
{
	return transport->incoming_bandwidth_kbps;
}

uint64_t ack_system_get_counter(ack_system_t* transport, ack_system_counter_t counter)
{
	return transport->counters[counter];
}

// -------------------------------------------------------------------------------------------------

struct transport_t
{
	uint64_t sequence;
	sequence_buffer_t reliable_sent_packets;
	sequence_buffer_t reliable_received_packets;
	sequence_buffer_t fragment_reassembly;
};

struct fragment_reassembly_data_t
{
	int packet_header_bytes;
	int packet_size;
	uint8_t* packet;

	int fragment_count_so_far;
	int fragments_total;
	uint8_t* fragment_received;
};

static void s_fragment_reassembly_data_cleanup(void* data, void* mem_ctx)
{
	fragment_reassembly_data_t* reassembly_data = (fragment_reassembly_data_t*)data;
	CUTE_FREE(reassembly_data->packet, mem_ctx);
	CUTE_FREE(reassembly_data->fragment_received, mem_ctx);
}

transport_t* transport_make(const transport_configuration_t* config)
{
}

void transport_destroy(transport_t* transport)
{
}

void transport_reset(transport_t* tranpsport)
{
}

int transport_send_reliably_and_in_order(transport_t* transport, void* data, int size, uint64_t* sequence)
{
}

int transport_send_fire_and_forget(transport_t* transport, void* data, int size)
{
}

int transport_recieve(transport_t* transport, void** data, int* size, uint64_t* sequence)
{
}

void transport_free(transport_t* transport, void* data)
{
}

int transport_process_packet(transport_t* transport, void* data, int size)
{
}

void transport_process_acks(transport_t* transport, uint16_t* acks, int ack_count)
{
}

void transport_resend_unacked_fragments(transport_t* transport)
{
}

// fragment header
// prefix byte : zero for fragment, nonzero otherwise
// 8 bytes sequence
// if fragment:
	// 2 bytes fragment count
	// 2 bytes fragment index
	// 2 bytes fragment size
// total: 15 bytes for fragment, 1 byte s

}
