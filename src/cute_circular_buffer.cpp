/*
	Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

#include <cute_circular_buffer.h>
#include <cute_alloc.h>
#include <cute_c_runtime.h>
#include <cute_multithreading.h>

#include <internal/cute_alloc_internal.h>

CF_CircularBuffer cf_make_circular_buffer(int initial_size_in_bytes)
{
	CF_CircularBuffer buffer = { 0 };
	buffer.size_left.i = initial_size_in_bytes;
	buffer.capacity = initial_size_in_bytes;
	buffer.data = (uint8_t*)CF_ALLOC(initial_size_in_bytes);
	return buffer;
}

void cf_destroy_circular_buffer(CF_CircularBuffer* buffer)
{
	CF_FREE(buffer->data);
	CF_MEMSET(buffer, 0, sizeof(*buffer));
}

void cf_circular_buffer_reset(CF_CircularBuffer* buffer)
{
	buffer->index0 = 0;
	buffer->index1 = 0;
	buffer->size_left.i = buffer->capacity;
}

int cf_circular_buffer_push(CF_CircularBuffer* buffer, const void* data, int size)
{
	if (cf_atomic_get(&buffer->size_left) < size) {
		return -1;
	}

	int bytes_to_end = buffer->capacity - buffer->index1;
	if (size > bytes_to_end) {
		CF_MEMCPY(buffer->data + buffer->index1, data, bytes_to_end);
		CF_MEMCPY(buffer->data, (uint8_t*)data + bytes_to_end, size - bytes_to_end);
		buffer->index1 = (size - bytes_to_end) % buffer->capacity;
	} else {
		CF_MEMCPY(buffer->data + buffer->index1, data, size);
		buffer->index1 = (buffer->index1 + size) % buffer->capacity;
	}

	cf_atomic_add(&buffer->size_left, -size);

	return 0;
}

int cf_circular_buffer_pull(CF_CircularBuffer* buffer, void* data, int size)
{
	if (buffer->capacity - cf_atomic_get(&buffer->size_left) < size) {
		return -1;
	}

	int bytes_to_end = buffer->capacity - buffer->index0;
	if (size > bytes_to_end) {
		CF_MEMCPY(data, buffer->data + buffer->index0, bytes_to_end);
		CF_MEMCPY((uint8_t*)data + bytes_to_end, buffer->data, size - bytes_to_end);
		buffer->index0 = (size - bytes_to_end) % buffer->capacity;
	} else {
		CF_MEMCPY(data, buffer->data + buffer->index0, size);
		buffer->index0 = (buffer->index0 + size) % buffer->capacity;
	}

	cf_atomic_add(&buffer->size_left, size);

	return 0;
}

int cf_circular_buffer_grow(CF_CircularBuffer* buffer, int new_size_in_bytes)
{
	uint8_t* old_data = buffer->data;
	uint8_t* new_data = (uint8_t*)CF_ALLOC(new_size_in_bytes);
	if (!new_data) return -1;

	int index0 = buffer->index0;
	int index1 = buffer->index1;

	if (index0 < index1) {
		CF_MEMCPY(new_data + index0, old_data + index0, index1 - index0);
	} else {
		CF_MEMCPY(new_data, old_data, index1);
		int offset_from_end = buffer->capacity - index0;
		CF_MEMCPY(new_data + new_size_in_bytes - offset_from_end, old_data + index0, offset_from_end);
	}

	CF_FREE(old_data);
	buffer->data = new_data;
	cf_atomic_add(&buffer->size_left, new_size_in_bytes - buffer->capacity);
	buffer->capacity = new_size_in_bytes;

	return 0;
}

