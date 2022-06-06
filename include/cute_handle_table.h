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

#ifndef CUTE_HANDLE_TABLE_H
#define CUTE_HANDLE_TABLE_H

#include "cute_defines.h"

struct cf_handle_allocator_t;

typedef uint64_t cf_handle_t;
#define CUTE_INVALID_HANDLE (~0ULL)

CUTE_API cf_handle_allocator_t* CUTE_CALL cf_handle_allocator_make(int initial_capacity, void* user_allocator_context /*= NULL*/);
CUTE_API void CUTE_CALL cf_handle_allocator_destroy(cf_handle_allocator_t* table);

CUTE_API cf_handle_t CUTE_CALL cf_handle_allocator_alloc(cf_handle_allocator_t* table, uint32_t index, uint16_t type /*= 0*/);
CUTE_API uint32_t CUTE_CALL cf_handle_allocator_get_index(cf_handle_allocator_t* table, cf_handle_t handle);
CUTE_API uint16_t CUTE_CALL cf_handle_allocator_get_type(cf_handle_allocator_t* table, cf_handle_t handle);
CUTE_API void CUTE_CALL cf_handle_allocator_update_index(cf_handle_allocator_t* table, cf_handle_t handle, uint32_t index);
CUTE_API void CUTE_CALL cf_handle_allocator_free(cf_handle_allocator_t* table, cf_handle_t handle);
CUTE_API int CUTE_CALL cf_handle_allocator_is_handle_valid(cf_handle_allocator_t* table, cf_handle_t handle);

// -------------------------------------------------------------------------------------------------

struct cf_handle_table_t
{
	#ifdef CUTE_CPP

	CUTE_INLINE cf_handle_table_t(int initial_capacity = 0, void* user_allocator_context = NULL)
		: m_alloc(cf_handle_allocator_make(initial_capacity, user_allocator_context))
	{}

	CUTE_INLINE ~cf_handle_table_t()
	{
		cf_handle_allocator_destroy(m_alloc);
		m_alloc = NULL;
	}

	CUTE_INLINE cf_handle_t alloc_handle(uint32_t index, uint16_t type = 0)
	{
		return cf_handle_allocator_alloc(m_alloc, index, type);
	}

	CUTE_INLINE cf_handle_t alloc_handle()
	{
		return cf_handle_allocator_alloc(m_alloc, ~0, 0);
	}

	CUTE_INLINE uint32_t get_index(cf_handle_t handle)
	{
		return cf_handle_allocator_get_index(m_alloc, handle);
	}

	CUTE_INLINE uint16_t get_type(cf_handle_t handle)
	{
		return cf_handle_allocator_get_type(m_alloc, handle);
	}

	CUTE_INLINE void update_index(cf_handle_t handle, uint32_t index)
	{
		cf_handle_allocator_update_index(m_alloc, handle, index);
	}

	CUTE_INLINE void free_handle(cf_handle_t handle)
	{
		cf_handle_allocator_free(m_alloc, handle);
	}

	CUTE_INLINE bool is_valid(cf_handle_t handle)
	{
		return !!cf_handle_allocator_is_handle_valid(m_alloc, handle);
	}

	#endif // CUTE_CPP


	cf_handle_allocator_t* m_alloc;
};

#ifdef CUTE_CPP

namespace cute
{
using handle_table_t = cf_handle_table_t;
using handle_allocator_t = cf_handle_allocator_t;

typedef uint64_t handle_t;

CUTE_INLINE handle_allocator_t* handle_allocator_make(int initial_capacity, void* user_allocator_context = NULL) { return cf_handle_allocator_make(initial_capacity, user_allocator_context); }
CUTE_INLINE void  handle_allocator_destroy(handle_allocator_t* table) { handle_allocator_destroy(table); }

CUTE_INLINE handle_t  handle_allocator_alloc(handle_allocator_t* table, uint32_t index, uint16_t type = 0) { return cf_handle_allocator_alloc(table, index, type = 0); }
CUTE_INLINE uint32_t  handle_allocator_get_index(handle_allocator_t* table, handle_t handle) { return cf_handle_allocator_get_index(table, handle); }
CUTE_INLINE uint16_t  handle_allocator_get_type(handle_allocator_t* table, handle_t handle) { return cf_handle_allocator_get_type(table, handle); }
CUTE_INLINE void  handle_allocator_update_index(handle_allocator_t* table, handle_t handle, uint32_t index) { cf_handle_allocator_update_index(table, handle, index); }
CUTE_INLINE void  handle_allocator_free(handle_allocator_t* table, handle_t handle) { cf_handle_allocator_free(table, handle); }
CUTE_INLINE int  handle_allocator_is_handle_valid(handle_allocator_t* table, handle_t handle) { return cf_handle_allocator_is_handle_valid(table, handle); }
}

#endif // CUTE_CPP

#endif // CUTE_HANDLE_TABLE_H
