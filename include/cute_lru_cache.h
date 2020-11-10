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

#ifndef CUTE_LRU_CACHE_H
#define CUTE_LRU_CACHE_H

#include <cute_dictionary.h>
#include <cute_doubly_list.h>

namespace cute
{

/**
 * Implements a least-recently-used cache. Not particularly well tested. This is here to
 * potentially implement an upper limit in RAM consumption for png/aseprite caches, but is
 * not yet used within Cute Framework.
 * 
 * Use at your own risk.
 */

template <typename K, typename T>
struct lru_cache
{
	lru_cache(int capacity, void* user_allocator_context);
	~lru_cache();

	T* mru();
	T* lru();

	T* find(const K& key);
	error_t find(const K& key, T* val_out);

	T* insert(const K& key);
	T* insert(const K& key, const T& val);
	void remove(const K& key);
	void clear();

	int count() const;
	list_t* list();
	const list_t* list() const;

	static T* node_to_item(list_node_t* node);
	static const T* node_to_item(const list_node_t* node);

private:
	struct entry_t
	{
		list_node_t node;
		K key;
		T item;
	};

	int m_capacity;
	int m_count;
	list_t m_list;
	dictionary<K, entry_t> m_entries;

	void update(list_node_t* node);
};

// -------------------------------------------------------------------------------------------------

template <typename K, typename T>
lru_cache<K, T>::lru_cache(int capacity, void* user_allocator_context)
	: m_capacity(capacity)
	, m_count(0)
	, m_entries(capacity, user_allocator_context)
{
	list_init(&m_list);
}

template <typename K, typename T>
lru_cache<K, T>::~lru_cache()
{
}

template <typename K, typename T>
T* lru_cache<K, T>::mru()
{
	if (m_count) {
		list_node_t* mru = list_front(&m_list);
		return node_to_item(mru);
	} else {
		return NULL;
	}
}

template <typename K, typename T>
T* lru_cache<K, T>::lru()
{
	if (m_count) {
		list_node_t* lru = list_back(&m_list);
		return node_to_item(lru);
	} else {
		return NULL;
	}
}

template <typename K, typename T>
T* lru_cache<K, T>::find(const K& key)
{
	entry_t* entry = m_entries.find(key);
	if (!entry) return NULL;
	update(&entry->node);
	return &entry->item;
}

template <typename K, typename T>
error_t lru_cache<K, T>::find(const K& key, T* val_out)
{
	entry_t* entry = m_entries.find(key);
	if (!entry) error_failure("Unable to find dictionary entry.");
	update(&entry->node);
	*val_out = entry->item;
	return error_success();
}

template <typename K, typename T>
T* lru_cache<K, T>::insert(const K& key)
{
	if (m_count < m_capacity) {
		m_count++;
	} else {
		list_node_t* lru = list_back(&m_list);
		list_remove(lru);
		entry_t* entry = CUTE_LIST_HOST(entry_t, node, lru);
		m_entries.remove(entry->key);
	}

	entry_t* entry = m_entries.insert(key);
	entry->key = key;
	list_init_node(&entry->node);
	list_push_front(&m_list, &entry->node);
	return &entry->item;
}

template <typename K, typename T>
T* lru_cache<K, T>::insert(const K& key, const T& val)
{
	if (m_count < m_capacity) {
		m_count++;
	} else {
		list_node_t* lru = list_back(&m_list);
		list_remove(lru);
		entry_t* entry = CUTE_LIST_HOST(entry_t, node, lru);
		m_entries.remove(entry->key);
	}

	entry_t* entry = m_entries.insert(key);
	entry->key = key;
	entry->item = val;
	list_init_node(&entry->node);
	list_push_front(&m_list, &entry->node);
	return &entry->item;
}

template <typename K, typename T>
void lru_cache<K, T>::remove(const K& key)
{
	entry_t* entry = m_entries.find(key);
	if (entry) {
		CUTE_ASSERT(m_count > 0);
		m_count--;
		list_remove(&entry->node);
		m_entries.remove(key);
	}
}

template <typename K, typename T>
void lru_cache<K, T>::clear()
{
	m_count = 0;
	list_init(&m_list);
	m_entries.clear();
}

template <typename K, typename T>
int lru_cache<K, T>::count() const
{
	return m_count;
}

template <typename K, typename T>
list_t* lru_cache<K, T>::list()
{
	return &m_list;
}

template <typename K, typename T>
const list_t* lru_cache<K, T>::list() const
{
	return &m_list;
}

template <typename K, typename T>
T* lru_cache<K, T>::node_to_item(list_node_t* node)
{
	entry_t* entry = CUTE_LIST_HOST(entry_t, node, node);
	return &entry->item;
}

template <typename K, typename T>
const T* lru_cache<K, T>::node_to_item(const list_node_t* node)
{
	entry_t* entry = CUTE_LIST_HOST(entry_t, node, node);
	return &entry->item;
}

// -------------------------------------------------------------------------------------------------

template <typename K, typename T>
void lru_cache<K, T>::update(list_node_t* node)
{
	list_remove(node);
	list_push_front(&m_list, node);
}

}

#endif // CUTE_LRU_CACHE_H
