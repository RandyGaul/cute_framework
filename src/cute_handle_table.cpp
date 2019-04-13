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

union handle_entry_t
{
	struct
	{
		uint64_t user_index : 32;
		uint64_t generation : 32;
	} data;
	uint64_t val;
};

static CUTE_INLINE handle_entry_t* s_handles(handle_table_t* table)
{
	return (handle_entry_t*)table->handles;
}

int handle_table_init(handle_table_t* table, int capacity, void* user_allocator_context)
{
	CUTE_PLACEMENT_NEW(table) handle_table_t;

	if (capacity > UINT32_MAX) {
		error_set("`capacity` of `handle_table_init` must be `UINT32_MAX` or less.");
		return -1;
	}

	table->capacity = capacity;
	table->handles = CUTE_ALLOC(sizeof(handle_t) * capacity, user_allocator_context);
	CUTE_CHECK_POINTER(table->handles);
	table->mem_ctx = user_allocator_context;

	handle_entry_t* handles = s_handles(table);
	int last_index = capacity - 1;
	for (int i = 0; i < last_index; ++i)
	{
		handle_entry_t handle;
		handle.data.user_index = i + 1;
		handle.data.generation = 0;
		handles[i] = handle;
	}

	handle_entry_t last_handle;
	last_handle.data.user_index = UINT32_MAX;
	last_handle.data.generation = 0;
	handles[last_index] = last_handle;
	return 0;

cute_error:
	CUTE_FREE(table->handles, user_allocator_context);
	return -1;
}

void handle_table_cleanup(handle_table_t* table)
{
	CUTE_FREE(table->handles, table->mem_ctx);
	CUTE_PLACEMENT_NEW(table) handle_table_t;
}

handle_t handle_table_alloc(handle_table_t* table, uint32_t index)
{
	int freelist_index = table->freelist;
	if (freelist_index == UINT32_MAX) {
		error_set("Handle table is full; unable to allocate another handle.");
		return CUTE_INVALID_HANDLE;
	}

	// Pop freelist.
	handle_entry_t* handles = s_handles(table);
	table->freelist = handles[freelist_index].data.user_index;
	CUTE_ASSERT(table->size < table->capacity);
	table->size++;

	// Setup handle indices.
	handles[freelist_index].data.user_index = index;
	handle_t handle = (((uint64_t)freelist_index) << 32) | handles[freelist_index].data.generation;
	return handle;
}

static CUTE_INLINE uint32_t s_table_index(handle_t handle)
{
	return (uint32_t)((handle & 0xFFFFFFFF00000000ULL) >> 32);
}

uint32_t handle_table_get_index(handle_table_t* table, handle_t handle)
{
	handle_entry_t* handles = s_handles(table);
	uint32_t table_index = s_table_index(handle);
	return handles[table_index].data.user_index;
}

void handle_table_update_index(handle_table_t* table, handle_t handle, uint32_t index)
{
	handle_entry_t* handles = s_handles(table);
	uint32_t table_index = s_table_index(handle);
	handles[table_index].data.user_index = index;
}

void handle_table_free(handle_table_t* table, handle_t handle)
{
	// Push handle onto freelist.
	handle_entry_t* handles = s_handles(table);
	uint32_t table_index = s_table_index(handle);
	handles[table_index].data.user_index = table->freelist;
	handles[table_index].data.generation++;
	table->freelist = table_index;
	table->size--;
}

}
