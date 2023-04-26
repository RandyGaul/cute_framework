/**
    @file pico_qt.h
    @brief A simple quadtree library

    ----------------------------------------------------------------------------
    Licensing information at end of header
    ----------------------------------------------------------------------------

    Features:
    ---------
    - Written in C99
    - Single header library for easy build system integration
    - Simple and concise API
    - Permissive license (zlib or public domain)

    Summary:
    --------
    This library builds and manages [Quadtrees](https://en.wikipedia.org/wiki/Quadtree).

    A quadtree is a data structure that can be used to perform efficient spatial
    queries. Items (values + bounds) are inserted into the tree. During this
    process, space in a quadtree is subdivided to make subsequent retrieval
    fast. Queries return values for all items that are contained within or
    overlap the search area.

    Currently, values are numeric. If uintptr_t is used they can also store a
    pointer. An integer value could represent an entity ID, an array index, a
    key for a hashtable etc...

    Depth:
    ------
    There is a tradeoff between space and time complexity. Lower depth values
    have smaller space requirements, but higher search times. Indeed, a tree
    with a depth of zero reduces to a linear bounds check. Higher values speed
    up searches, but at the cost of increased space. There are, however,
    diminishing returns with regard to increasing the max depth too high.
    Eventually all of the additional space is wasted with no benefit to
    performance.

    Usage:
    ------

    To use this library in your project, add the following

    > #define PICO_QT_IMPLEMENTATION
    > #include "pico_qt.h"

    to a source file (once), then simply include the header normally.

    Constants:
    --------
    - PICO_QT_NODE_CAPACITY (default: 16)
    - PICO_QT_QUERY_CAPACITY (default: 256)

    Must be defined before PICO_QT_IMPLEMENTATION
*/

#ifndef PICO_QT_H
#define PICO_QT_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef PICO_QT_USE_DOUBLE
/**
 * @brief Double precision floating point type
 */
typedef double qt_float;
#else
/**
 * @brief Single precision floating point type
 */
typedef float  qt_float;
#endif

#ifdef PICO_QT_USE_UINTPTR
/**
 * @brief Value data type that can store an integer or pointer
 */
typedef uintptr_t qt_value_t;
#else
/**
 * @brief Value data type that can store an integer
 */
typedef uint32_t qt_value_t;
#endif

/**
 * @brief Quadtree data structure
 */
typedef struct qt_t qt_t;

/**
 * @brief Rectangle for representing bounds
 */
typedef struct
{
    qt_float x, y, w, h;
} qt_rect_t;

/**
 * @brief Utility function for creating a rectangle
 */
qt_rect_t qt_make_rect(qt_float x, qt_float y, qt_float w, qt_float h);

/**
 * @brief Creates a quadtree with the specified global bounds
 *
 * @param bounds    The global bounds
 * @param max_depth Maximum height of the quadtree. See the summary for more
 *
 * @returns A quadtree instance
 */
qt_t* qt_create(qt_rect_t bounds, int max_depth);

/**
 * @brief Destroys a quadtree
 * @param qt The quadtree instance to destroy
 */
void qt_destroy(qt_t* qt);

/**
 * @brief Removes all nodes in the tree
 * @param qt The quadtree instance
 */
void qt_reset(qt_t* qt);

/**
 * @brief Inserts a value with the specified bounds into a quadtree
 *
 * @param qt     The quadtree instance
 * @param bounds The bounds associated with the value
 * @param value  The value to store in the tree
 */
void qt_insert(qt_t* qt, qt_rect_t bounds, qt_value_t value);

/**
 * @brief Searches for and removes a value in a quadtree
 *
 * This function is very inefficient. If numerous values need to be removed and
 * reinserted it is advisable to simply rebuild the tree.
 *
 * @param qt    The quadtree instance
 * @param value The value to remove
 * @returns True if the item was found, and false otherwise
 */
bool qt_remove(qt_t* qt, qt_value_t value);

/**
 * @brief Returns all values associated with items that are either overlapping
 * or contained within the search area
 *
 * @param qt   The quadtree instance
 * @param area The search area
 * @param size The number of values returned
 *
 * @returns The values of items contained within the search area. This array is
 * dynamically allocated and should be deallocated by using `qt_free` after use
 */
qt_value_t* qt_query(qt_t* qt, qt_rect_t area, int* size);

/**
 * @brief Function for deallocating the output of `qt_query`
 */
void qt_free(qt_value_t* array);

/**
 * @brief Removes all items in the tree
 *
 * This function preserves the internal structure of the tree making it much
 * faster than `qt_reset`. Reinserting values is probably faster too, however,
 * repeated calls to this function may result in fragmentation of the tree. The
 * function `qt_clean` can repair this fragmentation, however, it is expensive.
 *
 * @param qt The quadtree instance
 */
void qt_clear(qt_t* qt);

/**
 * @brief Resets the tree and reinserts all items
 *
 * This function can repair fragmentation resulting from repeated use of
 * `qt_remove` or `qt_clear`. This is an expensive operation since it must
 * extract all of the items from the tree, remove all of the nodes, and then
 * reinsert all of the items.
 *
 * @param qt The quadtree instance
 */
void qt_clean(qt_t* qt);

#ifdef __cplusplus
}
#endif

#endif // PICO_QT_H

#ifdef PICO_QT_IMPLEMENTATION

/*=============================================================================
 * Macros and constants
 *============================================================================*/

#ifdef NDEBUG
    #define PICO_QT_ASSERT(expr) ((void)0)
#else
    #ifndef PICO_QT_ASSERT
        #include <assert.h>
        #define PICO_QT_ASSERT(expr) (assert(expr))
    #endif
#endif

#if !defined(PICO_QT_MALLOC) || !defined(PICO_QT_REALLOC) || !defined(PICO_QT_FREE)
#include <stdlib.h>
#define PICO_QT_MALLOC(size)       (malloc(size))
#define PICO_QT_REALLOC(ptr, size) (realloc(ptr, size))
#define PICO_QT_FREE(ptr)          (free(ptr))
#endif

#ifndef PICO_QT_MEMCPY
    #include <string.h>
    #define PICO_QT_MEMCPY memcpy
#endif

#ifndef PICO_QT_MEMSET
    #include <string.h>
    #define PICO_QT_MEMSET memset
#endif

/*=============================================================================
 * Internal aliases
 *============================================================================*/

#define QT_ASSERT         PICO_QT_ASSERT
#define QT_MEMCPY         PICO_QT_MEMCPY
#define QT_MEMSET         PICO_QT_MEMSET
#define QT_MALLOC         PICO_QT_MALLOC
#define QT_REALLOC        PICO_QT_REALLOC
#define QT_FREE           PICO_QT_FREE

/*=============================================================================
 * Internal data structures
 *============================================================================*/

typedef struct qt_node_t qt_node_t;

typedef struct qt_array_header_t
{
    int capacity;
    int size;
    char* data;
    uint32_t cookie;
} qt_array_header_t;

// An optional empty macro used to markup dynamic arrays. This is useful to help remind
// us that a particular pointer is a dynamic array, and not merely a pointer.
#define qt_array

// Fetches the header `qt_array_header_t` of a dynamic array.
#define qt_hdr(a) ((qt_array_header_t*)a - 1)

// Helper to assert this pointer is indeed a dynamic array.
// Like a "canary in the coal mine".
#define QT_ARRAY_CANARY(a) ((a) ? QT_ASSERT(qt_hdr(a)->cookie == QT_ARRAY_COOKIE) : (void)0)

// A magic number for `QT_ARRAY_CANARY`.
#define QT_ARRAY_COOKIE 0xE6F7E359

// Returns the number of elements in the array.
// This is a proper l-value, so you can do e.g. qt_array_size(a)--
#define qt_array_len(a) (qt_hdr(a)->size)

// Returns the number of elements in the array.
// Passing in NULL will return 0.
#define qt_array_size(a) (QT_ARRAY_CANARY(a), a ? qt_hdr(a)->size : 0)

// Returns the capacity of the array.
#define qt_array_capacity(a) (QT_ARRAY_CANARY(a), (a) ? qt_hdr(a)->capacity : 0)

// Makes sure the capacity of the array can fit `n` elements.
#define qt_array_fit(a, n) ((n) <= qt_array_capacity(a) ? 0 : (*(void**)&(a) = qt_array_fit_impl((a), (n), sizeof(*a))))

// Pushes an element onto the array. Will resize itself as necessary.
#define qt_array_push(a, ...) (QT_ARRAY_CANARY(a), qt_array_fit((a), 1 + ((a) ? qt_array_size(a) : 0)), (a)[qt_array_len(a)++] = (__VA_ARGS__))

// Clears the array.
#define qt_array_clear(a) (QT_ARRAY_CANARY(a), a ? (void)(qt_array_len(a) = 0) : (void)0)

// Free's up a dynamic array.
#define qt_array_destroy(a) (QT_ARRAY_CANARY(a), a ? QT_FREE(qt_hdr(a)) : (void)0, a = NULL)

// Overwrites the item at the index with the item at the end of the array.
// This is fast, but it changes the order of the array Fortunately, order
// doesn't matter in this case.
#define qt_array_remove(a, i) (QT_ARRAY_CANARY(a), a[i] = a[--qt_array_len(a)])

struct qt_node_t
{
    int        depth;
    int        max_depth;
    qt_rect_t  bounds[4];
    qt_node_t* nodes[4];
    qt_array qt_value_t* values;
    qt_array qt_rect_t*  rects;
};

typedef struct qt_free_node_t
{
    struct qt_free_node_t* next;
} qt_free_node_t;

typedef struct qt_node_allocator_t
{
    qt_free_node_t* free_list;
    qt_array qt_node_t** blocks;
} qt_node_allocator_t;

struct qt_t
{
    qt_rect_t  bounds;
    qt_node_t* root;

    // A custom allocator is used here to allocate individual nodes.
    // This attempts to pack all the nodes together in memory to try and
    // keep them in similar cache lines. This is slightly superior to calling
    // `QT_MALLOC` once per node, where traversing the tree (if cold and not
    // currently in the CPU cache) would incur a cache miss for every single
    // node no matter what.
    qt_node_allocator_t arena;
};

/*=============================================================================
 * Internal function declarations
 *============================================================================*/

static void* qt_array_fit_impl(const void* array, int new_size, size_t element_size);
static bool qt_rect_contains(const qt_rect_t* r1, const qt_rect_t* r2);
static bool qt_rect_overlaps(const qt_rect_t* r1, const qt_rect_t* r2);
static qt_node_t* qt_node_create(qt_t* qt, qt_rect_t bounds, int depth, int max_depth);
static void qt_node_destroy(qt_t* qt, qt_node_t* node);
static void qt_node_insert(qt_t* qt, qt_node_t* node, const qt_rect_t* bounds, qt_value_t value);
static bool qt_node_remove(qt_node_t* node, qt_value_t value);
static qt_array qt_rect_t* qt_node_all_rects(const qt_node_t* node, qt_array qt_rect_t* rects);
static qt_array qt_value_t* qt_node_all_values(const qt_node_t* node, qt_array qt_value_t* values);
static qt_array qt_value_t* qt_node_query(const qt_node_t* node, const qt_rect_t* area, qt_array qt_value_t* values);
static void qt_node_clear(qt_node_t* node);

/*=============================================================================
 * Public API implementation
 *============================================================================*/

qt_t* qt_create(qt_rect_t bounds, int max_depth)
{
    qt_t* qt = (qt_t*)QT_MALLOC(sizeof(qt_t));
    QT_MEMSET(qt, 0, sizeof(*qt));

    if (!qt)
        return NULL;

    qt->bounds = bounds;
    qt->root = qt_node_create(qt, bounds, 0, max_depth);

    return qt;
}

void qt_destroy(qt_t* qt)
{
    QT_ASSERT(qt);

    qt_node_destroy(qt, qt->root);
    for (int i = 0; i < qt_array_size(qt->arena.blocks); ++i)
    {
        QT_FREE(qt->arena.blocks[i]);
    }
    qt_array_destroy(qt->arena.blocks);
    QT_FREE(qt);
}

void qt_reset(qt_t* qt)
{
    QT_ASSERT(qt);

    int max_depth = qt->root->max_depth;

    qt_node_destroy(qt, qt->root);

    qt->root = qt_node_create(qt, qt->bounds, 0, max_depth);
}

void qt_insert(qt_t* qt, qt_rect_t bounds, qt_value_t value)
{
    QT_ASSERT(qt);
    qt_node_insert(qt, qt->root, &bounds, value);
}

bool qt_remove(qt_t* qt, qt_value_t value)
{
    QT_ASSERT(qt);
    return qt_node_remove(qt->root, value);
}

qt_value_t* qt_query(qt_t* qt, qt_rect_t area, int* size)
{
    QT_ASSERT(qt);
    QT_ASSERT(size);

    // Size must be valid
    if (!size)
        return NULL;

    // Start query the root node
    qt_array qt_value_t* values = qt_node_query(qt->root, &area, NULL);

    // If no results then return NULL
    if (!values)
    {
        *size = 0;
        return NULL;
    }

    // Set size and return
    *size = qt_array_size(values);

    return values;
}

void qt_free(qt_value_t* array)
{
    if (!array)
        return;

    qt_array_destroy(array);
}

void qt_clear(qt_t* qt)
{
    QT_ASSERT(qt);
    qt_node_clear(qt->root);
}

void qt_clean(qt_t* qt)
{
    QT_ASSERT(qt);

    qt_array qt_rect_t* rects = qt_node_all_rects(qt->root, NULL);
    qt_array qt_value_t* values = qt_node_all_values(qt->root, NULL);
    QT_ASSERT(qt_array_size(rects) == qt_array_size(values));
    qt_reset(qt);

    for (int i = 0; i < qt_array_size(rects); i++)
    {
        qt_insert(qt, rects[i], values[i]);
    }

    qt_array_destroy(rects);
    qt_array_destroy(values);
}

/*=============================================================================
 * Internal function definitions
 *============================================================================*/

static int qt_max(int a, int b)
{
    return a > b ? a : b;
}

// Don't call this directly -- use `qt_array_fit` instead.
static void* qt_array_fit_impl(const void* array, int new_size, size_t element_size)
{
    QT_ARRAY_CANARY(array);

    // Double the old capacity, or pick at least 16 for the starting size.
    // This helps unnecessarily numerous realloc's for low capacities when starting out.
    int new_capacity = qt_max(2 * qt_array_capacity(array), qt_max(new_size, 16));
    QT_ASSERT(new_size <= new_capacity);

    // Total size of the header struct `qt_array_header_t` along with the size of all
    // elements packed together in a single allocation.
    size_t total_size = sizeof(qt_array_header_t) + new_capacity * element_size;
    qt_array_header_t* hdr;

    if (array)
    {
        // Realloc of the header isn't new.
        // This expands the capacity.
        hdr = (qt_array_header_t*)QT_REALLOC(qt_hdr(array), total_size);
    }

    else
    {
        // Create a new array if the pointer passed in was NULL, as NULL means an empty array.
        hdr = (qt_array_header_t*)QT_MALLOC(total_size);
        hdr->size = 0;
    }

    hdr->cookie = QT_ARRAY_COOKIE; // For sanity checks with `QT_ARRAY_CANARY`.
    hdr->capacity = new_capacity;
    hdr->data = (char*)(hdr + 1); // For debugging convenience.

    return (void*)(hdr + 1);
}

qt_rect_t qt_make_rect(qt_float x, qt_float y, qt_float w, qt_float h)
{
    qt_rect_t r = { x, y, w, h }; return r;
}

static bool qt_rect_contains(const qt_rect_t* r1, const qt_rect_t* r2)
{
    QT_ASSERT(r1);
    QT_ASSERT(r2);

    return r1->x <= r2->x &&
           r1->y <= r2->y &&
           r1->x + r1->w >= r2->x + r2->w &&
           r1->y + r1->h >= r2->y + r2->h;
}

static bool qt_rect_overlaps(const qt_rect_t* r1, const qt_rect_t* r2)
{
    QT_ASSERT(r1);
    QT_ASSERT(r2);

    return r1->x + r1->w >= r2->x &&
           r1->y + r1->h >= r2->y &&
           r2->x + r2->w >= r1->x &&
           r2->y + r2->h >= r1->y;
}

static qt_node_t* qt_node_create(qt_t* qt, qt_rect_t bounds, int depth, int max_depth)
{
    if (!qt->arena.free_list)
    {
        // Allocate space for more nodes and add them to the free list.
        const int block_count = 128;
        qt_node_t* nodes = (qt_node_t*)QT_MALLOC(sizeof(qt_node_t) * block_count);
        for (int i = 0; i < block_count; ++i)
        {
            qt_free_node_t* node = (qt_free_node_t*)(nodes + i);
            node->next = qt->arena.free_list;
            qt->arena.free_list = node;
        }
        qt_array_push(qt->arena.blocks, nodes);
    }

    // Pop a node off of the free list.
    qt_node_t* node = (qt_node_t*)qt->arena.free_list;
    qt->arena.free_list = qt->arena.free_list->next;
    QT_MEMSET(node, 0, sizeof(qt_node_t));

    node->depth = depth;
    node->max_depth = max_depth;

    // Calculate subdivided bounds
    bounds.w /= 2.0f;
    bounds.h /= 2.0f;

    // Calculates bounds of subtrees
    node->bounds[0] = qt_make_rect(bounds.x,
                                   bounds.y,
                                   bounds.w,
                                   bounds.h);

    node->bounds[1] = qt_make_rect(bounds.x + bounds.w,
                                   bounds.y,
                                   bounds.w,
                                   bounds.h);

    node->bounds[2] = qt_make_rect(bounds.x,
                                   bounds.y + bounds.h,
                                   bounds.w,
                                   bounds.h);

    node->bounds[3] = qt_make_rect(bounds.x + bounds.w,
                                   bounds.y + bounds.h,
                                   bounds.w,
                                   bounds.h);

    return node;
}

static void qt_node_destroy(qt_t* qt, qt_node_t* node)
{
    QT_ASSERT(node);
    
    qt_array_destroy(node->values);
    qt_array_destroy(node->rects);

    // Recursively destroy nodes
    for (int i = 0; i < 4; i++)
    {
        if (node->nodes[i])
            qt_node_destroy(qt, node->nodes[i]);
    }

    // Free current node by pushing it onto the free list
    qt_free_node_t* free_node = (qt_free_node_t*)node;
    free_node->next = qt->arena.free_list;
    qt->arena.free_list = free_node;
}

static void qt_node_insert(qt_t* qt, qt_node_t* node, const qt_rect_t* rect, qt_value_t value)
{
    QT_ASSERT(node);
    QT_ASSERT(rect);

    // The purpose of this function is to optimally fit the item into a subtree.
    // This occurs when the item is no longer fully contained within a subtree,
    // or the depth limit has been reached.

    // Checks to see if the depth limit has been reached. If it hasn't, try to
    // fit the item into a subtree
    if (node->depth + 1 < node->max_depth)
    {
        // Loop over child nodes
        for (int i = 0; i < 4; i++)
        {
            // Check if subtree contains the bounds
            if (qt_rect_contains(&node->bounds[i], rect))
            {
                // If child node does not exist, then create it
                if (!node->nodes[i])
                {
                    node->nodes[i] = qt_node_create(qt,
                                                    node->bounds[i],
                                                    node->depth + 1,
                                                    node->max_depth);
                }

                // Recursively try to insert the item into the subtree
                qt_node_insert(qt, node->nodes[i], rect, value);
                return;
            }
        }
    }

    // If none of the children fully contain the bounds, or the maximum depth
    // has been reached, then the item belongs to this node
    qt_array_push(node->rects, *rect);
    qt_array_push(node->values, value);
}

static bool qt_node_remove(qt_node_t* node, qt_value_t value)
{
    QT_ASSERT(node);

    // Searches the items in this node and, if found, removes the item with the
    // specified value
    for (int i = 0; i < qt_array_size(node->rects); i++)
    {
        // If value is found, then remove it and it's bounds from the node
        if (node->values[i] == value)
        {
            qt_array_remove(node->rects, i);
            qt_array_remove(node->values, i);
            return true;
        }
    }

    // If the item wasn't found, recursively search the subtrees of this node
    for (int i = 0; i < 4; i++)
    {
        if (node->nodes[i])
        {
            if (qt_node_remove(node->nodes[i], value))
                return true;
        }
    }

    // Value wasn't found
    return false;
}

static qt_array qt_rect_t* qt_node_all_rects(const qt_node_t* node, qt_array qt_rect_t* rects)
{
    QT_ASSERT(node);

    // Add all values in this node into the array
    for (int i = 0; i < qt_array_size(node->rects); ++i)
    {
        qt_array_push(rects, node->rects[i]);
    }

    // Recursively add all values found in the subtrees
    for (int i = 0; i < 4; i++)
    {
        if (node->nodes[i])
            rects = qt_node_all_rects(node->nodes[i], rects);
    }

    return rects;
}

static qt_array qt_value_t* qt_node_all_values(const qt_node_t* node, qt_array qt_value_t* values)
{
    QT_ASSERT(node);

    // Add all values in this node into the array
    for (int i = 0; i < qt_array_size(node->values); ++i)
    {
        qt_array_push(values, node->values[i]);
    }

    // Recursively add all values found in the subtrees
    for (int i = 0; i < 4; i++)
    {
        if (node->nodes[i])
            values = qt_node_all_values(node->nodes[i], values);
    }

    return values;
}

static qt_array qt_value_t* qt_node_query(const qt_node_t* node, const qt_rect_t* area, qt_array qt_value_t* values)
{
    QT_ASSERT(node);
    QT_ASSERT(area);

    // Searches for items in this node that intersect the area and adds them to
    // the array
    for (int i = 0; i < qt_array_size(node->rects); i++)
    {
        const qt_rect_t* rect = &node->rects[i];

        if (qt_rect_overlaps(area, rect))
            qt_array_push(values, node->values[i]);
    }

    // Loop over subtrees
    for (int i = 0; i < 4; i++)
    {
        if (node->nodes[i])
        {
            // If the area contains the the entire subtree, all items in the
            // subtree match and are recursively added to the array
            if (qt_rect_contains(area, &node->bounds[i]))
            {
                values = qt_node_all_values(node->nodes[i], values);
            }
            else
            {
                // Otherwise, if the area intersects the bounds of the subtree,
                // the subtree is recursively searched for items intersecting
                // or contained within the area
                if (qt_rect_overlaps(area, &node->bounds[i]))
                {
                    values = qt_node_query(node->nodes[i], area, values);
                }
            }
        }
    }

    return values;
}

static void qt_node_clear(qt_node_t* node)
{
    qt_array_clear(node->rects);
    qt_array_clear(node->values);

    for (int i = 0; i < 4; i++)
    {
        if (node->nodes[i])
            qt_node_clear(node->nodes[i]);
    }
}

#endif // PICO_QT_IMPLEMENTATION

/*
    ----------------------------------------------------------------------------
    This software is available under two licenses (A) or (B). You may choose
    either one as you wish:
    ----------------------------------------------------------------------------

    (A) The zlib License

    Copyright (c) 2022 James McLean

    This software is provided 'as-is', without any express or implied warranty.
    In no event will the authors be held liable for any damages arising from the
    use of this software.

    Permission is granted to anyone to use this software for any purpose,
    including commercial applications, and to alter it and redistribute it
    freely, subject to the following restrictions:

    1. The origin of this software must not be misrepresented; you must not
    claim that you wrote the original software. If you use this software in a
    product, an acknowledgment in the product documentation would be appreciated
    but is not required.

    2. Altered source versions must be plainly marked as such, and must not be
    misrepresented as being the original software.

    3. This notice may not be removed or altered from any source distribution.

    ----------------------------------------------------------------------------

    (B) Public Domain (www.unlicense.org)

    This is free and unencumbered software released into the public domain.

    Anyone is free to copy, modify, publish, use, compile, sell, or distribute
    this software, either in source code form or as a compiled binary, for any
    purpose, commercial or non-commercial, and by any means.

    In jurisdictions that recognize copyright laws, the author or authors of
    this software dedicate any and all copyright interest in the software to the
    public domain. We make this dedication for the benefit of the public at
    large and to the detriment of our heirs and successors. We intend this
    dedication to be an overt act of relinquishment in perpetuity of all present
    and future rights to this software under copyright law.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
    ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
    WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
