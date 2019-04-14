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

#include <internal/cute_defines_internal.h>
#include <internal/cute_transport_internal.h>
#include <internal/cute_serialize_internal.h>

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

void sequence_buffer_cleanup(sequence_buffer_t* buffer)
{
	CUTE_FREE(buffer->entry_sequence, buffer->mem_ctx);
	CUTE_FREE(buffer->entry_data, buffer->mem_ctx);
	CUTE_MEMSET(buffer, 0, sizeof(sequence_buffer_t));
}

void sequence_buffer_reset(sequence_buffer_t* buffer)
{
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

void* sequence_buffer_insert(sequence_buffer_t* buffer, uint16_t sequence, sequence_buffer_cleanup_entry_fn* cleanup_fn)
{
	if (s_sequence_greater_than(sequence + 1, buffer->sequence)) {
		s_sequence_buffer_remove_entries(buffer, buffer->sequence, sequence, cleanup_fn);
		buffer->sequence = sequence + 1;
	} else if (s_sequence_less_than(sequence, buffer->sequence - ((uint16_t)buffer->capacity))) {
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
	CUTE_ASSERT(ack);
	CUTE_ASSERT(ack_bits);
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

}
