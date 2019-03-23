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

#include <cute_handle_table.h>
#include <cute_alloc.h>
#include <cute_error.h>
#include <cute_c_runtime.h>

#include <internal/cute_defines_internal.h>

namespace cute
{

struct handle_internal_t
{
	union
	{
		struct
		{
			uint64_t user_index : 16;
			uint64_t table_index  : 16;
			uint64_t generation   : 32;
		} data;
		uint64_t val;
	} u;
};

static CUTE_INLINE handle_internal_t s_handle(handle_t handle)
{
	handle_internal_t h;
	h.u.val = handle;
	return h;
}

static CUTE_INLINE handle_t s_handle(handle_internal_t handle)
{
	return handle.u.val;
}

int handle_table_init(handle_table_t* table, int initial_capacity, void* user_allocator_context)
{
	CUTE_PLACEMENT_NEW(table) handle_table_t;

	if (initial_capacity > UINT16_MAX) {
		error_set("`initial_capacity` of `handle_table_init` must be `UINT16_MAX` or less.");
		return -1;
	}

	table->capacity = initial_capacity;
	table->handles = (handle_t*)CUTE_ALLOC(sizeof(handle_t) * initial_capacity, user_allocator_context);
	CUTE_CHECK_POINTER(table->handles);
	table->mem_ctx = user_allocator_context;

	handle_internal_t* handles = (handle_internal_t*)table->handles;
	int last_index = initial_capacity - 1;
	for (int i = 0; i < last_index; ++i)
	{
		handle_internal_t handle;
		handle.u.data.user_index = i + 1;
		handle.u.data.table_index = 0;
		handle.u.data.generation = 0;
		handles[i] = handle;
	}

	handle_internal_t last_handle;
	last_handle.u.data.user_index = UINT16_MAX;
	last_handle.u.data.table_index = 0;
	last_handle.u.data.generation = 0;
	handles[last_index] = last_handle;
	return 0;

cute_error:
	CUTE_FREE(table->handles, user_allocator_context);
	return -1;
}

void handle_table_clean_up(handle_table_t* table)
{
	CUTE_FREE(table->handles, table->mem_ctx);
	CUTE_PLACEMENT_NEW(table) handle_table_t;
}

handle_t handle_table_alloc(handle_table_t* table, uint16_t index)
{
	int freelist_index = table->freelist;
	if (freelist_index == UINT16_MAX) {
		error_set("Handle table is full; unable to allocate another handle.");
		return CUTE_INVALID_HANDLE;
	}

	// Pop freelist.
	handle_internal_t* handles = (handle_internal_t*)table->handles;
	handle_internal_t handle = handles[freelist_index];
	table->freelist = handle.u.data.user_index;
	CUTE_ASSERT(table->size < table->capacity);
	table->size++;

	// Setup handle indices.
	handle.u.data.user_index = index;
	handle.u.data.table_index = freelist_index;

	return s_handle(handle);
}

handle_t handle_table_update_index(handle_table_t* table, handle_t handle, uint16_t index)
{
	handle_internal_t h = s_handle(handle);
	h.u.data.user_index = index;
	handle = s_handle(h);
	table->handles[h.u.data.table_index] = handle;
	table->size++;
	return handle;
}

void handle_table_free(handle_table_t* table, handle_t handle)
{
	// Push handle onto freelist.
	handle_internal_t h = s_handle(handle);
	uint16_t table_index = h.u.data.table_index;
	h.u.data.table_index = table->freelist;
	h.u.data.generation++;
	table->handles[table_index] = s_handle(h);
	table->freelist = table_index;
}

}
