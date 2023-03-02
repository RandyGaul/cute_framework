/*
	Cute Framework
	Copyright (C) 2023 Randy Gaul https://randygaul.github.io/

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

#ifndef CUTE_MAP_H
#define CUTE_MAP_H

#include "cute_defines.h"
#include "cute_c_runtime.h"
#include "cute_alloc.h"

//--------------------------------------------------------------------------------------------------
// C API

#ifndef CUTE_NO_SHORTHAND_API
/**
 * @function map
 * @category map
 * @brief    An empty macro used in the C API to markup hastables.
 * @example > Showcase of base map features.
 *     map CF_V2* pts = NULL;
 *     mset(pts, 0, cf_v2(3, 5)); // Contructs a new table on-the-spot.
 *                             // The table is *hidden* behind `pts`.
 *     mset(pts, 10, cf_v2(-1, -1);
 *     mset(pts, -2, cf_v2(0, 0));
 *     
 *     // Use `mget` to fetch values.
 *     CF_V2 a = mget(pts, 0);
 *     CF_V2 b = mget(pts, 10);
 *     CF_V2 c = mget(pts, -2);
 *     
 *     // Loop over {key, item} pairs like so:
 *     const uint64_t* keys = mkeys(pts);
 *     for (int i = 0; i < mcount(pts); ++i) {
 *         uint64_t key = keys[i];
 *         CF_V2 v = pts[i];
 *         // ...
 *     }
 *     
 *     mfree(pts);
 * @remarks  This is an optional and _completely_ empty macro. It's only purpose is to provide a bit of visual indication a type is a
 *           map. One downside of the C-macro API is the opaque nature of the pointer type. Since the macros use polymorphism
 *           on typed pointers, there's no actual map struct type. It can get really annoying to sometimes forget if a pointer is an
 *           array, a map, or just a pointer. This macro can be used to markup the type to make it much more clear for function
 *           parameters or struct member definitions. It's saying "Hey, I'm a map!" to mitigate this downside.
 * @related  map mset madd mget mfind mget_ptr mfind_ptr mhas mdel mclear mkeys mitems mswap msize mcount mfree
 */
#define map

/**
 * @function mset
 * @category map
 * @brief    Add's a {key, item} pair.
 * @param    h        The map. Can be `NULL`. Needs to be a pointer to the type of items in the table.
 * @param    k        The key for lookups. Each {key, item} pair must be unique. Keys are always typecasted to `uint64_t` e.g. you can use pointers as keys.
 * @param    ...      An item to place in the table.
 * @example > Set and get a few elements from a map.
 *     map int* table = NULL;
 *     mset(table, 0, 5);
 *     mset(table, 1, 12);
 *     CUTE_ASSERT(mget(table, 0) == 5);
 *     CUTE_ASSERT(mget(table, 1) == 12);
 *     mfree(table);
 * @return   Returns a pointer to the item set into the table.
 * @remarks  If the item does not exist in the table it is added. The pointer returned is not stable. Internally the table can be resized,
 *           invalidating _all_ pointers to any elements within the table. Therefor, no items may store pointers to themselves or other items.
 *           Indices however, are totally fine.
 * @related  map mset madd mget mfind mget_ptr mfind_ptr mhas mdel mclear mkeys mitems mswap msize mcount mfree
 */
#define mset(h, k, ...) cf_map_set(h, k, (__VA_ARGS__))

/**
 * @function madd
 * @category map
 * @brief    Add's a {key, item} pair.
 * @param    h        The map. Can be `NULL`. Needs to be a pointer to the type of items in the table.
 * @param    k        The key for lookups. Each {key, item} pair must be unique. Keys are always typecasted to `uint64_t` e.g. you can use pointers as keys.
 * @param    ...      An item to place in the table.
 * @example > Set and get a few elements from a map.
 *     map int* table = NULL;
 *     madd(table, 0, 5);
 *     madd(table, 1, 12);
 *     CUTE_ASSERT(mget(table, 0) == 5);
 *     CUTE_ASSERT(mget(table, 1) == 12);
 *     mfree(table);
 * @return   Returns a pointer to the item set into the table.
 * @remarks  This function works the same as `mset`. If the item already exists in the table, it's simply updated to a new value.
 *           The pointer returned is not stable. Internally the table can be resized, invalidating _all_ pointers to any elements
 *           within the table. Therefor, no items may store pointers to themselves or other items. Indices however, are totally fine.
 * @related  map mset madd mget mfind mget_ptr mfind_ptr mhas mdel mclear mkeys mitems mswap msize mcount mfree
 */
#define madd(h, k, ...) cf_map_add(h, k, (__VA_ARGS__))

/**
 * @function mget
 * @category map
 * @brief    Fetches the item that `k` maps to.
 * @param    h        The map. Can be `NULL`. Needs to be a pointer to the type of items in the table.
 * @param    k        The key for lookups. Each {key, item} pair must be unique. Keys are always typecasted to `uint64_t` e.g. you can use pointers as keys.
 * @example > Set and get a few elements from a map.
 *     map int* table = NULL;
 *     madd(table, 0, 5);
 *     madd(table, 1, 12);
 *     CUTE_ASSERT(mget(table, 0) == 5);
 *     CUTE_ASSERT(mget(table, 1) == 12);
 *     mfree(table);
 * @return   Returns a pointer to the item set into the table.
 * @remarks  Items are returned by value, not pointer. If the item doesn't exist a zero'd out item is instead returned. If you want to get a pointer
 *           (so you can see if it's `NULL` in case the item didn't exist, then use `mget_ptr`). You can also call `mhas` for a bool. This function does
 *           the same as `mfind`.
 * @related  map mset madd mget mfind mget_ptr mfind_ptr mhas mdel mclear mkeys mitems mswap msize mcount mfree
 */
#define mget(h, k) cf_map_get(h, k)

/**
 * @function mfind
 * @category map
 * @brief    Fetches the item that `k` maps to.
 * @param    h        The map. Can be `NULL`. Needs to be a pointer to the type of items in the table.
 * @param    k        The key for lookups. Each {key, item} pair must be unique. Keys are always typecasted to `uint64_t` e.g. you can use pointers as keys.
 * @example > Set and get a few elements from a map.
 *     map int* table = NULL;
 *     madd(table, 0, 5);
 *     madd(table, 1, 12);
 *     CUTE_ASSERT(mfind(table, 0) == 5);
 *     CUTE_ASSERT(mfind(table, 1) == 12);
 *     mfree(table);
 * @return   Returns a pointer to the item set into the table.
 * @remarks  Items are returned by value, not pointer. If the item doesn't exist a zero'd out item is instead returned. If you want to get a pointer
 *           (so you can see if it's `NULL` in case the item didn't exist, then use `mfind_ptr`). You can also call `mhas` for a bool. This function does
 *           the same as `mget`.
 * @related  map mset madd mget mfind mget_ptr mfind_ptr mhas mdel mclear mkeys mitems mswap msize mcount mfree
 */
#define mfind(h, k) cf_map_find(h, k)

/**
 * @function mget_ptr
 * @category map
 * @brief    Fetches a pointer to the item that `k` maps to.
 * @param    h        The map. Can be `NULL`. Needs to be a pointer to the type of items in the table.
 * @param    k        The key for lookups. Each {key, item} pair must be unique. Keys are always typecasted to `uint64_t` e.g. you can use pointers as keys.
 * @example > Set and get a few elements from a map.
 *     map CF_V2* table = NULL;
 *     madd(table, 10, cf_v2(-1, 1));
 *     CF_V2* v = mget_ptr(table, 10);
 *     CUTE_ASSERT(v);
 *     CUTE_ASSERT(v->x == -1);
 *     CUTE_ASSERT(v->y == 1);
 *     mfree(table);
 * @return   Returns a pointer to an item. Returns `NULL` if not found.
 * @remarks  If you want to fetch an item by value, you can use `mget` or `mfind`. Does the same thing as `mfind_ptr`.
 * @related  map mset madd mget mfind mget_ptr mfind_ptr mhas mdel mclear mkeys mitems mswap msize mcount mfree
 */
#define mget_ptr(h, k) cf_map_get_ptr(h, k)

/**
 * @function mfind_ptr
 * @category map
 * @brief    Fetches a pointer to the item that `k` maps to.
 * @param    h        The map. Can be `NULL`. Needs to be a pointer to the type of items in the table.
 * @param    k        The key for lookups. Each {key, item} pair must be unique. Keys are always typecasted to `uint64_t` e.g. you can use pointers as keys.
 * @example > Set and get a few elements from a map.
 *     map CF_V2* table = NULL;
 *     madd(table, 10, cf_v2(-1, 1));
 *     CF_V2* v = mfind_ptr(table, 10);
 *     CUTE_ASSERT(v);
 *     CUTE_ASSERT(v->x == -1);
 *     CUTE_ASSERT(v->y == 1);
 *     mfree(table);
 * @return   Returns a pointer to an item. Returns `NULL` if not found.
 * @remarks  If you want to fetch an item by value, you can use `mget` or `mfind`. Does the same thing as `mget_ptr`.
 * @related  map mset madd mget mfind mget_ptr mfind_ptr mhas mdel mclear mkeys mitems mswap msize mcount mfree
 */
#define mfind_ptr(h, k) cf_map_find_ptr(h, k)

/**
 * @function mhas
 * @category map
 * @brief    Check to see if an item exists in the table.
 * @param    h        The map. Can be `NULL`. Needs to be a pointer to the type of items in the table.
 * @param    k        The key for lookups. Each {key, item} pair must be unique. Keys are always typecasted to `uint64_t` e.g. you can use pointers as keys.
 * @example > Checks if an item exists in the table.
 *     map v2* table = NULL;
 *     madd(table, 10, V2(-1, 1));
 *     CUTE_ASSERT(mhas(table, 10));
 *     mfree(table);
 * @return   Returns true if the item was found, false otherwise.
 * @related  map mset madd mget mfind mget_ptr mfind_ptr mhas mdel mclear mkeys mitems mswap msize mcount mfree
 */
#define mhas(h, k) cf_map_has(h, k)

/**
 * @function mdel
 * @category map
 * @brief    Removes an item from the table.
 * @param    h        The map. Can be `NULL`. Needs to be a pointer to the type of items in the table.
 * @param    k        The key for lookups. Each {key, item} pair must be unique. Keys are always typecasted to `uint64_t` e.g. you can use pointers as keys.
 * @example > Removes an item in the table.
 *     map CF_V2* table = NULL;
 *     madd(table, 10, cf_v2(-1, 1));
 *     mdel(table, 10);
 *     mfree(table);
 * @remarks  Asserts if the item does not exist.
 * @related  map mset madd mget mfind mget_ptr mfind_ptr mhas mdel mclear mkeys mitems mswap msize mcount mfree
 */
#define mdel(h, k) cf_map_del(h, k)

/**
 * @function mclear
 * @category map
 * @brief    Clears the map.
 * @param    h        The map. Can be `NULL`. Needs to be a pointer to the type of items in the table.
 * @remarks  The count of items will now be zero. Does not free any memory. Call `mfree` when you are done.
 * @related  map mset madd mget mfind mget_ptr mfind_ptr mhas mdel mclear mkeys mitems mswap msize mcount mfree
 */
#define mclear(h) cf_map_clear(h)

/**
 * @function mkeys
 * @category map
 * @brief    Get a const pointer to the array of keys.
 * @param    h        The map. Can be `NULL`. Needs to be a pointer to the type of items in the table.
 * @example > Loop over all {key, item} pairs of a table.
 *     map CF_V2* table = my_table();
 *     const uint64_t* keys = mkeys(table);
 *     for (int i = 0; i < mcount(table); ++i) {
 *         uint64_t key = keys[i];
 *         CF_V2 item = table[i];
 *         // ...
 *     }
 * @remarks  The keys are type `uint64_t`.
 * @related  map mset madd mget mfind mget_ptr mfind_ptr mhas mdel mclear mkeys mitems mswap msize mcount mfree
 */
#define mkeys(h) cf_map_keys(h)

/**
 * @function mitems
 * @category map
 * @brief    Get a pointer to the array of items.
 * @param    h        The map. Can be `NULL`. Needs to be a pointer to the type of items in the table.
 * @example > Loop over all {key, item} pairs of a table.
 *     map CF_V2* table = my_table();
 *     const uint64_t* keys = mkeys(table);
 *     for (int i = 0; i < mcount(table); ++i) {
 *         uint64_t key = keys[i];
 *         CF_V2 item = table[i]; // Could also do `mitems(table)` here.
 *         // ...
 *     }
 * @remarks  This macro doesn't do much as `h` is already a valid pointer to the items.
 * @related  map mset madd mget mfind mget_ptr mfind_ptr mhas mdel mclear mkeys mitems mswap msize mcount mfree
 */
#define mitems(h) cf_map_items(h)

/**
 * @function mswap
 * @category map
 * @brief    Swaps internal ordering of two {key, item} pairs without ruining the hashing.
 * @param    h        The map. Can be `NULL`. Needs to be a pointer to the type of items in the table.
 * @param    index_a  Index to the first item to swap.
 * @param    index_b  Index to the second item to swap.
 * @example > Loop over all {key, item} pairs of a table.
 *     map CF_V2* table = my_table();
 *     const uint64_t* keys = mkeys(table);
 *     for (int i = 0; i < mcount(table); ++i) {
 *         for (int j = 0; j < mcount(table); ++j) {
 *             if (my_need_swap(table, i, j)) {
 *                 mswap(table, i, j);
 *             }
 *         }
 *     }
 * @remarks  Use this for e.g. implementing a priority queue on top of the hash table.
 * @related  map mset madd mget mfind mget_ptr mfind_ptr mhas mdel mclear mkeys mitems mswap msize mcount mfree
 */
#define mswap(h, index_a, index_b) cf_map_swap(h, index_a, index_b)

/**
 * @function msize
 * @category map
 * @brief    The number of {key, item} pairs in the table.
 * @param    h        The map. Can be `NULL`. Needs to be a pointer to the type of items in the table.
 * @remarks  `h` can be `NULL`.
 * @related  map mset madd mget mfind mget_ptr mfind_ptr mhas mdel mclear mkeys mitems mswap msize mcount mfree
 */
#define msize(h) cf_map_size(h)

/**
 * @function mcount
 * @category map
 * @brief    The number of {key, item} pairs in the table.
 * @param    h        The map. Can be `NULL`. Needs to be a pointer to the type of items in the table.
 * @remarks  `h` can be `NULL`.
 * @related  map mset madd mget mfind mget_ptr mfind_ptr mhas mdel mclear mkeys mitems mswap msize mcount mfree
 */
#define mcount(h) cf_map_count(h)

/**
 * @function mfree
 * @category map
 * @brief    Frees up all resources used and sets `h` to `NULL`.
 * @param    h        The map. Can be `NULL`. Needs to be a pointer to the type of items in the table.
 * @remarks  `h` can be `NULL`.
 * @related  map mset madd mget mfind mget_ptr mfind_ptr mhas mdel mclear mkeys mitems mswap msize mcount mfree
 */
#define mfree(h) cf_map_free(h)
#endif // CUTE_NO_SHORTHAND_API

//--------------------------------------------------------------------------------------------------
// Longform C API.

#define cf_map
#define cf_map_set(h, k, ...) ((h) ? (h) : (*(void**)&(h) = cf_map_make_impl(sizeof(uint64_t), sizeof(*(h)), 1)), CF_HCANARY(h), h[-1] = (__VA_ARGS__), *(void**)&(h) = cf_map_insert_impl(CF_HHDR(h), (uint64_t)k), h + CF_HHDR(h)->return_index)
#define cf_map_add(h, k, ...) cf_map_set(h, k, (__VA_ARGS__))
#define cf_map_get(h, k) ((h)[cf_map_find_impl(CF_HHDR(h), (uint64_t)k)])
#define cf_map_find(h, k) cf_map_get(h, k)
#define cf_map_get_ptr(h, k) (cf_map_find_impl(CF_HHDR(h), (uint64_t)k), CF_HHDR(h)->return_index < 0 ? NULL : (h) + CF_HHDR(h)->return_index)
#define cf_map_find_ptr(h, k) cf_map_get_ptr(h, k)
#define cf_map_has(h, k) (h ? cf_map_remove_impl(CF_HHDR(h), (uint64_t)k) : NULL)
#define cf_map_del(h, k) (h ? cf_map_remove_impl(CF_HHDR(h), (uint64_t)k) : (void)0)
#define cf_map_clear(h) (CF_HCANARY(h), cf_map_clear_impl(CF_HHDR(h)))
#define cf_map_keys(h) (CF_HCANARY(h), h ? (const uint64_t*)cf_map_keys_impl(CF_HHDR(h))) : (const uint64_t*)NULL)
#define cf_map_items(h) (CF_HCANARY(h), h)
#define cf_map_swap(h, index_a, index_b) (CF_HCANARY(h), cf_map_swap_impl(CF_HHDR(h), index_a, index_b))
#define cf_map_size(h) (h ? cf_map_count_impl(CF_HHDR(h)) : 0)
#define cf_map_count(h) cf_map_size(h)
#define cf_map_free(h) do { CF_HCANARY(h); if (h) cf_map_free_impl(CF_HHDR(h)); h = NULL; } while (0)

//--------------------------------------------------------------------------------------------------
// Hidden API - Not intended for direct use.

#define CF_HHDR(h) (((CF_Hhdr*)(h - 1) - 1)) // Converts pointer from the user-array to table header.
#define CF_HCOOKIE 0xE6F7E359 // Magic number used for sanity/type checks.
#define CF_HCANARY(h) (h ? CUTE_ASSERT(CF_HHDR(h)->cookie == CF_HCOOKIE) : (void)0) // Sanity/type check.

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

typedef struct CF_Hslot
{
	uint32_t key_hash;
	int item_index;
	int base_count;
} CF_Hslot;

typedef struct CF_Hhdr
{
	int key_size;
	int item_size;
	int item_capacity;
	int count;
	int slot_capacity;
	CF_Hslot* slots;
	void* items_key;
	int* items_slot_index;
	int return_index;
	void* hidden_item;
	void* items_data;
	void* temp_key;
	void* temp_item;
	uint32_t cookie;
} CF_Hhdr;

CUTE_API void* CUTE_CALL cf_map_make_impl(int key_size, int item_size, int capacity);
CUTE_API void CUTE_CALL cf_map_free_impl(CF_Hhdr* table);
CUTE_API void* CUTE_CALL cf_map_insert_impl(CF_Hhdr* table, uint64_t key);
CUTE_API void* CUTE_CALL cf_map_insert_impl2(CF_Hhdr* table, const void* key, const void* item);
CUTE_API void* CUTE_CALL cf_map_insert_impl3(CF_Hhdr* table, const void* key);
CUTE_API void CUTE_CALL cf_map_remove_impl(CF_Hhdr* table, uint64_t key);
CUTE_API void CUTE_CALL cf_map_remove_impl2(CF_Hhdr* table, const void* key);
CUTE_API bool CUTE_CALL cf_map_has_impl(CF_Hhdr* table, uint64_t key);
CUTE_API int CUTE_CALL cf_map_find_impl(const CF_Hhdr* table, uint64_t key);
CUTE_API int CUTE_CALL cf_map_find_impl2(const CF_Hhdr* table, const void* key);
CUTE_API int CUTE_CALL cf_map_count_impl(const CF_Hhdr* table);
CUTE_API void* CUTE_CALL cf_map_items_impl(const CF_Hhdr* table);
CUTE_API void* CUTE_CALL cf_map_keys_impl(const CF_Hhdr* table);
CUTE_API void CUTE_CALL cf_map_clear_impl(CF_Hhdr* table);
CUTE_API void CUTE_CALL cf_map_swap_impl(CF_Hhdr* table, int index_a, int index_b);

#ifdef __cplusplus
}
#endif // __cplusplus

//--------------------------------------------------------------------------------------------------
// C++ API

#ifdef CUTE_CPP

namespace Cute
{

// General purpose {key, item} pair mapping via internal hash table.
// Keys are treated as mere byte buffers (Plain Old Data).
// Items have contructors/destructors called, but are *not* allowed to store references/pointers to themselves.
template <typename K, typename T>
struct Map
{
	Map();
	Map(const Map<K, T>& other);
	Map(Map<K, T>&& other);
	Map(int capacity);
	~Map();

	T& get(const K& key);
	T& find(const K& key) { return get(key); }
	const T& get(const K& key) const;
	const T& find(const K& key) const { return get(key); }
	T* try_get(const K& key);
	T* try_find(const K& key) { return try_get(key); }
	const T* try_get(const K& key) const;
	const T* try_find(const K& key) const { return try_get(key); }
	bool has(const K& key) const { return try_get(key) ? true : false; }

	T* insert(const K& key);
	T* insert(const K& key, const T& val);
	T* insert(const K& key, T&& val);
	void remove(const K& key);

	void clear();

	int count() const;
	T* items();
	const T* items() const;
	T* vals() { return items(); }
	const T* vals() const { return items(); }
	K* keys();
	const K* keys() const;

	void swap(int index_a, int index_b);

	Map<K, T>& operator=(const Map<K, T>& rhs);
	Map<K, T>& operator=(Map<K, T>&& rhs);

private:
	CF_Hhdr* m_table = NULL;
};

// -------------------------------------------------------------------------------------------------

template <typename K, typename T>
Map<K, T>::Map()
{
	m_table = CF_HHDR((T*)cf_map_make_impl(sizeof(K), sizeof(T), 32));
}

template <typename K, typename T>
Map<K, T>::Map(const Map<K, T>& other)
{
	int n = other.count();
	const T* items = other.items();
	const K* keys = other.keys();
	if (n) {
		m_table = CF_HHDR((T*)cf_map_make_impl(sizeof(K), sizeof(T), n));
		for (int i = 0; i < n; ++i) {
			insert(keys[i], items[i]);
		}
	}
}

template <typename K, typename T>
Map<K, T>::Map(Map<K, T>&& other)
{
	m_table = other.m_table;
	other.m_table = NULL;
}

template <typename K, typename T>
Map<K, T>::Map(int capacity)
{
	m_table = CF_HHDR((T*)cf_map_make_impl(sizeof(K), sizeof(T), capacity));
}

template <typename K, typename T>
Map<K, T>::~Map()
{
	T* elements = items();
	for (int i = 0; i < count(); ++i) {
		(elements + i)->~T();
	}
	cf_map_free_impl(m_table);
	m_table = NULL;
}

template <typename K, typename T>
T& Map<K, T>::get(const K& key)
{
	int index = cf_map_find_impl2(m_table, &key);
	return items()[index];
}

template <typename K, typename T>
const T& Map<K, T>::get(const K& key) const
{
	int index = cf_map_find_impl2(m_table, &key);
	return items()[index];
}

template <typename K, typename T>
T* Map<K, T>::try_get(const K& key)
{
	if (!m_table) return NULL;
	int index = cf_map_find_impl2(m_table, &key);
	if (index >= 0) return items() + index;
	else return NULL;
}

template <typename K, typename T>
const T* Map<K, T>::try_get(const K& key) const
{
	if (!m_table) return NULL;
	int index = cf_map_find_impl2(m_table, &key);
	if (index >= 0) return items() + index;
	else return NULL;
}

template <typename K, typename T>
T* Map<K, T>::insert(const K& key)
{
	m_table = CF_HHDR((T*)cf_map_insert_impl3(m_table, &key));
	int index = m_table->return_index;
	if (index < 0) return NULL;
	T* result = items() + index;
	CUTE_PLACEMENT_NEW(result) T();
	return result;
}

template <typename K, typename T>
T* Map<K, T>::insert(const K& key, const T& val)
{
	m_table = CF_HHDR((T*)cf_map_insert_impl2(m_table, &key, &val));
	int index = m_table->return_index;
	if (index < 0) return NULL;
	T* result = items() + index;
	CUTE_PLACEMENT_NEW(result) T(val);
	return result;
}

template <typename K, typename T>
T* Map<K, T>::insert(const K& key, T&& val)
{
	m_table = CF_HHDR((T*)cf_map_insert_impl2(m_table, &key, &val));
	int index = m_table->return_index;
	if (index < 0) return NULL;
	T* result = items() + index;
	CUTE_PLACEMENT_NEW(result) T(move(val));
	return result;
}

template <typename K, typename T>
void Map<K, T>::remove(const K& key)
{
	T* slot = try_find(key);
	if (slot) {
		slot->~T();
		cf_map_remove_impl2(m_table, &key);
	}
}

template <typename K, typename T>
void Map<K, T>::clear()
{
	T* elements = items();
	for (int i = 0; i < count(); ++i) {
		(elements + i)->~T();
	}
	if (m_table) cf_map_clear_impl(m_table);
}

template <typename K, typename T>
int Map<K, T>::count() const
{
	return m_table ? cf_map_count_impl(m_table) : 0;
}

template <typename K, typename T>
T* Map<K, T>::items()
{
	return m_table ? (T*)(m_table + 1) + 1 : NULL;
}

template <typename K, typename T>
const T* Map<K, T>::items() const
{
	return m_table ? (const T*)(m_table + 1) + 1 : NULL;
}

template <typename K, typename T>
K* Map<K, T>::keys()
{
	return m_table ? (K*)cf_map_keys_impl(m_table) : NULL;
}

template <typename K, typename T>
const K* Map<K, T>::keys() const
{
	return m_table ? (const K*)cf_map_keys_impl(m_table) : NULL;
}

template <typename K, typename T>
void Map<K, T>::swap(int index_a, int index_b)
{
	cf_map_swap_impl(m_table, index_a, index_b);
}

template <typename K, typename T>
Map<K, T>& Map<K, T>::operator=(const Map<K, T>& rhs)
{
	clear();
	int n = rhs.count();
	const T* items = rhs.items();
	const K* keys = rhs.keys();
	if (n) {
		m_table = CF_HHDR((T*)cf_map_make_impl(sizeof(K), sizeof(T), n));
		for (int i = 0; i < n; ++i) {
			insert(keys[i], items[i]);
		}
	}
	return *this;
}

template <typename K, typename T>
Map<K, T>& Map<K, T>::operator=(Map<K, T>&& rhs)
{
	this->~Map<K, T>();
	m_table = rhs.m_table;
	rhs.m_table = NULL;
	return *this;
}

}

#endif // CUTE_CPP

#endif // CUTE_MAP_H
