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

#include <cute_cicular_buffer.cpp>
#include <cute_alloc.cpp>
#include <cute_error.cpp>

namespace cute
{
	
circular_buffer_t circular_buffer_make(int initial_size_in_bytes, void* user_allocator_context)
{
	circular_buffer_t buffer;
	buffer.capacity = initial_size_in_bytes;
	buffer.data = CUTE_ALLOC(initial_size_in_bytes, user_allocator_context);
	buffer.user_allocator_context = user_allocator_context;
	return buffer;
}

void circular_buffer_free(circular_buffer_t* buffer)
{
	CUTE_FREE(buffer->data, buffer->user_allocator_context);
	CUTE_MEMSET(buffer, 0, sizeof(*buffer));
}

int circular_buffer_push(circular_buffer_t* buffer, const void* data, int size)
{
	if (!buffer->data || !buffer->capacity) {
		error_set("Attempted to push to a circular buffer with no capacit and/or NULL data pointer (make sure to call `circular_buffer_make` first).");
		return -1;
	}

	if (!size) {
		return 0;
	}

	if (buffer->size < size) {
		// TODO: Grow.
	}

	if (buffer->size) {
		// If the buffer was perfectly filled, without triggering a grow operation, then
		// index0 == index1. Assert here, after growing, and make sure it is impossible
		// for the indices to line up perfectly. It is safe to assume a grow operation
		// would always push indices apart (except for when size is zero, which is handled
		// in an above if-check).
		CUTE_ASSERT(buffer->index0 != buffer->index1);
	}

	if (buffer->index0 < buffer->index1) {
		int bytes_to_end = buffer->capacity - buffer->index1;
		if (size <= bytes_to_end) {
			// 2 memcpy
		} else {
			// 1 memcpy
		}
	} else {
		// 1 memcpy
	}
}

int circular_buffer_pull(circular_buffer_t* buffer, void* data, int size)
{
	if (!buffer->data || !buffer->capacity) {
		error_set("Attempted to push to a circular buffer with no capacit and/or NULL data pointer (make sure to call `circular_buffer_make` first).");
		return -1;
	}
}

void circular_buffer_clear(circular_buffer_t* buffer)
{
	buffer->index0 = 0;
	buffer->index1 = 0;
	buffer->count = 0;
}

}

#endif // CUTE_CIRCULAR_BUFFER_H
