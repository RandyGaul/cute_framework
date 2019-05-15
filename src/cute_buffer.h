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

#ifndef BUFFER_H
#define BUFFER_H

// TODO: Delete me (in favor of cute_array.h).

#include <cute_defines.h>
#include <cute_alloc.h>

#define CUTE_BUFFER_GROW(ctx, count, capacity, data, type, new_cap, mem_ctx) \
	do { \
		int new_capacity = new_cap; \
		void* new_data = CUTE_ALLOC(sizeof(type) * new_capacity, mem_ctx); \
		CUTE_MEMCPY(new_data, ctx->data, sizeof(type) * ctx->count); \
		CUTE_FREE(ctx->data, mem_ctx); \
		ctx->data = (type*)new_data; \
		ctx->capacity = new_capacity; \
	} while (0)

#define CUTE_CHECK_BUFFER_GROW(ctx, count, capacity, data, type, initial, mem_ctx) \
	do { \
		if (ctx->count == ctx->capacity) \
		{ \
			CUTE_BUFFER_GROW(ctx, count, capacity, data, type, ctx->capacity ? ctx->capacity * 2 : initial, mem_ctx); \
		} \
	} while (0)

#define CUTE_BUFFER_SWAP_WITH_LAST(ctx, index, count, data) \
	do { \
		ctx->data[index] = ctx->data[--ctx->count]; \
	} while (0)

namespace cute
{

struct buffer_t
{
	buffer_t() {}
	buffer_t(int stride) : stride(stride) {}

	int stride = 0;
	int count = 0;
	int capacity = 0;
	void* data = 0;
	void* user_allocator_ctx = NULL;
};

buffer_t CUTE_INLINE buffer_make(int stride) { return buffer_t(stride); }

extern CUTE_API void CUTE_CALL buffer_push(buffer_t* buf, const void* element);
extern CUTE_API void CUTE_CALL buffer_at(buffer_t* buf, int i, void* out);
extern CUTE_API void CUTE_CALL buffer_set(buffer_t* buf, int i, const void* element);
extern CUTE_API void CUTE_CALL buffer_pop(buffer_t* buf, void* out);
extern CUTE_API void CUTE_CALL buffer_grow(buffer_t* buf, int new_capacity);
extern CUTE_API void CUTE_CALL buffer_check_grow(buffer_t* buf, int initial_capacity);
extern CUTE_API void CUTE_CALL buffer_clear(buffer_t* buf);
extern CUTE_API void CUTE_CALL buffer_free(buffer_t* buf);

}

#endif // BUFFER_H
