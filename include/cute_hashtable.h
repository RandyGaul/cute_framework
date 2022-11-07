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

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#define HHDR(h) ((cf_hhdr_t*)(h) - 1)
#define HCOOKIE 0xE6F7E359
#define HCANARY(h) (h ? CUTE_ASSERT(HHDR(h)->cookie == HCOOKIE) : 0) // Sanity/type check.

#define hset(h, k, ...) ((h) ? (h) : (*(void**)&(h) = cf_hmake(sizeof(uint64_t), sizeof(*(h)), 16)), HCANARY(h), h[0] = (__VA_ARGS__), h + cf_hinsert(HHDR(h), (uint64_t)k))
#define hadd(h, k, ...) hset(h, k, (__VA_ARGS__))
#define hget(h, k) ((h)[cf_hfind(HHDR(h), (uint64_t)k)])
#define hfind(h, k) hget(h, k)
#define hhas(h, k) (h ? cf_hhas(HHDR(h), (uint64_t)k) : NULL)
#define hdel(h, k) (h ? cf_hdel(HHDR(h), (uint64_t)k) : 0)
#define hclear(h) (HCANARY(h), cf_hclear(HHDR(h)))
#define hkeys(h) (HCANARY(h), (uint64_t*)cf_hkeys(HHDR(h))))
#define hswap(h, index_a, index_b) (HCANARY(h), cf_hswap(HHDR(h), index_a, index_b))
#define hsize(h) (h ? cf_hcount(HHDR(h)) : 0)
#define hcount(h) hsize(h)
#define hfree(h) do { HCANARY(h); cf_hfree(HHDR(h)); h = NULL; } while (0)

//--------------------------------------------------------------------------------------------------
// Hidden API - Not intended for direct use.

typedef struct cf_hslot_t
{
	uint64_t key_hash;
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
	void* items_data;
	void* temp_key;
	void* temp_item;
	uint32_t cookie;
} cf_hhdr_t;

CUTE_API void* CUTE_CALL cf_hmake(int key_size, int item_size, int capacity);
CUTE_API void CUTE_CALL cf_hfree(cf_hhdr_t* table);
CUTE_API int CUTE_CALL cf_hinsert(cf_hhdr_t* table, uint64_t key);
CUTE_API int CUTE_CALL cf_hinsert2(cf_hhdr_t* table, const void* key, const void* item);
CUTE_API void CUTE_CALL cf_hdel(cf_hhdr_t* table, uint64_t key);
CUTE_API void CUTE_CALL cf_hdel2(cf_hhdr_t* table, const void* key);
CUTE_API bool CUTE_CALL cf_hhas(cf_hhdr_t* table, uint64_t key);
CUTE_API int CUTE_CALL cf_hfind(const cf_hhdr_t* table, uint64_t key);
CUTE_API int CUTE_CALL cf_hfind2(const cf_hhdr_t* table, const void* key);
CUTE_API int CUTE_CALL cf_hcount(const cf_hhdr_t* table);
CUTE_API void* CUTE_CALL cf_hitems(const cf_hhdr_t* table);
CUTE_API void* CUTE_CALL cf_hkeys(const cf_hhdr_t* table);
CUTE_API void CUTE_CALL cf_hclear(cf_hhdr_t* table);
CUTE_API void CUTE_CALL cf_hswap(cf_hhdr_t* table, int index_a, int index_b);

#ifdef __cplusplus
}
#endif // __cplusplus

//--------------------------------------------------------------------------------------------------
// C++ API

#ifdef CUTE_CPP

namespace cute
{

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
	m_table = HHDR(cf_hmake(sizeof(K), sizeof(T), 32));
}

template <typename K, typename T>
dictionary<K, T>::dictionary(int capacity)
{
	m_table = HHDR(cf_hmake(sizeof(K), sizeof(T), capacity));
}

template <typename K, typename T>
dictionary<K, T>::~dictionary()
{
	cf_hfree(m_table);
	m_table = NULL;
}

template <typename K, typename T>
T* dictionary<K, T>::find(const K& key)
{
	int index = cf_hfind2(m_table, &key);
	if (index > 0) return items() + index;
	else return NULL;
}

template <typename K, typename T>
const T* dictionary<K, T>::find(const K& key) const
{
	int index = cf_hfind2(m_table, &key);
	if (index > 0) return items() + index;
	else return NULL;
}

template <typename K, typename T>
T* dictionary<K, T>::insert(const K& key)
{
	int index = cf_hinsert(m_table, &key);
	if (index < 0) return NULL;
	T* result = items() + index;
	CUTE_PLACEMENT_NEW(result) T();
	return result;
}

template <typename K, typename T>
T* dictionary<K, T>::insert(const K& key, const T& val)
{
	int index = cf_hinsert2(m_table, &key, &val);
	if (index < 0) return NULL;
	T* result = items() + index;
	CUTE_PLACEMENT_NEW(result) T(val);
	return result;
}

template <typename K, typename T>
T* dictionary<K, T>::insert(const K& key, T&& val)
{
	int index = cf_hinsert2(m_table, &key, &val);
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
		cf_hdel2(m_table, &key);
	}
}

template <typename K, typename T>
void dictionary<K, T>::clear()
{
	cf_hclear(m_table);
}

template <typename K, typename T>
int dictionary<K, T>::count() const
{
	return cf_hcount(m_table);
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
	return (K*)cf_hkeys(m_table);
}

template <typename K, typename T>
const K* dictionary<K, T>::keys() const
{
	return (const K*)cf_hkeys(m_table);
}

template <typename K, typename T>
void dictionary<K, T>::swap(int index_a, int index_b)
{
	cf_hswap(m_table, index_a, index_b);
}

}

#endif // CUTE_CPP

#endif // CUTE_HASHTABLE_H
