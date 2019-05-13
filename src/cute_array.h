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

#include <cute_defines.h>
#include <cute_c_runtime.h>
#include <cute_alloc.h>

/* 
	Implements a basic growable array data structure for POD items. Items can
	use constructors and destructors, but must have trivial "memcpy" assignment
	operators (such as the default compiler-generated assignment operator).

	cute::array does *not* work as a drop-in replacement for std::vector as it
	has a few special properties. Here is the list of important points for using
	this array data structure.

	1. Items stored in the array is assumed to be POD. Sometimes the assignment
	   operator is used, and sometimes `CUTE_MEMCPY` or `CUTE_MEMMOVE` is used.
	   Items in the array should *not* have specialized assignment operators
	   beyond simply copying byte-for-byte POD data.
	2. There is no way to perform a proper "deep-copy" by using the assignment
	   operator on cute::array.
	3. Items stored in the array do have constructors and destructors called, but
	   only upon insertion or removal (not during grow or other operations). The
	   idea is to facilitate easy initializing of values stored in the array, such
	   as working with an array of arrays.
	4. The assignment operator on cute::array does *not* perform a "deep-copy".
	5. No iterators.
	6. No rvalue semantics are supported. Instead, the `steal_from` function can
	   be used to cleanup any current items, and then steal items from another
	   cute::array.
*/

namespace cute
{

template <typename T>
struct array
{
	array();
	explicit array(void* user_allocator_context);
	explicit array(int capacity, void* user_allocator_context);
	~array();

	T& add();
	T& add(const T& item);
	T& insert(int index);
	T& insert(int index, const T& item);
	void set(int index, const T& item);
	void remove(int index);
	T& pop();
	void unordered_remove(int index);
	void clear();
	void ensure_capacity(int num_elements);
	void steal_from(array<T>& steal_from_me);

	int capacity() const;
	int count() const;

	T& operator[](int index);
	const T& operator[](int index) const;

	T* operator+(int index);
	const T* operator+(int index) const;

	T& last();
	const T& last() const;

	T* data();
	const T* data() const;

private:
	int capacity_ = 0;
	int count_ = 0;
	T* items_ = NULL;
	void* mem_ctx_ = NULL;
};

// -------------------------------------------------------------------------------------------------

template <typename T>
array<T>::array()
{
}

template <typename T>
array<T>::array(void* user_allocator_context)
	: mem_ctx_(user_allocator_context)
{
}

template <typename T>
array<T>::array(int capacity, void* user_allocator_context)
	: capacity_(capacity)
	, mem_ctx_(user_allocator_context)
{
	items_ = (T*)CUTE_ALLOC(sizeof(T), mem_ctx_);
	CUTE_ASSERT(items_);
}

template <typename T>
array<T>::~array()
{
	for (int i = 0; i < count_; ++i)
	{
		T* slot = items_ + i;
		slot->~T();
	}
	CUTE_FREE(items_, mem_ctx_);
}

template <typename T>
T& array<T>::add()
{
	ensure_capacity(count_ + 1);
	T* slot = items_ + count_++;
	CUTE_PLACEMENT_NEW(slot) T;
	return *slot;
}

template <typename T>
T& array<T>::add(const T& item)
{
	ensure_capacity(count_ + 1);
	T* slot = items_ + count_++;
	CUTE_PLACEMENT_NEW(slot) T(item);
	return *slot;
}

template <typename T>
T& array<T>::insert(int index)
{
	CUTE_ASSERT(index >= 0 && index < capacity_);
	add();
	for (int i = count_ - 1; i > index; --i) items_[i] = items_[i - 1];
	return items_[index];
}

template <typename T>
T& array<T>::insert(int index, const T& item)
{
	CUTE_ASSERT(index >= 0 && index < capacity_);
	add();
	CUTE_MEMMOVE(items_ + index + 1, items_ + index, sizeof(T) * count_);
	T* slot = items_ + index;
	CUTE_PLACEMENT_NEW(slot) T(item);
	return *slot;
}

template <typename T>
void array<T>::set(int index, const T& item)
{
	CUTE_ASSERT(index >= 0 && index < capacity_);
	T* slot = items_ + index;
	*slot = item;
}

template <typename T>
void array<T>::remove(int index)
{
	CUTE_ASSERT(index >= 0 && index < capacity_);
	T* slot = items_ + index;
	slot->~T();
	CUTE_MEMMOVE(items + index, items + index + 1, sizeof(T) * count_);
	--count_;
}

template <typename T>
T& array<T>::pop()
{
	return items_[--count_];
}

template <typename T>
void array<T>::unordered_remove(int index)
{
	CUTE_ASSERT(index >= 0 && index < capacity_);
	T* slot = items_ + index;
	slot->~T();
	items_[index] = items_[--count_];
}

template <typename T>
void array<T>::clear()
{
	for (int i = 0; i < count_; ++i)
	{
		T* slot = items_ + i;
		slot->~T();
	}
	count_ = 0;
}

template <typename T>
void array<T>::ensure_capacity(int num_elements)
{
	if (num_elements > capacity_) {
		int new_capacity = capacity_ ? capacity_ * 2 : 256;
		while (new_capacity < num_elements)
		{
			new_capacity <<= 1;
			CUTE_ASSERT(new_capacity); // Detect overflow.
		}

		size_t new_size = sizeof(T) * new_capacity;
		T* new_items = (T*)CUTE_ALLOC(new_size, mem_ctx_);
		CUTE_ASSERT(new_items);
		CUTE_MEMCPY(new_items, items_, sizeof(T) * count_);
		CUTE_FREE(items_, mem_ctx_);
		items_ = new_items;
		capacity_ = new_capacity;
	}
}

template <typename T>
void array<T>::steal_from(array<T>& steal_from_me)
{
	this->~array<T>();
	capacity_ = steal_from_me.capacity_;
	count_ = steal_from_me.count_;
	items_ = steal_from_me.items_;
	mem_ctx_ = steal_from_me.mem_ctx_;
	steal_from_me->array<T>();
}

template <typename T>
int array<T>::capacity() const
{
	return capacity_;
}

template <typename T>
int array<T>::count() const
{
	return count_;
}

template <typename T>
T& array<T>::operator[](int index)
{
	CUTE_ASSERT(index >= 0 && index < capacity_);
	return items_[index];
}

template <typename T>
const T& array<T>::operator[](int index) const
{
	CUTE_ASSERT(index >= 0 && index < capacity_);
	return items_[index];
}

template <typename T>
T* array<T>::data()
{
	return items_;
}

template <typename T>
const T* array<T>::data() const
{
	return items_;
}

template <typename T>
T* array<T>::operator+(int index)
{
	CUTE_ASSERT(index >= 0 && index < capacity_);
	return items_ + index;
}

template <typename T>
const T* array<T>::operator+(int index) const
{
	CUTE_ASSERT(index >= 0 && index < capacity_);
	return items_ + index;
}

template <typename T>
T& array<T>::last()
{
	return (*this)[count_ - 1];
}

template <typename T>
const T& array<T>::last() const
{
	return (*this)[count_ - 1];
}

}
