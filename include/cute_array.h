/*
	Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

#ifndef CF_ARRAY_H
#define CF_ARRAY_H

#include "cute_defines.h"
#include "cute_c_runtime.h"
#include "cute_alloc.h"
#include "cute/ckit.h"

// Shortform array macros (apush, adel, etc.) are provided by ckit.h.
// You may use them directly if you wish, or use the cf_ longform prefixes below.

//--------------------------------------------------------------------------------------------------
// C API

/**
 * @function dyna
 * @category array
 * @brief    An empty macro used in the C API to markup dynamic arrays.
 * @example > Creating a dynamic array, pushing some elements into the array, and freeing it up afterwards.
 *           dyna int* a = NULL;
 *           apush(a, 5);
 *           CF_ASSERT(asize(a) == 1);
 *           asetlen(a, 0);
 *           CF_ASSERT(asize(a) == 0);
 *           afree(a);
 * @remarks  This is an optional and _completely_ empty macro. It's only purpose is to provide a bit of visual indication a type is a
 *           dynamic array. One downside of the C-macro API is the opaque nature of the pointer type. Since the macros use polymorphism
 *           on typed pointers, there's no actual array struct type. It can get really annoying to sometimes forget if a pointer is an
 *           array, a hashtable, or just a pointer. This macro can be used to markup the type to make it much more clear for function
 *           parameters or struct member definitions. It's saying "Hey, I'm a dynamic array!" to mitigate this downside.
 * @related  dyna cf_array_size cf_array_push cf_array_pop cf_array_free cf_array_hash
 */
#define dyna CK_DYNA

// Shortform macros (asize, acount, acap, afit, apush, apop, aend, alast, aclear,
// asetlen, aset, arev, ahash, adel, astatic, afree) are provided by ckit.h.

//--------------------------------------------------------------------------------------------------
// Longform C API.
// These map cf_array_* names to the shortform ckit macros.

/**
 * @function cf_array_size
 * @category array
 * @brief    Returns the number of elements in the array, or 0 if NULL.
 * @param    a             The array.
 * @return   Returns the number of elements in `a`.
 * @remarks  Shortform: `asize(a)`.
 * @related  cf_array_size cf_array_count cf_array_capacity cf_array_fit cf_array_push cf_array_pop cf_array_free
 */
#define cf_array_size(a) asize(a)

/**
 * @function cf_array_count
 * @category array
 * @brief    Returns the number of elements in the array, or 0 if NULL. Same as `cf_array_size`.
 * @param    a             The array.
 * @return   Returns the number of elements in `a`.
 * @remarks  Shortform: `acount(a)`.
 * @related  cf_array_size cf_array_size cf_array_capacity cf_array_fit cf_array_push cf_array_pop cf_array_free
 */
#define cf_array_count(a) acount(a)

/**
 * @function cf_array_capacity
 * @category array
 * @brief    Returns the capacity of the array (number of elements allocated), or 0 if NULL.
 * @param    a             The array.
 * @return   Returns the number of elements `a` can hold before the next reallocation.
 * @remarks  Shortform: `acap(a)`.
 * @related  cf_array_size cf_array_fit cf_array_push cf_array_free
 */
#define cf_array_capacity(a) acap(a)

/**
 * @function cf_array_fit
 * @category array
 * @brief    Ensures the array can hold at least `n` elements, growing if necessary.
 * @param    a             The array. Modified in-place.
 * @param    n             The minimum number of elements to reserve capacity for.
 * @remarks  Shortform: `afit(a, n)`.
 * @related  cf_array_size cf_array_capacity cf_array_push cf_array_free
 */
#define cf_array_fit(a, n) afit(a, n)

/**
 * @function cf_array_push
 * @category array
 * @brief    Appends an element to the end of the array, growing if necessary.
 * @param    a             The array. Modified in-place.
 * @param    ...           The element to append.
 * @remarks  Shortform: `apush(a, ...)`.
 * @related  cf_array_pop cf_array_size cf_array_fit cf_array_free
 */
#define cf_array_push(a, ...) apush(a, __VA_ARGS__)

/**
 * @function cf_array_pop
 * @category array
 * @brief    Removes and returns the last element of the array.
 * @param    a             The array. Must be non-NULL and non-empty.
 * @return   Returns the removed element.
 * @remarks  Shortform: `apop(a)`.
 * @related  cf_array_push cf_array_last cf_array_size
 */
#define cf_array_pop(a) apop(a)

/**
 * @function cf_array_end
 * @category array
 * @brief    Returns a pointer one past the last element.
 * @param    a             The array.
 * @return   Returns `a + asize(a)`.
 * @remarks  Shortform: `aend(a)`.
 * @related  cf_array_last cf_array_size
 */
#define cf_array_end(a) aend(a)

/**
 * @function cf_array_last
 * @category array
 * @brief    Returns the last element of the array.
 * @param    a             The array. Must be non-NULL and non-empty.
 * @return   Returns `a[asize(a) - 1]`.
 * @remarks  Shortform: `alast(a)`.
 * @related  cf_array_pop cf_array_end cf_array_size
 */
#define cf_array_last(a) alast(a)

/**
 * @function cf_array_clear
 * @category array
 * @brief    Sets the element count to zero without freeing memory.
 * @param    a             The array.
 * @remarks  Shortform: `aclear(a)`.
 * @related  cf_array_free cf_array_size cf_array_setlen
 */
#define cf_array_clear(a) aclear(a)

/**
 * @function cf_array_setlen
 * @category array
 * @brief    Directly sets the element count. The array must be non-NULL and have enough capacity.
 * @param    a             The array. Must be non-NULL.
 * @param    n             The new element count.
 * @remarks  Shortform: `asetlen(a, n)`. Use `cf_array_fit` first to ensure sufficient capacity.
 * @related  cf_array_clear cf_array_size cf_array_fit
 */
#define cf_array_setlen(a, n) asetlen(a, n)

/**
 * @function cf_array_set
 * @category array
 * @brief    Copies the contents of array `b` into array `a`, resizing `a` if needed.
 * @param    a             The destination array. Modified in-place.
 * @param    b             The source array.
 * @remarks  Shortform: `aset(a, b)`.
 * @related  cf_array_size cf_array_fit cf_array_free
 */
#define cf_array_set(a, b) aset(a, b)

/**
 * @function cf_array_reverse
 * @category array
 * @brief    Reverses the order of elements in the array in-place.
 * @param    a             The array.
 * @remarks  Shortform: `arev(a)`.
 * @related  cf_array_size
 */
#define cf_array_reverse(a) arev(a)

/**
 * @function cf_array_hash
 * @category array
 * @brief    Returns a hash of all the bytes in the array using FNV1a.
 * @param    a             The array.
 * @return   Returns a `uint64_t` hash.
 * @remarks  Shortform: `ahash(a)`.
 * @related  cf_array_size
 */
#define cf_array_hash(a) ahash(a)

/**
 * @function cf_array_del
 * @category array
 * @brief    Removes an element by swapping it with the last element (unordered).
 * @param    a             The array.
 * @param    i             The index of the element to remove.
 * @remarks  Shortform: `adel(a, i)`. Does not preserve order.
 * @related  cf_array_pop cf_array_size
 */
#define cf_array_del(a, i) adel(a, i)

/**
 * @function cf_array_static
 * @category array
 * @brief    Initializes a dynamic array backed by a static (stack) buffer.
 * @param    a             The array pointer. Modified in-place.
 * @param    buffer        A stack buffer to use as initial storage.
 * @param    buffer_size   Size of `buffer` in bytes.
 * @remarks  Shortform: `astatic(a, buffer, buffer_size)`. The array will copy out of the static
 *           buffer and switch to heap allocation if it outgrows the buffer.
 * @related  cf_array_fit cf_array_free
 */
#define cf_array_static(a, buffer, buffer_size) astatic(a, buffer, buffer_size)

/**
 * @function cf_array_free
 * @category array
 * @brief    Frees the array and sets the pointer to NULL.
 * @param    a             The array. Modified in-place. Safe to call on NULL.
 * @remarks  Shortform: `afree(a)`.
 * @related  cf_array_push cf_array_clear cf_array_size
 */
#define cf_array_free(a) afree(a)

//--------------------------------------------------------------------------------------------------
// C++ API

#ifdef CF_CPP

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

#define CF_ARRAY_ENSURE_CAPACITY(capacity)                      \
    int num_elements = (capacity);                              \
    if (num_elements > m_capacity) {                            \
        if (m_capacity == 0) m_capacity = 8;                    \
        while (m_capacity < num_elements) m_capacity *= 2;      \
        T* new_ptr = (T*)cf_alloc(sizeof(T) * m_capacity);      \
        for (int i = 0; i < m_count; ++i) {                     \
            CF_PLACEMENT_NEW(new_ptr + i) T(cf_move(m_ptr[i])); \
        }                                                       \
        for (int i = 0; i < m_count; ++i) {                     \
            m_ptr[i].~T();                                      \
        }                                                       \
        cf_free(m_ptr);                                         \
        m_ptr = new_ptr;                                        \
    }

#define CF_ARRAY_CLEAR()                \
	for (int i = 0; i < m_count; i++) { \
		m_ptr[i].~T();                  \
	}                                   \
	m_count = 0;                        \

template <typename T>
Array<T>::Array(CF_InitializerList<T> list)
{
	int count = (int)list.size();
	CF_ARRAY_ENSURE_CAPACITY(count);
	T* ptr = m_ptr;
	for (const T* i = list.begin(); i < list.end(); ++i) {
		CF_PLACEMENT_NEW(ptr++) T(*i);
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
		CF_PLACEMENT_NEW(m_ptr + i) T(other_ptr[i]);
	}
	m_count = count;
}

template <typename T>
inline void swap(T& a, T& b) { T tmp = cf_move(a); a = cf_move(b); b = cf_move(tmp); }

template <typename T>
Array<T>::Array(Array<T>&& other)
{
	swap(m_capacity, other.m_capacity);
	swap(m_count, other.m_count);
	swap(m_ptr, other.m_ptr);
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
	cf_free(m_ptr);
}

template <typename T>
T& Array<T>::add()
{
	CF_ARRAY_ENSURE_CAPACITY(m_count + 1);
	return *CF_PLACEMENT_NEW(m_ptr + m_count++) T();
}

template <typename T>
T& Array<T>::add(const T& item)
{
	CF_ARRAY_ENSURE_CAPACITY(m_count + 1);
	return *CF_PLACEMENT_NEW(m_ptr + m_count++) T(item);
}

template <typename T>
T& Array<T>::add(T&& item)
{
	CF_ARRAY_ENSURE_CAPACITY(m_count + 1);
	return *CF_PLACEMENT_NEW(m_ptr + m_count++) T(cf_move(item));
}

template <typename T>
T Array<T>::pop()
{
	CF_ASSERT(m_count > 0);
	T val = cf_move(m_ptr[m_count - 1]);
	m_ptr[m_count - 1].~T();
	m_count--;
	return val;
}

template <typename T>
void Array<T>::unordered_remove(int index)
{
	CF_ASSERT(index >= 0 && index < m_count);
	const int last = m_count - 1;
	if (index != last) {
		m_ptr[index] = cf_move(m_ptr[last]);
	}
	m_ptr[last].~T();
	--m_count;
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
	if (m_count < count) {
		for (int i = m_count; i < count; ++i) {
			CF_PLACEMENT_NEW(m_ptr + i) T();
		}
	} else {
		for (int i = count; i < m_count; ++i) {
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
	CF_ARRAY_ENSURE_CAPACITY(count);
	if (m_count < count) {
		for (int i = m_count; i < count; ++i) {
			CF_PLACEMENT_NEW(m_ptr + i) T();
		}
	}
	m_count = count;
}

template <typename T>
void Array<T>::reverse()
{
	if (m_count <= 1) return;
	T* a = m_ptr;
	T* b = m_ptr + (m_count - 1);
	while (a < b) {
		T t = cf_move(*a);
		*a = cf_move(*b);
		*b = cf_move(t);
		++a; --b;
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
	CF_ASSERT(index >= 0 && index < m_count);
	return m_ptr[index];
}

template <typename T>
const T& Array<T>::operator[](int index) const
{
	CF_ASSERT(index >= 0 && index < m_count);
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
	CF_ASSERT(index >= 0 && index < m_count);
	return m_ptr + index;
}

template <typename T>
const T* Array<T>::operator+(int index) const
{
	CF_ASSERT(index >= 0 && index < m_count);
	return m_ptr + index;
}

template <typename T>
Array<T>& Array<T>::operator=(const Array<T>& rhs)
{
	if (this == &rhs) return *this;
	CF_ARRAY_CLEAR();
	CF_ARRAY_ENSURE_CAPACITY(rhs.m_count);
	for (int i = 0; i < rhs.m_count; i++) {
		CF_PLACEMENT_NEW(m_ptr + i) T(rhs.m_ptr[i]);
	}
	m_count = rhs.m_count;
	return *this;
}

template <typename T>
Array<T>& Array<T>::operator=(Array<T>&& rhs)
{
	swap(m_capacity, rhs.m_capacity);
	swap(m_count, rhs.m_count);
	swap(m_ptr, rhs.m_ptr);
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

#endif // CF_CPP

#endif // CF_ARRAY_H
