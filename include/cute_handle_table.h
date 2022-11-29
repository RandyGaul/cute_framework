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

//--------------------------------------------------------------------------------------------------
// C API

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

typedef struct CF_HandleTable CF_HandleTable;

typedef uint64_t CF_Handle;
#define CUTE_INVALID_HANDLE (~0ULL)

CUTE_API CF_HandleTable* CUTE_CALL cf_make_handle_allocator(int initial_capacity);
CUTE_API void CUTE_CALL cf_destroy_handle_allocator(CF_HandleTable* table);

CUTE_API CF_Handle CUTE_CALL cf_handle_allocator_alloc(CF_HandleTable* table, uint32_t index, uint16_t type /*= 0*/);
CUTE_API uint32_t CUTE_CALL cf_handle_allocator_get_index(CF_HandleTable* table, CF_Handle handle);
CUTE_API uint16_t CUTE_CALL cf_handle_allocator_get_type(CF_HandleTable* table, CF_Handle handle);
CUTE_API bool CUTE_CALL cf_handle_allocator_is_active(CF_HandleTable* table, CF_Handle handle);
CUTE_API void CUTE_CALL cf_handle_allocator_activate(CF_HandleTable* table, CF_Handle handle);
CUTE_API void CUTE_CALL cf_handle_allocator_deactivate(CF_HandleTable* table, CF_Handle handle);
CUTE_API void CUTE_CALL cf_handle_allocator_update_index(CF_HandleTable* table, CF_Handle handle, uint32_t index);
CUTE_API void CUTE_CALL cf_handle_allocator_free(CF_HandleTable* table, CF_Handle handle);
CUTE_API int CUTE_CALL cf_handle_allocator_is_handle_valid(CF_HandleTable* table, CF_Handle handle);

#ifdef __cplusplus
}
#endif // __cplusplus

//--------------------------------------------------------------------------------------------------
// C++ API

#ifdef CUTE_CPP

namespace cute
{

using Handle = uint64_t;

struct HandleTable
{
	CUTE_INLINE HandleTable(int initial_capacity = 0)
		: m_alloc(cf_make_handle_allocator(initial_capacity))
	{
	}

	CUTE_INLINE ~HandleTable()
	{
		cf_destroy_handle_allocator(m_alloc);
		m_alloc = NULL;
	}

	CUTE_INLINE CF_Handle alloc_handle(uint32_t index, uint16_t type = 0)
	{
		return cf_handle_allocator_alloc(m_alloc, index, type);
	}

	CUTE_INLINE CF_Handle alloc_handle()
	{
		return cf_handle_allocator_alloc(m_alloc, ~0, 0);
	}

	CUTE_INLINE uint32_t get_index(CF_Handle handle)
	{
		return cf_handle_allocator_get_index(m_alloc, handle);
	}

	CUTE_INLINE uint16_t get_type(CF_Handle handle)
	{
		return cf_handle_allocator_get_type(m_alloc, handle);
	}

	CUTE_INLINE void update_index(CF_Handle handle, uint32_t index)
	{
		cf_handle_allocator_update_index(m_alloc, handle, index);
	}

	CUTE_INLINE void free_handle(CF_Handle handle)
	{
		cf_handle_allocator_free(m_alloc, handle);
	}

	CUTE_INLINE bool is_valid(CF_Handle handle)
	{
		return !!cf_handle_allocator_is_handle_valid(m_alloc, handle);
	}

	CUTE_INLINE bool is_active(CF_Handle handle)
	{
		return cf_handle_allocator_is_active(m_alloc, handle);
	}

	CUTE_INLINE void activate(CF_Handle handle)
	{
		cf_handle_allocator_activate(m_alloc, handle);
	}

	CUTE_INLINE void deactivate(CF_Handle handle)
	{
		cf_handle_allocator_deactivate(m_alloc, handle);
	}
	
	CF_HandleTable* m_alloc;
};

}

#endif // CUTE_CPP

#endif // CUTE_HANDLE_TABLE_H
