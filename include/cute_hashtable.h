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

#ifndef CUTE_HASHTABLE_H
#define CUTE_HASHTABLE_H

#include "cute_defines.h"
#include "cute_c_runtime.h"
#include "cute_alloc.h"

//--------------------------------------------------------------------------------------------------
// C API

// A hashtable for storing and looking up {key, item} pairs.
// Implemented in C with macros to support fancy polymorphism.
// The API works on typed pointers, just create one as NULL and start going.
// Free it with `hfree` when done.
// 
// Example:
// 
//     v2* pts = NULL;
//     hset(pts, 0, V2(3, 5)); // Contructs a new table on-the-spot.
//                             // The table is *hidden* behind `pts`.
//     hset(pts, 10, v2(-1, -1);
//     hset(pts, -2, v2(0, 0));
//     
//     // Use `hget` to fetch values.
//     v2 a = hget(pts, 0);
//     v2 b = hget(pts, 10);
//     v2 c = hget(pts, -2);
//     
//     // Loop over {key, item} pairs like so:
//     const uint64_t* keys = hkeys(pts);
//     for (int i = 0; i < hcount(pts); ++i) {
//         uint64_t key = keys[i];
//         v2 v = pts[i];
//         // ...
//     }
//     
//     hfree(pts);

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#ifndef CUTE_NO_SHORTHAND_API
/**
 * Add's a {key, item} pair. Creates a new table if `h` is NULL. Call `hfree` when done.
 * Keys are always typecasted to `uint64_t` e.g. you can use pointers as keys.
 * 
 * h   - The hashtable. Will create a new table if h is NULL.
 *       h needs to be a pointer to the type of items in the table.
 * k   - The key for lookups. Must be unique. Will be typecasted to uint64_t.
 * ... - An item to place into the table, by value.
 * 
 * Example:
 * 
 *     int* table = NULL;
 *     hset(table, 0, 5);
 *     hset(table, 1, 12);
 *     CUTE_ASSERT(hget(table, 0) == 5);
 *     CUTE_ASSERT(hget(table, 1) == 12);
 *     hfree(table);
 */
#define hset(h, k, ...) cf_hashtable_set(h, k, (__VA_ARGS__))

/**
 * Add's a {key, item} pair. Creates a new table if `h` is NULL. Call `hfree` when done.
 * Keys are always typecasted to `uint64_t` e.g. you can use pointers as keys.
 * 
 * h   - The hashtable. Will create a new table if h is NULL.
 *       h needs to be a pointer for the type of your items in the table.
 * k   - The key for lookups. Must be unique. Will be typecasted to uint64_t.
 * ... - An item to place into the table, by value.
 * 
 * Example:
 * 
 *     int* table = NULL;
 *     hadd(table, 0, 5);
 *     hadd(table, 1, 12);
 *     CUTE_ASSERT(hget(table, 0) == 5);
 *     CUTE_ASSERT(hget(table, 1) == 12);
 *     hfree(table);
 */
#define hadd(h, k, ...) cf_hashtable_add(h, k, (__VA_ARGS__))

/**
 * Fetches the item that `k` maps to.
 * 
 * h   - The hashtable. Will assert h is NULL.
 *       h needs to be a pointer to the type of items in the table.
 * k   - The key for lookups. Must be unique. Will be typecasted to uint64_t.
 * 
 * Items are returned by value, not pointer. If the item doesn't exist a zero'd out item
 * is instead returned. If you want to get a pointer (so you can see if it's `NULL` in
 * case the item didn't exist, then use `hget_ptr`). You can also call `hhas` for a bool.
 * 
 * Example:
 * 
 *     v2* table = NULL;
 *     hadd(table, 10, V2(-1, 1));
 *     v2 v = hget(table, 10);
 *     CUTE_ASSERT(v.x == -1);
 *     CUTE_ASSERT(v.y == 1);
 *     hfree(table);
 */
#define hget(h, k) cf_hashtable_get(h, k)

/**
 * Fetches the item that `k` maps to.
 * 
 * h   - The hashtable. Will assert if h is NULL.
 *       h needs to be a pointer to the type of items in the table.
 * k   - The key for lookups. Must be unique. Will be typecasted to uint64_t.
 * 
 * Items are returned by value, not pointer. If the item doesn't exist a zero'd out item
 * is instead returned. If you want to get a pointer (so you can see if it's `NULL` in
 * case the item didn't exist, then use `hget_ptr`). You can also call `hhas` for a bool.
 * 
 * Example:
 * 
 *     v2* table = NULL;
 *     hadd(table, 10, V2(-1, 1));
 *     v2 v = hfind(table, 10);
 *     CUTE_ASSERT(v.x == -1);
 *     CUTE_ASSERT(v.y == 1);
 *     hfree(table);
 */
#define hfind(h, k) cf_hashtable_find(h, k)

/**
 * Fetches a pointer to the item that `k` maps to. Returns NULL if not found.
 * 
 * h   - The hashtable. Can be NULL.
 *       h needs to be a pointer to the type of items in the table.
 * k   - The key for lookups. Must be unique. Will be typecasted to uint64_t.
 * 
 * Example:
 * 
 *     v2* table = NULL;
 *     hadd(table, 10, V2(-1, 1));
 *     v2* v = hget_ptr(table, 10);
 *     CUTE_ASSERT(v);
 *     CUTE_ASSERT(v->x == -1);
 *     CUTE_ASSERT(v->y == 1);
 *     hfree(table);
 */
#define hget_ptr(h, k) cf_hashtable_get_ptr(h, k)

/**
 * Fetches a pointer to the item that `k` maps to. Returns NULL if not found.
 * 
 * h   - The hashtable. Can be NULL.
 *       h needs to be a pointer to the type of items in the table.
 * k   - The key for lookups. Must be unique. Will be typecasted to uint64_t.
 * 
 * Example:
 * 
 *     v2* table = NULL;
 *     hadd(table, 10, V2(-1, 1));
 *     v2* v = hfind_ptr(table, 10);
 *     CUTE_ASSERT(v);
 *     CUTE_ASSERT(v->x == -1);
 *     CUTE_ASSERT(v->y == 1);
 *     hfree(table);
 */
#define hfind_ptr(h, k) cf_hashtable_find_ptr(h, k)

/**
 * Check to see if an item exists in the table.
 * 
 * h   - The hashtable. Can be NULL.
 *       h needs to be a pointer to the type of items in the table.
 * k   - The key for lookups. Must be unique. Will be typecasted to uint64_t.
 * 
 * Example:
 * 
 *     v2* table = NULL;
 *     hadd(table, 10, V2(-1, 1));
 *     CUTE_ASSERT(hhas(table, 10));
 *     hfree(table);
 */
#define hhas(h, k) cf_hashtable_has(h, k)

/**
 * Removes an item from the table. Asserts if the item does not exist.
 * 
 * h   - The hashtable. Can be NULL.
 *       h needs to be a pointer to the type of items in the table.
 * k   - The key for lookups. Must be unique. Will be typecasted to uint64_t.
 * 
 * Example:
 * 
 *     v2* table = NULL;
 *     hadd(table, 10, V2(-1, 1));
 *     hdel(table, 10);
 *     hfree(table);
 */
#define hdel(h, k) cf_hashtable_del(h, k)

/**
 * Clears the hashtable. The count of items will now be zero.
 * Does not free any memory. Call `hfree` when you are done.
 * 
 * h   - The hashtable. Can be NULL.
 *       h needs to be a pointer to the type of items in the table.
 */
#define hclear(h) cf_hashtable_clear(h)

/**
 * Get a const pointer to the array of keys. The keys are type uint64_t.
 * 
 * h   - The hashtable. Can be NULL.
 *       h needs to be a pointer to the type of items in the table.
 * 
 * Example:
 * 
 *     v2* table = my_table();
 *     const uint64_t* keys = hkeys(table);
 *     for (int i = 0; i < hcount(table); ++i) {
 *         uint64_t key = keys[i];
 *         v2 item = table[i];
 *         // ...
 *     }
 */
#define hkeys(h) cf_hashtable_keys(h)

/**
 * Get a pointer to the array of items.
 * This macro doesn't do much as `h` is already a valid pointer to the items.
 * 
 * h   - The hashtable. Can be NULL.
 *       h needs to be a pointer to the type of items in the table.
 * 
 * Example:
 * 
 *     v2* table = my_table();
 *     const uint64_t* keys = hkeys(table);
 *     for (int i = 0; i < hcount(table); ++i) {
 *         uint64_t key = keys[i];
 *         v2 item = table[i]; // Could also do `hitems(table)` here.
 *         // ...
 *     }
 */
#define hitems(h) cf_hashtable_items(h)

/**
 * Swaps internal ordering of two {key, item} pairs without ruining the hashing.
 * Use this for e.g. implementing a priority queue on top of the hash table.
 * 
 * h       - The hashtable. Can be NULL.
 *           h needs to be a pointer to the type of items in the table.
 * index_a - Index to the first item to swap.
 * index_b - Index to the second item to swap.
 * 
 * Example:
 * 
 *     v2* table = my_table();
 *     const uint64_t* keys = hkeys(table);
 *     for (int i = 0; i < hcount(table); ++i) {
 *         for (int j = 0; j < hcount(table); ++j) {
 *             if (my_need_swap(table, i, j)) {
 *                 hswap(h, i, j);
 *             }
 *         }
 *     }
 */
#define hswap(h, index_a, index_b) cf_hashtable_swap(h, index_a, index_b)

/**
 * The number of {key, item} pairs in the table.
 * h can be NULL.
 */
#define hsize(h) cf_hashtable_size(h)

/**
 * The number of {key, item} pairs in the table.
 * h can be NULL.
 */
#define hcount(h) cf_hashtable_count(h)

/**
 * Frees up all resources used and sets h to NULL.
 * h can be NULL.
 */
#define hfree(h) cf_hashtable_free(h)
#endif // CUTE_NO_SHORTHAND_API

//--------------------------------------------------------------------------------------------------
// Longform C API.

#define cf_hashtable_set(h, k, ...) ((h) ? (h) : (*(void**)&(h) = cf_hashtable_make_impl(sizeof(uint64_t), sizeof(*(h)), 1)), CF_HCANARY(h), h[-1] = (__VA_ARGS__), *(void**)&(h) = cf_hashtable_insert_impl(CF_HHDR(h), (uint64_t)k), h + CF_HHDR(h)->return_index)
#define cf_hashtable_add(h, k, ...) cf_hashtable_set(h, k, (__VA_ARGS__))
#define cf_hashtable_get(h, k) ((h)[cf_hashtable_find_impl(CF_HHDR(h), (uint64_t)k)])
#define cf_hashtable_find(h, k) cf_hashtable_get(h, k)
#define cf_hashtable_get_ptr(h, k) (cf_hashtable_find_impl(CF_HHDR(h), (uint64_t)k), CF_HHDR(h)->return_index < 0 ? NULL : (h) + CF_HHDR(h)->return_index)
#define cf_hashtable_find_ptr(h, k) cf_hashtable_get_ptr(h, k)
#define cf_hashtable_has(h, k) (h ? cf_hashtable_remove_impl(CF_HHDR(h), (uint64_t)k) : NULL)
#define cf_hashtable_del(h, k) (h ? cf_hashtable_remove_impl(CF_HHDR(h), (uint64_t)k) : 0)
#define cf_hashtable_clear(h) (CF_HCANARY(h), cf_hashtable_clear_impl(CF_HHDR(h)))
#define cf_hashtable_keys(h) (CF_HCANARY(h), h ? (const uint64_t*)cf_hashtable_keys_impl(CF_HHDR(h))) : (const uint64_t*)NULL)
#define cf_hashtable_items(h) (CF_HCANARY(h), h)
#define cf_hashtable_swap(h, index_a, index_b) (CF_HCANARY(h), cf_hashtable_swap_impl(CF_HHDR(h), index_a, index_b))
#define cf_hashtable_size(h) (h ? cf_hashtable_count_impl(CF_HHDR(h)) : 0)
#define cf_hashtable_count(h) cf_hashtable_size(h)
#define cf_hashtable_free(h) do { CF_HCANARY(h); if (h) cf_hashtable_free_impl(CF_HHDR(h)); h = NULL; } while (0)

//--------------------------------------------------------------------------------------------------
// Hidden API - Not intended for direct use.

#define CF_HHDR(h) (((cf_hhdr_t*)(h - 1) - 1)) // Converts pointer from the user-array to table header.
#define CF_HCOOKIE 0xE6F7E359 // Magic number used for sanity/type checks.
#define CF_HCANARY(h) (h ? CUTE_ASSERT(CF_HHDR(h)->cookie == CF_HCOOKIE) : 0) // Sanity/type check.

typedef struct cf_hslot_t
{
	uint32_t key_hash;
	int item_index;
	int base_count;
} cf_hslot_t;

typedef struct cf_hhdr_t
{
	int key_size;
	int item_size;
	int item_capacity;
	int count;
	int slot_capacity;
	cf_hslot_t* slots;
	void* items_key;
	int* items_slot_index;
	int return_index;
	void* hidden_item;
	void* items_data;
	void* temp_key;
	void* temp_item;
	uint32_t cookie;
} cf_hhdr_t;

CUTE_API void* CUTE_CALL cf_hashtable_make_impl(int key_size, int item_size, int capacity);
CUTE_API void CUTE_CALL cf_hashtable_free_impl(cf_hhdr_t* table);
CUTE_API void* CUTE_CALL cf_hashtable_insert_impl(cf_hhdr_t* table, uint64_t key);
CUTE_API void* CUTE_CALL cf_hashtable_insert_impl2(cf_hhdr_t* table, const void* key, const void* item);
CUTE_API void* CUTE_CALL cf_hashtable_insert_impl3(cf_hhdr_t* table, const void* key);
CUTE_API void CUTE_CALL cf_hashtable_remove_impl(cf_hhdr_t* table, uint64_t key);
CUTE_API void CUTE_CALL cf_hashtable_remove_impl2(cf_hhdr_t* table, const void* key);
CUTE_API bool CUTE_CALL cf_hashtable_has_impl(cf_hhdr_t* table, uint64_t key);
CUTE_API int CUTE_CALL cf_hashtable_find_impl(const cf_hhdr_t* table, uint64_t key);
CUTE_API int CUTE_CALL cf_hashtable_find_impl2(const cf_hhdr_t* table, const void* key);
CUTE_API int CUTE_CALL cf_hashtable_count_impl(const cf_hhdr_t* table);
CUTE_API void* CUTE_CALL cf_hashtable_items_impl(const cf_hhdr_t* table);
CUTE_API void* CUTE_CALL cf_hashtable_keys_impl(const cf_hhdr_t* table);
CUTE_API void CUTE_CALL cf_hashtable_clear_impl(cf_hhdr_t* table);
CUTE_API void CUTE_CALL cf_hashtable_swap_impl(cf_hhdr_t* table, int index_a, int index_b);

#ifdef __cplusplus
}
#endif // __cplusplus

//--------------------------------------------------------------------------------------------------
// C++ API

#ifdef CUTE_CPP

namespace cute
{

// General purpose {key, item} pair mapping via internal hash table.
// Keys are treated as mere byte buffers (Plain Old Data).
// Items have contructors/destructors called, but are *not* allowed to store references/pointers to themselves.
template <typename K, typename T>
struct dictionary
{
	dictionary();
	dictionary(int capacity);
	~dictionary();

	T* find(const K& key);
	const T* find(const K& key) const;

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

private:
	cf_hhdr_t* m_table;
};

// -------------------------------------------------------------------------------------------------

template <typename K, typename T>
dictionary<K, T>::dictionary()
{
	m_table = CF_HHDR((T*)cf_hashtable_make_impl(sizeof(K), sizeof(T), 32));
}

template <typename K, typename T>
dictionary<K, T>::dictionary(int capacity)
{
	m_table = CF_HHDR((T*)cf_hashtable_make_impl(sizeof(K), sizeof(T), capacity));
}

template <typename K, typename T>
dictionary<K, T>::~dictionary()
{
	T* elements = items();
	for (int i = 0; i < count(); ++i) {
		(elements + i)->~T();
	}
	cf_hashtable_free_impl(m_table);
	m_table = NULL;
}

template <typename K, typename T>
T* dictionary<K, T>::find(const K& key)
{
	int index = cf_hashtable_find_impl2(m_table, &key);
	if (index >= 0) return items() + index;
	else return NULL;
}

template <typename K, typename T>
const T* dictionary<K, T>::find(const K& key) const
{
	int index = cf_hashtable_find_impl2(m_table, &key);
	if (index > 0) return items() + index;
	else return NULL;
}

template <typename K, typename T>
T* dictionary<K, T>::insert(const K& key)
{
	m_table = CF_HHDR((T*)cf_hashtable_insert_impl3(m_table, &key));
	int index = m_table->return_index;
	if (index < 0) return NULL;
	T* result = items() + index;
	CUTE_PLACEMENT_NEW(result) T();
	return result;
}

template <typename K, typename T>
T* dictionary<K, T>::insert(const K& key, const T& val)
{
	m_table = CF_HHDR((T*)cf_hashtable_insert_impl2(m_table, &key, &val));
	int index = m_table->return_index;
	if (index < 0) return NULL;
	T* result = items() + index;
	CUTE_PLACEMENT_NEW(result) T(val);
	return result;
}

template <typename K, typename T>
T* dictionary<K, T>::insert(const K& key, T&& val)
{
	m_table = CF_HHDR((T*)cf_hashtable_insert_impl2(m_table, &key, &val));
	int index = m_table->return_index;
	if (index < 0) return NULL;
	T* result = items() + index;
	CUTE_PLACEMENT_NEW(result) T(move(val));
	return result;
}

template <typename K, typename T>
void dictionary<K, T>::remove(const K& key)
{
	T* slot = find(key);
	if (slot) {
		slot->~T();
		cf_hashtable_remove_impl2(m_table, &key);
	}
}

template <typename K, typename T>
void dictionary<K, T>::clear()
{
	cf_hashtable_clear_impl(m_table);
}

template <typename K, typename T>
int dictionary<K, T>::count() const
{
	return cf_hashtable_count_impl(m_table);
}

template <typename K, typename T>
T* dictionary<K, T>::items()
{
	return (T*)(m_table + 1) + 1;
}

template <typename K, typename T>
const T* dictionary<K, T>::items() const
{
	return (const T*)(m_table + 1) + 1;
}

template <typename K, typename T>
K* dictionary<K, T>::keys()
{
	return (K*)cf_hashtable_keys_impl(m_table);
}

template <typename K, typename T>
const K* dictionary<K, T>::keys() const
{
	return (const K*)cf_hashtable_keys_impl(m_table);
}

template <typename K, typename T>
void dictionary<K, T>::swap(int index_a, int index_b)
{
	cf_hashtable_swap_impl(m_table, index_a, index_b);
}

}

#endif // CUTE_CPP

#endif // CUTE_HASHTABLE_H
