/*
	Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

#ifndef CF_HASHTABLE_H
#define CF_HASHTABLE_H

#include "cute_defines.h"
#include "cute_c_runtime.h"
#include "cute_alloc.h"

//--------------------------------------------------------------------------------------------------
// C API

/**
 * @function htbl
 * @category hash
 * @brief    An empty macro used in the C API to markup hastables.
 * @example > Showcase of base htbl features.
 *     htbl CF_V2* pts = NULL;
 *     hset(pts, 0, cf_v2(3, 5)); // Contructs a new table on-the-spot. The table is *hidden* behind `pts`.
 *     hset(pts, 10, cf_v2(-1, -1);
 *     hset(pts, -2, cf_v2(0, 0));
 *
 *     // Use `hget` to fetch values.
 *     CF_V2 a = hget(pts, 0);
 *     CF_V2 b = hget(pts, 10);
 *     CF_V2 c = hget(pts, -2);
 *
 *     // Loop over {key, item} pairs like so:
 *     const uint64_t* keys = hkeys(pts);
 *     for (int i = 0; i < hcount(pts); ++i) {
 *         uint64_t key = keys[i];
 *         CF_V2 v = pts[i];
 *         // ...
 *     }
 *
 *     hfree(pts);
 * @remarks  This is an optional and _completely_ empty macro. It's only purpose is to provide a bit of visual indication a type is a
 *           hashtable. One downside of the C-macro API is the opaque nature of the pointer type. Since the macros use polymorphism
 *           on typed pointers, there's no actual hashtable struct type. It can get really annoying to sometimes forget if a pointer is an
 *           array, a hashtable, or just a pointer. This macro can be used to markup the type to make it much more clear for function
 *           parameters or struct member definitions. It's saying "Hey, I'm a hashtable!" to mitigate this downside.
 * @related  htbl hset hadd hget hfind hget_ptr hfind_ptr hhas hdel hclear hkeys hitems hswap hsize hcount hfree
 */
#define htbl

/**
 * @function hset
 * @category hash
 * @brief    Add's a {key, item} pair.
 * @param    h        The hashtable. Can be `NULL`. Needs to be a pointer to the type of items in the table.
 * @param    k        The key for lookups. Each {key, item} pair must be unique. Keys are always typecasted to `uint64_t` e.g. you can use pointers as keys.
 * @param    ...      An item to place in the table.
 * @example > Set and get a few elements from a hashtable.
 *     htbl int* table = NULL;
 *     hset(table, 0, 5);
 *     hset(table, 1, 12);
 *     CF_ASSERT(hget(table, 0) == 5);
 *     CF_ASSERT(hget(table, 1) == 12);
 *     hfree(table);
 * @return   Returns a pointer to the item set into the table.
 * @remarks  If the item does not exist in the table it is added. The pointer returned is not stable. Internally the table can be resized,
 *           invalidating _all_ pointers to any elements within the table. Therefor, no items may store pointers to themselves or other items.
 *           Indices however, are totally fine.
 * @related  htbl hset hadd hget hfind hget_ptr hfind_ptr hhas hdel hclear hkeys hitems hswap hsize hcount hfree
 */
#define hset(h, k, ...) cf_hashtable_set(h, k, (__VA_ARGS__))

/**
 * @function hadd
 * @category hash
 * @brief    Add's a {key, item} pair.
 * @param    h        The hashtable. Can be `NULL`. Needs to be a pointer to the type of items in the table.
 * @param    k        The key for lookups. Each {key, item} pair must be unique. Keys are always typecasted to `uint64_t` e.g. you can use pointers as keys.
 * @param    ...      An item to place in the table.
 * @example > Set and get a few elements from a hashtable.
 *     htbl int* table = NULL;
 *     hadd(table, 0, 5);
 *     hadd(table, 1, 12);
 *     CF_ASSERT(hget(table, 0) == 5);
 *     CF_ASSERT(hget(table, 1) == 12);
 *     hfree(table);
 * @return   Returns a pointer to the item set into the table.
 * @remarks  This function works the same as `hset`. If the item already exists in the table, it's simply updated to a new value.
 *           The pointer returned is not stable. Internally the table can be resized, invalidating _all_ pointers to any elements
 *           within the table. Therefor, no items may store pointers to themselves or other items. Indices however, are totally fine.
 * @related  htbl hset hadd hget hfind hget_ptr hfind_ptr hhas hdel hclear hkeys hitems hswap hsize hcount hfree
 */
#define hadd(h, k, ...) cf_hashtable_add(h, k, (__VA_ARGS__))

/**
 * @function hget
 * @category hash
 * @brief    Fetches the item that `k` maps to.
 * @param    h        The hashtable. Can be `NULL`. Needs to be a pointer to the type of items in the table.
 * @param    k        The key for lookups. Each {key, item} pair must be unique. Keys are always typecasted to `uint64_t` e.g. you can use pointers as keys.
 * @example > Set and get a few elements from a hashtable.
 *     htbl int* table = NULL;
 *     hadd(table, 0, 5);
 *     hadd(table, 1, 12);
 *     CF_ASSERT(hget(table, 0) == 5);
 *     CF_ASSERT(hget(table, 1) == 12);
 *     hfree(table);
 * @return   Returns a pointer to the item set into the table.
 * @remarks  Items are returned by value, not pointer. If the item doesn't exist a zero'd out item is instead returned. If you want to get a pointer
 *           (so you can see if it's `NULL` in case the item didn't exist, then use `hget_ptr`). You can also call `hhas` for a bool. This function does
 *           the same as `hfind`.
 * @related  htbl hset hadd hget hfind hget_ptr hfind_ptr hhas hdel hclear hkeys hitems hswap hsize hcount hfree
 */
#define hget(h, k) cf_hashtable_get(h, k)

/**
 * @function hfind
 * @category hash
 * @brief    Fetches the item that `k` maps to.
 * @param    h        The hashtable. Can be `NULL`. Needs to be a pointer to the type of items in the table.
 * @param    k        The key for lookups. Each {key, item} pair must be unique. Keys are always typecasted to `uint64_t` e.g. you can use pointers as keys.
 * @example > Set and get a few elements from a hashtable.
 *     htbl int* table = NULL;
 *     hadd(table, 0, 5);
 *     hadd(table, 1, 12);
 *     CF_ASSERT(hfind(table, 0) == 5);
 *     CF_ASSERT(hfind(table, 1) == 12);
 *     hfree(table);
 * @return   Returns a pointer to the item set into the table.
 * @remarks  Items are returned by value, not pointer. If the item doesn't exist a zero'd out item is instead returned. If you want to get a pointer
 *           (so you can see if it's `NULL` in case the item didn't exist, then use `hfind_ptr`). You can also call `hhas` for a bool. This function does
 *           the same as `hget`.
 * @related  htbl hset hadd hget hfind hget_ptr hfind_ptr hhas hdel hclear hkeys hitems hswap hsize hcount hfree
 */
#define hfind(h, k) cf_hashtable_find(h, k)

/**
 * @function hget_ptr
 * @category hash
 * @brief    Fetches a pointer to the item that `k` maps to.
 * @param    h        The hashtable. Can be `NULL`. Needs to be a pointer to the type of items in the table.
 * @param    k        The key for lookups. Each {key, item} pair must be unique. Keys are always typecasted to `uint64_t` e.g. you can use pointers as keys.
 * @example > Set and get a few elements from a hashtable.
 *     htbl CF_V2* table = NULL;
 *     hadd(table, 10, cf_v2(-1, 1));
 *     CF_V2* v = hget_ptr(table, 10);
 *     CF_ASSERT(v);
 *     CF_ASSERT(v->x == -1);
 *     CF_ASSERT(v->y == 1);
 *     hfree(table);
 * @return   Returns a pointer to an item. Returns `NULL` if not found.
 * @remarks  If you want to fetch an item by value, you can use `hget` or `hfind`. Does the same thing as `hfind_ptr`.
 * @related  htbl hset hadd hget hfind hget_ptr hfind_ptr hhas hdel hclear hkeys hitems hswap hsize hcount hfree
 */
#define hget_ptr(h, k) cf_hashtable_get_ptr(h, k)

/**
 * @function hfind_ptr
 * @category hash
 * @brief    Fetches a pointer to the item that `k` maps to.
 * @param    h        The hashtable. Can be `NULL`. Needs to be a pointer to the type of items in the table.
 * @param    k        The key for lookups. Each {key, item} pair must be unique. Keys are always typecasted to `uint64_t` e.g. you can use pointers as keys.
 * @example > Set and get a few elements from a hashtable.
 *     htbl CF_V2* table = NULL;
 *     hadd(table, 10, cf_v2(-1, 1));
 *     CF_V2* v = hfind_ptr(table, 10);
 *     CF_ASSERT(v);
 *     CF_ASSERT(v->x == -1);
 *     CF_ASSERT(v->y == 1);
 *     hfree(table);
 * @return   Returns a pointer to an item. Returns `NULL` if not found.
 * @remarks  If you want to fetch an item by value, you can use `hget` or `hfind`. Does the same thing as `hget_ptr`.
 * @related  htbl hset hadd hget hfind hget_ptr hfind_ptr hhas hdel hclear hkeys hitems hswap hsize hcount hfree
 */
#define hfind_ptr(h, k) cf_hashtable_find_ptr(h, k)

/**
 * @function hhas
 * @category hash
 * @brief    Check to see if an item exists in the table.
 * @param    h        The hashtable. Can be `NULL`. Needs to be a pointer to the type of items in the table.
 * @param    k        The key for lookups. Each {key, item} pair must be unique. Keys are always typecasted to `uint64_t` e.g. you can use pointers as keys.
 * @example > Checks if an item exists in the table.
 *     htbl v2* table = NULL;
 *     hadd(table, 10, V2(-1, 1));
 *     CF_ASSERT(hhas(table, 10));
 *     hfree(table);
 * @return   Returns true if the item was found, false otherwise.
 * @related  htbl hset hadd hget hfind hget_ptr hfind_ptr hhas hdel hclear hkeys hitems hswap hsize hcount hfree
 */
#define hhas(h, k) cf_hashtable_has(h, k)

/**
 * @function hdel
 * @category hash
 * @brief    Removes an item from the table.
 * @param    h        The hashtable. Can be `NULL`. Needs to be a pointer to the type of items in the table.
 * @param    k        The key for lookups. Each {key, item} pair must be unique. Keys are always typecasted to `uint64_t` e.g. you can use pointers as keys.
 * @example > Removes an item in the table.
 *     htbl CF_V2* table = NULL;
 *     hadd(table, 10, cf_v2(-1, 1));
 *     hdel(table, 10);
 *     hfree(table);
 * @remarks  Asserts if the item does not exist.
 * @related  htbl hset hadd hget hfind hget_ptr hfind_ptr hhas hdel hclear hkeys hitems hswap hsize hcount hfree
 */
#define hdel(h, k) cf_hashtable_del(h, k)

/**
 * @function hclear
 * @category hash
 * @brief    Clears the hashtable.
 * @param    h        The hashtable. Can be `NULL`. Needs to be a pointer to the type of items in the table.
 * @remarks  The count of items will now be zero. Does not free any memory. Call `hfree` when you are done.
 * @related  htbl hset hadd hget hfind hget_ptr hfind_ptr hhas hdel hclear hkeys hitems hswap hsize hcount hfree
 */
#define hclear(h) cf_hashtable_clear(h)

/**
 * @function hkeys
 * @category hash
 * @brief    Get a const pointer to the array of keys.
 * @param    h        The hashtable. Can be `NULL`. Needs to be a pointer to the type of items in the table.
 * @example > Loop over all {key, item} pairs of a table.
 *     htbl CF_V2* table = my_table();
 *     const uint64_t* keys = hkeys(table);
 *     for (int i = 0; i < hcount(table); ++i) {
 *         uint64_t key = keys[i];
 *         CF_V2 item = table[i];
 *         // ...
 *     }
 * @remarks  The keys are type `uint64_t`.
 * @related  htbl hset hadd hget hfind hget_ptr hfind_ptr hhas hdel hclear hkeys hitems hswap hsize hcount hfree
 */
#define hkeys(h) cf_hashtable_keys(h)

/**
 * @function hitems
 * @category hash
 * @brief    Get a pointer to the array of items.
 * @param    h        The hashtable. Can be `NULL`. Needs to be a pointer to the type of items in the table.
 * @example > Loop over all {key, item} pairs of a table.
 *     htbl CF_V2* table = my_table();
 *     const uint64_t* keys = hkeys(table);
 *     for (int i = 0; i < hcount(table); ++i) {
 *         uint64_t key = keys[i];
 *         CF_V2 item = table[i]; // Could also do `hitems(table)` here.
 *         // ...
 *     }
 * @remarks  This macro doesn't do much as `h` is already a valid pointer to the items.
 * @related  htbl hset hadd hget hfind hget_ptr hfind_ptr hhas hdel hclear hkeys hitems hswap hsize hcount hfree
 */
#define hitems(h) cf_hashtable_items(h)

/**
 * @function hswap
 * @category hash
 * @brief    Swaps internal ordering of two {key, item} pairs without ruining the hashing.
 * @param    h        The hashtable. Can be `NULL`. Needs to be a pointer to the type of items in the table.
 * @param    index_a  Index to the first item to swap.
 * @param    index_b  Index to the second item to swap.
 * @example > Loop over all {key, item} pairs of a table.
 *     htbl CF_V2* table = my_table();
 *     const uint64_t* keys = hkeys(table);
 *     for (int i = 0; i < hcount(table); ++i) {
 *         for (int j = 0; j < hcount(table); ++j) {
 *             if (my_need_swap(table, i, j)) {
 *                 hswap(table, i, j);
 *             }
 *         }
 *     }
 * @remarks  Use this for e.g. implementing a priority queue on top of the hash table.
 * @related  htbl hset hadd hget hfind hget_ptr hfind_ptr hhas hdel hclear hkeys hitems hswap hsize hcount hfree hsort hssort hsisort
 */
#define hswap(h, index_a, index_b) cf_hashtable_swap(h, index_a, index_b)

/**
 * @function hsort
 * @category hash
 * @brief    Sorts the {key, item} pairs in the table by keys.
 * @param    h        The hashtable. Can be `NULL`. Needs to be a pointer to the type of items in the table.
 * @remarks  The keys and items returned by `hkeys` and `hitems` will be sorted. Recall that all keys in the hashtable are treated
 *           as `uint64_t`, so the sorting simply sorts the keys from least to greatest as `uint64_t`. This is _not_ a stable sort.
 * @related  htbl hswap hsort hssort hsisort
 */
#define hsort(h) cf_hashtable_sort(h)

/**
 * @function hssort
 * @category hash
 * @brief    Sorts the {key, item} pairs in the table by keys, where the keys are treated as C-strings.
 * @param    h        The hashtable. Can be `NULL`. Needs to be a pointer to the type of items in the table.
 * @remarks  The keys and items returned by `hkeys` and `hitems` will be sorted. Normally it's not valid to store strings as keys,
 *           since all keys are simply typecasted to `uint64_t`. However, if you use `sintern` each unique string has a unique and
 *           stable pointer, making them valid keys for hashtables. This is _not_ a stable sort.
 * @related  htbl hswap hsort hssort hsisort sintern
 */
#define hssort(h) cf_hashtable_ssort(h)


/**
 * @function hsisort
 * @category hash
 * @brief    Sorts the {key, item} pairs in the table by keys, where the keys are treated as C-strings (case ignored).
 * @param    h        The hashtable. Can be `NULL`. Needs to be a pointer to the type of items in the table.
 * @remarks  The keys and items returned by `hkeys` and `hitems` will be sorted. Normally it's not valid to store strings as keys,
 *           since all keys are simply typecasted to `uint64_t`. However, if you use `sintern` each unique string has a unique and
 *           stable pointer, making them valid keys for hashtables. This is _not_ a stable sort.
 * @related  htbl hswap hsort hssort hsisort sintern
 */
#define hsisort(h) cf_hashtable_sisort(h)

/**
 * @function hsize
 * @category hash
 * @brief    The number of {key, item} pairs in the table.
 * @param    h        The hashtable. Can be `NULL`. Needs to be a pointer to the type of items in the table.
 * @remarks  `h` can be `NULL`.
 * @related  htbl hset hadd hget hfind hget_ptr hfind_ptr hhas hdel hclear hkeys hitems hswap hsize hcount hfree
 */
#define hsize(h) cf_hashtable_size(h)

/**
 * @function hcount
 * @category hash
 * @brief    The number of {key, item} pairs in the table.
 * @param    h        The hashtable. Can be `NULL`. Needs to be a pointer to the type of items in the table.
 * @remarks  `h` can be `NULL`.
 * @related  htbl hset hadd hget hfind hget_ptr hfind_ptr hhas hdel hclear hkeys hitems hswap hsize hcount hfree
 */
#define hcount(h) cf_hashtable_count(h)

/**
 * @function hfree
 * @category hash
 * @brief    Frees up all resources used and sets `h` to `NULL`.
 * @param    h        The hashtable. Can be `NULL`. Needs to be a pointer to the type of items in the table.
 * @remarks  `h` can be `NULL`.
 * @related  htbl hset hadd hget hfind hget_ptr hfind_ptr hhas hdel hclear hkeys hitems hswap hsize hcount hfree
 */
#define hfree(h) cf_hashtable_free(h)

//--------------------------------------------------------------------------------------------------
// Longform C API.

#define cf_htbl
#define cf_hashtable_set(h, k, ...) ((h) ? (h) : (*(void**)&(h) = cf_hashtable_make_impl(sizeof(uint64_t), sizeof(*(h)), 1)), CF_HCANARY(h), h[-1] = (__VA_ARGS__), *(void**)&(h) = cf_hashtable_insert_impl(CF_HHDR(h), (uint64_t)k), h + CF_HHDR(h)->return_index)
#define cf_hashtable_add(h, k, ...) cf_hashtable_set(h, k, (__VA_ARGS__))
#define cf_hashtable_get(h, k) ((h)[cf_hashtable_find_impl(CF_HHDR(h), (uint64_t)k)])
#define cf_hashtable_find(h, k) cf_hashtable_get(h, k)
#define cf_hashtable_get_ptr(h, k) (cf_hashtable_find_impl(CF_HHDR(h), (uint64_t)k), CF_HHDR(h)->return_index < 0 ? NULL : (h) + CF_HHDR(h)->return_index)
#define cf_hashtable_find_ptr(h, k) cf_hashtable_get_ptr(h, k)
#define cf_hashtable_has(h, k) ((h) ? cf_hashtable_has_impl(CF_HHDR(h), (uint64_t)k) : false)
#define cf_hashtable_del(h, k) ((h) ? cf_hashtable_remove_impl(CF_HHDR(h), (uint64_t)k) : (void)0)
#define cf_hashtable_clear(h) (CF_HCANARY(h), cf_hashtable_clear_impl(CF_HHDR(h)))
#define cf_hashtable_keys(h) (CF_HCANARY(h), h ? (const uint64_t*)cf_hashtable_keys_impl(CF_HHDR(h)) : (const uint64_t*)NULL)
#define cf_hashtable_items(h) (CF_HCANARY(h), h)
#define cf_hashtable_swap(h, index_a, index_b) (CF_HCANARY(h), cf_hashtable_swap_impl(CF_HHDR(h), index_a, index_b))
#define cf_hashtable_sort(h) (*(void**)&(h) = cf_hashtable_sort_impl(CF_HHDR(h)))
#define cf_hashtable_ssort(h) (*(void**)&(h) = cf_hashtable_ssort_impl(CF_HHDR(h)))
#define cf_hashtable_sisort(h) (*(void**)&(h) = cf_hashtable_sisort_impl(CF_HHDR(h)))
#define cf_hashtable_size(h) (h ? cf_hashtable_count_impl(CF_HHDR(h)) : 0)
#define cf_hashtable_count(h) cf_hashtable_size(h)
#define cf_hashtable_free(h) do { CF_HCANARY(h); if (h) cf_hashtable_free_impl(CF_HHDR(h)); h = NULL; } while (0)

//--------------------------------------------------------------------------------------------------
// Hidden API - Not intended for direct use.

#define CF_HHDR(h) (((CF_Hhdr*)(h - 1) - 1)) // Converts pointer from the user-array to table header.
#define CF_HCOOKIE 0xE6F7E359 // Magic number used for sanity/type checks.
#define CF_HCANARY(h) (h ? CF_ASSERT(CF_HHDR(h)->cookie == CF_HCOOKIE) : (void)0) // Sanity/type check.

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

CF_API void* CF_CALL cf_hashtable_make_impl(int key_size, int item_size, int capacity);
CF_API void CF_CALL cf_hashtable_free_impl(CF_Hhdr* table);
CF_API void* CF_CALL cf_hashtable_insert_impl(CF_Hhdr* table, uint64_t key);
CF_API void* CF_CALL cf_hashtable_insert_impl2(CF_Hhdr* table, const void* key, const void* item);
CF_API void* CF_CALL cf_hashtable_insert_impl3(CF_Hhdr* table, const void* key);
CF_API void CF_CALL cf_hashtable_remove_impl(CF_Hhdr* table, uint64_t key);
CF_API void CF_CALL cf_hashtable_remove_impl2(CF_Hhdr* table, const void* key);
CF_API bool CF_CALL cf_hashtable_has_impl(CF_Hhdr* table, uint64_t key);
CF_API int CF_CALL cf_hashtable_find_impl(const CF_Hhdr* table, uint64_t key);
CF_API int CF_CALL cf_hashtable_find_impl2(const CF_Hhdr* table, const void* key);
CF_API int CF_CALL cf_hashtable_count_impl(const CF_Hhdr* table);
CF_API void* CF_CALL cf_hashtable_items_impl(const CF_Hhdr* table);
CF_API void* CF_CALL cf_hashtable_keys_impl(const CF_Hhdr* table);
CF_API void CF_CALL cf_hashtable_clear_impl(CF_Hhdr* table);
CF_API void CF_CALL cf_hashtable_swap_impl(CF_Hhdr* table, int index_a, int index_b);
CF_API void* CF_CALL cf_hashtable_sort_impl(CF_Hhdr* table);
CF_API void* CF_CALL cf_hashtable_ssort_impl(CF_Hhdr* table);
CF_API void* CF_CALL cf_hashtable_sisort_impl(CF_Hhdr* table);

#ifdef __cplusplus
}
#endif // __cplusplus

//--------------------------------------------------------------------------------------------------
// C++ API

#ifdef CF_CPP

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
	T* add(const K& key) { return insert(key); }
	T* add(const K& key, const T& val) { return insert(key, val); }
	T* add(const K& key, T&& val) { return insert(key, val); }
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

	template <typename P>
	void sort_by_keys(P predicate);

	template <typename P>
	void sort_by_items(P predicate);

	Map<K, T>& operator=(const Map<K, T>& rhs);
	Map<K, T>& operator=(Map<K, T>&& rhs);

private:
	CF_Hhdr* m_table = NULL;

	template <typename P>
	void sort_keys(int offset, int count, P predicate);

	template <typename P>
	void sort_items(int offset, int count, P predicate);
};

// -------------------------------------------------------------------------------------------------

template <typename K, typename T>
Map<K, T>::Map()
{
	m_table = CF_HHDR((T*)cf_hashtable_make_impl(sizeof(K), sizeof(T), 32));
}

template <typename K, typename T>
Map<K, T>::Map(const Map<K, T>& other)
{
	int n = other.count();
	const T* items = other.items();
	const K* keys = other.keys();
	if (n) {
		m_table = CF_HHDR((T*)cf_hashtable_make_impl(sizeof(K), sizeof(T), n));
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
	m_table = CF_HHDR((T*)cf_hashtable_make_impl(sizeof(K), sizeof(T), capacity));
}

template <typename K, typename T>
Map<K, T>::~Map()
{
	T* elements = items();
	for (int i = 0; i < count(); ++i) {
		(elements + i)->~T();
	}
	cf_hashtable_free_impl(m_table);
	m_table = NULL;
}

template <typename K, typename T>
T& Map<K, T>::get(const K& key)
{
	int index = cf_hashtable_find_impl2(m_table, &key);
	return items()[index];
}

template <typename K, typename T>
const T& Map<K, T>::get(const K& key) const
{
	int index = cf_hashtable_find_impl2(m_table, &key);
	return items()[index];
}

template <typename K, typename T>
T* Map<K, T>::try_get(const K& key)
{
	if (!m_table) return NULL;
	int index = cf_hashtable_find_impl2(m_table, &key);
	if (index >= 0) return items() + index;
	else return NULL;
}

template <typename K, typename T>
const T* Map<K, T>::try_get(const K& key) const
{
	if (!m_table) return NULL;
	int index = cf_hashtable_find_impl2(m_table, &key);
	if (index >= 0) return items() + index;
	else return NULL;
}

template <typename K, typename T>
T* Map<K, T>::insert(const K& key)
{
	m_table = CF_HHDR((T*)cf_hashtable_insert_impl3(m_table, &key));
	int index = m_table->return_index;
	if (index < 0) return NULL;
	T* result = items() + index;
	CF_PLACEMENT_NEW(result) T();
	return result;
}

template <typename K, typename T>
T* Map<K, T>::insert(const K& key, const T& val)
{
	m_table = CF_HHDR((T*)cf_hashtable_insert_impl2(m_table, &key, &val));
	int index = m_table->return_index;
	if (index < 0) return NULL;
	T* result = items() + index;
	CF_PLACEMENT_NEW(result) T(val);
	return result;
}

template <typename K, typename T>
T* Map<K, T>::insert(const K& key, T&& val)
{
	m_table = CF_HHDR((T*)cf_hashtable_insert_impl2(m_table, &key, &val));
	int index = m_table->return_index;
	if (index < 0) return NULL;
	T* result = items() + index;
	CF_PLACEMENT_NEW(result) T(cf_move(val));
	return result;
}

template <typename K, typename T>
void Map<K, T>::remove(const K& key)
{
	T* slot = try_find(key);
	if (slot) {
		slot->~T();
		cf_hashtable_remove_impl2(m_table, &key);
	}
}

template <typename K, typename T>
void Map<K, T>::clear()
{
	T* elements = items();
	for (int i = 0; i < count(); ++i) {
		(elements + i)->~T();
	}
	if (m_table) cf_hashtable_clear_impl(m_table);
}

template <typename K, typename T>
int Map<K, T>::count() const
{
	return m_table ? cf_hashtable_count_impl(m_table) : 0;
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
	return m_table ? (K*)cf_hashtable_keys_impl(m_table) : NULL;
}

template <typename K, typename T>
const K* Map<K, T>::keys() const
{
	return m_table ? (const K*)cf_hashtable_keys_impl(m_table) : NULL;
}

template <typename K, typename T>
void Map<K, T>::swap(int index_a, int index_b)
{
	cf_hashtable_swap_impl(m_table, index_a, index_b);
}

template <typename K, typename T>
Map<K, T>& Map<K, T>::operator=(const Map<K, T>& rhs)
{
	clear();
	int n = rhs.count();
	const T* items = rhs.items();
	const K* keys = rhs.keys();
	if (n) {
		m_table = CF_HHDR((T*)cf_hashtable_make_impl(sizeof(K), sizeof(T), n));
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

template <typename K, typename T>
template <typename P>
void Map<K, T>::sort_keys(int offset, int count, P predicate)
{
	if (count <= 1) return;

	auto key = [=, this](int index) -> K& {
		uint8_t* keys = (uint8_t*)m_table->items_key;
		void* k = keys + index * m_table->key_size;
		return *(K*)k;
	};
	auto swap = [=, this](int ia, int ib) { cf_hashtable_swap_impl(m_table, offset + ia, offset + ib); };

	const K& pivot_key = key(count - 1);
	int lo = 0;
	for (int hi = 0; hi < count - 1; ++hi) {
		const K& hi_key = key(hi);
		if (predicate(hi_key, pivot_key)) {
			swap(lo, hi);
			lo++;
		}
	}

	swap(count - 1, lo);

	sort_keys(0, lo, predicate);
	sort_keys(lo + 1, count - 1 - lo, predicate);
}

template <typename K, typename T>
template <typename P>
void Map<K, T>::sort_by_keys(P predicate)
{
	sort_keys(0, m_table->count, predicate);
}

template <typename K, typename T>
template <typename P>
void Map<K, T>::sort_items(int offset, int count, P predicate)
{
	if (count <= 1) return;

	auto item = [=, this](int index) -> T& {
		uint8_t* items = (uint8_t*)m_table->items_data;
		void* item = items + index * m_table->item_size;
		return *(T*)item;
	};
	auto swap = [=, this](int ia, int ib) { cf_hashtable_swap_impl(m_table, offset + ia, offset + ib); };

	const K& pivot = item(count - 1);
	int lo = 0;
	for (int hi = 0; hi < count - 1; ++hi) {
		const K& val = item(hi);
		if (predicate(val, pivot)) {
			swap(lo, hi);
			lo++;
		}
	}

	swap(count - 1, lo);

	sort_items(0, lo, predicate);
	sort_items(lo + 1, count - 1 - lo, predicate);
}

template <typename K, typename T>
template <typename P>
void Map<K, T>::sort_by_items(P predicate)
{
	sort_items(0, m_table->count, predicate);
}

}

#endif // CF_CPP

#endif // CF_HASHTABLE_H
