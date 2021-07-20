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

#include <cute_alloc.h>

#include <internal/cute_protocol_internal.h>
using namespace cute;

CUTE_TEST_CASE(test_hash_table_basic, "Create table, insert some elements, remove them, free table.");
int test_hash_table_basic()
{
	protocol::hashtable_t table;
	protocol::hashtable_init(&table, 8, 8, 20, NULL);

	uint64_t key = 5;
	uint64_t item = 10;

	protocol::hashtable_insert(&table, &key, &item);

	void* item_ptr = protocol::hashtable_find(&table, &key);
	CUTE_TEST_CHECK_POINTER(item_ptr);
	CUTE_TEST_ASSERT(*(uint64_t*)item_ptr == item);

	protocol::hashtable_cleanup(&table);

	return 0;
}

CUTE_TEST_CASE(test_hash_table_hammer, "Fill up table, many lookups, and reset, all a few times.");
int test_hash_table_hammer()
{
	protocol::hashtable_t table;
	protocol::hashtable_init(&table, 8, 8, 128, NULL);

	uint64_t keys[128];
	uint64_t items[128];

	for (int i = 0; i < 128; ++i)
	{
		keys[i] = i;
		items[i] = i * 2;
	}

	for (int iters = 0; iters < 10; ++iters)
	{
		for (int i = 0; i < 128; ++i)
			protocol::hashtable_insert(&table, keys + i, items + i);

		for (int i = 0; i < 128; ++i)
		{
			void* item_ptr = protocol::hashtable_find(&table, keys + i);
			CUTE_TEST_CHECK_POINTER(item_ptr);
			CUTE_TEST_ASSERT(*(uint64_t*)item_ptr == items[i]);
		}

		for (int i = 0; i < 128; ++i)
			protocol::hashtable_remove(&table, keys + i);
	}

	protocol::hashtable_cleanup(&table);

	return 0;
}

CUTE_TEST_CASE(test_hash_table_set, "Make sure the table also operates as a set (zero size value).");
int test_hash_table_set()
{
	protocol::hashtable_t table;
	protocol::hashtable_init(&table, 8, 0, 20, NULL);

	uint64_t key = 5;

	protocol::hashtable_insert(&table, &key, NULL);

	void* item_ptr = protocol::hashtable_find(&table, &key);
	CUTE_TEST_CHECK_POINTER(item_ptr);

	protocol::hashtable_cleanup(&table);

	return 0;
}
