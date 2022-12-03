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

//--------------------------------------------------------------------------------------------------
// C API

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

typedef struct CF_ListNode
{
	struct CF_ListNode* next; /*= this;*/
	struct CF_ListNode* prev; /*= this;*/
} CF_ListNode;

typedef struct CF_List
{
	CF_ListNode nodes;
} CF_List;

#define CUTE_LIST_NODE(T, member, ptr) ((CF_ListNode*)((uintptr_t)ptr + CUTE_OFFSET_OF(T, member)))
#define CUTE_LIST_HOST(T, member, ptr) ((T*)((uintptr_t)ptr - CUTE_OFFSET_OF(T, member)))

CUTE_INLINE void cf_list_init_node(CF_ListNode* node)
{
	node->next = node;
	node->prev = node;
}

CUTE_INLINE void cf_list_init(CF_List* list)
{
	cf_list_init_node(&list->nodes);
}

CUTE_INLINE void cf_list_push_front(CF_List* list, CF_ListNode* node)
{
	node->next = list->nodes.next;
	node->prev = &list->nodes;
	list->nodes.next->prev = node;
	list->nodes.next = node;
}

CUTE_INLINE void cf_list_push_back(CF_List* list, CF_ListNode* node)
{
	node->prev = list->nodes.prev;
	node->next = &list->nodes;
	list->nodes.prev->next = node;
	list->nodes.prev = node;
}

CUTE_INLINE void cf_list_remove(CF_ListNode* node)
{
	node->prev->next = node->next;
	node->next->prev = node->prev;
	cf_list_init_node(node);
}

CUTE_INLINE CF_ListNode* cf_list_pop_front(CF_List* list)
{
	CF_ListNode* node = list->nodes.next;
	cf_list_remove(node);
	return node;
}

CUTE_INLINE CF_ListNode* cf_list_pop_back(CF_List* list)
{
	CF_ListNode* node = list->nodes.prev;
	cf_list_remove(node);
	return node;
}

CUTE_INLINE int cf_list_empty(CF_List* list)
{
	return list->nodes.next == list->nodes.prev && list->nodes.next == &list->nodes;
}

CUTE_INLINE CF_ListNode* cf_list_begin(CF_List* list)
{
	return list->nodes.next;
}

CUTE_INLINE CF_ListNode* cf_list_end(CF_List* list)
{
	return &list->nodes;
}

CUTE_INLINE CF_ListNode* cf_list_front(CF_List* list)
{
	return list->nodes.next;
}

CUTE_INLINE CF_ListNode* cf_list_back(CF_List* list)
{
	return list->nodes.prev;
}

#ifdef __cplusplus
}
#endif // __cplusplus

//--------------------------------------------------------------------------------------------------
// C++ API

#ifdef CUTE_CPP

namespace Cute
{

using list_t = CF_List;
using list_node_t = CF_ListNode;

CUTE_INLINE void list_init_node(list_node_t* node) { cf_list_init_node((CF_ListNode*)node); };
CUTE_INLINE void list_init(list_t* list) { cf_list_init(list); };
CUTE_INLINE void list_push_front(list_t* list, list_node_t* node) { cf_list_push_front(list, (CF_ListNode*)node); };
CUTE_INLINE void list_push_back(list_t* list, list_node_t* node) { cf_list_push_back(list, (CF_ListNode*)node); };
CUTE_INLINE void list_remove(list_node_t* node) { cf_list_remove((CF_ListNode*)node); };
CUTE_INLINE list_node_t* list_pop_front(list_t* list) { return (list_node_t*)cf_list_pop_front(list); };
CUTE_INLINE list_node_t* list_pop_back(list_t* list) { return (list_node_t*)cf_list_pop_back(list); };
CUTE_INLINE int list_empty(list_t* list) { return cf_list_empty(list); };
CUTE_INLINE list_node_t* list_begin(list_t* list) { return (list_node_t*)cf_list_begin(list); };
CUTE_INLINE list_node_t* list_end(list_t* list) { return (list_node_t*)cf_list_end(list); };
CUTE_INLINE list_node_t* list_front(list_t* list) { return (list_node_t*)cf_list_front(list); };
CUTE_INLINE list_node_t* list_back(list_t* list) { return (list_node_t*)cf_list_back(list); };

}

#endif // CUTE_CPP

#endif // CUTE_DOUBLY_LINKED_LIST_H
