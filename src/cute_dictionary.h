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

#include <cute_defines.h>
#include <cute_hashtable.h>

namespace cute
{

template <typename T, typename K>
struct dictionary
{
	dictionary();
	dictionary(void* user_allocator_context);
	dictionary(int capacity, void* user_allocator_context);

	T* find(const K& key);
	const T* find(const K& key) const;

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
	hashtable_t table;
};

// -------------------------------------------------------------------------------------------------

template <typename K, typename T>
dictionary<K, T>::dictionary()
{
	hashtable_init(&table, sizeof(T), sizeof(K), 256, NULL);
}

template <typename K, typename T>
dictionary<K, T>::dictionary(void* user_allocator_context)
{
	hashtable_init(&table, sizeof(T), sizeof(K), 256, user_allocator_context);
}

template <typename K, typename T>
dictionary<K, T>::dictionary(int capacity, void* user_allocator_context)
{
	hashtable_init(&table, sizeof(T), sizeof(K), capacity, user_allocator_context);
}

template <typename K, typename T>
T* dictionary<K, T>::find(const K& key)
{
	return (T*)hashtable_find(&table, &key);
}

template <typename K, typename T>
const T* dictionary<K, T>::find(const K& key) const
{
	return (const T*)hashtable_find(&table, &key);
}

template <typename K, typename T>
T* dictionary<K, T>::insert(const K& key, const T& val)
{
	return (T*)hashtable_insert(&table, &key, &val);
}

template <typename K, typename T>
void dictionary<K, T>::remove(const K& key)
{
	hashtable_remove(&table, &key);
}

template <typename K, typename T>
void dictionary<K, T>::clear()
{
	hashtable_clear(&table);
}

template <typename K, typename T>
int dictionary<K, T>::count() const
{
	return hashtable_count(&table);
}

template <typename K, typename T>
T* dictionary<K, T>::items()
{
	return (T*)hashtable_items(&table);
}

template <typename K, typename T>
const T* dictionary<K, T>::items() const
{
	return (const T*)hashtable_items(&table);
}

template <typename K, typename T>
K* dictionary<K, T>::keys()
{
	return (K*)hashtable_keys(&table);
}

template <typename K, typename T>
const K* dictionary<K, T>::keys() const
{
	return (const K*)hashtable_keys(&table);
}

template <typename K, typename T>
void dictionary<K, T>::swap(int index_a, int index_b)
{
	hashtable_swap(&table, index_a, index_b);
}

}

#endif // CUTE_DICTIONARY_H
