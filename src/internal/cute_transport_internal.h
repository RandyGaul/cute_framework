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

#ifndef CUTE_TRANSPORT_INTERNAL_H
#define CUTE_TRANSPORT_INTERNAL_H

#include <cute_defines.h>

#define CUTE_TRANSPORT_PACKET_PAYLOAD_MAX (1200)

// TODO: Pack header sizes down to a minimum.
// TODO: Look for optimizations on single-fragment sending.
// TODO: Audit default values and sizes.
// TODO: Look for places to use memory pools.
// TODO: Audit bounds checking.
// TODO: Fire and forget should have an upper-bound on packet size (configurable) and a max configurable size.
// TODO: Reliable resending rate.

namespace cute
{

struct sequence_buffer_t
{
	uint16_t sequence;
	int capacity;
	int stride;
	uint32_t* entry_sequence;
	uint8_t* entry_data;
	void* udata;
	void* mem_ctx;
};

// TODO: Place this on the sequence buffer itself to minimize parameters.
typedef void (sequence_buffer_cleanup_entry_fn)(void* data, uint16_t sequence, void* udata, void* mem_ctx);

extern CUTE_API int CUTE_CALL sequence_buffer_init(sequence_buffer_t* buffer, int capacity, int stride, void* udata, void* mem_ctx);
extern CUTE_API void CUTE_CALL sequence_buffer_cleanup(sequence_buffer_t* buffer, sequence_buffer_cleanup_entry_fn* cleanup_fn = NULL);
extern CUTE_API void CUTE_CALL sequence_buffer_reset(sequence_buffer_t* buffer, sequence_buffer_cleanup_entry_fn* cleanup_fn = NULL);

extern CUTE_API void* CUTE_CALL sequence_buffer_insert(sequence_buffer_t* buffer, uint16_t sequence, sequence_buffer_cleanup_entry_fn* cleanup_fn = NULL);
extern CUTE_API void CUTE_CALL sequence_buffer_remove(sequence_buffer_t* buffer, uint16_t sequence, sequence_buffer_cleanup_entry_fn* cleanup_fn = NULL);
extern CUTE_API int CUTE_CALL sequence_buffer_is_empty(sequence_buffer_t* sequence_buffer, uint16_t sequence);
extern CUTE_API void* CUTE_CALL sequence_buffer_find(sequence_buffer_t* sequence_buffer, uint16_t sequence);
extern CUTE_API void* CUTE_CALL sequence_buffer_at_index(sequence_buffer_t* sequence_buffer, int index);
extern CUTE_API void CUTE_CALL sequence_buffer_generate_ack_bits(sequence_buffer_t* sequence_buffer, uint16_t* ack, uint32_t* ack_bits);

// -------------------------------------------------------------------------------------------------

#define CUTE_PACKET_QUEUE_MAX_ENTRIES (1024)

struct packet_queue_t
{
	int count;
	int index0;
	int index1;
	int sizes[CUTE_PACKET_QUEUE_MAX_ENTRIES];
	void* packets[CUTE_PACKET_QUEUE_MAX_ENTRIES];
};

extern CUTE_API void CUTE_CALL packet_queue_init(packet_queue_t* q);
extern CUTE_API int CUTE_CALL packet_queue_push(packet_queue_t* q, void* packet, int size);
extern CUTE_API int CUTE_CALL packet_queue_pop(packet_queue_t* q, void** packet, int *size);

// -------------------------------------------------------------------------------------------------

#define CUTE_ACK_SYSTEM_HEADER_SIZE (2 + 2 + 4)
#define CUTE_ACK_SYSTEM_MAX_PACKET_SIZE 1200

struct ack_system_config_t
{
	int use_ipv4 = 0;
	int use_ipv6 = 1;
	int max_packet_size = CUTE_ACK_SYSTEM_MAX_PACKET_SIZE;
	int initial_ack_capacity = 256;
	int sent_packets_sequence_buffer_size = 256;
	int received_packets_sequence_buffer_size = 256;

	int (*send_packet_fn)(uint16_t sequence, void* packet, int size, void* udata) = NULL;
	int (*open_packet_fn)(uint16_t sequence, void* packet, int size, void* udata) = NULL;

	void* udata = NULL;
	void* user_allocator_context = NULL;
};

struct ack_system_t;

extern CUTE_API ack_system_t* CUTE_CALL ack_system_make(const ack_system_config_t* config);
extern CUTE_API void CUTE_CALL ack_system_destroy(ack_system_t* transport);
extern CUTE_API void CUTE_CALL ack_system_reset(ack_system_t* transport);

extern CUTE_API uint16_t CUTE_CALL ack_system_get_sequence(ack_system_t* ack_system);
extern CUTE_API int CUTE_CALL ack_system_send_packet(ack_system_t* transport, void* data, int size, uint16_t* sequence = NULL);
extern CUTE_API int CUTE_CALL ack_system_receive_packet(ack_system_t* transport, void* data, int size);

extern CUTE_API uint16_t* CUTE_CALL ack_system_get_acks(ack_system_t* transport);
extern CUTE_API int CUTE_CALL ack_system_get_acks_count(ack_system_t* transport);
extern CUTE_API void CUTE_CALL ack_system_clear_acks(ack_system_t* transport);

extern CUTE_API void CUTE_CALL ack_system_update(ack_system_t* transport, float dt);
extern CUTE_API float CUTE_CALL ack_system_rtt(ack_system_t* transport);
extern CUTE_API float CUTE_CALL ack_system_packet_loss(ack_system_t* transport);
extern CUTE_API float CUTE_CALL ack_system_bandwidth_outgoing_kbps(ack_system_t* transport);
extern CUTE_API float CUTE_CALL ack_system_bandwidth_incoming_kbps(ack_system_t* transport);

enum ack_system_counter_t
{
	ACK_SYSTEM_COUNTERS_PACKETS_SENT,
	ACK_SYSTEM_COUNTERS_PACKETS_RECEIVED,
	ACK_SYSTEM_COUNTERS_PACKETS_ACKED,
	ACK_SYSTEM_COUNTERS_PACKETS_STALE,
	ACK_SYSTEM_COUNTERS_PACKETS_INVALID,
	ACK_SYSTEM_COUNTERS_PACKETS_TOO_LARGE_TO_SEND,
	ACK_SYSTEM_COUNTERS_PACKETS_TOO_LARGE_TO_RECEIVE,

	ACK_SYSTEM_COUNTERS_MAX
};

extern CUTE_API uint64_t CUTE_CALL ack_system_get_counter(ack_system_t* transport, ack_system_counter_t counter);

// -------------------------------------------------------------------------------------------------

#define CUTE_TRANSPORT_HEADER_SIZE (1 + 2 + 2 + 2 + 2)
#define CUTE_TRANSPORT_MAX_FRAGMENT_SIZE 1100
#define CUTE_TRANSPORT_SEND_QUEUE_MAX_ENTRIES (1024)

CUTE_STATIC_ASSERT(CUTE_ACK_SYSTEM_MAX_PACKET_SIZE + CUTE_TRANSPORT_HEADER_SIZE < 1253, "Must fit within Cute Protocol's payload limit.");

struct transport_config_t
{
	int fragment_size = CUTE_TRANSPORT_MAX_FRAGMENT_SIZE;
	int max_packet_size = CUTE_TRANSPORT_MAX_FRAGMENT_SIZE * 4;
	int max_fragments_in_flight = 8;
	int max_size_single_send = CUTE_MB * 20;

	ack_system_t* ack_system = NULL;

	void* user_allocator_context = NULL;
};

struct transport_t;

extern CUTE_API transport_t* CUTE_CALL transport_make(const transport_config_t* config);
extern CUTE_API void CUTE_CALL transport_destroy(transport_t* transport);
extern CUTE_API void CUTE_CALL transport_reset(transport_t* tranpsport);

extern CUTE_API int CUTE_CALL transport_send_reliably_and_in_order(transport_t* transport, void* data, int size);
extern CUTE_API int CUTE_CALL transport_send_fire_and_forget(transport_t* transport, void* data, int size);

extern CUTE_API int CUTE_CALL transport_recieve_reliably_and_in_order(transport_t* transport, void** data, int* size);
extern CUTE_API int CUTE_CALL transport_recieve_fire_and_forget(transport_t* transport, void** data, int* size);
extern CUTE_API void CUTE_CALL transport_free(transport_t* transport, void* data);

extern CUTE_API int CUTE_CALL transport_process_packet(transport_t* transport, void* data, int size, uint16_t sequence);
extern CUTE_API void CUTE_CALL transport_process_acks(transport_t* transport);
extern CUTE_API void CUTE_CALL transport_clear_acks(transport_t* transport);
extern CUTE_API void CUTE_CALL transport_resend_unacked_fragments(transport_t* transport);
extern CUTE_API int CUTE_CALL transport_unacked_fragment_count(transport_t* transport);

}

#endif // CUTE_TRANSPORT_INTERNAL_H
