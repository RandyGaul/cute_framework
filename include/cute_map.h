/*
	Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

#ifndef CF_MAP_H
#define CF_MAP_H

#include "cute_defines.h"
#include "cute_c_runtime.h"
#include "cute_alloc.h"
#include "cute/ckit.h"

//--------------------------------------------------------------------------------------------------
// C API

/**
 * @function CK_MAP
 * @category hash
 * @brief    Declares a map (hashtable) type for values of type `T`.
 * @param    T            The value type stored in the map.
 * @example > Showcase of basic map features.
 *           CK_MAP(CF_V2) pts = NULL;
 *           map_set(pts, 0, cf_v2(3, 5));   // Constructs a new table on-the-spot. The table is *hidden* behind `pts`.
 *           map_set(pts, 10, cf_v2(-1, -1));
 *           map_set(pts, 2, cf_v2(0, 0));
 *           
 *           // Use `map_get` to fetch values.
 *           CF_V2 a = map_get(pts, 0);
 *           CF_V2 b = map_get(pts, 10);
 *           CF_V2 c = map_get(pts, 2);
 *	         
 *           // Loop over {key, item} pairs like so:
 *           uint64_t* keys = map_keys(pts);
 *           for (int i = 0; i < map_size(pts); ++i) {
 *               uint64_t key = keys[i];
 *               CF_V2 v = pts[i];
 *               // ...
 *           }
 *           
 *           map_free(pts);
 * @remarks  `CK_MAP(T)` resolves to `T*`. The map data structure is hidden just before the pointer in memory
 *           (stretchy-buffer style). An empty/uninitialized map is simply `NULL`. One downside of the C-macro
 *           API is the opaque nature of the pointer type. Since the macros use polymorphism on typed pointers,
 *           there's no actual map struct type visible. `CK_MAP(T)` helps visually indicate a type is a map, not
 *           just a plain pointer. Keys are always `uint64_t`. Use `sintern` for string keys (casting the pointer
 *           to `uint64_t`).
 * @related  map_set map_get map_get_ptr map_has map_del map_clear map_keys map_items map_swap map_size map_free
 */

/**
 * @function map_set
 * @category hash
 * @brief    Adds or updates a {key, item} pair in the map.
 * @param    m            The map. Can be `NULL`. Must be declared with `CK_MAP(T)`.
 * @param    k            The key for lookups. Keys are always typecast to `uint64_t`, so you can use pointers as keys.
 * @param    v            The item to place in the map.
 * @example > Set and get a few elements from a map.
 *           CK_MAP(int) table = NULL;
 *           map_set(table, 0, 5);
 *           map_set(table, 1, 12);
 *           CF_ASSERT(map_get(table, 0) == 5);
 *           CF_ASSERT(map_get(table, 1) == 12);
 *           map_free(table);
 * @remarks  If the key already exists, the value is overwritten. The map may reallocate internally, invalidating
 *           _all_ pointers to elements within the map. Therefore, items may not store pointers to themselves or
 *           other items in the same map. Indices, however, are stable.
 * @related  CK_MAP map_get map_get_ptr map_has map_del map_clear map_keys map_items map_swap map_size map_free
 */
#define cf_map_set(m, k, v) map_set(m, k, v)

/**
 * @function map_get
 * @category hash
 * @brief    Fetches the item that `k` maps to.
 * @param    m            The map. Can be `NULL`. Must be declared with `CK_MAP(T)`.
 * @param    k            The key for lookups. Keys are always typecast to `uint64_t`.
 * @return   Returns the item by value. If the key doesn't exist, returns a zero'd item.
 * @example > Set and get a few elements from a map.
 *           CK_MAP(int) table = NULL;
 *           map_set(table, 0, 5);
 *           map_set(table, 1, 12);
 *           CF_ASSERT(map_get(table, 0) == 5);
 *           CF_ASSERT(map_get(table, 1) == 12);
 *           map_free(table);
 * @remarks  Items are returned by value, not pointer. If you want a pointer (to check for `NULL` when the item
 *           doesn't exist), use `map_get_ptr`. You can also call `map_has` to check existence.
 * @related  CK_MAP map_set map_get_ptr map_has map_del map_clear map_keys map_items map_swap map_size map_free
 */
#define cf_map_get(m, k) map_get(m, k)

/**
 * @function map_get_ptr
 * @category hash
 * @brief    Fetches a pointer to the item that `k` maps to.
 * @param    m            The map. Can be `NULL`. Must be declared with `CK_MAP(T)`.
 * @param    k            The key for lookups. Keys are always typecast to `uint64_t`.
 * @return   Returns a pointer to the item, or `NULL` if not found.
 * @example > Get a pointer to an element in a map.
 *           CK_MAP(CF_V2) table = NULL;
 *           map_set(table, 10, cf_v2(-1, 1));
 *           CF_V2* v = map_get_ptr(table, 10);
 *           CF_ASSERT(v);
 *           CF_ASSERT(v->x == -1);
 *           CF_ASSERT(v->y == 1);
 *           map_free(table);
 * @remarks  If you want to fetch an item by value, use `map_get`. The returned pointer is not stable across
 *           insertions -- adding new items may cause reallocation and invalidate all pointers.
 * @related  CK_MAP map_set map_get map_has map_del map_clear map_keys map_items map_swap map_size map_free
 */
#define cf_map_get_ptr(m, k) map_get_ptr(m, k)

/**
 * @function map_has
 * @category hash
 * @brief    Check if a key exists in the map.
 * @param    m            The map. Can be `NULL`. Must be declared with `CK_MAP(T)`.
 * @param    k            The key for lookups. Keys are always typecast to `uint64_t`.
 * @return   Returns true if the key was found, false otherwise.
 * @example > Check if an item exists in the map.
 *           CK_MAP(CF_V2) table = NULL;
 *           map_set(table, 10, cf_v2(-1, 1));
 *           CF_ASSERT(map_has(table, 10));
 *           CF_ASSERT(!map_has(table, 99));
 *           map_free(table);
 * @related  CK_MAP map_set map_get map_get_ptr map_del map_clear map_keys map_items map_swap map_size map_free
 */
#define cf_map_has(m, k) map_has(m, k)

/**
 * @function map_del
 * @category hash
 * @brief    Removes an item from the map.
 * @param    m            The map. Can be `NULL`. Must be declared with `CK_MAP(T)`.
 * @param    k            The key for lookups. Keys are always typecast to `uint64_t`.
 * @return   Returns 1 if the item was found and deleted, 0 if not found.
 * @example > Remove an item from the map.
 *           CK_MAP(CF_V2) table = NULL;
 *           map_set(table, 10, cf_v2(-1, 1));
 *           map_del(table, 10);
 *           CF_ASSERT(!map_has(table, 10));
 *           map_free(table);
 * @related  CK_MAP map_set map_get map_get_ptr map_has map_clear map_keys map_items map_swap map_size map_free
 */
#define cf_map_del(m, k) map_del(m, k)

/**
 * @function map_clear
 * @category hash
 * @brief    Removes all items from the map without freeing memory.
 * @param    m            The map. Can be `NULL`. Must be declared with `CK_MAP(T)`.
 * @remarks  After clearing, `map_size` returns 0 but the backing memory is retained. Call `map_free` when done.
 * @related  CK_MAP map_set map_get map_get_ptr map_has map_del map_keys map_items map_swap map_size map_free
 */
#define cf_map_clear(m) map_clear(m)

/**
 * @function map_keys
 * @category hash
 * @brief    Get a pointer to the array of keys.
 * @param    m            The map. Can be `NULL`. Must be declared with `CK_MAP(T)`.
 * @return   Returns `uint64_t*` pointing to the keys array, or `NULL` if map is empty.
 * @example > Loop over all {key, item} pairs.
 *           CK_MAP(CF_V2) table = my_table();
 *           uint64_t* keys = map_keys(table);
 *           for (int i = 0; i < map_size(table); ++i) {
 *               uint64_t key = keys[i];
 *               CF_V2 item = table[i];
 *               // ...
 *           }
 * @remarks  The keys array is parallel to the items array. Use with `map_size` for iteration bounds.
 * @related  CK_MAP map_set map_get map_get_ptr map_has map_del map_clear map_items map_swap map_size map_free
 */
#define cf_map_keys(m) map_keys(m)

/**
 * @function map_items
 * @category hash
 * @brief    Get a pointer to the array of items (values).
 * @param    m            The map. Can be `NULL`. Must be declared with `CK_MAP(T)`.
 * @return   Returns the map pointer itself (which points to the items array).
 * @example > Loop over all {key, item} pairs.
 *           CK_MAP(CF_V2) table = my_table();
 *           uint64_t* keys = map_keys(table);
 *           CF_V2* items = map_items(table);  // Same as just using `table` directly
 *           for (int i = 0; i < map_size(table); ++i) {
 *               uint64_t key = keys[i];
 *               CF_V2 item = items[i];  // Or just: table[i]
 *               // ...
 *           }
 * @remarks  Since `CK_MAP(T)` is `T*`, the map pointer itself is already the items array. This macro exists
 *           for symmetry with `map_keys` and clarity in iteration code.
 * @related  CK_MAP map_set map_get map_get_ptr map_has map_del map_clear map_keys map_swap map_size map_free
 */
#define cf_map_items(m) map_items(m)

/**
 * @function map_swap
 * @category hash
 * @brief    Swaps two {key, item} pairs by index without breaking the hash lookup.
 * @param    m            The map. Can be `NULL`. Must be declared with `CK_MAP(T)`.
 * @param    index_a      Index of the first item to swap.
 * @param    index_b      Index of the second item to swap.
 * @example > Swap elements during iteration (e.g., for custom sorting).
 *           CK_MAP(CF_V2) table = my_table();
 *           for (int i = 0; i < map_size(table); ++i) {
 *               for (int j = i + 1; j < map_size(table); ++j) {
 *                   if (my_need_swap(table, i, j)) {
 *                       map_swap(table, i, j);
 *                   }
 *               }
 *           }
 * @remarks  Use this for implementing priority queues or custom sort orders on top of the map.
 * @related  CK_MAP map_sort map_ssort map_keys map_items map_size
 */
#define cf_map_swap(m, i, j) map_swap(m, i, j)

/**
 * @function map_sort
 * @category hash
 * @brief    Sorts the {key, item} pairs by values using a comparator.
 * @param    m            The map. Can be `NULL`. Must be declared with `CK_MAP(T)`.
 * @param    cmp          Comparator function: `int cmp(const void* a, const void* b)`. Receives pointers to items.
 * @remarks  The keys and items returned by `map_keys` and `map_items` will be sorted. This is _not_ a stable sort.
 * @related  CK_MAP map_ssort map_swap map_keys map_items map_size
 */
#define cf_map_sort(m, cmp) map_sort(m, cmp)

/**
 * @function map_ssort
 * @category hash
 * @brief    Sorts the {key, item} pairs by keys, treating keys as interned string pointers.
 * @param    m            The map. Can be `NULL`. Must be declared with `CK_MAP(T)`.
 * @param    ignore_case  If true, sorts case-insensitively.
 * @remarks  Normally you can't store strings as keys since keys are `uint64_t`. However, if you use `sintern`,
 *           each unique string gets a unique stable pointer, making them valid map keys. This is _not_ a stable sort.
 * @related  CK_MAP map_sort map_swap map_keys map_items map_size sintern
 */
#define cf_map_ssort(m, ignore_case) map_ssort(m, ignore_case)

/**
 * @function map_size
 * @category hash
 * @brief    Returns the number of {key, item} pairs in the map.
 * @param    m            The map. Can be `NULL`. Must be declared with `CK_MAP(T)`.
 * @return   Returns the count as `int`. Returns 0 if `m` is `NULL`.
 * @related  CK_MAP map_set map_get map_get_ptr map_has map_del map_clear map_keys map_items map_swap map_free
 */
#define cf_map_size(m) map_size(m)

/**
 * @function map_free
 * @category hash
 * @brief    Frees all memory used by the map and sets it to `NULL`.
 * @param    m            The map. Can be `NULL`. Must be declared with `CK_MAP(T)`.
 * @remarks  After calling, `m` will be `NULL`.
 * @related  CK_MAP map_set map_get map_get_ptr map_has map_del map_clear map_keys map_items map_swap map_size
 */
#define cf_map_free(m) map_free(m)

//--------------------------------------------------------------------------------------------------
// Longform C API aliases.
// The cf_map_* macros map to the shortform map_* macros from ckit.h.

#define cf_map_add(m, k, v) map_add(m, k, v)
#define cf_map_find(m, k) map_find(m, k)

//--------------------------------------------------------------------------------------------------
// C++ API

#ifdef CF_CPP

namespace Cute
{

// General purpose {key, item} pair mapping via CK_MAP (stretchy-buffer style).
// Keys are always uint64_t. Use sintern for string keys (casting the pointer to uint64_t).
// Items have constructors/destructors called, but are *not* allowed to store references/pointers to themselves.
template <typename T>
struct Map
{
	Map() = default;
	Map(const Map<T>& other);
	Map(Map<T>&& other);
	~Map();

	T& get(uint64_t key);
	T& find(uint64_t key) { return get(key); }
	const T& get(uint64_t key) const;
	const T& find(uint64_t key) const { return get(key); }
	T* try_get(uint64_t key);
	T* try_find(uint64_t key) { return try_get(key); }
	const T* try_get(uint64_t key) const;
	const T* try_find(uint64_t key) const { return try_get(key); }
	bool has(uint64_t key) const { return try_get(key) != NULL; }

	T* insert(uint64_t key);
	T* insert(uint64_t key, const T& val);
	T* insert(uint64_t key, T&& val);
	T* add(uint64_t key) { return insert(key); }
	T* add(uint64_t key, const T& val) { return insert(key, val); }
	T* add(uint64_t key, T&& val) { return insert(key, cf_move(val)); }
	void remove(uint64_t key);

	// Pointer-key overloads (template prevents int/0 ambiguity with uint64_t overloads).
	template <typename P> T& get(P* key) { return get((uint64_t)(uintptr_t)key); }
	template <typename P> T& find(P* key) { return get((uint64_t)(uintptr_t)key); }
	template <typename P> const T& get(P* key) const { return get((uint64_t)(uintptr_t)key); }
	template <typename P> const T& find(P* key) const { return get((uint64_t)(uintptr_t)key); }
	template <typename P> T* try_get(P* key) { return try_get((uint64_t)(uintptr_t)key); }
	template <typename P> T* try_find(P* key) { return try_get((uint64_t)(uintptr_t)key); }
	template <typename P> const T* try_get(P* key) const { return try_get((uint64_t)(uintptr_t)key); }
	template <typename P> const T* try_find(P* key) const { return try_get((uint64_t)(uintptr_t)key); }
	template <typename P> bool has(P* key) const { return has((uint64_t)(uintptr_t)key); }
	template <typename P> T* insert(P* key) { return insert((uint64_t)(uintptr_t)key); }
	template <typename P> T* insert(P* key, const T& val) { return insert((uint64_t)(uintptr_t)key, val); }
	template <typename P> T* insert(P* key, T&& val) { return insert((uint64_t)(uintptr_t)key, cf_move(val)); }
	template <typename P> T* add(P* key) { return insert((uint64_t)(uintptr_t)key); }
	template <typename P> T* add(P* key, const T& val) { return insert((uint64_t)(uintptr_t)key, val); }
	template <typename P> T* add(P* key, T&& val) { return insert((uint64_t)(uintptr_t)key, cf_move(val)); }
	template <typename P> void remove(P* key) { remove((uint64_t)(uintptr_t)key); }

	void clear();

	int count() const;
	T* items();
	const T* items() const;
	T* vals() { return items(); }
	const T* vals() const { return items(); }
	uint64_t* keys();
	const uint64_t* keys() const;

	void swap(int index_a, int index_b);
	void sort(int (*cmp)(const void* a, const void* b));

	template <typename Fn>
	void sort_by_items(Fn fn);

	Map<T>& operator=(const Map<T>& rhs);
	Map<T>& operator=(Map<T>&& rhs);

private:
	CK_MAP(T) m_map = nullptr;
};

// -------------------------------------------------------------------------------------------------

template <typename T>
Map<T>::Map(const Map<T>& other)
{
	int n = other.count();
	const T* vals = other.items();
	const uint64_t* ks = other.keys();
	for (int i = 0; i < n; ++i) {
		insert(ks[i], vals[i]);
	}
}

template <typename T>
Map<T>::Map(Map<T>&& other)
{
	m_map = other.m_map;
	other.m_map = nullptr;
}

template <typename T>
Map<T>::~Map()
{
	T* elements = items();
	for (int i = 0; i < count(); ++i) {
		(elements + i)->~T();
	}
	if (m_map) ck_map_free_impl(ck_map_hdr(m_map));
}

template <typename T>
T& Map<T>::get(uint64_t key)
{
	void* ptr = m_map ? ck_map_get_ptr_impl(ck_map_hdr(m_map), key) : nullptr;
	CF_ASSERT(ptr);
	return *(T*)ptr;
}

template <typename T>
const T& Map<T>::get(uint64_t key) const
{
	void* ptr = m_map ? ck_map_get_ptr_impl(ck_map_hdr(m_map), key) : nullptr;
	CF_ASSERT(ptr);
	return *(const T*)ptr;
}

template <typename T>
T* Map<T>::try_get(uint64_t key)
{
	return m_map ? (T*)ck_map_get_ptr_impl(ck_map_hdr(m_map), key) : nullptr;
}

template <typename T>
const T* Map<T>::try_get(uint64_t key) const
{
	return m_map ? (const T*)ck_map_get_ptr_impl(ck_map_hdr(m_map), key) : nullptr;
}

template <typename T>
T* Map<T>::insert(uint64_t key)
{
	// Reserve space first, then placement new.
	T dummy;
	CF_MEMSET(&dummy, 0, sizeof(T));
	ck_map_set_stretchy((void**)&m_map, key, &dummy, (int)sizeof(T));
	T* result = (T*)ck_map_get_ptr_impl(ck_map_hdr(m_map), key);
	CF_ASSERT(result);
	CF_PLACEMENT_NEW(result) T();
	return result;
}

template <typename T>
T* Map<T>::insert(uint64_t key, const T& val)
{
	T dummy;
	CF_MEMSET(&dummy, 0, sizeof(T));
	ck_map_set_stretchy((void**)&m_map, key, &dummy, (int)sizeof(T));
	T* result = (T*)ck_map_get_ptr_impl(ck_map_hdr(m_map), key);
	CF_ASSERT(result);
	CF_PLACEMENT_NEW(result) T(val);
	return result;
}

template <typename T>
T* Map<T>::insert(uint64_t key, T&& val)
{
	T dummy;
	CF_MEMSET(&dummy, 0, sizeof(T));
	ck_map_set_stretchy((void**)&m_map, key, &dummy, (int)sizeof(T));
	T* result = (T*)ck_map_get_ptr_impl(ck_map_hdr(m_map), key);
	CF_ASSERT(result);
	CF_PLACEMENT_NEW(result) T(cf_move(val));
	return result;
}

template <typename T>
void Map<T>::remove(uint64_t key)
{
	T* slot = try_get(key);
	if (slot) {
		slot->~T();
		ck_map_del_impl(ck_map_hdr(m_map), key);
	}
}

template <typename T>
void Map<T>::clear()
{
	T* elements = items();
	for (int i = 0; i < count(); ++i) {
		(elements + i)->~T();
	}
	if (m_map) ck_map_clear_impl(ck_map_hdr(m_map));
}

template <typename T>
int Map<T>::count() const
{
	return map_size(m_map);
}

template <typename T>
T* Map<T>::items()
{
	return m_map;
}

template <typename T>
const T* Map<T>::items() const
{
	return m_map;
}

template <typename T>
uint64_t* Map<T>::keys()
{
	return map_keys(m_map);
}

template <typename T>
const uint64_t* Map<T>::keys() const
{
	return map_keys(m_map);
}

template <typename T>
void Map<T>::swap(int index_a, int index_b)
{
	if (m_map) ck_map_swap_impl(ck_map_hdr(m_map), index_a, index_b);
}

template <typename T>
void Map<T>::sort(int (*cmp)(const void* a, const void* b))
{
	if (m_map) ck_map_sort_impl(ck_map_hdr(m_map), cmp);
}

template <typename T>
template <typename Fn>
void Map<T>::sort_by_items(Fn fn)
{
	// Insertion sort using the callable on T values.
	int n = count();
	for (int i = 1; i < n; ++i) {
		int j = i;
		while (j > 0 && fn(items()[j], items()[j - 1])) {
			swap(j, j - 1);
			--j;
		}
	}
}

template <typename T>
Map<T>& Map<T>::operator=(const Map<T>& rhs)
{
	if (this == &rhs) return *this;
	clear();
	int n = rhs.count();
	const T* vals = rhs.items();
	const uint64_t* ks = rhs.keys();
	for (int i = 0; i < n; ++i) {
		insert(ks[i], vals[i]);
	}
	return *this;
}

template <typename T>
Map<T>& Map<T>::operator=(Map<T>&& rhs)
{
	T* elements = items();
	for (int i = 0; i < count(); ++i) {
		(elements + i)->~T();
	}
	if (m_map) ck_map_free_impl(ck_map_hdr(m_map));
	m_map = rhs.m_map;
	rhs.m_map = nullptr;
	return *this;
}

}

#endif // CF_CPP

#endif // CF_MAP_H
