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

#include <cute_buffer.h>
#include <cute_alloc.h>
#include <cute_c_runtime.h>

namespace cute
{

void buffer_push(buffer_t* buf, const void* element)
{
	uint8_t* data = (uint8_t*)buf->data;
	CUTE_ASSERT(data);
	CUTE_MEMCPY(data + (buf->stride * buf->count++), element, buf->stride);
}

void buffer_at(buffer_t* buf, int i, void* out)
{
	uint8_t* data = (uint8_t*)buf->data;
	CUTE_MEMCPY(out, data + i * buf->stride, buf->stride);
}

void buffer_set(buffer_t* buf, int i, const void* element)
{
	uint8_t* data = (uint8_t*)buf->data;
	CUTE_MEMCPY(data + i * buf->stride, element, buf->stride);
}

void buffer_pop(buffer_t* buf, void* out)
{
	buffer_at(buf, --buf->count, out);
}

void buffer_grow(buffer_t* buf, int new_capacity)
{
	void* new_data = CUTE_ALLOC(buf->stride * new_capacity, buf->user_allocator_ctx);
	CUTE_MEMCPY(new_data, buf->data, buf->stride * buf->count);
	CUTE_FREE(buf->data, buf->user_allocator_ctx);
	buf->data = new_data;
	buf->capacity = new_capacity;
}

void buffer_check_grow(buffer_t* buf, int initial_capacity)
{
	if (buf->count == buf->capacity) {
		int new_capacity = buf->capacity ? buf->capacity * 2 : initial_capacity;
		buffer_grow(buf, new_capacity);
	}
}

void buffer_clear(buffer_t* buf)
{
	buf->count = 0;
}

void buffer_free(buffer_t* buf)
{
	CUTE_FREE(buf->data, buf->user_allocator_ctx);
	CUTE_MEMSET(buf, 0, sizeof(buffer_t));
}

}
