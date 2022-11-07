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

#ifndef CUTE_ARRAY_H
#define CUTE_ARRAY_H

#include "cute_defines.h"
#include "cute_c_runtime.h"
#include "cute_alloc.h"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

typedef struct cf_ahdr_t
{
	int size;
	int capacity;
	bool is_static;
	char* data;
	uint32_t cookie;
} cf_ahdr_t;

#define AHDR(a) ((cf_ahdr_t*)a - 1)
#define ACOOKIE 0xE6F7E359
#define ACANARY(a) do { if (a) CUTE_ASSERT(AHDR(a)->cookie == ACOOKIE); } while (0) // Detects buffer underruns.

#define alen(a) (AHDR(a)->size)
#define acount(a) alen(a)
#define asize(a) (a ? alen(a) : 0)
#define acap(a) ((a) ? AHDR(a)->capacity : 0)
#define afit(a, n) ((n) <= acap(a) ? 0 : (*(void**)&(a) = cf_agrow((a), (n), sizeof(*a))))
#define apush(a, ...) do { ACANARY(a); afit((a), 1 + ((a) ? alen(a) : 0)); (a)[alen(a)++] = (__VA_ARGS__); } while (0)
#define apop(a) (a[--alen(a)])
#define afree(a) do { ACANARY(a); if (!AHDR(a)->is_static) CUTE_FREE(AHDR(a)); a = NULL; } while (0)
#define aend(a) (a + asize(a))
#define aclear(a) do { ACANARY(a); if (a) alen(a) = 0; } while (0)
#define aset(a, b) (*(void**)&(a) = cf_aset((void*)(a), (void*)(b), sizeof(*a)))
#define areverse(a) 
#define ahash(a) cf_fnv1a(a, asize(a))
#define astatic(a, buffer, buffer_size) (*(void**)&(a) = cf_astatic(buffer, buffer_size, sizeof(*a)))

CUTE_API void* CUTE_CALL cf_agrow(const void* a, int new_size, size_t element_size);
CUTE_API void* CUTE_CALL cf_astatic(const void* a, int capacity, size_t element_size);
CUTE_API void* CUTE_CALL cf_aset(const void* a, const void* b, size_t element_size);

#ifdef __cplusplus
}
#endif // __cplusplus

#ifdef CUTE_CPP

/**
 * Implements a basic growable array data structure. Constructors and destructors are called, but this
 * class does *not* act as a drop-in replacement for std::vector, as there are no iterators. Your elements
 * CAN NOT store a pointer or reference to themselves or other elements.
 *
 * The main purpose of this class is to reduce the lines of code included compared to std::vector,
 * and also more importantly to have fast debug performance.
 */

namespace cute
{

template <typename T>
struct array
{
	array() { }
	array(cf_initializer_list<T> list);
	array(const array<T>& other);
	array(array<T>&& other);
	array(int capacity);
	~array();

	static array<T> steal_from(array<T>* steal_from_me);
	static array<T> steal_from(array<T>& steal_from_me);
	static array<T> steal_from(T* c_api_array);

	T& add();
	T& add(const T& item);
	T& add(T&& item);
	T pop();
	void unordered_remove(int index);
	void clear();
	void ensure_capacity(int num_elements);
	void ensure_count(int count);
	void set_count(int count);
	void reverse();

	int capacity() const;
	int count() const;
	int size() const;
	bool empty() const { return size() > 0; }

	T* begin();
	const T* begin() const;
	T* end();
	const T* end() const;

	T& operator[](int index);
	const T& operator[](int index) const;

	T* operator+(int index);
	const T* operator+(int index) const;

	const array<T>& operator=(const array<T>& rhs);
	const array<T>& operator=(array<T>&& rhs);

	T& last();
	const T& last() const;

	T* data();
	const T* data() const;

private:
	T* m_ptr = NULL;
};

// -------------------------------------------------------------------------------------------------

template <typename T>
array<T>::array(cf_initializer_list<T> list)
{
	afit(m_ptr, (int)list.size());
	for (const T* i = list.begin(); i < list.end(); ++i) {
		add(*i);
	}
}

template <typename T>
array<T>::array(const array<T>& other)
{
	afit(m_ptr, (int)other.count());
	for (int i = 0; i < other.count(); ++i) {
		add(other[i]);
	}
}

template <typename T>
array<T>::array(array<T>&& other)
{
	steal_from(&other);
}

template <typename T>
array<T>::array(int capacity)
{
	afit(m_ptr, capacity);
}

template <typename T>
array<T>::~array()
{
	for (int i = 0; i < asize(m_ptr); ++i) {
		T* slot = m_ptr + i;
		slot->~T();
	}
	afree(m_ptr);
}

template <typename T>
array<T> array<T>::steal_from(array<T>* steal_from_me)
{
	array<T> result;
	result.m_ptr = steal_from_me->m_ptr;
	steal_from_me->m_ptr = NULL;
	return result;
}

template <typename T>
array<T> array<T>::steal_from(array<T>& steal_from_me)
{
	array<T> result;
	result.m_ptr = steal_from_me.m_ptr;
	steal_from_me.m_ptr = NULL;
	return result;
}

template <typename T>
array<T> array<T>::steal_from(T* c_api_array)
{
	array<T> result;
	result.m_ptr = c_api_array;
	return result;
}

template <typename T>
T& array<T>::add()
{
	afit(m_ptr, asize(m_ptr) + 1);
	T* slot = m_ptr + alen(m_ptr)++;
	CUTE_PLACEMENT_NEW(slot) T;
	return *slot;
}

template <typename T>
T& array<T>::add(const T& item)
{
	afit(m_ptr, asize(m_ptr) + 1);
	T* slot = m_ptr + alen(m_ptr)++;
	CUTE_PLACEMENT_NEW(slot) T(item);
	return *slot;
}

template <typename T>
T& array<T>::add(T&& item)
{
	afit(m_ptr, asize(m_ptr) + 1);
	T* slot = m_ptr + alen(m_ptr)++;
	CUTE_PLACEMENT_NEW(slot) T(move(item));
	return *slot;
}

template <typename T>
T array<T>::pop()
{
	CUTE_ASSERT(!empty());
	T* slot = m_ptr + alen(m_ptr) - 1;
	T val = move(apop(m_ptr));
	slot->~T();
	return val;
}

template <typename T>
void array<T>::unordered_remove(int index)
{
	CUTE_ASSERT(index >= 0 && index < asize(m_ptr));
	T* slot = m_ptr + index;
	slot->~T();
	m_ptr[index] = m_ptr[--alen(m_ptr)];
}

template <typename T>
void array<T>::clear()
{
	aclear(m_ptr);
}

template <typename T>
void array<T>::ensure_capacity(int num_elements)
{
	afit(m_ptr, num_elements);
}

template <typename T>
void array<T>::set_count(int count)
{
	CUTE_ASSERT(count < acap(m_ptr) || !count);
	afit(m_ptr, count);
	if (asize(m_ptr) > count) {
		for (int i = count; i < asize(m_ptr); ++i) {
			T* slot = m_ptr + i;
			slot->~T();
		}
	} else if (asize(m_ptr) < count) {
		for (int i = asize(m_ptr); i < count; ++i) {
			T* slot = m_ptr + i;
			CUTE_PLACEMENT_NEW(slot) T;
		}
	}
	if (m_ptr) alen(m_ptr) = count;
}

template <typename T>
void array<T>::ensure_count(int count)
{
	if (!count) return;
	int old_count = alen(m_ptr);
	afit(m_ptr, count);
	if (alen(m_ptr) < count) {
		alen(m_ptr) = count;
		for (int i = old_count; i < count; ++i) {
			T* slot = m_ptr + i;
			CUTE_PLACEMENT_NEW(slot) T;
		}
	}
}

template <typename T>
void array<T>::reverse()
{
	T* a = m_ptr;
	T* b = m_ptr + (asize(m_ptr) - 1);

	while (a < b) {
		T t = *a;
		*a = *b;
		*b = t;
		++a;
		--b;
	}
}

template <typename T>
int array<T>::capacity() const
{
	return acap(m_ptr);
}

template <typename T>
int array<T>::count() const
{
	return asize(m_ptr);
}

template <typename T>
int array<T>::size() const
{
	return asize(m_ptr);
}

template <typename T>
T* array<T>::begin()
{
	return m_ptr;
}

template <typename T>
const T* array<T>::begin() const
{
	return m_ptr;
}

template <typename T>
T* array<T>::end()
{
	return m_ptr + count();
}

template <typename T>
const T* array<T>::end() const
{
	return m_ptr + count();
}

template <typename T>
T& array<T>::operator[](int index)
{
	CUTE_ASSERT(index >= 0 && index < count());
	return m_ptr[index];
}

template <typename T>
const T& array<T>::operator[](int index) const
{
	CUTE_ASSERT(index >= 0 && index < count());
	return m_ptr[index];
}

template <typename T>
T* array<T>::data()
{
	return m_ptr;
}

template <typename T>
const T* array<T>::data() const
{
	return m_ptr;
}

template <typename T>
T* array<T>::operator+(int index)
{
	CUTE_ASSERT(index >= 0 && index < count());
	return m_ptr + index;
}

template <typename T>
const T* array<T>::operator+(int index) const
{
	CUTE_ASSERT(index >= 0 && index < count());
	return m_ptr + index;
}

template <typename T>
const array<T>& array<T>::operator=(const array<T>& rhs)
{
	set_count(0);
	afit(m_ptr, (int)rhs.count());
	for (int i = 0; i < rhs.count(); ++i) {
		add(rhs[i]);
	}
	return *this;
}

template <typename T>
const array<T>& array<T>::operator=(array<T>&& rhs)
{
	return (*this = steal_from(rhs));
}

template <typename T>
T& array<T>::last()
{
	return *(aend(m_ptr) - 1);
}

template <typename T>
const T& array<T>::last() const
{
	return *(aend(m_ptr) - 1);
}

}

#endif // CUTE_CPP

#endif // CUTE_ARRAY_H
