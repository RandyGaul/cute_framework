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

//--------------------------------------------------------------------------------------------------
// C API

#ifndef CUTE_NO_SHORTHAND_API
/**
 * This is *optional* and _completely_ empty macro. It's only purpose is to provide a bit of visual
 * indication a type is a dynamic array. One downside of the C-macro API is the opaque nature of the pointer
 * type. Since the macros use polymorphism on typed pointers, there's no actual array struct type.
 * 
 * It can get really annoying to sometimes forget if a pointer is an array, a hashtable, or just a
 * pointer. This macro can be used to markup the type to make it much more clear for function parameters
 * or struct member definitions. It's saying "Hey, I'm a dynamic array!" to mitigate this downside.
 */
#define dyna

/**
 * Gets the number of elements in the array. Must not be NULL.
 * It's a proper l-value so you can assign or increment it.
 * 
 * Example:
 * 
 *     int* a = NULL;
 *     apush(a, 5);
 *     CUTE_ASSERT(alen(a) == 1);
 *     alen(a)--;
 *     CUTE_ASSERT(alen(a) == 0);
 *     afree(a);
 */
#define alen(a) cf_array_len(a)

/**
 * Gets the number of elements in the array. Can be NULL.
 * 
 * Example:
 * 
 *     int* a = NULL;
 *     apush(a, 5);
 *     CUTE_ASSERT(asize(a) == 1);
 *     afree(a);
 */
#define asize(a) cf_array_size(a)

/**
 * Gets the number of elements in the array. Can be NULL.
 * 
 * Example:
 * 
 *     int* a = NULL;
 *     apush(a, 5);
 *     CUTE_ASSERT(acount(a) == 1);
 *     afree(a);
 */
#define acount(a) cf_array_count(a)

/**
 * Gets the capacity of the array. The capacity automatically grows if the size
 * of the array grows over the capacity. You can use `afit` to ensure a minimum
 * capacity as an optimization.
 */
#define acap(a) cf_array_capacity(a)

/**
 * Ensures the capacity of the array is at least n elements.
 * Does not change the size/count of the array.
 */
#define afit(a, n) cf_array_fit(a, n)

/**
 * Pushes an element onto the back of the array.
 * 
 * a    - The array. If NULL a new array will get allocated and returned.
 * ...  - The value to push onto the back of the array.
 * 
 * Example:
 * 
 *     int* a = NULL;
 *     apush(a, 5);
 *     apush(a, 13);
 *     CUTE_ASSERT(a[0] == 5);
 *     CUTE_ASSERT(a[1] == 13);
 *     CUTE_ASSERT(asize(a) == 2);
 *     afree(a);
 */
#define apush(a, ...) cf_array_push(a, (__VA_ARGS__))

/**
 * Pops and returns an element off the back of the array. Cannot be NULL.
 */
#define apop(a) cf_array_pop(a)

/**
 * Returns a pointer one element beyond the end of the array.
 */
#define aend(a) cf_array_end(a)

/**
 * Returns the last element in the array.
 */
#define alast(a) cf_array_last(a)

/**
 * Sets the array's count to zero. Does not free any resources.
 */
#define aclear(a) cf_array_clear(a)

/**
 * Copies the array b into array a. Will automatically fit a if needed.
 */
#define aset(a, b) cf_array_set(a, b)

/**
 * Returns the hash of all the bytes in the array.
 */
#define ahash(a) cf_array_hash(a)

/**
 * Creates an array with an initial static storage backing. Will grow onto the heap
 * if the size becomes too large.
 * 
 * a           - A typed pointer, can be NULL. Will be assigned + returnde back to you.
 * buffer      - Pointer to a static memory buffer.
 * buffer_size - The size of `buffer` in bytes.
 */
#define astatic(a, buffer, buffer_size) cf_array_static(a, buffer, buffer_size)

/**
 * Frees up all resources used by the array. Sets a to NULL.
 */
#define afree(a) cf_array_free(a)
#endif // CUTE_NO_SHORTHAND_API

//--------------------------------------------------------------------------------------------------
// Longform C API.

#define cf_array_len(a) (CF_ACANARY(a), CF_AHDR(a)->size)
#define cf_array_size(a) (a ? cf_array_len(a) : 0)
#define cf_array_count(a) cf_array_size(a)
#define cf_array_capacity(a) ((a) ? CF_AHDR(a)->capacity : 0)
#define cf_array_fit(a, n) ((n) <= cf_array_capacity(a) ? 0 : (*(void**)&(a) = cf_agrow((a), (n), sizeof(*a))))
#define cf_array_push(a, ...) (CF_ACANARY(a), cf_array_fit((a), 1 + ((a) ? cf_array_len(a) : 0)), (a)[cf_array_len(a)++] = (__VA_ARGS__))
#define cf_array_pop(a) (a[--cf_array_len(a)])
#define cf_array_end(a) (a + cf_array_size(a))
#define cf_array_last(a) (a[cf_array_len(a) - 1])
#define cf_array_clear(a) (CF_ACANARY(a), (a) ? cf_array_len(a) = 0 : 0)
#define cf_array_set(a, b) (*(void**)&(a) = cf_aset((void*)(a), (void*)(b), sizeof(*a)))
#define cf_array_hash(a) cf_fnv1a(a, cf_array_size(a))
#define cf_array_static(a, buffer, buffer_size) (*(void**)&(a) = cf_astatic(buffer, buffer_size, sizeof(*a)))
#define cf_array_free(a) do { CF_ACANARY(a); if (a && !CF_AHDR(a)->is_static) CUTE_FREE(CF_AHDR(a)); a = NULL; } while (0)

//--------------------------------------------------------------------------------------------------
// Hidden API - Not intended for direct use.

#define CF_AHDR(a) ((CF_Ahdr*)a - 1)
#define CF_ACOOKIE 0xE6F7E359
#define CF_ACANARY(a) ((a) ? CUTE_ASSERT(CF_AHDR(a)->cookie == CF_ACOOKIE) : (void)0) // Detects buffer underruns.

// *Hidden* array header.
typedef struct CF_Ahdr
{
	int size;
	int capacity;
	bool is_static;
	char* data;
	uint32_t cookie;
} CF_Ahdr;

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

CUTE_API void* CUTE_CALL cf_agrow(const void* a, int new_size, size_t element_size);
CUTE_API void* CUTE_CALL cf_astatic(const void* a, int capacity, size_t element_size);
CUTE_API void* CUTE_CALL cf_aset(const void* a, const void* b, size_t element_size);

#ifdef __cplusplus
}
#endif // __cplusplus

//--------------------------------------------------------------------------------------------------
// C++ API

#ifdef CUTE_CPP

/**
 * Implements a basic growable array data structure.
 *
 * The main purpose of this class is to reduce the lines of code included compared to std::vector,
 * and also more importantly to have fast debug performance via manual inlining.
 */

namespace Cute
{

template <typename T>
struct Array
{
	Array() { }
	Array(CF_InitializerList<T> list);
	Array(const Array<T>& other);
	Array(Array<T>&& other);
	Array(int capacity);
	~Array();

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
	bool empty() const;

	T* begin();
	const T* begin() const;
	T* end();
	const T* end() const;

	T& operator[](int index);
	const T& operator[](int index) const;

	T* operator+(int index);
	const T* operator+(int index) const;

	Array<T>& operator=(const Array<T>& rhs);
	Array<T>& operator=(Array<T>&& rhs);
	Array<T>& steal_from(Array<T>* steal_from_me);
	Array<T>& steal_from(Array<T>& steal_from_me);

	T& last();
	const T& last() const;

	T* data();
	const T* data() const;

private:
	int m_capacity = 0;
	int m_count = 0;
	T* m_ptr = NULL;
};

// -------------------------------------------------------------------------------------------------

// Manually force-inline a few functions via macros to help with debug performance.

#define CF_ARRAY_ENSURE_CAPACITY(capacity)                     \
	int num_elements = capacity;                               \
	if (num_elements > m_capacity) {                           \
		if (m_capacity == 0) {                                 \
			m_capacity = 8;                                    \
		}                                                      \
		while (num_elements < m_capacity) {                    \
			m_capacity *= 2;                                   \
		}                                                      \
		T* new_ptr = (T*)CUTE_ALLOC(sizeof(T) * num_elements); \
		for (int i = 0; i < m_count; ++i) {                    \
			CUTE_PLACEMENT_NEW(new_ptr + i) T(move(m_ptr[i])); \
		}                                                      \
		CUTE_FREE(m_ptr);                                      \
		m_ptr = new_ptr;                                       \
	}                                                          \

#define CF_ARRAY_CLEAR()                \
	for (int i = 0; i < m_count; i++) { \
		m_ptr[i].~T();                  \
	}                                   \
	m_count = 0;                        \

template <typename T>
Array<T>::Array(CF_InitializerList<T> list)
{
	int count = (int)list.size();
	T* ptr = m_ptr;
	CF_ARRAY_ENSURE_CAPACITY(count);
	for (const T* i = list.begin(); i < list.end(); ++i) {
		CUTE_PLACEMENT_NEW(ptr++) T(*i);
	}
	m_count = count;
}

template <typename T>
Array<T>::Array(const Array<T>& other)
{
	int count = other.count();
	T* other_ptr = other.m_ptr;
	CF_ARRAY_ENSURE_CAPACITY(count);
	for (int i = 0; i < count; ++i) {
		CUTE_PLACEMENT_NEW(m_ptr + i) T(other_ptr[i]);
	}
	m_count = count;
}

template <typename T>
Array<T>::Array(Array<T>&& other)
{
	m_capacity = other.m_capacity;
	m_count = other.m_count;
	m_ptr = other.m_ptr;
	CUTE_MEMSET(&other, 0, sizeof(other));
}

template <typename T>
Array<T>::Array(int capacity)
{
	CF_ARRAY_ENSURE_CAPACITY(capacity);
}

template <typename T>
Array<T>::~Array()
{
	for (int i = 0; i < m_count; ++i) {
		m_ptr[i].~T();
	}
	CUTE_FREE(m_ptr);
}

template <typename T>
Array<T>& Array<T>::steal_from(Array<T>* steal_from_me)
{
	m_capacity = steal_from_me->m_capacity;
	m_count = steal_from_me->m_count;
	m_ptr = steal_from_me->m_ptr;
	CUTE_MEMSET(steal_from_me, 0, sizeof(*steal_from_me));
	return *this;
}

template <typename T>
Array<T>& Array<T>::steal_from(Array<T>& steal_from_me)
{
	m_capacity = steal_from_me.m_capacity;
	m_count = steal_from_me.m_count;
	m_ptr = steal_from_me.m_ptr;
	CUTE_MEMSET(&steal_from_me, 0, sizeof(steal_from_me));
	return *this;
}

template <typename T>
T& Array<T>::add()
{
	CF_ARRAY_ENSURE_CAPACITY(m_count + 1);
	return *CUTE_PLACEMENT_NEW(m_ptr + m_count++) T();
}

template <typename T>
T& Array<T>::add(const T& item)
{
	CF_ARRAY_ENSURE_CAPACITY(m_count + 1);
	return *CUTE_PLACEMENT_NEW(m_ptr + m_count++) T(item);
}

template <typename T>
T& Array<T>::add(T&& item)
{
	CF_ARRAY_ENSURE_CAPACITY(m_count + 1);
	return *CUTE_PLACEMENT_NEW(m_ptr + m_count++) T(move(item));
}

template <typename T>
T Array<T>::pop()
{
	CUTE_ASSERT(m_count > 0);
	T val = move(m_ptr[m_count - 1]);
	m_ptr[m_count - 1].~T();
	m_count--;
	return val;
}

template <typename T>
void Array<T>::unordered_remove(int index)
{
	m_ptr[index].~T();
	m_ptr[index] = move(m_ptr[--m_count]);
}

template <typename T>
void Array<T>::clear()
{
	CF_ARRAY_CLEAR();
}

template <typename T>
void Array<T>::set_count(int count)
{
	CF_ARRAY_ENSURE_CAPACITY(count);
	if (count > m_count) {
		for (int i = count; ++i; i < m_count) {
			CUTE_PLACEMENT_NEW(m_ptr + i) T();
		}
	} else {
		for (int i = m_count; i < count; ++i) {
			m_ptr[i].~T();
		}
	}
	m_count = count;
}

template <typename T>
void Array<T>::ensure_capacity(int capacity)
{
	CF_ARRAY_ENSURE_CAPACITY(capacity);
}

template <typename T>
void Array<T>::ensure_count(int count)
{
	if (m_count < count) {
		for (int i = m_count; i < count; ++i) {
			CUTE_PLACEMENT_NEW(m_ptr + i) T();
		}
	}
	m_count = count;
}

template <typename T>
void Array<T>::reverse()
{
	T* a = m_ptr;
	T* b = m_ptr + (m_count - 1);

	while (a < b) {
		T t = move(*a);
		*a = move(*b);
		*b = move(t);
		++a;
		--b;
	}
}

template <typename T>
int Array<T>::capacity() const
{
	return m_capacity;
}

template <typename T>
int Array<T>::count() const
{
	return m_count;
}

template <typename T>
int Array<T>::size() const
{
	return m_count;
}

template <typename T>
bool Array<T>::empty() const
{
	return m_count == 0;
}

template <typename T>
T* Array<T>::begin()
{
	return m_ptr;
}

template <typename T>
const T* Array<T>::begin() const
{
	return m_ptr;
}

template <typename T>
T* Array<T>::end()
{
	return m_ptr + m_count;
}

template <typename T>
const T* Array<T>::end() const
{
	return m_ptr + m_count;
}

template <typename T>
T& Array<T>::operator[](int index)
{
	CUTE_ASSERT(index >= 0 && index < m_count);
	return m_ptr[index];
}

template <typename T>
const T& Array<T>::operator[](int index) const
{
	CUTE_ASSERT(index >= 0 && index < m_count);
	return m_ptr[index];
}

template <typename T>
T* Array<T>::data()
{
	return m_ptr;
}

template <typename T>
const T* Array<T>::data() const
{
	return m_ptr;
}

template <typename T>
T* Array<T>::operator+(int index)
{
	CUTE_ASSERT(index >= 0 && index < m_count);
	return m_ptr + index;
}

template <typename T>
const T* Array<T>::operator+(int index) const
{
	CUTE_ASSERT(index >= 0 && index < m_count);
	return m_ptr + index;
}

template <typename T>
Array<T>& Array<T>::operator=(const Array<T>& rhs)
{
	CF_ARRAY_CLEAR();
	CF_ARRAY_ENSURE_CAPACITY(rhs.m_count);
	for (int i = 0; i < rhs.m_count; i++) {
		CUTE_PLACEMENT_NEW(m_ptr + i) T(rhs.m_ptr[i]);
	}
	m_count = rhs.m_count;
	return *this;
}

template <typename T>
Array<T>& Array<T>::operator=(Array<T>&& rhs)
{
	CF_ARRAY_CLEAR();
	m_capacity = rhs.m_capacity;
	m_count = rhs.m_count;
	m_ptr = rhs.m_ptr;
	CUTE_MEMSET(&rhs, 0, sizeof(rhs));
	return *this;
}

template <typename T>
T& Array<T>::last()
{
	return *(m_ptr + m_count - 1);
}

template <typename T>
const T& Array<T>::last() const
{
	return *(m_ptr + m_count - 1);
}

}

#endif // CUTE_CPP

#endif // CUTE_ARRAY_H
