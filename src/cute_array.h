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
	void resize(int new_count);

	int capacity() const;
	int count() const;

	T& operator[](int index);
	const T& operator[](int index) const;

	T* operator+(int index);
	const T* operator+(int index) const;

	T* data();
	const T* data() const;

private:
	int capacity_ = 0;
	int count_ = 0;
	T* items_ = NULL;
	void * mem_ctx_;

	void ensure_capacity(int num_elements);
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
	CUTE_FREE(items_, mem_ctx_);
}

template <typename T>
T& array<T>::add()
{
	ensure_capacity(count_ + 1);
	T* slot = items_ + count_++;
	return *slot;
}

template <typename T>
T& array<T>::add(const T& item)
{
	ensure_capacity(count_ + 1);
	T* slot = items_ + count_++;
	*slot = item;
	return *slot;
}

template <typename T>
T& array<T>::insert(int index)
{
	CUTE_ASSERT(index >= 0 && index < capacity_);
	add();
	for(int i = count_ - 1; i > index; --i) items_[i] = items_[i - 1];
	return items_[index];
}

template <typename T>
T& array<T>::insert(int index, const T& item)
{
	add();
	CUTE_MEMMOVE(items_ + index + 1, items_ + index, sizeof(T) * count_);
	T* slot = items_ + index;
	*slot = item;
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
	items_[index] = items_[--count_];
}

template <typename T>
void array<T>::clear()
{
	count_ = 0;
}

template <typename T>
void array<T>::resize(int new_count)
{
	ensure_capacity(new_count);
	count_ = new_count;
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
T& array<T>::operator[]( int index )
{
	CUTE_ASSERT(index >= 0 && index < capacity_);
	return items_[index];
}

template <typename T>
const T& array<T>::operator[]( int index ) const
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
void array<T>::ensure_capacity(int num_elements)
{
	if (num_elements > capacity_) {
		int new_capacity = capacity_ ? capacity_ * 2 : 256;
		while (new_capacity < num_elements)
		{
			new_capacity <<= 1;
			CUTE_ASSERT(new_capacity); // Detect overflow.
		}

		T* new_items = (T*)CUTE_ALLOC(sizeof(T) * new_capacity, mem_ctx_);
		CUTE_ASSERT(new_items);
		CUTE_MEMCPY(new_items, items_, sizeof(T) * count_);
		CUTE_FREE(items_, mem_ctx_);
		items_ = new_items;
		capacity_ = new_capacity;
	}
}

}
