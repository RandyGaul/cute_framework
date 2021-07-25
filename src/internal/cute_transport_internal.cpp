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
#include <cute_handle_table.h>
#include <cute_error.h>
#include <cute_array.h>

#include <internal/cute_transport_internal.h>
#include <internal/cute_serialize_internal.h>

#include <math.h>
#include <float.h>

namespace cute
{
	
#define CUTE_CHECK(X) if (X) ret = -1;

// Sequence buffer implementation strategy comes from Glenn's online articles:
// https://gafferongames.com/post/reliable_ordered_messages/

int sequence_buffer_init(sequence_buffer_t* buffer, int capacity, int stride, void* udata, void* mem_ctx)
{
	CUTE_MEMSET(buffer, 0, sizeof(sequence_buffer_t));
	buffer->capacity = capacity;
	buffer->stride = stride;
	buffer->entry_sequence = (uint32_t*)CUTE_ALLOC(sizeof(uint32_t) * capacity, mem_ctx);
	buffer->entry_data = (uint8_t*)CUTE_ALLOC(stride * capacity, mem_ctx);
	buffer->udata = udata;
	buffer->mem_ctx = mem_ctx;
	CUTE_MEMSET(buffer->entry_sequence, ~0, sizeof(uint32_t) * buffer->capacity);
	sequence_buffer_reset(buffer);
	return 0;
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
			if (cleanup_fn && buffer->entry_sequence[index] != 0xFFFFFFFF) {
				cleanup_fn(buffer->entry_data + buffer->stride * index, buffer->entry_sequence[index], buffer->udata, buffer->mem_ctx);
			}
			buffer->entry_sequence[index] = 0xFFFFFFFF;
		}
	} else {
		for (int i = 0; i < buffer->capacity; ++i)
		{
			if (cleanup_fn && buffer->entry_sequence[i] != 0xFFFFFFFF) {
				cleanup_fn(buffer->entry_data + buffer->stride * i, buffer->entry_sequence[i], buffer->udata, buffer->mem_ctx);
			}
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
		cleanup_fn(buffer->entry_data + buffer->stride * (sequence % buffer->capacity), buffer->entry_sequence[index], buffer->udata, buffer->mem_ctx);
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
		if (cleanup_fn) cleanup_fn(buffer->entry_data + buffer->stride * index, buffer->entry_sequence[index], buffer->udata, buffer->mem_ctx);
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

void packet_queue_init(packet_queue_t* q)
{
	q->count = 0;
	q->index0 = 0;
	q->index1 = 0;
}

int packet_queue_push(packet_queue_t* q, void* packet, int size)
{
	if (q->count >= CUTE_PACKET_QUEUE_MAX_ENTRIES) {
		return -1;
	} else {
		q->count++;
		q->sizes[q->index1] = size;
		q->packets[q->index1] = packet;
		q->index1 = (q->index1 + 1) % CUTE_PACKET_QUEUE_MAX_ENTRIES;
		return 0;
	}
}

int packet_queue_pop(packet_queue_t* q, void** packet, int* size)
{
	if (q->count <= 0) {
		return -1;
	} else {
		q->count--;
		*size = q->sizes[q->index0];
		*packet = q->packets[q->index0];
		q->index0 = (q->index0 + 1) % CUTE_PACKET_QUEUE_MAX_ENTRIES;
		return 0;
	}
}

// -------------------------------------------------------------------------------------------------

struct send_queue_item_t
{
	int fragment_index;
	int fragment_count;
	int final_fragment_size;

	int size;
	uint8_t* packet;
};

struct send_queue_t
{
	int count;
	int index0;
	int index1;
	send_queue_item_t items[CUTE_TRANSPORT_SEND_QUEUE_MAX_ENTRIES];
};

static void s_send_queue_init(send_queue_t* q)
{
	q->count = 0;
	q->index0 = 0;
	q->index1 = 0;
}

static int s_send_queue_push(send_queue_t* q, const send_queue_item_t* item)
{
	CUTE_ASSERT(q->count >= 0);
	CUTE_ASSERT(q->index0 >= 0 && q->index0 < CUTE_TRANSPORT_SEND_QUEUE_MAX_ENTRIES);
	CUTE_ASSERT(q->index1 >= 0 && q->index1 < CUTE_TRANSPORT_SEND_QUEUE_MAX_ENTRIES);

	if (q->count < CUTE_TRANSPORT_SEND_QUEUE_MAX_ENTRIES) {
		int next_index = (q->index1 + 1) % CUTE_TRANSPORT_SEND_QUEUE_MAX_ENTRIES;
		q->items[next_index] = *item;
		q->index1 = next_index;
		++q->count;
		return 0;
	} else {
		return -1;
	}
}

static void s_send_queue_pop(send_queue_t* q)
{
	CUTE_ASSERT(q->count >= 0 && q->count < CUTE_TRANSPORT_SEND_QUEUE_MAX_ENTRIES);
	CUTE_ASSERT(q->index0 >= 0 && q->index0 < CUTE_TRANSPORT_SEND_QUEUE_MAX_ENTRIES);
	CUTE_ASSERT(q->index1 >= 0 && q->index1 < CUTE_TRANSPORT_SEND_QUEUE_MAX_ENTRIES);

	int next_index = (q->index0 + 1) % CUTE_TRANSPORT_SEND_QUEUE_MAX_ENTRIES;
	q->index0 = next_index;
	--q->count;
}

static int s_send_queue_peek(send_queue_t* q, send_queue_item_t** item)
{
	CUTE_ASSERT(q->count >= 0 && q->count < CUTE_TRANSPORT_SEND_QUEUE_MAX_ENTRIES);
	CUTE_ASSERT(q->index0 >= 0 && q->index0 < CUTE_TRANSPORT_SEND_QUEUE_MAX_ENTRIES);
	CUTE_ASSERT(q->index1 >= 0 && q->index1 < CUTE_TRANSPORT_SEND_QUEUE_MAX_ENTRIES);

	if (q->count > 0) {
		int next_index = (q->index0 + 1) % CUTE_TRANSPORT_SEND_QUEUE_MAX_ENTRIES;
		*item = q->items + next_index;
		return 0;
	} else {
		return -1;
	}
}

// -------------------------------------------------------------------------------------------------

struct fragment_t
{
	int index;
	double timestamp;
	handle_t handle;
	uint8_t* data;
	int size;
};

struct fragment_reassembly_entry_t
{
	int received_final_fragment;
	int packet_size;
	uint8_t* packet;

	int fragment_count_so_far;
	int fragments_total;
	uint8_t* fragment_received;
};

struct packet_assembly_t
{
	uint16_t reassembly_sequence;
	sequence_buffer_t fragment_reassembly;
	packet_queue_t assembled_packets;
};

static void s_fragment_reassembly_entry_cleanup(void* data, uint16_t sequence, void* udata, void* mem_ctx)
{
	fragment_reassembly_entry_t* reassembly = (fragment_reassembly_entry_t*)data;
	CUTE_FREE(reassembly->packet, mem_ctx);
	CUTE_FREE(reassembly->fragment_received, mem_ctx);
}

static int s_packet_assembly_init(packet_assembly_t* assembly, int max_fragments_in_flight, void* mem_ctx)
{
	int ret = 0;
	int reassembly_init = 0;

	assembly->reassembly_sequence = 0;

	CUTE_CHECK(sequence_buffer_init(&assembly->fragment_reassembly, max_fragments_in_flight, sizeof(fragment_reassembly_entry_t), NULL, mem_ctx));
	reassembly_init = 1;
	packet_queue_init(&assembly->assembled_packets);

	if (ret) {
		if (reassembly_init) sequence_buffer_cleanup(&assembly->fragment_reassembly);
	}

	return ret;
}

static void s_packet_assembly_cleanup(packet_assembly_t* assembly)
{
	sequence_buffer_cleanup(&assembly->fragment_reassembly, s_fragment_reassembly_entry_cleanup);
}

// -------------------------------------------------------------------------------------------------

struct ack_system_t;

struct transport_t
{
	int fragment_size;
	int max_packet_size;
	int max_fragments_in_flight;
	int max_size_single_send;

	send_queue_t send_queue;

	array<fragment_t> fragments;
	handle_allocator_t* fragment_handle_table;
	sequence_buffer_t sent_fragments;

	ack_system_t* ack_system;
	packet_assembly_t reliable_and_in_order_assembly;
	packet_assembly_t fire_and_forget_assembly;

	void* mem_ctx;
	void* udata;

	uint8_t fire_and_forget_buffer[CUTE_TRANSPORT_MAX_FRAGMENT_SIZE + CUTE_TRANSPORT_HEADER_SIZE];
};

// -------------------------------------------------------------------------------------------------

struct sent_packet_t
{
	double timestamp;
	int acked;
	int size;
};

static void s_sent_packet_cleanup(void* data, uint16_t sequence, void* udata, void* mem_ctx)
{
	sent_packet_t* packet = (sent_packet_t*)data;
	packet->acked = 0;
}

struct received_packet_t
{
	double timestamp;
	int size;
};

ack_system_t* ack_system_make(const ack_system_config_t* config)
{
	int ret = 0;
	int sent_packets_init = 0;
	int received_packets_init = 0;
	void* mem_ctx = config->user_allocator_context;

	if (!config->send_packet_fn) return NULL;
	if (config->max_packet_size > CUTE_TRANSPORT_PACKET_PAYLOAD_MAX) return NULL;

	ack_system_t* ack_system = (ack_system_t*)CUTE_ALLOC(sizeof(ack_system_t), mem_ctx);
	if (!ack_system) return NULL;

	ack_system->time = 0;
	ack_system->max_packet_size = config->max_packet_size;
	ack_system->index = config->index;
	ack_system->send_packet_fn = config->send_packet_fn;
	ack_system->udata = config->udata;
	ack_system->mem_ctx = config->user_allocator_context;

	ack_system->sequence = 0;
	CUTE_PLACEMENT_NEW(&ack_system->acks) array<uint16_t>(config->user_allocator_context);
	ack_system->acks.ensure_capacity(config->initial_ack_capacity);
	CUTE_CHECK(sequence_buffer_init(&ack_system->sent_packets, config->sent_packets_sequence_buffer_size, sizeof(sent_packet_t), NULL, mem_ctx));
	sent_packets_init = 1;
	CUTE_CHECK(sequence_buffer_init(&ack_system->received_packets, config->received_packets_sequence_buffer_size, sizeof(received_packet_t), NULL, mem_ctx));
	received_packets_init = 1;

	ack_system->rtt = 0;
	ack_system->packet_loss = 0;
	ack_system->outgoing_bandwidth_kbps = 0;
	ack_system->incoming_bandwidth_kbps = 0;

	for (int i = 0; i < ACK_SYSTEM_COUNTERS_MAX; ++i) {
		ack_system->counters[i] = 0;
	}

	if (ret) {
		if (ack_system)
		{
			if (sent_packets_init) sequence_buffer_cleanup(&ack_system->sent_packets);
			if (received_packets_init) sequence_buffer_cleanup(&ack_system->received_packets);
		}
		CUTE_FREE(ack_system, config->user_allocator_context);
	}

	return ret ? NULL : ack_system;
}

void ack_system_destroy(ack_system_t* ack_system)
{
	ack_system->acks.~array<uint16_t>();
	sequence_buffer_cleanup(&ack_system->sent_packets);
	sequence_buffer_cleanup(&ack_system->received_packets);
	CUTE_FREE(ack_system, ack_system->mem_ctx);
}

void ack_system_reset(ack_system_t* ack_system)
{
	ack_system->sequence = 0;

	ack_system->acks.clear();
	sequence_buffer_reset(&ack_system->sent_packets);
	sequence_buffer_reset(&ack_system->received_packets);

	ack_system->rtt = 0;
	ack_system->packet_loss = 0;
	ack_system->outgoing_bandwidth_kbps = 0;
	ack_system->incoming_bandwidth_kbps = 0;

	for (int i = 0; i < ACK_SYSTEM_COUNTERS_MAX; ++i) {
		ack_system->counters[i] = 0;
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

error_t ack_system_send_packet(ack_system_t* ack_system, void* data, int size, uint16_t* sequence_out)
{
	if (size > ack_system->max_packet_size || size > CUTE_ACK_SYSTEM_MAX_PACKET_SIZE) {
		ack_system->counters[ACK_SYSTEM_COUNTERS_PACKETS_TOO_LARGE_TO_SEND]++;
		return error_failure("Exceeded max packet size in ack system.");
	}

	uint16_t sequence = ack_system->sequence++;
	uint16_t ack;
	uint32_t ack_bits;

	sequence_buffer_generate_ack_bits(&ack_system->received_packets, &ack, &ack_bits);
	sent_packet_t* packet = (sent_packet_t*)sequence_buffer_insert(&ack_system->sent_packets, sequence, s_sent_packet_cleanup);
	CUTE_ASSERT(packet);

	packet->timestamp = ack_system->time;
	packet->acked = 0;
	packet->size = size + CUTE_ACK_SYSTEM_HEADER_SIZE;

	uint8_t buffer[CUTE_TRANSPORT_PACKET_PAYLOAD_MAX];
	int header_size = s_write_ack_system_header(buffer, sequence, ack, ack_bits);
	CUTE_ASSERT(header_size == CUTE_ACK_SYSTEM_HEADER_SIZE);
	CUTE_ASSERT(size + header_size < CUTE_TRANSPORT_PACKET_PAYLOAD_MAX);
	CUTE_MEMCPY(buffer + header_size, data, size);
	if (sequence_out) *sequence_out = sequence;
	error_t err = ack_system->send_packet_fn(ack_system->index, buffer, size + header_size, ack_system->udata);
	if (err.is_error()) {
		ack_system->counters[ACK_SYSTEM_COUNTERS_PACKETS_INVALID]++;
		return err;
	}

	ack_system->counters[ACK_SYSTEM_COUNTERS_PACKETS_SENT]++;

	return error_success();
}

uint16_t ack_system_get_sequence(ack_system_t* ack_system)
{
	return ack_system->sequence;
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

error_t ack_system_receive_packet(ack_system_t* ack_system, void* data, int size)
{
	if (size > ack_system->max_packet_size || size > CUTE_ACK_SYSTEM_MAX_PACKET_SIZE) {
		ack_system->counters[ACK_SYSTEM_COUNTERS_PACKETS_TOO_LARGE_TO_RECEIVE]++;
		return error_failure("Exceeded max packet size in ack system.");
	}

	ack_system->counters[ACK_SYSTEM_COUNTERS_PACKETS_RECEIVED]++;

	uint16_t sequence;
	uint16_t ack;
	uint32_t ack_bits;
	uint8_t* buffer = (uint8_t*)data;

	int header_size = s_read_ack_system_header(buffer, size, &sequence, &ack, &ack_bits);
	if (header_size < 0) {
		ack_system->counters[ACK_SYSTEM_COUNTERS_PACKETS_INVALID]++;
		return error_failure("Failed to write ack header.");
	}
	CUTE_ASSERT(header_size == CUTE_ACK_SYSTEM_HEADER_SIZE);

	if (s_sequence_is_stale(&ack_system->received_packets, sequence)) {
		ack_system->counters[ACK_SYSTEM_COUNTERS_PACKETS_STALE]++;
		return error_failure("The provided sequence number was stale.");
	}

	received_packet_t* packet = (received_packet_t*)sequence_buffer_insert(&ack_system->received_packets, sequence);
	CUTE_ASSERT(packet);
	packet->timestamp = ack_system->time;
	packet->size = size;

	for (int i = 0; i < 32; ++i)
	{
		int bit_was_set = ack_bits & 1;
		ack_bits >>= 1;

		if (bit_was_set) {
			uint16_t ack_sequence = ack - ((uint16_t)i);
			sent_packet_t* sent_packet = (sent_packet_t*)sequence_buffer_find(&ack_system->sent_packets, ack_sequence);

			if (sent_packet && !sent_packet->acked) {
				ack_system->acks.add(ack_sequence);
				ack_system->counters[ACK_SYSTEM_COUNTERS_PACKETS_ACKED]++;
				sent_packet->acked = 1;

				float rtt = (float)(ack_system->time - sent_packet->timestamp);
				ack_system->rtt += (rtt - ack_system->rtt) * 0.001f;
				if (ack_system->rtt < 0) ack_system->rtt = 0;
			}
		}
	}

	return error_success();
}

uint16_t* ack_system_get_acks(ack_system_t* ack_system)
{
	return ack_system->acks.data();
}

int ack_system_get_acks_count(ack_system_t* ack_system)
{
	return ack_system->acks.count();
}

void ack_system_clear_acks(ack_system_t* ack_system)
{
	ack_system->acks.clear();
}

static CUTE_INLINE double s_calc_packet_loss(double packet_loss, sequence_buffer_t* sent_packets)
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

	double loss = (double)packet_drop_count / (double)packet_count;
	packet_loss += (loss - packet_loss) * 0.1;
	if (packet_loss < 0) packet_loss = 0;
	return packet_loss;
}

static CUTE_INLINE double s_calc_bandwidth(double bandwidth, sequence_buffer_t* sent_packets)
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
		double sent_bandwidth = (((double)bytes_sent / 1024.0) / (end_timestamp - start_timestamp));
		bandwidth += (sent_bandwidth - bandwidth) * 0.1f;
		if (bandwidth < 0) bandwidth = 0;
	}

	return bandwidth;
}

void ack_system_update(ack_system_t* ack_system, double dt)
{
	ack_system->time += dt;
	ack_system->packet_loss = s_calc_packet_loss(ack_system->packet_loss, &ack_system->sent_packets);
	ack_system->incoming_bandwidth_kbps = s_calc_bandwidth(ack_system->incoming_bandwidth_kbps, &ack_system->sent_packets);
	ack_system->outgoing_bandwidth_kbps = s_calc_bandwidth(ack_system->outgoing_bandwidth_kbps, &ack_system->received_packets);
}

double ack_system_rtt(ack_system_t* ack_system)
{
	return ack_system->rtt;
}

double ack_system_packet_loss(ack_system_t* ack_system)
{
	return ack_system->packet_loss;
}

double ack_system_bandwidth_outgoing_kbps(ack_system_t* ack_system)
{
	return ack_system->outgoing_bandwidth_kbps;
}

double ack_system_bandwidth_incoming_kbps(ack_system_t* ack_system)
{
	return ack_system->incoming_bandwidth_kbps;
}

uint64_t ack_system_get_counter(ack_system_t* ack_system, ack_system_counter_t counter)
{
	return ack_system->counters[counter];
}

// -------------------------------------------------------------------------------------------------

struct fragment_entry_t
{
	handle_t fragment_handle;
};

transport_t* transport_make(const transport_config_t* config)
{
	if (!config->send_packet_fn) return NULL;

	int ret = 0;
	int table_init = 0;
	int sequence_sent_fragments_init = 0;
	int assembly_reliable_init = 0;
	int assembly_unreliable_init = 0;

	transport_t* transport = (transport_t*)CUTE_ALLOC(sizeof(transport_t), config->user_allocator_context);
	if (!transport) return NULL;
	transport->fragment_size = config->fragment_size;
	transport->max_packet_size = config->max_packet_size;
	transport->max_fragments_in_flight = config->max_fragments_in_flight;
	transport->max_size_single_send = config->max_size_single_send;
	transport->udata = config->udata;

	CUTE_PLACEMENT_NEW(&transport->fragments) array<fragment_t>(config->user_allocator_context);

	ack_system_config_t ack_config;
	ack_config.index = config->index;
	ack_config.send_packet_fn = config->send_packet_fn;
	ack_config.udata = config->udata;
	transport->ack_system = ack_system_make(&ack_config);
	transport->mem_ctx = config->user_allocator_context;

	transport->fragment_handle_table = handle_allocator_make(config->send_receive_queue_size, transport->mem_ctx);
	table_init = 1;
	CUTE_CHECK(sequence_buffer_init(&transport->sent_fragments, config->send_receive_queue_size, sizeof(fragment_entry_t), transport, transport->mem_ctx));
	sequence_sent_fragments_init = 1;

	CUTE_CHECK(s_packet_assembly_init(&transport->reliable_and_in_order_assembly, config->send_receive_queue_size, transport->mem_ctx));
	assembly_reliable_init = 1;
	CUTE_CHECK(s_packet_assembly_init(&transport->fire_and_forget_assembly, config->send_receive_queue_size, transport->mem_ctx));
	assembly_unreliable_init = 1;

	s_send_queue_init(&transport->send_queue);

	if (ret) {
		if (table_init) handle_allocator_destroy(transport->fragment_handle_table);
		if (sequence_sent_fragments_init) sequence_buffer_cleanup(&transport->sent_fragments);
		if (assembly_reliable_init) s_packet_assembly_cleanup(&transport->reliable_and_in_order_assembly);
		if (assembly_unreliable_init) s_packet_assembly_cleanup(&transport->fire_and_forget_assembly);
		CUTE_FREE(transport, config->user_allocator_context);
	}

	return ret ? NULL : transport;
}

static void s_transport_cleanup_packet_queue(transport_t* transport, packet_queue_t* q)
{
	int index = q->index0;
	while (q->count--)
	{
		int next_index = index + 1 % CUTE_PACKET_QUEUE_MAX_ENTRIES;
		CUTE_FREE(q->packets[index], transport->mem_ctx);
		index = next_index;
	}
}

void transport_destroy(transport_t* transport)
{
	if (!transport) return;
	s_transport_cleanup_packet_queue(transport, &transport->reliable_and_in_order_assembly.assembled_packets);
	s_transport_cleanup_packet_queue(transport, &transport->fire_and_forget_assembly.assembled_packets);

	sequence_buffer_cleanup(&transport->sent_fragments);
	handle_allocator_destroy(transport->fragment_handle_table);
	s_packet_assembly_cleanup(&transport->reliable_and_in_order_assembly);
	s_packet_assembly_cleanup(&transport->fire_and_forget_assembly);
	for (int i = 0; i < transport->fragments.count(); ++i)
	{
		fragment_t* fragment = transport->fragments + i;
		CUTE_FREE(fragment->data, transport->mem_ctx);
	}
	transport->fragments.~array<fragment_t>();
	ack_system_destroy(transport->ack_system);
	CUTE_FREE(transport, config->user_allocator_context);
}

void transport_reset(transport_t* transport)
{
	CUTE_ASSERT(0); // TODO: Implement me.
	ack_system_reset(transport->ack_system);
}

static CUTE_INLINE int s_transport_write_header(uint8_t* buffer, int size, uint8_t prefix, uint16_t sequence, uint16_t fragment_count, uint16_t fragment_index, uint16_t fragment_size)
{
	if (size < CUTE_TRANSPORT_HEADER_SIZE) return -1;
	uint8_t* buffer_start = buffer;
	write_uint8(&buffer, prefix);
	write_uint16(&buffer, sequence);
	write_uint16(&buffer, fragment_count);
	write_uint16(&buffer, fragment_index);
	write_uint16(&buffer, fragment_size);
	return (int)(buffer - buffer_start);
}

static error_t s_transport_send_fragments(transport_t* transport)
{
	CUTE_ASSERT(transport->fragments.count() <= transport->max_fragments_in_flight);
	if (transport->fragments.count() == transport->max_fragments_in_flight) {
		return error_failure("Too many fragments already in flight.");
	}

	double timestamp = transport->ack_system->time;
	uint16_t reassembly_sequence = transport->reliable_and_in_order_assembly.reassembly_sequence;
	int fragments_space_available_send = transport->max_fragments_in_flight - transport->fragments.count();
	int fragment_size = transport->fragment_size;

	while (fragments_space_available_send)
	{
		send_queue_item_t* item;
		if (s_send_queue_peek(&transport->send_queue, &item) < 0) {
			break;
		}

		uint8_t* data_ptr = item->packet + item->fragment_index * fragment_size;
		int fragment_count_left = item->fragment_count - item->fragment_index;
		int fragment_count_to_send = fragments_space_available_send < fragment_count_left ? fragments_space_available_send : fragment_count_left;
		//if (item->fragment_index + fragment_count_to_send > item->fragment_count) __debugbreak();
		CUTE_ASSERT(item->fragment_index + fragment_count_to_send <= item->fragment_count);

		for (int i = 0; i < fragment_count_to_send; ++i)
		{
			// Allocate fragment.
			uint16_t fragment_header_index = (uint16_t)(item->fragment_index + i);
			int this_fragment_size = fragment_header_index != item->fragment_count - 1 ? fragment_size : item->final_fragment_size;
			uint8_t* fragment_src = data_ptr + fragment_size * i;
			CUTE_ASSERT(this_fragment_size <= CUTE_ACK_SYSTEM_MAX_PACKET_SIZE);

			int fragment_index = transport->fragments.count();
			fragment_t* fragment = &transport->fragments.add();
			handle_t fragment_handle = handle_allocator_alloc(transport->fragment_handle_table, fragment_index);

			fragment->index = fragment_header_index;
			fragment->timestamp = timestamp;
			fragment->handle = fragment_handle;
			fragment->data = (uint8_t*)CUTE_ALLOC(fragment_size + CUTE_TRANSPORT_HEADER_SIZE, transport->mem_ctx);
			fragment->size = this_fragment_size;
			// TODO: Memory pool on sent fragments.

			// Write the transport header.
			int header_size = s_transport_write_header(fragment->data, this_fragment_size + CUTE_TRANSPORT_HEADER_SIZE, 1, reassembly_sequence, item->fragment_count, fragment_header_index, (uint16_t)this_fragment_size);
			if (header_size != CUTE_TRANSPORT_HEADER_SIZE) {
				CUTE_FREE(fragment->data, transport->mem_ctx);
				transport->fragments.pop();
				return error_failure("Failed to write transport header.");
			}

			// Copy over the `data` from user.
			CUTE_MEMCPY(fragment->data + header_size, fragment_src, this_fragment_size);

			// Send to ack system.
			uint16_t sequence;
			error_t err = ack_system_send_packet(transport->ack_system, fragment->data, this_fragment_size + CUTE_TRANSPORT_HEADER_SIZE, &sequence);
			if (err.is_error()) {
				CUTE_FREE(fragment->data, transport->mem_ctx);
				transport->fragments.pop();
				return err;
			}

			// If all succeeds, record fragment entry. Hopefully it will be acked later.
			fragment_entry_t* fragment_entry = (fragment_entry_t*)sequence_buffer_insert(&transport->sent_fragments, sequence);
			CUTE_ASSERT(fragment_entry);
			fragment_entry->fragment_handle = fragment_handle;
		}

		if (item->fragment_index + fragment_count_to_send == item->fragment_count) {
			s_send_queue_pop(&transport->send_queue);
			CUTE_FREE(item->packet, transport->mem_ctx);
			transport->reliable_and_in_order_assembly.reassembly_sequence++;
		} else {
			item->fragment_index += fragment_count_to_send;
		}

		fragments_space_available_send -= fragment_count_to_send;
	}

	return error_success();
}

error_t s_send_reliably(transport_t* transport, const void* data, int size)
{
	if (size < 1) return error_failure("Negative `size` not allowed.");
	if (size > transport->max_size_single_send) return error_failure("`size` exceeded `max_size_single_send` from `transport->config`.");

	int fragment_size = transport->fragment_size;
	int fragment_count = size / fragment_size;
	int final_fragment_size = size - (fragment_count * fragment_size);
	if (final_fragment_size > 0) fragment_count++;

	send_queue_item_t send_item;
	send_item.fragment_index = 0;
	send_item.fragment_count = fragment_count;
	send_item.final_fragment_size = final_fragment_size;
	send_item.size = size;
	send_item.packet = (uint8_t*)CUTE_ALLOC(size, transport->mem_ctx);
	if (!send_item.packet) return error_failure("Failed allocation.");
	CUTE_MEMCPY(send_item.packet, data, size);

	if (s_send_queue_push(&transport->send_queue, &send_item) < 0) {
		return error_failure("Send queue for reliable-and-in-order packets is full. Increase `CUTE_TRANSPORT_SEND_QUEUE_MAX_ENTRIES` or send packets less frequently.");
	}

	s_transport_send_fragments(transport);

	return error_success();
}

error_t s_send(transport_t* transport, const void* data, int size)
{
	if (size < 1) return error_failure("Negative `size` is not valid.");
	if (size > transport->max_size_single_send) return error_failure("`size` exceeded `max_size_single_send` config param.");

	int fragment_size = transport->fragment_size;
	int fragment_count = size / fragment_size;
	int final_fragment_size = size - (fragment_count * fragment_size);
	if (final_fragment_size > 0) fragment_count++;

	double timestamp = transport->ack_system->time;
	uint16_t reassembly_sequence = transport->fire_and_forget_assembly.reassembly_sequence++;
	uint8_t* buffer = transport->fire_and_forget_buffer;

	uint8_t* data_ptr = (uint8_t*)data;
	for (int i = 0; i < fragment_count; ++i)
	{
		int this_fragment_size = i != fragment_count - 1 ? fragment_size : final_fragment_size;
		uint8_t* fragment_src = data_ptr + fragment_size * i;
		CUTE_ASSERT(this_fragment_size <= CUTE_ACK_SYSTEM_MAX_PACKET_SIZE);

		// Write the transport header.
		int header_size = s_transport_write_header(buffer, this_fragment_size + CUTE_TRANSPORT_HEADER_SIZE, 0, reassembly_sequence, fragment_count, (uint16_t)i, (uint16_t)this_fragment_size);
		if (header_size != CUTE_TRANSPORT_HEADER_SIZE) {
			return error_failure("Failed writing transport header -- incorrect size of bytes written (this is probably a bug).");
		}

		// Copy over fragment data from src.
		CUTE_MEMCPY(buffer + CUTE_TRANSPORT_HEADER_SIZE, fragment_src, this_fragment_size);

		// Send to ack system.
		uint16_t sequence;
		error_t err = ack_system_send_packet(transport->ack_system, buffer, this_fragment_size + CUTE_TRANSPORT_HEADER_SIZE, &sequence);
		if (err.is_error()) return err;
	}

	return error_success();
}

error_t transport_send(transport_t* transport, const void* data, int size, bool send_reliably)
{
	if (send_reliably) {
		return s_send_reliably(transport, data, size);
	} else {
		return s_send(transport, data, size);
	}
}

error_t transport_receive_reliably_and_in_order(transport_t* transport, void** data, int* size)
{
	packet_assembly_t* assembly = &transport->reliable_and_in_order_assembly;
	if (packet_queue_pop(&assembly->assembled_packets, data, size) < 0) {
		*data = NULL;
		*size = 0;
		return error_failure("No data.");
	} else {
		return error_success();
	}
}

error_t transport_receive_fire_and_forget(transport_t* transport, void** data, int* size)
{
	packet_assembly_t* assembly = &transport->fire_and_forget_assembly;
	if (packet_queue_pop(&assembly->assembled_packets, data, size) < 0) {
		*data = NULL;
		*size = 0;
		return error_failure("No data.");
	} else {
		return error_success();
	}
}

void transport_free_packet(transport_t* transport, void* data)
{
	CUTE_FREE(data, transport->mem_ctx);
}

error_t transport_process_packet(transport_t* transport, void* data, int size)
{
	if (size < CUTE_TRANSPORT_HEADER_SIZE) return error_failure("`size` is too small to fit `CUTE_TRANSPORT_HEADER_SIZE`.");
	error_t err = ack_system_receive_packet(transport->ack_system, data, size);
	if (err.is_error()) return err;

	// Read transport header.
	uint8_t* buffer = (uint8_t*)data + CUTE_ACK_SYSTEM_HEADER_SIZE;
	uint8_t prefix = read_uint8(&buffer);
	uint16_t reassembly_sequence = read_uint16(&buffer);
	uint16_t fragment_count = read_uint16(&buffer);
	uint16_t fragment_index = read_uint16(&buffer);
	uint16_t fragment_size = read_uint16(&buffer);
	int total_packet_size = fragment_count * transport->fragment_size;

	if (total_packet_size > transport->max_size_single_send) {
		return error_failure("Packet exceeded `max_size_single_send` limit.");
	}

	if (fragment_index > fragment_count) {
		return error_failure("Fragment index out of bounds.");
	}

	if (fragment_size > transport->fragment_size) {
		return error_failure("Fragment size somehow didn't match `transport->fragment_size`.");
	}

	packet_assembly_t* assembly;
	if (prefix) {
		assembly = &transport->reliable_and_in_order_assembly;
	} else {
		assembly = &transport->fire_and_forget_assembly;
	}

	// Build reassembly if it doesn't exist yet.
	fragment_reassembly_entry_t* reassembly = (fragment_reassembly_entry_t*)sequence_buffer_find(&assembly->fragment_reassembly, reassembly_sequence);
	if (!reassembly) {
		if (s_sequence_less_than(reassembly_sequence, assembly->fragment_reassembly.sequence)) {
			return error_failure("Old sequence encountered (this packet was already reassembled fully).");
		}
		reassembly = (fragment_reassembly_entry_t*)sequence_buffer_insert(&assembly->fragment_reassembly, reassembly_sequence, s_fragment_reassembly_entry_cleanup);
		if (!reassembly) {
			return error_failure("Sequence for this reassembly is stale.");
		}
		reassembly->received_final_fragment = 0;
		reassembly->packet_size = total_packet_size;
		reassembly->packet = (uint8_t*)CUTE_ALLOC(total_packet_size, transport->mem_ctx);
		if (!reassembly->packet) return error_failure("Failed allocation.");
		reassembly->fragment_received = (uint8_t*)CUTE_ALLOC(fragment_count, transport->mem_ctx);
		if (!reassembly->fragment_received) {
			CUTE_FREE(reassembly->packet, transport->mem_ctx);
			return error_failure("Full packet not yet received.");
		}
		CUTE_MEMSET(reassembly->fragment_received, 0, fragment_count);
		reassembly->fragment_count_so_far = 0;
		reassembly->fragments_total = fragment_count;
	}

	if (fragment_count != reassembly->fragments_total) {
		return error_failure("Full packet not yet received.");
	}

	if (reassembly->fragment_received[fragment_index]) {
		return error_success();
	}

	// Copy in fragment pieces into a single large packet buffer.
	reassembly->fragment_count_so_far++;
	reassembly->fragment_received[fragment_index] = 1;

	uint8_t* packet_fragment = reassembly->packet + fragment_index * transport->fragment_size;
	CUTE_MEMCPY(packet_fragment, buffer, fragment_size);

	if (fragment_index == fragment_count - 1) {
		reassembly->received_final_fragment = 1;
		reassembly->packet_size -= transport->fragment_size - fragment_size;
		total_packet_size -= transport->fragment_size - fragment_size;
	}

	// Store completed packet for retrieval by user.
	if (reassembly->fragment_count_so_far == fragment_count) {
		uint16_t assembled_sequence = reassembly_sequence;
		if (packet_queue_push(&assembly->assembled_packets, reassembly->packet, reassembly->packet_size) < 0) {
			//TODO: Log. Dropped packet since reassembly buffer was too small.
			CUTE_FREE(reassembly->packet, transport->mem_ctx);
			CUTE_ASSERT(false); // ??? Is this allowed ???
		}
		reassembly->packet = NULL;
		sequence_buffer_remove(&assembly->fragment_reassembly, reassembly_sequence, s_fragment_reassembly_entry_cleanup);
	}

	return error_success();
}

void transport_process_acks(transport_t* transport)
{
	uint16_t* acks = ack_system_get_acks(transport->ack_system);
	int acks_count = ack_system_get_acks_count(transport->ack_system);

	for (int i = 0; i < acks_count; ++i)
	{
		uint16_t sequence = acks[i];
		fragment_entry_t* fragment_entry = (fragment_entry_t*)sequence_buffer_find(&transport->sent_fragments, sequence);
		if (fragment_entry) {
			handle_t h = fragment_entry->fragment_handle;
			if (handle_allocator_is_handle_valid(transport->fragment_handle_table, h)) {
				uint32_t index = handle_allocator_get_index(transport->fragment_handle_table, h);
				CUTE_ASSERT((int)index < transport->fragments.count());
				handle_allocator_free(transport->fragment_handle_table, h);
				fragment_t* fragment = transport->fragments + index;
				CUTE_FREE(fragment->data, transport->mem_ctx);
				handle_t last_handle = transport->fragments[transport->fragments.count() - 1].handle;
				if (handle_allocator_is_handle_valid(transport->fragment_handle_table, last_handle)) {
					handle_allocator_update_index(transport->fragment_handle_table, last_handle, index);
				}
				transport->fragments.unordered_remove(index);
				sequence_buffer_remove(&transport->sent_fragments, sequence);
			}
		}
	}

	ack_system_clear_acks(transport->ack_system);
}

void transport_resend_unacked_fragments(transport_t* transport)
{
	// Resend unacked fragments which were previously sent.
	double timestamp = transport->ack_system->time;
	int count = transport->fragments.count();
	fragment_t* fragments = transport->fragments.data();

	for (int i = 0; i < count;)
	{
		fragment_t* fragment = fragments + i;
		if (fragment->timestamp + 0.01f >= timestamp) {
			++i;
			continue;
		}

		// Send to ack system.
		uint16_t sequence;
		error_t err = ack_system_send_packet(transport->ack_system, fragment->data, fragment->size + CUTE_TRANSPORT_HEADER_SIZE, &sequence);
		if (err.is_error()) {
			// Remove failed fragments (this should never happen, and is only here for safety).
			handle_allocator_free(transport->fragment_handle_table, fragment->handle);
			CUTE_FREE(fragment->data, transport->mem_ctx);
			transport->fragments.unordered_remove(i);
			--count;
			continue;
		} else {
			 ++i;
		}

		fragment->timestamp = timestamp;
		fragment_entry_t* fragment_entry = (fragment_entry_t*)sequence_buffer_insert(&transport->sent_fragments, sequence);
		CUTE_ASSERT(fragment_entry);
		fragment_entry->fragment_handle = fragment->handle;
	}

	// Send off any available fragments from the send queue.
	s_transport_send_fragments(transport);
}

int transport_unacked_fragment_count(transport_t* transport)
{
	return transport->fragments.count();
}

void transport_update(transport_t* transport, double dt)
{
	ack_system_update(transport->ack_system, dt);
	transport_process_acks(transport);
	transport_resend_unacked_fragments(transport);
}

}
