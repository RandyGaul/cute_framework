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

#ifndef CUTE_HASHTABLE_H
#define CUTE_HASHTABLE_H

#include <cute_defines.h>

namespace cute
{

struct hashtable_slot_t
{
	uint64_t key_hash;
	int item_index;
	int base_count;
};

struct hashtable_t
{
	int count;
	int slot_capacity;
	hashtable_slot_t* slots;

	int key_size;
	int item_size;
	int item_capacity;
	void* items_key;
	int* items_slot_index;
	void* items_data;

	void* temp_key;
	void* temp_item;
	void* mem_ctx;
};

CUTE_API int CUTE_CALL hashtable_init(hashtable_t* table, int key_size, int item_size, int capacity, void* mem_ctx);
CUTE_API void CUTE_CALL hashtable_cleanup(hashtable_t* table);

CUTE_API void* CUTE_CALL hashtable_insert(hashtable_t* table, const void* key, const void* item);
CUTE_API void CUTE_CALL hashtable_remove(hashtable_t* table, const void* key);
CUTE_API void CUTE_CALL hashtable_clear(hashtable_t* table);
CUTE_API void* CUTE_CALL hashtable_find(const hashtable_t* table, const void* key);
CUTE_API int CUTE_CALL hashtable_count(const hashtable_t* table);
CUTE_API void* CUTE_CALL hashtable_items(const hashtable_t* table);
CUTE_API void* CUTE_CALL hashtable_keys(const hashtable_t* table);
CUTE_API void CUTE_CALL hashtable_swap(hashtable_t* table, int index_a, int index_b);

}

#endif // CUTE_HASHTABLE_H
