/*
	Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

#ifndef CF_DOUBLY_LINKED_LIST_H
#define CF_DOUBLY_LINKED_LIST_H

#include "cute_defines.h"

//--------------------------------------------------------------------------------------------------
// C API

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

/**
 * @struct   CF_ListNode
 * @category list
 * @brief    A node in a circular doubly-linked list.
 * @remarks  This node is _intrusive_ and to be placed within your struct/object definitions as a member.
 * @related  CF_ListNode CF_List CF_LIST_NODE CF_LIST_HOST cf_list_init_node cf_list_init cf_list_push_front cf_list_push_back cf_list_remove cf_list_pop_front cf_list_pop_back cf_list_empty cf_list_begin cf_list_end cf_list_front cf_list_back
 */
typedef struct CF_ListNode
{
	/* @member Pointer to the next node in the list. */
	struct CF_ListNode* next;

	/* @member Pointer to the previous node in the list. */
	struct CF_ListNode* prev;
} CF_ListNode;
// @end

/**
 * @struct   CF_List
 * @category list
 * @brief    A circular doubly-linked list.
 * @related  CF_ListNode CF_List CF_LIST_NODE CF_LIST_HOST cf_list_init_node cf_list_init cf_list_push_front cf_list_push_back cf_list_remove cf_list_pop_front cf_list_pop_back cf_list_empty cf_list_begin cf_list_end cf_list_front cf_list_back
 */
typedef struct CF_List
{
	/* @member Pointer to the first node in a circular doubly-linked list. */
	CF_ListNode nodes;
} CF_List;
// @end

/**
 * @function CF_LIST_NODE
 * @category list
 * @brief    Converts a pointer to a host struct/object to a specific `CF_ListNode` member.
 * @param    T          The type of the host.
 * @param    member     The name of the host member.
 * @param    ptr        A pointer to the host.
 * @return   Returns a pointer to a `CF_ListNode`.
 * @remarks  This doubly-linked list is intrusive, meaning you place nodes inside of objects. These helper macros are to
 *           easily convert to/from host pointer and node pointer.
 * @example > Converting from host to node pointer.
 *     struct MyStruct {
 *         int a;
 *         float b;
 *         CF_ListNode node;
 *     };
 *     
 *     MyStruct x = get_struct();
 *     CF_ListNode* node = CF_LIST_NODE(MyStruct, node, &x);
 *     
 *     // Do whatever is needed with the node.
 *     do_stuff(node);
 * @related  CF_ListNode CF_List CF_LIST_NODE CF_LIST_HOST cf_list_init_node cf_list_init cf_list_push_front cf_list_push_back cf_list_remove cf_list_pop_front cf_list_pop_back cf_list_empty cf_list_begin cf_list_end cf_list_front cf_list_back
 */
#define CF_LIST_NODE(T, member, ptr) ((CF_ListNode*)((uintptr_t)ptr + CF_OFFSET_OF(T, member)))

/**
 * @function CF_LIST_HOST
 * @category list
 * @brief    Converts a pointer to a node to a pointer to its host struct/object.
 * @param    T          The type of the host.
 * @param    member     The name of the host member.
 * @param    ptr        A pointer to the host.
 * @return   Returns a pointer to a the host struct/object of type `T`.
 * @remarks  This doubly-linked list is intrusive, meaning you place nodes inside of objects. These helper macros are to
 *           easily convert to/from host pointer and node pointer.
 * @example > Converting from node to host pointer.
 *     struct MyStruct {
 *         int a;
 *         float b;
 *         CF_ListNode node;
 *     };
 *     
 *     CF_ListNode* node_ptr = get_node();
 *     MyStruct* my = CF_LIST_HOST(MyStruct, node, node_ptr);
 *     
 *     // Do whatever is needed with the host:
 *     do_stuff(my);
 * @related  CF_ListNode CF_List CF_LIST_NODE CF_LIST_HOST cf_list_init_node cf_list_init cf_list_push_front cf_list_push_back cf_list_remove cf_list_pop_front cf_list_pop_back cf_list_empty cf_list_begin cf_list_end cf_list_front cf_list_back
 */
#define CF_LIST_HOST(T, member, ptr) ((T*)((uintptr_t)ptr - CF_OFFSET_OF(T, member)))

/**
 * @function cf_list_init_node
 * @category list
 * @brief    Intializes a node.
 * @param    node       The node.
 * @remarks  As this list is circular, each node is initialized to point to itself.
 * @related  CF_ListNode CF_List CF_LIST_NODE CF_LIST_HOST cf_list_init_node cf_list_init cf_list_push_front cf_list_push_back cf_list_remove cf_list_pop_front cf_list_pop_back cf_list_empty cf_list_begin cf_list_end cf_list_front cf_list_back
 */
CF_INLINE void cf_list_init_node(CF_ListNode* node)
{
	node->next = node;
	node->prev = node;
}

/**
 * @function cf_list_init
 * @category list
 * @brief    Intializes a list.
 * @param    list       The list.
 * @remarks  As an optimization the list contains a dummy node inside of it. To traverse this list, use `cf_list_begin` and
 *           `cf_list_end` in a for loop. See `cf_list_begin` for an example.
 * @related  CF_ListNode CF_List CF_LIST_NODE CF_LIST_HOST cf_list_init_node cf_list_init cf_list_push_front cf_list_push_back cf_list_remove cf_list_pop_front cf_list_pop_back cf_list_empty cf_list_begin cf_list_end cf_list_front cf_list_back
 */
CF_INLINE void cf_list_init(CF_List* list)
{
	cf_list_init_node(&list->nodes);
}

/**
 * @function cf_list_push_front
 * @category list
 * @brief    Pushes a node onto the front of the list.
 * @param    list       The list.
 * @param    node       The node.
 * @related  CF_ListNode CF_List CF_LIST_NODE CF_LIST_HOST cf_list_init_node cf_list_init cf_list_push_front cf_list_push_back cf_list_remove cf_list_pop_front cf_list_pop_back cf_list_empty cf_list_begin cf_list_end cf_list_front cf_list_back
 */
CF_INLINE void cf_list_push_front(CF_List* list, CF_ListNode* node)
{
	node->next = list->nodes.next;
	node->prev = &list->nodes;
	list->nodes.next->prev = node;
	list->nodes.next = node;
}

/**
 * @function cf_list_push_back
 * @category list
 * @brief    Pushes a node onto the back of the list.
 * @param    list       The list.
 * @param    node       The node.
 * @related  CF_ListNode CF_List CF_LIST_NODE CF_LIST_HOST cf_list_init_node cf_list_init cf_list_push_front cf_list_push_back cf_list_remove cf_list_pop_front cf_list_pop_back cf_list_empty cf_list_begin cf_list_end cf_list_front cf_list_back
 */
CF_INLINE void cf_list_push_back(CF_List* list, CF_ListNode* node)
{
	node->prev = list->nodes.prev;
	node->next = &list->nodes;
	list->nodes.prev->next = node;
	list->nodes.prev = node;
}

/**
 * @function cf_list_remove
 * @category list
 * @brief    Removes a node from the list.
 * @param    List       The list.
 * @related  CF_ListNode CF_List CF_LIST_NODE CF_LIST_HOST cf_list_init_node cf_list_init cf_list_push_front cf_list_push_back cf_list_remove cf_list_pop_front cf_list_pop_back cf_list_empty cf_list_begin cf_list_end cf_list_front cf_list_back
 */
CF_INLINE void cf_list_remove(CF_ListNode* node)
{
	node->prev->next = node->next;
	node->next->prev = node->prev;
	cf_list_init_node(node);
}

/**
 * @function cf_list_pop_front
 * @category list
 * @brief    Pops a node off the front of a list.
 * @param    List       The list.
 * @related  CF_ListNode CF_List CF_LIST_NODE CF_LIST_HOST cf_list_init_node cf_list_init cf_list_push_front cf_list_push_back cf_list_remove cf_list_pop_front cf_list_pop_back cf_list_empty cf_list_begin cf_list_end cf_list_front cf_list_back
 */
CF_INLINE CF_ListNode* cf_list_pop_front(CF_List* list)
{
	CF_ListNode* node = list->nodes.next;
	cf_list_remove(node);
	return node;
}

/**
 * @function cf_list_pop_back
 * @category list
 * @brief    Pops a node off the back of a list.
 * @param    List       The list.
 * @related  CF_ListNode CF_List CF_LIST_NODE CF_LIST_HOST cf_list_init_node cf_list_init cf_list_push_front cf_list_push_back cf_list_remove cf_list_pop_front cf_list_pop_back cf_list_empty cf_list_begin cf_list_end cf_list_front cf_list_back
 */
CF_INLINE CF_ListNode* cf_list_pop_back(CF_List* list)
{
	CF_ListNode* node = list->nodes.prev;
	cf_list_remove(node);
	return node;
}

/**
 * @function cf_list_empty
 * @category list
 * @brief    Returns true if a list is empty.
 * @param    List       The list.
 * @related  CF_ListNode CF_List CF_LIST_NODE CF_LIST_HOST cf_list_init_node cf_list_init cf_list_push_front cf_list_push_back cf_list_remove cf_list_pop_front cf_list_pop_back cf_list_empty cf_list_begin cf_list_end cf_list_front cf_list_back
 */
CF_INLINE bool cf_list_empty(CF_List* list)
{
	return list->nodes.next == list->nodes.prev && list->nodes.next == &list->nodes;
}

/**
 * @function cf_list_begin
 * @category list
 * @brief    Returns a pointer to the first node in the list.
 * @param    List       The list.
 * @example > Looping over a list with a for loop.
 *     for (CF_Node* n = cf_list_begin(list); n != cf_list_end(list); n = n->next) {
 *         do_stuff(n);
 *     }
 * @remarks  Since the list is circular with a single dummy node it can be confusing to loop over. To help make this simpler, use
 *           `cf_list_begin` and `cf_list_end` to perform a loop over the list.
 * @related  CF_ListNode CF_List CF_LIST_NODE CF_LIST_HOST cf_list_init_node cf_list_init cf_list_push_front cf_list_push_back cf_list_remove cf_list_pop_front cf_list_pop_back cf_list_empty cf_list_begin cf_list_end cf_list_front cf_list_back
 */
CF_INLINE CF_ListNode* cf_list_begin(CF_List* list)
{
	return list->nodes.next;
}

/**
 * @function cf_list_end
 * @category list
 * @brief    Returns a pointer to one passed the end of the list (to the dummy node).
 * @param    List       The list.
 * @example > Looping over a list with a for loop.
 *     for (CF_Node* n = cf_list_begin(list); n != cf_list_end(list); n = n->next) {
 *         do_stuff(n);
 *     }
 * @remarks  Since the list is circular with a single dummy node it can be confusing to loop over. To help make this simpler, use
 *           `cf_list_begin` and `cf_list_end` to perform a loop over the list.
 * @related  CF_ListNode CF_List CF_LIST_NODE CF_LIST_HOST cf_list_init_node cf_list_init cf_list_push_front cf_list_push_back cf_list_remove cf_list_pop_front cf_list_pop_back cf_list_empty cf_list_begin cf_list_end cf_list_front cf_list_back
 */
CF_INLINE CF_ListNode* cf_list_end(CF_List* list)
{
	return &list->nodes;
}

/**
 * @function cf_list_front
 * @category list
 * @brief    Returns a pointer to the first element in the list.
 * @param    List       The list.
 * @remarks  Check to see if this is a valid node in the list with `node != cf_list_end(list)`.
 * @related  CF_ListNode CF_List CF_LIST_NODE CF_LIST_HOST cf_list_init_node cf_list_init cf_list_push_front cf_list_push_back cf_list_remove cf_list_pop_front cf_list_pop_back cf_list_empty cf_list_begin cf_list_end cf_list_front cf_list_back
 */
CF_INLINE CF_ListNode* cf_list_front(CF_List* list)
{
	return list->nodes.next;
}

/**
 * @function cf_list_back
 * @category list
 * @brief    Returns a pointer to the last element in the list.
 * @param    List       The list.
 * @remarks  Check to see if this is a valid node in the list with `node != cf_list_end(list)`.
 * @related  CF_ListNode CF_List CF_LIST_NODE CF_LIST_HOST cf_list_init_node cf_list_init cf_list_push_front cf_list_push_back cf_list_remove cf_list_pop_front cf_list_pop_back cf_list_empty cf_list_begin cf_list_end cf_list_front cf_list_back
 */
CF_INLINE CF_ListNode* cf_list_back(CF_List* list)
{
	return list->nodes.prev;
}

#ifdef __cplusplus
}
#endif // __cplusplus

//--------------------------------------------------------------------------------------------------
// C++ API

#ifdef CF_CPP

namespace Cute
{

using list_t = CF_List;
using list_node_t = CF_ListNode;

CF_INLINE void list_init_node(list_node_t* node) { cf_list_init_node((CF_ListNode*)node); };
CF_INLINE void list_init(list_t* list) { cf_list_init(list); };
CF_INLINE void list_push_front(list_t* list, list_node_t* node) { cf_list_push_front(list, (CF_ListNode*)node); };
CF_INLINE void list_push_back(list_t* list, list_node_t* node) { cf_list_push_back(list, (CF_ListNode*)node); };
CF_INLINE void list_remove(list_node_t* node) { cf_list_remove((CF_ListNode*)node); };
CF_INLINE list_node_t* list_pop_front(list_t* list) { return (list_node_t*)cf_list_pop_front(list); };
CF_INLINE list_node_t* list_pop_back(list_t* list) { return (list_node_t*)cf_list_pop_back(list); };
CF_INLINE int list_empty(list_t* list) { return cf_list_empty(list); };
CF_INLINE list_node_t* list_begin(list_t* list) { return (list_node_t*)cf_list_begin(list); };
CF_INLINE list_node_t* list_end(list_t* list) { return (list_node_t*)cf_list_end(list); };
CF_INLINE list_node_t* list_front(list_t* list) { return (list_node_t*)cf_list_front(list); };
CF_INLINE list_node_t* list_back(list_t* list) { return (list_node_t*)cf_list_back(list); };

}

#endif // CF_CPP

#endif // CF_DOUBLY_LINKED_LIST_H
