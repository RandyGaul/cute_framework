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

#ifndef CUTE_DICTIONARY_H
#define CUTE_DICTIONARY_H

#include "cute_defines.h"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#define DICTIONARY_STRING_BLOCK_MAX 128

typedef struct cf_dictionary_string_block_t
{
	size_t len; /*= 0;*/
	uint8_t data[DICTIONARY_STRING_BLOCK_MAX]; /*= { 0 };*/
} cf_dictionary_string_block_t;

CUTE_INLINE cf_dictionary_string_block_t cf_s_dictionary_make_block(const char* key)
{
	cf_dictionary_string_block_t block;
	block.len = CUTE_STRLEN(key);
	CUTE_ASSERT(block.len < DICTIONARY_STRING_BLOCK_MAX - 1);
	CUTE_STRCPY((char*)block.data, key);
	return block;
}

CUTE_INLINE cf_dictionary_string_block_t cf_s_dictionary_make_block_len(const char* key, size_t key_len)
{
	cf_dictionary_string_block_t block;
	block.len = key_len;
	CUTE_ASSERT(block.len < DICTIONARY_STRING_BLOCK_MAX - 1);
	CUTE_STRNCPY((char*)block.data, key, key_len);
	return block;
}

#ifdef __cplusplus
}
#endif // __cplusplus

#ifdef CUTE_CPP

#include "cute_hashtable.h"
#include "cute_c_runtime.h"
#include "cute_result.h"

/**
 * `cf_dictionary` implements a thin type wrapper with templates over a C-style hashtable implementation.
 * Internally no contructors or destructors are ever called, meaning all data stored is considered POD.
 *
 * There is a specialization in the latter portion for const char* keys -- strings are stored in-place
 * inside of blocks of size `DICTIONARY_STRING_BLOCK_MAX`. The implementation will break if any key is
 * ever longer than this max. This limitation is here to keep the implementation and cache behavior
 * as simple as possible.
 */

template <typename K, typename T>
struct cf_dictionary
{
	cf_dictionary();
	cf_dictionary(void* user_allocator_context);
	cf_dictionary(int capacity, void* user_allocator_context);
	~cf_dictionary();

	T* find(const K& key);
	const T* find(const K& key) const;
	cf_result_t find(const K& key, T* val_out);
	cf_result_t find(const K& key, T* val_out) const;

	T* insert(const K& key);
	T* insert(const K& key, const T& val);
	void remove(const K& key);

	void clear();

	int count() const;
	T* items();
	const T* items() const;
	K* keys();
	const K* keys() const;

	void swap(int index_a, int index_b);

private:
	cf_hashtable_t m_table;
};

// -------------------------------------------------------------------------------------------------

template <typename K, typename T>
cf_dictionary<K, T>::cf_dictionary()
{
	cf_hashtable_init(&m_table, sizeof(K), sizeof(T), 256, NULL);
}

template <typename K, typename T>
cf_dictionary<K, T>::cf_dictionary(void* user_allocator_context)
{
	cf_hashtable_init(&m_table, sizeof(K), sizeof(T), 256, user_allocator_context);
}

template <typename K, typename T>
cf_dictionary<K, T>::cf_dictionary(int capacity, void* user_allocator_context)
{
	cf_hashtable_init(&m_table, sizeof(K), sizeof(T), capacity, user_allocator_context);
}

template <typename K, typename T>
cf_dictionary<K, T>::~cf_dictionary()
{
	T* items_ptr = items();
	int items_count = count();
	for (int i = 0; i < items_count; ++i) (items_ptr + i)->~T();
	cf_hashtable_cleanup(&m_table);
}

template <typename K, typename T>
T* cf_dictionary<K, T>::find(const K& key)
{
	return (T*)cf_hashtable_find(&m_table, &key);
}

template <typename K, typename T>
const T* cf_dictionary<K, T>::find(const K& key) const
{
	return (const T*)cf_hashtable_find(&m_table, &key);
}

template <typename K, typename T>
cf_result_t cf_dictionary<K, T>::find(const K& key, T* val_out)
{
	T* ptr = (T*)cf_hashtable_find(&m_table, &key);
	if (ptr) {
		*val_out = *ptr;
		return cf_result_success();
	} else {
		return cf_result_error("Unable to find dictionary entry.");
	}
}

template <typename K, typename T>
cf_result_t cf_dictionary<K, T>::find(const K& key, T* val_out) const
{
	const T* ptr = (const T*)cf_hashtable_find(&m_table, &key);
	if (ptr) {
		*val_out = *ptr;
		return cf_result_success();
	} else {
		return cf_result_error("Unable to find dictionary entry.");
	}
}

template <typename K, typename T>
T* cf_dictionary<K, T>::insert(const K& key)
{
	T* slot = (T*)cf_hashtable_insert(&m_table, &key, NULL);
	CUTE_PLACEMENT_NEW(slot) T();
	return slot;
}

template <typename K, typename T>
T* cf_dictionary<K, T>::insert(const K& key, const T& val)
{
	T* slot = (T*)cf_hashtable_insert(&m_table, &key, &val);
	CUTE_PLACEMENT_NEW(slot) T(val);
	return slot;
}

template <typename K, typename T>
void cf_dictionary<K, T>::remove(const K& key)
{
	T* slot = find(key);
	slot->~T();
	cf_hashtable_remove(&m_table, &key);
}

template <typename K, typename T>
void cf_dictionary<K, T>::clear()
{
	cf_hashtable_clear(&m_table);
}

template <typename K, typename T>
int cf_dictionary<K, T>::count() const
{
	return cf_hashtable_count(&m_table);
}

template <typename K, typename T>
T* cf_dictionary<K, T>::items()
{
	return (T*)cf_hashtable_items(&m_table);
}

template <typename K, typename T>
const T* cf_dictionary<K, T>::items() const
{
	return (const T*)cf_hashtable_items(&m_table);
}

template <typename K, typename T>
K* cf_dictionary<K, T>::keys()
{
	return (K*)cf_hashtable_keys(&m_table);
}

template <typename K, typename T>
const K* cf_dictionary<K, T>::keys() const
{
	return (const K*)cf_hashtable_keys(&m_table);
}

template <typename K, typename T>
void cf_dictionary<K, T>::swap(int index_a, int index_b)
{
	cf_hashtable_swap(&m_table, index_a, index_b);
}

// -------------------------------------------------------------------------------------------------
// const char* specialization
// Forces all strings to be less than `DICTIONARY_STRING_BLOCK_MAX` characters, asserts otherwise.
// The limitation is for simplicity of implementation.

template <typename T>
struct cf_dictionary<const char*, T>
{
	cf_dictionary();
	cf_dictionary(void* user_allocator_context);
	cf_dictionary(int capacity, void* user_allocator_context);
	~cf_dictionary();

	T* find(const char* key);
	const T* find(const char* key) const;
	T* find(const char* key, size_t key_len);
	const T* find(const char* key, size_t key_len) const;
	cf_result_t find(const char* key, T* val_out);
	cf_result_t find(const char* key, T* val_out) const;
	cf_result_t find(const char* key, size_t key_len, T* val_out);
	cf_result_t find(const char* key, size_t key_len, T* val_out) const;

	T* insert(const char* key, const T& val);
	void remove(const char* key);
	T* insert(const char* key, size_t key_len, const T& val);
	void remove(const char* key, size_t key_len);

	void clear();

	int count() const;
	T* items();
	const T* items() const;
	cf_dictionary_string_block_t* keys();
	const cf_dictionary_string_block_t* keys() const;

	void swap(int index_a, int index_b);

private:
	cf_hashtable_t m_table;
};

template <typename T>
cf_dictionary<const char*, T>::cf_dictionary()
{
	cf_hashtable_init(&m_table, sizeof(cf_dictionary_string_block_t), sizeof(T), 256, NULL);
}

template <typename T>
cf_dictionary<const char*, T>::cf_dictionary(void* user_allocator_context)
{
	cf_hashtable_init(&m_table, sizeof(cf_dictionary_string_block_t), sizeof(T), 256, user_allocator_context);
}

template <typename T>
cf_dictionary<const char*, T>::cf_dictionary(int capacity, void* user_allocator_context)
{
	cf_hashtable_init(&m_table, sizeof(cf_dictionary_string_block_t), sizeof(T), capacity, user_allocator_context);
}

template <typename T>
cf_dictionary<const char*, T>::~cf_dictionary()
{
	cf_hashtable_cleanup(&m_table);
}

template <typename T>
T* cf_dictionary<const char*, T>::find(const char* key)
{
	cf_dictionary_string_block_t block = cf_s_dictionary_make_block(key);
	return (T*)cf_hashtable_find(&m_table, &block);
}

template <typename T>
const T* cf_dictionary<const char*, T>::find(const char* key) const
{
	cf_dictionary_string_block_t block = cf_s_dictionary_make_block(key);
	return (T*)cf_hashtable_find(&m_table, &block);
}

template <typename T>
T* cf_dictionary<const char*, T>::find(const char* key, size_t key_len)
{
	cf_dictionary_string_block_t block = cf_s_dictionary_make_block(key, key_len);
	return (T*)cf_hashtable_find(&m_table, &block);
}

template <typename T>
const T* cf_dictionary<const char*, T>::find(const char* key, size_t key_len) const
{
	cf_dictionary_string_block_t block = cf_s_dictionary_make_block(key, key_len);
	return (T*)cf_hashtable_find(&m_table, &block);
}

template <typename T>
cf_result_t cf_dictionary<const char*, T>::find(const char* key, T* val_out)
{
	cf_dictionary_string_block_t block = cf_s_dictionary_make_block(key);
	T* ptr = (T*)cf_hashtable_find(&m_table, &block);
	if (ptr) {
		*val_out = *ptr;
		return cf_result_success();
	} else {
		return cf_result_error("Unable to find dictionary entry.");
	}
}

template <typename T>
cf_result_t cf_dictionary<const char*, T>::find(const char* key, T* val_out) const
{
	cf_dictionary_string_block_t block = cf_s_dictionary_make_block(key);
	T* ptr = (T*)cf_hashtable_find(&m_table, &block);
	if (ptr) {
		*val_out = *ptr;
		return cf_result_success();
	} else {
		return cf_result_error("Unable to find dictionary entry.");
	}
}

template <typename T>
cf_result_t cf_dictionary<const char*, T>::find(const char* key, size_t key_len, T* val_out)
{
	T* ptr = find(key, key_len);
	if (ptr) {
		*val_out = *ptr;
		return cf_result_success();
	} else {
		return cf_result_error("Unable to find dictionary entry.");
	}
}

template <typename T>
cf_result_t cf_dictionary<const char*, T>::find(const char* key, size_t key_len, T* val_out) const
{
	const T* ptr = find(key, key_len);
	if (ptr) {
		*val_out = *ptr;
		return cf_result_success();
	} else {
		return cf_result_error("Unable to find dictionary entry.");
	}
}

template <typename T>
T* cf_dictionary<const char*, T>::insert(const char* key, const T& val)
{
	cf_dictionary_string_block_t block = cf_s_dictionary_make_block(key);
	return (T*)cf_hashtable_insert(&m_table, &block, &val);
}

template <typename T>
void cf_dictionary<const char*, T>::remove(const char* key, size_t key_len)
{
	cf_dictionary_string_block_t block = cf_s_dictionary_make_block(key, key_len);
	cf_hashtable_remove(&m_table, &block);
}

template <typename T>
T* cf_dictionary<const char*, T>::insert(const char* key, size_t key_len, const T& val)
{
	cf_dictionary_string_block_t block = cf_s_dictionary_make_block(key, key_len);
	return (T*)cf_hashtable_insert(&m_table, &block, &val);
}

template <typename T>
void cf_dictionary<const char*, T>::remove(const char* key)
{
	cf_dictionary_string_block_t block = cf_s_dictionary_make_block(key);
	cf_hashtable_remove(&m_table, &block);
}

template <typename T>
void cf_dictionary<const char*, T>::clear()
{
	cf_hashtable_clear(&m_table);
}

template <typename T>
int cf_dictionary<const char*, T>::count() const
{
	return cf_hashtable_count(&m_table);
}

template <typename T>
T* cf_dictionary<const char*, T>::items()
{
	return (T*)cf_hashtable_items(&m_table);
}

template <typename T>
const T* cf_dictionary<const char*, T>::items() const
{
	return (const T*)cf_hashtable_items(&m_table);
}

template <typename T>
cf_dictionary_string_block_t* cf_dictionary<const char*, T>::keys()
{
	return (cf_dictionary_string_block_t*)cf_hashtable_keys(&m_table);
}

template <typename T>
const cf_dictionary_string_block_t* cf_dictionary<const char*, T>::keys() const
{
	return (const cf_dictionary_string_block_t*)cf_hashtable_keys(&m_table);
}

template <typename T>
void cf_dictionary<const char*, T>::swap(int index_a, int index_b)
{
	cf_hashtable_swap(&m_table, index_a, index_b);
}

namespace cute
{
template <typename K, typename T> using dictionary = cf_dictionary<K, T>;
}

#endif // CUTE_CPP

#endif // CUTE_DICTIONARY_H
