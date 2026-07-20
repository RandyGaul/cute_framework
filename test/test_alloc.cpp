/*
	Cute Framework
	Copyright (C) 2026 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

#include "test_harness.h"

#include <cute.h>
using namespace Cute;

//--------------------------------------------------------------------------------------------------
// cf_allocator_override must forward CF_Allocator::udata to the custom function pointers.

static void* s_last_alloc_udata = NULL;
static void* s_last_free_udata = NULL;
static void* s_last_calloc_udata = NULL;
static void* s_last_realloc_udata = NULL;

static void* s_test_alloc(size_t size, void* udata)
{
	s_last_alloc_udata = udata;
	return malloc(size);
}

static void s_test_free(void* ptr, void* udata)
{
	s_last_free_udata = udata;
	free(ptr);
}

static void* s_test_calloc(size_t size, size_t count, void* udata)
{
	s_last_calloc_udata = udata;
	return calloc(size, count);
}

static void* s_test_realloc(void* ptr, size_t size, void* udata)
{
	s_last_realloc_udata = udata;
	return realloc(ptr, size);
}

TEST_CASE(test_allocator_forwards_udata)
{
	int sentinel = 0;
	void* expected_udata = &sentinel;

	CF_Allocator allocator;
	allocator.udata = expected_udata;
	allocator.alloc_fn = s_test_alloc;
	allocator.free_fn = s_test_free;
	allocator.calloc_fn = s_test_calloc;
	allocator.realloc_fn = s_test_realloc;
	cf_allocator_override(allocator);

	void* a = cf_alloc(16);
	REQUIRE(s_last_alloc_udata == expected_udata);

	void* c = cf_calloc(4, 4);
	REQUIRE(s_last_calloc_udata == expected_udata);

	void* r = cf_realloc(c, 32);
	REQUIRE(s_last_realloc_udata == expected_udata);

	cf_free(a);
	REQUIRE(s_last_free_udata == expected_udata);

	cf_free(r);

	cf_allocator_restore_default();

	return true;
}

//--------------------------------------------------------------------------------------------------
// Test suite.

TEST_SUITE(test_alloc)
{
	RUN_TEST_CASE(test_allocator_forwards_udata);
}
