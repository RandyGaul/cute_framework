/*
	Cute Framework
	Copyright (C) 2023 Randy Gaul https://randygaul.github.io/

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

#ifndef CF_TYPELESS_ARRAY_H
#define CF_TYPELESS_ARRAY_H

#include "cute_defines.h"

#ifdef CF_CPP

/**
 * Implements a dynamically growable array without any type information (no templates).
 *
 * This data structure is mostly here to implement cute_ecs.cpp, but is publicly available
 * here just in case someone has a use for it.
 */

struct CF_TypelessArray
{
	CF_TypelessArray();
	explicit CF_TypelessArray(size_t element_size);
	explicit CF_TypelessArray(size_t element_size, int capacity);
	~CF_TypelessArray();

	void* add();
	void* add(const void* item);
	void* insert(int index);
	void* insert(int index, const void* item);
	void set(int index, const void* item);
	void remove(int index);
	void* pop();
	void unordered_remove(int index);
	void copy(int src, int dst, int count = 1);
	void clear();
	void ensure_capacity(int num_elements);
	void steal_from(CF_TypelessArray* steal_from_me);

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

	size_t m_element_size = 0;
	int m_capacity = 0;
	int m_count = 0;
	void* m_items = NULL;
};

namespace Cute 
{

using typeless_array = CF_TypelessArray;

}

#endif // CF_CPP

#endif // CF_TYPELESS_ARRAY_H
