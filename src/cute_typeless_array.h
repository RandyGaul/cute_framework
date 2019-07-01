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

#ifndef CUTE_TYPELESS_ARRAY_H
#define CUTE_TYPELESS_ARRAY_H

#include <cute_defines.h>

namespace cute
{

struct typeless_array
{
	typeless_array();
	explicit typeless_array(int element_size, void* user_allocator_context);
	explicit typeless_array(int element_size, int capacity, void* user_allocator_context);
	~typeless_array();

	void* add();
	void* add(const void* item);
	void* insert(int index);
	void* insert(int index, const void* item);
	void set(int index, const void* item);
	void remove(int index);
	void* pop();
	void unordered_remove(int index);
	void clear();
	void ensure_capacity(int num_elements);
	void steal_from(typeless_array* steal_from_me);

	int capacity() const;
	int count() const;

	void* operator[](int index);
	const void* operator[](int index) const;

	void* operator+(int index);
	const void* operator+(int index) const;

	void* last();
	const void* last() const;

	void* data();
	const void* data() const;

private:
	int m_item_size = 0;
	int m_capacity = 0;
	int m_count = 0;
	void* m_items = NULL;
	void* m_mem_ctx = NULL;
};

}

#endif // CUTE_TYPELESS_ARRAY_H
