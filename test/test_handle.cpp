/*
	Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

#include "test_harness.h"

#include <cute_handle_table.h>
using namespace Cute;

/* Typical use-case example, alloc and free some handles. */
TEST_CASE(test_handle_basic)
{
	CF_HandleTable* table = cf_make_handle_allocator(1024);
	CHECK_POINTER(table);

	CF_Handle h0 = cf_handle_allocator_alloc(table, 7, 0);
	CF_Handle h1 = cf_handle_allocator_alloc(table, 13, 0);
	REQUIRE(h0 != CF_INVALID_HANDLE);
	REQUIRE(h1 != CF_INVALID_HANDLE);
	uint32_t index0 = cf_handle_allocator_get_index(table, h0);
	uint32_t index1 = cf_handle_allocator_get_index(table, h1);
	REQUIRE(index0 == 7);
	REQUIRE(index1 == 13);

	cf_handle_allocator_free(table, h0);
	cf_handle_allocator_free(table, h1);

	h0 = cf_handle_allocator_alloc(table, 4, 0);
	h1 = cf_handle_allocator_alloc(table, 267, 0);
	REQUIRE(h0 != CF_INVALID_HANDLE);
	REQUIRE(h1 != CF_INVALID_HANDLE);
	index0 = cf_handle_allocator_get_index(table, h0);
	index1 = cf_handle_allocator_get_index(table, h1);
	REQUIRE(index0 == 4);
	REQUIRE(index1 == 267);

	cf_handle_allocator_update_index(table, h1, 9);
	index1 = cf_handle_allocator_get_index(table, h1);
	REQUIRE(index1 == 9);

	cf_destroy_handle_allocator(table);

	return true;
}

/* Allocate right up the maximum size possible for the table. */
TEST_CASE(test_handle_large_loop)
{
	CF_HandleTable* table = cf_make_handle_allocator(1024);
	CHECK_POINTER(table);

	for (int i = 0; i < 1024; ++i)
	{
		CF_Handle h = cf_handle_allocator_alloc(table, i, 0);
		REQUIRE(h != CF_INVALID_HANDLE);
		REQUIRE(cf_handle_allocator_get_index(table, h) == (uint32_t)i);
	}

	cf_destroy_handle_allocator(table);

	return true;
}

/* "Soak test" to fill up the handle buffer and empty it a few times. */
TEST_CASE(test_handle_large_loop_and_free)
{
	CF_HandleTable* table = cf_make_handle_allocator(1024);
	CHECK_POINTER(table);
	CF_Handle* handles = (CF_Handle*)malloc(sizeof(CF_Handle) * 2014);

	for (int iters = 0; iters < 5; ++iters)
	{
		for (int i = 0; i < 1024; ++i)
		{
			CF_Handle h = cf_handle_allocator_alloc(table, i, 0);
			REQUIRE(h != CF_INVALID_HANDLE);
			REQUIRE(cf_handle_allocator_get_index(table, h) == (uint32_t)i);
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

	return true;
}

/* Allocating over 1024 entries should not result in failure. */
TEST_CASE(test_handle_alloc_too_many)
{
	CF_HandleTable* table = cf_make_handle_allocator(1024);
	CHECK_POINTER(table);

	for (int i = 0; i < 1024; ++i)
	{
		CF_Handle h = cf_handle_allocator_alloc(table, i, 0);
		REQUIRE(h != CF_INVALID_HANDLE);
		REQUIRE(cf_handle_allocator_get_index(table, h) == (uint32_t)i);
	}

	CF_Handle h = cf_handle_allocator_alloc(table, 0, 0);
	REQUIRE(h != CF_INVALID_HANDLE);

	cf_destroy_handle_allocator(table);

	return true;
}

TEST_SUITE(test_handle)
{
	RUN_TEST_CASE(test_handle_basic);
	RUN_TEST_CASE(test_handle_large_loop);
	RUN_TEST_CASE(test_handle_large_loop_and_free);
	RUN_TEST_CASE(test_handle_alloc_too_many);
}