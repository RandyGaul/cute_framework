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
#include <cute_array.h>

union cf_handle_entry_t
{
	struct
	{
		uint64_t user_index : 32;
		uint64_t user_type : 16;
		uint64_t generation : 16;
	} data;
	uint64_t val = 0;
};

struct cf_handle_allocator_t
{
	cf_handle_allocator_t(void* user_allocator_context)
		: m_handles(user_allocator_context)
		, m_mem_ctx(user_allocator_context)
	{
	}

	uint32_t m_freelist = ~0;
	cf_array<cf_handle_entry_t> m_handles;
	void* m_mem_ctx = NULL;
};

static void cf_s_add_elements_to_freelist(cf_handle_allocator_t* table, int first_index, int last_index)
{
	cf_handle_entry_t* m_handles = table->m_handles.data();
	for (int i = first_index; i < last_index; ++i)
	{
		cf_handle_entry_t handle;
		handle.data.user_index = i + 1;
		handle.data.generation = 0;
		m_handles[i] = handle;
	}

	cf_handle_entry_t last_handle;
	last_handle.data.user_index = UINT32_MAX;
	last_handle.data.generation = 0;
	m_handles[last_index] = last_handle;

	table->m_freelist = first_index;
}

cf_handle_allocator_t* cf_handle_allocator_make(int initial_capacity, void* user_allocator_context)
{
	cf_handle_allocator_t* table = (cf_handle_allocator_t*)CUTE_ALLOC(sizeof(cf_handle_allocator_t), user_allocator_context);
	CUTE_PLACEMENT_NEW(table) cf_handle_allocator_t(user_allocator_context);

	if (initial_capacity) {
		table->m_handles.ensure_capacity(initial_capacity);
		int last_index = table->m_handles.capacity() - 1;
		cf_s_add_elements_to_freelist(table, 0, last_index);
	}

	return table;
}

void cf_handle_allocator_destroy(cf_handle_allocator_t* table)
{
	if (!table) return;
	void* mem_ctx = table->m_mem_ctx;
	table->~cf_handle_allocator_t();
	CUTE_FREE(table, mem_ctx);
}

cf_handle_t cf_handle_allocator_alloc(cf_handle_allocator_t* table, uint32_t index, uint16_t type)
{
	int freelist_index = table->m_freelist;
	if (freelist_index == UINT32_MAX) {
		int first_index = table->m_handles.capacity();
		if (!first_index) first_index = 1;
		table->m_handles.ensure_capacity(first_index * 2);
		int last_index = table->m_handles.capacity() - 1;
		cf_s_add_elements_to_freelist(table, first_index, last_index);
		freelist_index = table->m_freelist;
	}

	// Pop m_freelist.
	cf_handle_entry_t* m_handles = table->m_handles.data();
	table->m_freelist = m_handles[freelist_index].data.user_index;

	// Setup handle indices.
	m_handles[freelist_index].data.user_index = index;
	m_handles[freelist_index].data.user_type = type;
	cf_handle_t handle = (((uint64_t)freelist_index) << 32) | (((uint64_t)type) << 16) | m_handles[freelist_index].data.generation;
	return handle;
}

static CUTE_INLINE uint32_t cf_s_table_index(cf_handle_t handle)
{
	return (uint32_t)((handle & 0xFFFFFFFF00000000ULL) >> 32);
}

uint32_t cf_handle_allocator_get_index(cf_handle_allocator_t* table, cf_handle_t handle)
{
	cf_handle_entry_t* m_handles = table->m_handles.data();
	uint32_t table_index = cf_s_table_index(handle);
	uint64_t generation = handle & 0xFFFF;
	CUTE_ASSERT(m_handles[table_index].data.generation == generation);
	return m_handles[table_index].data.user_index;
}

uint16_t cf_handle_allocator_get_type(cf_handle_allocator_t* table, cf_handle_t handle)
{
	cf_handle_entry_t* m_handles = table->m_handles.data();
	uint32_t table_index = cf_s_table_index(handle);
	uint64_t generation = handle & 0xFFFF;
	CUTE_ASSERT(m_handles[table_index].data.generation == generation);
	return m_handles[table_index].data.user_type;
}

void cf_handle_allocator_update_index(cf_handle_allocator_t* table, cf_handle_t handle, uint32_t index)
{
	cf_handle_entry_t* m_handles = table->m_handles.data();
	uint32_t table_index = cf_s_table_index(handle);
	uint64_t generation = handle & 0xFFFF;
	CUTE_ASSERT(m_handles[table_index].data.generation == generation);
	m_handles[table_index].data.user_index = index;
}

void cf_handle_allocator_free(cf_handle_allocator_t* table, cf_handle_t handle)
{
	// Push handle onto m_freelist.
	cf_handle_entry_t* m_handles = table->m_handles.data();
	uint32_t table_index = cf_s_table_index(handle);
	m_handles[table_index].data.user_index = table->m_freelist;
	m_handles[table_index].data.generation++;
	table->m_freelist = table_index;
}

int cf_handle_allocator_is_handle_valid(cf_handle_allocator_t* table, cf_handle_t handle)
{
	cf_handle_entry_t* m_handles = table->m_handles.data();
	uint32_t table_index = cf_s_table_index(handle);
	uint64_t generation = handle & 0xFFFF;
	uint64_t type = (handle & 0x00000000FFFF0000ULL) >> 16;
	bool match_generation = m_handles[table_index].data.generation == generation;
	bool match_type = m_handles[table_index].data.user_type == type;
	return match_generation && match_type;
}
