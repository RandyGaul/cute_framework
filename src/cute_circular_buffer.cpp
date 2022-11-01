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

#include <cute_circular_buffer.h>
#include <cute_alloc.h>
#include <cute_c_runtime.h>
#include <cute_concurrency.h>

cf_circular_buffer_t cf_make_circular_buffer(int initial_size_in_bytes, void* user_allocator_context)
{
	cf_circular_buffer_t buffer = { 0 };
	buffer.size_left.i = initial_size_in_bytes;
	buffer.capacity = initial_size_in_bytes;
	buffer.data = (uint8_t*)CUTE_ALLOC(initial_size_in_bytes, user_allocator_context);
	buffer.user_allocator_context = user_allocator_context;
	return buffer;
}

void cf_destroy_circular_buffer(cf_circular_buffer_t* buffer)
{
	CUTE_FREE(buffer->data, buffer->user_allocator_context);
	CUTE_MEMSET(buffer, 0, sizeof(*buffer));
}

void cf_circular_buffer_reset(cf_circular_buffer_t* buffer)
{
	buffer->index0 = 0;
	buffer->index1 = 0;
	buffer->size_left.i = buffer->capacity;
}

int cf_circular_buffer_push(cf_circular_buffer_t* buffer, const void* data, int size)
{
	if (cf_atomic_get(&buffer->size_left) < size) {
		return -1;
	}

	int bytes_to_end = buffer->capacity - buffer->index1;
	if (size > bytes_to_end) {
		CUTE_MEMCPY(buffer->data + buffer->index1, data, bytes_to_end);
		CUTE_MEMCPY(buffer->data, (uint8_t*)data + bytes_to_end, size - bytes_to_end);
		buffer->index1 = (size - bytes_to_end) % buffer->capacity;
	} else {
		CUTE_MEMCPY(buffer->data + buffer->index1, data, size);
		buffer->index1 = (buffer->index1 + size) % buffer->capacity;
	}

	cf_atomic_add(&buffer->size_left, -size);

	return 0;
}

int cf_circular_buffer_pull(cf_circular_buffer_t* buffer, void* data, int size)
{
	if (buffer->capacity - cf_atomic_get(&buffer->size_left) < size) {
		return -1;
	}

	int bytes_to_end = buffer->capacity - buffer->index0;
	if (size > bytes_to_end) {
		CUTE_MEMCPY(data, buffer->data + buffer->index0, bytes_to_end);
		CUTE_MEMCPY((uint8_t*)data + bytes_to_end, buffer->data, size - bytes_to_end);
		buffer->index0 = (size - bytes_to_end) % buffer->capacity;
	} else {
		CUTE_MEMCPY(data, buffer->data + buffer->index0, size);
		buffer->index0 = (buffer->index0 + size) % buffer->capacity;
	}

	cf_atomic_add(&buffer->size_left, size);

	return 0;
}

int cf_circular_buffer_grow(cf_circular_buffer_t* buffer, int new_size_in_bytes)
{
	uint8_t* old_data = buffer->data;
	uint8_t* new_data = (uint8_t*)CUTE_ALLOC(new_size_in_bytes, buffer->user_allocator_context);
	if (!new_data) return -1;

	int index0 = buffer->index0;
	int index1 = buffer->index1;

	if (index0 < index1) {
		CUTE_MEMCPY(new_data + index0, old_data + index0, index1 - index0);
	} else {
		CUTE_MEMCPY(new_data, old_data, index1);
		int offset_from_end = buffer->capacity - index0;
		CUTE_MEMCPY(new_data + new_size_in_bytes - offset_from_end, old_data + index0, offset_from_end);
	}

	CUTE_FREE(old_data, buffer->user_allocator_context);
	buffer->data = new_data;
	cf_atomic_add(&buffer->size_left, new_size_in_bytes - buffer->capacity);
	buffer->capacity = new_size_in_bytes;

	return 0;
}

