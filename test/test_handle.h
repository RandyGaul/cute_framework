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

#include <cute_handle_table.h>
using namespace Cute;

CF_TEST_CASE(test_handle_basic, "Typical use-case example, alloc and free some handles.");
int test_handle_basic()
{
	CF_HandleTable* table = cf_make_handle_allocator(1024);
	CF_TEST_CHECK_POINTER(table);

	CF_Handle h0 = cf_handle_allocator_alloc(table, 7, 0);
	CF_Handle h1 = cf_handle_allocator_alloc(table, 13, 0);
	CF_TEST_ASSERT(h0 != CF_INVALID_HANDLE);
	CF_TEST_ASSERT(h1 != CF_INVALID_HANDLE);
	uint32_t index0 = cf_handle_allocator_get_index(table, h0);
	uint32_t index1 = cf_handle_allocator_get_index(table, h1);
	CF_TEST_ASSERT(index0 == 7);
	CF_TEST_ASSERT(index1 == 13);

	cf_handle_allocator_free(table, h0);
	cf_handle_allocator_free(table, h1);

	h0 = cf_handle_allocator_alloc(table, 4, 0);
	h1 = cf_handle_allocator_alloc(table, 267, 0);
	CF_TEST_ASSERT(h0 != CF_INVALID_HANDLE);
	CF_TEST_ASSERT(h1 != CF_INVALID_HANDLE);
	index0 = cf_handle_allocator_get_index(table, h0);
	index1 = cf_handle_allocator_get_index(table, h1);
	CF_TEST_ASSERT(index0 == 4);
	CF_TEST_ASSERT(index1 == 267);

	cf_handle_allocator_update_index(table, h1, 9);
	index1 = cf_handle_allocator_get_index(table, h1);
	CF_TEST_ASSERT(index1 == 9);

	cf_destroy_handle_allocator(table);

	return 0;
}

CF_TEST_CASE(test_handle_large_loop, "Allocate right up the maximum size possible for the table.");
int test_handle_large_loop()
{
	CF_HandleTable* table = cf_make_handle_allocator(1024);
	CF_TEST_CHECK_POINTER(table);

	for (int i = 0; i < 1024; ++i)
	{
		CF_Handle h = cf_handle_allocator_alloc(table, i, 0);
		CF_TEST_ASSERT(h != CF_INVALID_HANDLE);
		CF_ASSERT(cf_handle_allocator_get_index(table, h) == (uint32_t)i);
	}

	cf_destroy_handle_allocator(table);

	return 0;
}

CF_TEST_CASE(test_handle_large_loop_and_free, "\"Soak test\" to fill up the handle buffer and empty it a few times.");
int test_handle_large_loop_and_free()
{
	CF_HandleTable* table = cf_make_handle_allocator(1024);
	CF_TEST_CHECK_POINTER(table);
	CF_Handle* handles = (CF_Handle*)malloc(sizeof(CF_Handle) * 2014);

	for (int iters = 0; iters < 5; ++iters)
	{
		for (int i = 0; i < 1024; ++i)
		{
			CF_Handle h = cf_handle_allocator_alloc(table, i, 0);
			CF_TEST_ASSERT(h != CF_INVALID_HANDLE);
			CF_ASSERT(cf_handle_allocator_get_index(table, h) == (uint32_t)i);
			handles[i] = h;
		}

		for (int i = 0; i < 1024; ++i)
		{
			CF_Handle h = handles[i];
			cf_handle_allocator_free(table, h);
		}
	}

	cf_destroy_handle_allocator(table);
	free(handles);

	return 0;
}

CF_TEST_CASE(test_handle_alloc_too_many, "Allocating over 1024 entries should not result in failure.");
int test_handle_alloc_too_many()
{
	CF_HandleTable* table = cf_make_handle_allocator(1024);
	CF_TEST_CHECK_POINTER(table);

	for (int i = 0; i < 1024; ++i)
	{
		CF_Handle h = cf_handle_allocator_alloc(table, i, 0);
		CF_TEST_ASSERT(h != CF_INVALID_HANDLE);
		CF_ASSERT(cf_handle_allocator_get_index(table, h) == (uint32_t)i);
	}

	CF_Handle h = cf_handle_allocator_alloc(table, 0, 0);
	CF_TEST_ASSERT(h != CF_INVALID_HANDLE);

	cf_destroy_handle_allocator(table);

	return 0;
}
