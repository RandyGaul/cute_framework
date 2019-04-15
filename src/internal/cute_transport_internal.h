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

namespace cute
{

struct sequence_buffer_t
{
	uint16_t sequence;
	int capacity;
	int stride;
	uint32_t* entry_sequence;
	uint8_t* entry_data;
	void* mem_ctx;
};

typedef void (sequence_buffer_cleanup_entry_fn)(void* data, void* mem_ctx);

extern CUTE_API int CUTE_CALL sequence_buffer_init(sequence_buffer_t* buffer, int capacity, int stride, void* mem_ctx);
extern CUTE_API void CUTE_CALL sequence_buffer_cleanup(sequence_buffer_t* buffer, sequence_buffer_cleanup_entry_fn* cleanup_fn = NULL);
extern CUTE_API void CUTE_CALL sequence_buffer_reset(sequence_buffer_t* buffer, sequence_buffer_cleanup_entry_fn* cleanup_fn = NULL);

extern CUTE_API void CUTE_CALL sequence_buffer_advance(sequence_buffer_t* buffer, uint16_t sequence);
extern CUTE_API void* CUTE_CALL sequence_buffer_insert(sequence_buffer_t* buffer, uint16_t sequence, sequence_buffer_cleanup_entry_fn* cleanup_fn = NULL);
extern CUTE_API void CUTE_CALL sequence_buffer_remove(sequence_buffer_t* buffer, uint16_t sequence, sequence_buffer_cleanup_entry_fn* cleanup_fn = NULL);
extern CUTE_API int CUTE_CALL sequence_buffer_is_empty(sequence_buffer_t* sequence_buffer, uint16_t sequence);
extern CUTE_API void* CUTE_CALL sequence_buffer_find(sequence_buffer_t* sequence_buffer, uint16_t sequence);
extern CUTE_API void* CUTE_CALL sequence_buffer_at_index(sequence_buffer_t* sequence_buffer, int index);
extern CUTE_API void CUTE_CALL sequence_buffer_generate_ack_bits(sequence_buffer_t* sequence_buffer, uint16_t* ack, uint32_t* ack_bits);

// -------------------------------------------------------------------------------------------------

struct transport_config_t
{
	int use_ipv4 = 0;
	int use_ipv6 = 1;
	int max_packet_size = 1200;
	int initial_ack_capacity = 256;
	int sent_packets_sequence_buffer_size = 256;
	int received_packets_sequence_buffer_size = 256;

	int (*send_packet_fn)(uint16_t sequence, void* packet, int size, void* udata) = NULL;
	int (*open_packet_fn)(uint16_t sequence, void* packet, int size, void* udata) = NULL;

	void* udata = NULL;
	void* user_allocator_context = NULL;
};

struct transport_t;

extern CUTE_API transport_t* CUTE_CALL transport_make(const transport_config_t* config);
extern CUTE_API void CUTE_CALL transport_destroy(transport_t* transport);
extern CUTE_API void CUTE_CALL transport_reset(transport_t* transport);

extern CUTE_API int CUTE_CALL transport_send_packet(transport_t* transport, void* data, int size, uint16_t* sequence);
extern CUTE_API int CUTE_CALL transport_receive_packet(transport_t* transport, void* data, int size);

extern CUTE_API uint16_t* CUTE_CALL transport_get_acks(transport_t* transport);
extern CUTE_API int CUTE_CALL transport_get_acks_count(transport_t* transport);
extern CUTE_API void CUTE_CALL transport_clear_acks(transport_t* transport);

extern CUTE_API void CUTE_CALL transport_update(transport_t* transport, float dt);
extern CUTE_API float CUTE_CALL transport_rtt(transport_t* transport);
extern CUTE_API float CUTE_CALL transport_packet_loss(transport_t* transport);
extern CUTE_API float CUTE_CALL transport_bandwidth_outgoing_kbps(transport_t* transport);
extern CUTE_API float CUTE_CALL transport_bandwidth_incoming_kbps(transport_t* transport);

enum transport_counter_t
{
	TRANSPORT_COUNTERS_PACKETS_SENT,
	TRANSPORT_COUNTERS_PACKETS_RECEIVED,
	TRANSPORT_COUNTERS_PACKETS_ACKED,
	TRANSPORT_COUNTERS_PACKETS_STALE,
	TRANSPORT_COUNTERS_PACKETS_INVALID,
	TRANSPORT_COUNTERS_PACKETS_TOO_LARGE_TO_SEND,
	TRANSPORT_COUNTERS_PACKETS_TOO_LARGE_TO_RECEIVE,

	TRANSPORT_COUNTERS_MAX
};

extern CUTE_API uint64_t CUTE_CALL transport_get_counter(transport_t* transport, transport_counter_t counter);

// -------------------------------------------------------------------------------------------------

struct block_transport_configuration_t
{
	int fragment_size;

	void* user_allocator_context = NULL;
};

struct block_transport_t;

extern CUTE_API block_transport_t* CUTE_CALL block_transport_make(const block_transport_configuration_t* config);
extern CUTE_API void CUTE_CALL block_transport_destroy(block_transport_t* block_transport);

}

#endif // CUTE_TRANSPORT_INTERNAL_H
