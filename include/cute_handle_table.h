/*
	Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

#ifndef CF_HANDLE_TABLE_H
#define CF_HANDLE_TABLE_H

#include "cute_defines.h"

//--------------------------------------------------------------------------------------------------
// C API

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

/**
 * @struct   CF_HandleTable
 * @category utility
 * @brief    An opaque pointer representing a handle table.
 * @remarks  The handle table stores a lookup table of unique `CF_Handle`s. Handles use a lookup table mechanism
 *           to map one integer to another. This is useful to implement object pools or other similar kinds of
 *           data structures. This is a rather low-level utility, mainly used to implement CF's ECS. If you want
 *           to see it in action you may peruse the CF source code. Otherwise, it's assumed you know what you're
 *           doing if you're reading this.
 * @related  CF_HandleTable CF_Handle cf_make_handle_allocator cf_handle_allocator_alloc cf_handle_allocator_get_index
 */
typedef struct CF_HandleTable CF_HandleTable;
// @end

/**
 * @struct   CF_Handle
 * @category utility
 * @brief    A handle stored within a `CF_HandleTable`.
 * @remarks  Each handle is unique from another and maps to a 32-bit index value, along with a 16-bit type value.
 * @related  CF_INVALID_HANDLE CF_HandleTable CF_Handle cf_make_handle_allocator cf_handle_allocator_alloc cf_handle_allocator_get_index cf_handle_allocator_get_type
 */
typedef uint64_t CF_Handle;
// @end

/**
 * @function CF_INVALID_HANDLE
 * @category utility
 * @brief    Defines an invalid handle.
 * @remarks  This can be used to initialize handle variables.
 * @related  CF_Handle CF_HandleTable
 */
#define CF_INVALID_HANDLE (~0ULL)

/**
 * @function cf_make_handle_allocator
 * @category utility
 * @brief    Returns a new `CF_HandleTable`.
 * @param    initial_capacity  The initial number of handles to store within. Grows internally as necessary.
 * @remarks  The handle table is used to generate `CF_Handle`s with `cf_handle_allocator_alloc`. Each handle is unique and maps
 *           to a 32-bit index along with a 16-bit type value. This is useful for adding a layer of indirection to systems that
 *           want to track object lifetimes, grant access to objects, while also decoupling object memory storage from their
 *           lifetime cycles. Free it up with `cf_destroy_handle_allocator` when done.
 * @related  CF_Handle CF_HandleTable cf_destroy_handle_allocator cf_handle_allocator_alloc
 */
CF_API CF_HandleTable* CF_CALL cf_make_handle_allocator(int initial_capacity);

/**
 * @function cf_destroy_handle_allocator
 * @category utility
 * @brief    Destroys a handle table created by `cf_make_handle_allocator`.
 * @param    table        The table.
 * @related  CF_Handle CF_HandleTable cf_destroy_handle_allocator cf_handle_allocator_alloc
 */
CF_API void CF_CALL cf_destroy_handle_allocator(CF_HandleTable* table);

/**
 * @function cf_handle_allocator_alloc
 * @category utility
 * @brief    Allocates and returns a unique `CF_Handle` that maps to `index` and `type`.
 * @param    table        The table.
 * @param    index        A 32-bit value the handle maps to. Can be fetched with `cf_handle_allocator_get_index`.
 * @param    type         A 16-bit value the handle maps to. Can be fetched with `cf_handle_allocator_get_type`.
 * @related  CF_Handle CF_HandleTable cf_handle_allocator_get_index cf_handle_allocator_get_index cf_handle_allocator_get_type cf_handle_allocator_active cf_handle_allocator_activate cf_handle_allocator_deactivate cf_handle_allocator_update_index cf_handle_allocator_free cf_handle_allocator_handle_valid
 */
CF_API CF_Handle CF_CALL cf_handle_allocator_alloc(CF_HandleTable* table, uint32_t index, uint16_t type);

/**
 * @function cf_handle_allocator_get_index
 * @category utility
 * @brief    Returns the 32-bit index value a `CF_Handle` maps to.
 * @param    table        The table.
 * @param    handle       A handle created by `cf_handle_allocator_alloc`.
 * @remarks  This function will return a valid value if the handle is valid. Check for validity with `cf_handle_allocator_handle_valid`. The
 *           function `cf_handle_allocator_active` does not affect handle validity.
 * @related  CF_Handle CF_HandleTable cf_handle_allocator_get_index cf_handle_allocator_get_index cf_handle_allocator_get_type cf_handle_allocator_active cf_handle_allocator_activate cf_handle_allocator_deactivate cf_handle_allocator_update_index cf_handle_allocator_free cf_handle_allocator_handle_valid
 */
CF_API uint32_t CF_CALL cf_handle_allocator_get_index(CF_HandleTable* table, CF_Handle handle);

/**
 * @function cf_handle_allocator_get_type
 * @category utility
 * @brief    Returns the 16-bit type value a `CF_Handle` maps to.
 * @param    table        The table.
 * @param    handle       A handle created by `cf_handle_allocator_alloc`.
 * @remarks  This function will return a valid value if the handle is valid. Check for validity with `cf_handle_allocator_handle_valid`. The
 *           function `cf_handle_allocator_active` does not affect handle validity.
 * @related  CF_Handle CF_HandleTable cf_handle_allocator_get_index cf_handle_allocator_get_index cf_handle_allocator_get_type cf_handle_allocator_active cf_handle_allocator_activate cf_handle_allocator_deactivate cf_handle_allocator_update_index cf_handle_allocator_free cf_handle_allocator_handle_valid
 */
CF_API uint16_t CF_CALL cf_handle_allocator_get_type(CF_HandleTable* table, CF_Handle handle);

/**
 * @function cf_handle_allocator_active
 * @category utility
 * @brief    Returns whether or not the handle is currently active.
 * @param    table        The table.
 * @param    handle       A handle created by `cf_handle_allocator_alloc`.
 * @remarks  Handles are initially created in an active state. You can toggle the active state with `cf_handle_allocator_activate` or `cf_handle_allocator_deactivate`.
 *           The active state _does not affect_ `cf_handle_allocator_handle_valid`.
 * @related  CF_Handle CF_HandleTable cf_handle_allocator_get_index cf_handle_allocator_get_index cf_handle_allocator_get_type cf_handle_allocator_active cf_handle_allocator_activate cf_handle_allocator_deactivate cf_handle_allocator_update_index cf_handle_allocator_free cf_handle_allocator_handle_valid
 */
CF_API bool CF_CALL cf_handle_allocator_active(CF_HandleTable* table, CF_Handle handle);

/**
 * @function cf_handle_allocator_activate
 * @category utility
 * @brief    Sets the active state of a handle to true.
 * @param    table        The table.
 * @param    handle       A handle created by `cf_handle_allocator_alloc`.
 * @remarks  Handles are initially created in an active state. You can toggle the active state with `cf_handle_allocator_activate` or `cf_handle_allocator_deactivate`.
 *           The active state _does not affect_ `cf_handle_allocator_handle_valid`.
 * @related  CF_Handle CF_HandleTable cf_handle_allocator_get_index cf_handle_allocator_get_index cf_handle_allocator_get_type cf_handle_allocator_active cf_handle_allocator_activate cf_handle_allocator_deactivate cf_handle_allocator_update_index cf_handle_allocator_free cf_handle_allocator_handle_valid
 */
CF_API void CF_CALL cf_handle_allocator_activate(CF_HandleTable* table, CF_Handle handle);

/**
 * @function cf_handle_allocator_deactivate
 * @category utility
 * @brief    Sets the active state of a handle to false.
 * @param    table        The table.
 * @param    handle       A handle created by `cf_handle_allocator_alloc`.
 * @remarks  Handles are initially created in an active state. You can toggle the active state with `cf_handle_allocator_activate` or `cf_handle_allocator_deactivate`.
 *           The active state _does not affect_ `cf_handle_allocator_handle_valid`.
 * @related  CF_Handle CF_HandleTable cf_handle_allocator_get_index cf_handle_allocator_get_index cf_handle_allocator_get_type cf_handle_allocator_active cf_handle_allocator_activate cf_handle_allocator_deactivate cf_handle_allocator_update_index cf_handle_allocator_free cf_handle_allocator_handle_valid
 */
CF_API void CF_CALL cf_handle_allocator_deactivate(CF_HandleTable* table, CF_Handle handle);

/**
 * @function cf_handle_allocator_update_index
 * @category utility
 * @brief    Sets 32-bit index value a handle maps to.
 * @param    table        The table.
 * @param    handle       A handle created by `cf_handle_allocator_alloc`.
 * @param    index        A new 32-bit index value to store.
 * @related  CF_Handle CF_HandleTable cf_handle_allocator_get_index cf_handle_allocator_get_index cf_handle_allocator_get_type cf_handle_allocator_active cf_handle_allocator_activate cf_handle_allocator_deactivate cf_handle_allocator_update_index cf_handle_allocator_free cf_handle_allocator_handle_valid
 */
CF_API void CF_CALL cf_handle_allocator_update_index(CF_HandleTable* table, CF_Handle handle, uint32_t index);

/**
 * @function cf_handle_allocator_update_type
 * @category utility
 * @brief    TODO
 */
void cf_handle_allocator_update_type(CF_HandleTable* table, CF_Handle handle, uint16_t type);

/**
 * @function cf_handle_allocator_free
 * @category utility
 * @brief    Marks a `CF_Handle` as invalid and frees up resources it used.
 * @param    table        The table.
 * @param    handle       A handle created by `cf_handle_allocator_alloc`.
 * @related  CF_Handle CF_HandleTable cf_handle_allocator_get_index cf_handle_allocator_get_index cf_handle_allocator_get_type cf_handle_allocator_active cf_handle_allocator_activate cf_handle_allocator_deactivate cf_handle_allocator_update_index cf_handle_allocator_free cf_handle_allocator_handle_valid
 */
CF_API void CF_CALL cf_handle_allocator_free(CF_HandleTable* table, CF_Handle handle);

/**
 * @function cf_handle_allocator_handle_valid
 * @category utility
 * @brief    Returns true if a `CF_Handle` is valid.
 * @param    table        The table.
 * @param    handle       A handle created by `cf_handle_allocator_alloc`.
 * @remarks  Handles are created in a valid state. They only become invalid when `cf_handle_allocator_free` is called.
 * @related  CF_Handle CF_HandleTable cf_handle_allocator_get_index cf_handle_allocator_get_index cf_handle_allocator_get_type cf_handle_allocator_active cf_handle_allocator_activate cf_handle_allocator_deactivate cf_handle_allocator_update_index cf_handle_allocator_free cf_handle_allocator_handle_valid
 */
CF_API int CF_CALL cf_handle_allocator_handle_valid(CF_HandleTable* table, CF_Handle handle);

#ifdef __cplusplus
}
#endif // __cplusplus

//--------------------------------------------------------------------------------------------------
// C++ API

#ifdef CF_CPP

namespace Cute
{

using Handle = uint64_t;

struct HandleTable
{
	CF_INLINE HandleTable(int initial_capacity = 0)
		: m_alloc(cf_make_handle_allocator(initial_capacity))
	{
	}

	CF_INLINE ~HandleTable()
	{
		cf_destroy_handle_allocator(m_alloc);
		m_alloc = NULL;
	}

	CF_INLINE CF_Handle alloc_handle(uint32_t index, uint16_t type = 0)
	{
		return cf_handle_allocator_alloc(m_alloc, index, type);
	}

	CF_INLINE CF_Handle alloc_handle()
	{
		return cf_handle_allocator_alloc(m_alloc, ~0, 0);
	}

	CF_INLINE uint32_t get_index(CF_Handle handle)
	{
		return cf_handle_allocator_get_index(m_alloc, handle);
	}

	CF_INLINE uint16_t get_type(CF_Handle handle)
	{
		return cf_handle_allocator_get_type(m_alloc, handle);
	}

	CF_INLINE void update_index(CF_Handle handle, uint32_t index)
	{
		cf_handle_allocator_update_index(m_alloc, handle, index);
	}

	CF_INLINE void update_type(CF_Handle handle, uint16_t type)
	{
		cf_handle_allocator_update_type(m_alloc, handle, type);
	}

	CF_INLINE void free_handle(CF_Handle handle)
	{
		cf_handle_allocator_free(m_alloc, handle);
	}

	CF_INLINE bool valid(CF_Handle handle)
	{
		return !!cf_handle_allocator_handle_valid(m_alloc, handle);
	}

	CF_INLINE bool active(CF_Handle handle)
	{
		return cf_handle_allocator_active(m_alloc, handle);
	}

	CF_INLINE void activate(CF_Handle handle)
	{
		cf_handle_allocator_activate(m_alloc, handle);
	}

	CF_INLINE void deactivate(CF_Handle handle)
	{
		cf_handle_allocator_deactivate(m_alloc, handle);
	}
	
	CF_HandleTable* m_alloc;
};

}

#endif // CF_CPP

#endif // CF_HANDLE_TABLE_H
