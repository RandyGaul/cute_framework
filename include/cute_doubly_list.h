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

#ifndef CUTE_DOUBLY_LINKED_LIST_H
#define CUTE_DOUBLY_LINKED_LIST_H

#include "cute_defines.h"

struct cf_list_node_t
{
	cf_list_node_t* next = this;
	cf_list_node_t* prev = this;
};

struct cf_list_t
{
	cf_list_node_t nodes;
};

#define CUTE_LIST_NODE(T, member, ptr) ((cf_list_node_t*)((uintptr_t)ptr + CUTE_OFFSET_OF(T, member)))
#define CUTE_LIST_HOST(T, member, ptr) ((T*)((uintptr_t)ptr - CUTE_OFFSET_OF(T, member)))

CUTE_INLINE void cf_list_init_node(cf_list_node_t* node)
{
	node->next = node;
	node->prev = node;
}

CUTE_INLINE void cf_list_init(cf_list_t* list)
{
	cf_list_init_node(&list->nodes);
}

CUTE_INLINE void cf_list_push_front(cf_list_t* list, cf_list_node_t* node)
{
	node->next = list->nodes.next;
	node->prev = &list->nodes;
	list->nodes.next->prev = node;
	list->nodes.next = node;
}

CUTE_INLINE void cf_list_push_back(cf_list_t* list, cf_list_node_t* node)
{
	node->prev = list->nodes.prev;
	node->next = &list->nodes;
	list->nodes.prev->next = node;
	list->nodes.prev = node;
}

CUTE_INLINE void cf_list_remove(cf_list_node_t* node)
{
	node->prev->next = node->next;
	node->next->prev = node->prev;
	cf_list_init_node(node);
}

CUTE_INLINE cf_list_node_t* cf_list_pop_front(cf_list_t* list)
{
	cf_list_node_t* node = list->nodes.next;
	cf_list_remove(node);
	return node;
}

CUTE_INLINE cf_list_node_t* cf_list_pop_back(cf_list_t* list)
{
	cf_list_node_t* node = list->nodes.prev;
	cf_list_remove(node);
	return node;
}

CUTE_INLINE int cf_list_empty(cf_list_t* list)
{
	return list->nodes.next == list->nodes.prev && list->nodes.next == &list->nodes;
}

CUTE_INLINE cf_list_node_t* cf_list_begin(cf_list_t* list)
{
	return list->nodes.next;
}

CUTE_INLINE cf_list_node_t* cf_list_end(cf_list_t* list)
{
	return &list->nodes;
}

CUTE_INLINE cf_list_node_t* cf_list_front(cf_list_t* list)
{
	return list->nodes.next;
}

CUTE_INLINE cf_list_node_t* cf_list_back(cf_list_t* list)
{
	return list->nodes.prev;
}

#ifdef CUTE_CPP

namespace cute
{

}

#endif // CUTE_CPP

#endif // CUTE_DOUBLY_LINKED_LIST_H
