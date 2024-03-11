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

//--------------------------------------------------------------------------------------------------
// C API

/**
 * @function dyna
 * @category array
 * @brief    An empty macro used in the C API to markup dynamic arrays.
 * @example > Creating a dynamic array, pushing some elements into the array, and freeing it up afterwards.
 *     dyna int* a = NULL;
 *     apush(a, 5);
 *     CF_ASSERT(alen(a) == 1);
 *     alen(a)--;
 *     CF_ASSERT(alen(a) == 0);
 *     afree(a);
 * @remarks  This is an optional and _completely_ empty macro. It's only purpose is to provide a bit of visual indication a type is a
 *           dynamic array. One downside of the C-macro API is the opaque nature of the pointer type. Since the macros use polymorphism
 *           on typed pointers, there's no actual array struct type. It can get really annoying to sometimes forget if a pointer is an
 *           array, a hashtable, or just a pointer. This macro can be used to markup the type to make it much more clear for function
 *           parameters or struct member definitions. It's saying "Hey, I'm a dynamic array!" to mitigate this downside.
 * @related  dyna asize acount acap afit apush apop aend alast aclear aset arev ahash astatic afree
 */
#define dyna

/**
 * @function alen
 * @category array
 * @brief    Returns the number of elements in the array.
 * @param    x             The x position of the window.
 * @param    y             The y position of the window.
 * @example > Creating an array, adding an element, then decrementing the count to zero before freeing the array.
 *     dyna int* a = NULL;
 *     apush(a, 5);
 *     CF_ASSERT(alen(a) == 1);
 *     alen(a)--;
 *     CF_ASSERT(alen(a) == 0);
 *     afree(a);
 * @remarks  `a` must not by `NULL`. This function returns a proper l-value, so you can assign to it, i.e. increment/decrement can be quite useful.
 * @related  dyna asize acount acap afit apush apop aend alast aclear aset arev ahash astatic afree
 */
#define alen(a) cf_array_len(a)

/**
 * @function asize
 * @category array
 * @brief    Returns the number of elements in the array.
 * @param    a             The array.
 * @example > Creating an array, getting the size of the array, then freeing it up afterwards.
 *     dyna int* a = NULL;
 *     apush(a, 5);
 *     CF_ASSERT(asize(a) == 1);
 *     afree(a);
 * @remarks  `a` can be `NULL`.
 * @related  dyna asize acount acap afit apush apop aend alast aclear aset arev ahash astatic afree
 */
#define asize(a) cf_array_size(a)

/**
 * @function acount
 * @category array
 * @brief    Returns the number of elements in the array.
 * @param    a             The array.
 * @example > Creating an array, getting the size of the array, then freeing it up afterwards.
 *     dyna int* a = NULL;
 *     apush(a, 5);
 *     CF_ASSERT(acount(a) == 1);
 *     afree(a);
 * @remarks  `a` can be `NULL`.
 * @related  dyna asize acount acap afit apush apop aend alast aclear aset arev ahash astatic afree
 */
#define acount(a) cf_array_count(a)

/**
 * @function acap
 * @category array
 * @brief    Returns the capacity of the array.
 * @param    a             The array.
 * @remarks  `a` can be `NULL`. The capacity automatically grows if the size of the array grows over the capacity.
 *            You can use `afit` to ensure a minimum capacity as an optimization.
 * @related  dyna asize acount acap afit apush apop aend alast aclear aset arev ahash astatic afree
 */
#define acap(a) cf_array_capacity(a)

/**
 * @function afit
 * @category array
 * @brief    Ensures the capacity of the array is at least `n` elements large.
 * @param    a             The array.
 * @param    n             The number of elements to resize the array's capacity to.
 * @return   Returns a pointer to the array.
 * @remarks  This function does not change the number of live elements, the count/size, of the array. Only the capacity.
 *           This function is only useful as an optimization to avoid extra/unnecessary internal allocations. `a` is
 *           automatically re-assigned to a new pointer if the array was internally regrown.
 * @related  dyna asize acount acap afit apush apop aend alast aclear aset arev ahash astatic afree
 */
#define afit(a, n) cf_array_fit(a, n)

/**
 * @function apush
 * @category array
 * @brief    Pushes an element onto the back of the array.
 * @param    a             The array. Can be `NULL`.
 * @param    ...           The element to push.
 * @return   Returns the pushed element.
 * @example > Pushing some elements onto an array and asserting their values.
 *     dyna int* a = NULL;
 *     apush(a, 5);
 *     apush(a, 13);
 *     CF_ASSERT(a[0] == 5);
 *     CF_ASSERT(a[1] == 13);
 *     CF_ASSERT(asize(a) == 2);
 *     afree(a);
 * @remarks  `a` is automatically re-assigned to a new pointer if the array was internally regrown. If `a` is `NULL` a new
 *           dynamic array is allocated on-the-spot for you, and assigned back to `a`.
 * @related  dyna asize acount acap afit apush apop aend alast aclear aset arev ahash astatic afree
 */
#define apush(a, ...) cf_array_push(a, (__VA_ARGS__))

/**
 * @function apop
 * @category array
 * @brief    Pops and returns an element off the back of the array.
 * @param    a             The array. Can not be `NULL`.
 * @return   Returns the popped element.
 * @remarks  The last element of the array is fetched and will be returned. The size of the array is decremented by one.
 * @related  dyna asize acount acap afit apush apop aend alast aclear aset arev ahash astatic afree
 */
#define apop(a) cf_array_pop(a)

/**
 * @function aend
 * @category array
 * @brief    Returns a pointer one element beyond the end of the array.
 * @param    a             The array. Can be `NULL`.
 * @related  dyna asize acount acap afit apush apop aend alast aclear aset arev ahash astatic afree
 */
#define aend(a) cf_array_end(a)

/**
 * @function alast
 * @category array
 * @brief    Returns the last element in the array.
 * @param    a             The array. Can not be `NULL`.
 * @related  dyna asize acount acap afit apush apop aend alast aclear aset arev ahash astatic afree
 */
#define alast(a) cf_array_last(a)

/**
 * @function aclear
 * @category array
 * @brief    Sets the array's count to zero. Does not free any resources.
 * @param    a             The array. Can be `NULL`.
 * @return   Returns zero.
 * @related  dyna asize acount acap afit apush apop aend alast aclear aset arev ahash astatic afree
 */
#define aclear(a) cf_array_clear(a)

/**
 * @function aset
 * @category array
 * @brief    Copies the array b into array a. Will automatically fit a if needed with `afit`.
 * @param    a             The array to copy into (destination).
 * @param    b             The array to copy from (source).
 * @return   Returns a pointer to `a`. `a` will automatically be reassigned to any new pointer.
 * @related  dyna asize acount acap afit apush apop aend alast aclear aset arev ahash astatic afree
 */
#define aset(a, b) cf_array_set(a, b)

/**
 * @function arev
 * @category array
 * @brief    Reverses the elements in an array.
 * @param    a             The array. Can be `NULL`.
 * @related  dyna asize acount acap afit apush apop aend alast aclear aset arev ahash astatic afree
 */
#define arev(a) cf_array_reverse(a)

/**
 * @function ahash
 * @category array
 * @brief    Returns the hash of all the bytes in the array.
 * @param    a             The array.
 * @related  dyna asize acount acap afit apush apop aend alast aclear aset arev ahash astatic afree
 */
#define ahash(a) cf_array_hash(a)

/**
 * @function astatic
 * @category array
 * @brief    Creates an array with an initial static storage backing. Will grow onto the heap if the size becomes too large.
 * @param    a             The array. Can be `NULL`.
 * @param    buffer        An initial buffer of memory to store the array within.
 * @param    buffer_size   The size of `buffer` in bytes.
 * @return   Returns a pointer to `a`. `a` will automatically be reassigned to any new pointer.
 * @remarks  This macro is useful as an optimization to avoid any dynamic memory allocation in the common case for implementing
 *           certain data structures (such as strings or stack vectors). As the array grows too large for the `buffer` it will
 *           dynamically grow into the heap.
 * @related  dyna asize acount acap afit apush apop aend alast aclear aset arev ahash astatic afree
 */
#define astatic(a, buffer, buffer_size) cf_array_static(a, buffer, buffer_size)

/**
 * @function afree
 * @category array
 * @brief    Frees up all resources used by the array.
 * @param    a             The array.
 * @remarks  Sets `a` to `NULL`.
 * @related  dyna asize acount acap afit apush apop aend alast aclear aset arev ahash astatic afree
 */
#define afree(a) cf_array_free(a)

//--------------------------------------------------------------------------------------------------
// Longform C API.

#ifdef __cplusplus
#define cf_array_len(a) (CF_ACANARY(a), CF_AHDR(a)->size)
#else
#define cf_array_len(a) (CF_AHDR(a)->size)
#endif
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
#define cf_array_reverse(a) (a ? cf_arev(a, sizeof(*a)) : NULL)
#define cf_array_hash(a) cf_fnv1a(a, cf_array_size(a))
#define cf_array_static(a, buffer, buffer_size) (*(void**)&(a) = cf_astatic(buffer, buffer_size, sizeof(*a)))
#define cf_array_free(a) do { CF_ACANARY(a); if (a && !CF_AHDR(a)->is_static) cf_free(CF_AHDR(a)); a = NULL; } while (0)

//--------------------------------------------------------------------------------------------------
// Hidden API - Not intended for direct use.

#define CF_AHDR(a) ((CF_Ahdr*)a - 1)
#define CF_ACOOKIE 0xE6F7E359

// If you see this macro asserting it likely means you didn't pass a proper dynamic string or array
// to one of the array/string macros. You must not pass string literals to these functions for certain input
// arguments, and instead, build a dynamic array/string using the C apis (e.g. `apush` or `sset`). This macro
// also helps detect heap/stack corruption such as buffer underruns.
#define CF_ACANARY(a) ((a) ? CF_ASSERT(CF_AHDR(a)->cookie == CF_ACOOKIE) : (void)0)

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

CF_API void* CF_CALL cf_agrow(const void* a, int new_size, size_t element_size);
CF_API void* CF_CALL cf_astatic(const void* a, int capacity, size_t element_size);
CF_API void* CF_CALL cf_aset(const void* a, const void* b, size_t element_size);
CF_API void* CF_CALL cf_arev(const void* a, size_t element_size);

#ifdef __cplusplus
}
#endif // __cplusplus

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
		while (m_capacity < num_elements) {                    \
			m_capacity *= 2;                                   \
		}                                                      \
		T* new_ptr = (T*)cf_alloc(sizeof(T) * m_capacity);     \
		for (int i = 0; i < m_count; ++i) {                    \
			CF_PLACEMENT_NEW(new_ptr + i) T(cf_move(m_ptr[i]));\
		}                                                      \
		cf_free(m_ptr);                                        \
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
Array<T>::Array(Array<T>&& other)
{
	m_capacity = other.m_capacity;
	m_count = other.m_count;
	m_ptr = other.m_ptr;
	CF_MEMSET(&other, 0, sizeof(other));
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
Array<T>& Array<T>::steal_from(Array<T>* steal_from_me)
{
	m_capacity = steal_from_me->m_capacity;
	m_count = steal_from_me->m_count;
	m_ptr = steal_from_me->m_ptr;
	CF_MEMSET(steal_from_me, 0, sizeof(*steal_from_me));
	return *this;
}

template <typename T>
Array<T>& Array<T>::steal_from(Array<T>& steal_from_me)
{
	m_capacity = steal_from_me.m_capacity;
	m_count = steal_from_me.m_count;
	m_ptr = steal_from_me.m_ptr;
	CF_MEMSET(&steal_from_me, 0, sizeof(steal_from_me));
	return *this;
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
	m_ptr[index].~T();
	if (index != --m_count) {
		m_ptr[index] = cf_move(m_ptr[m_count]);
	}
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
	T* a = m_ptr;
	T* b = m_ptr + (m_count - 1);

	while (a < b) {
		T t = cf_move(*a);
		*a = cf_move(*b);
		*b = cf_move(t);
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
	CF_ARRAY_CLEAR();
	m_capacity = rhs.m_capacity;
	m_count = rhs.m_count;
	m_ptr = rhs.m_ptr;
	CF_MEMSET(&rhs, 0, sizeof(rhs));
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
