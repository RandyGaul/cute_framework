/*
	Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

#include "test_harness.h"

#include <cute.h>
using namespace Cute;

#ifndef CF_STATIC
#	define CUTE_ASEPRITE_IMPLEMENTATION
#	include <cute/cute_aseprite.h>
#endif

#include <internal/cute_aseprite_cache_internal.h>

#include <internal/cute_girl.h>

/* Load an aseprite file and destroy it. */
TEST_CASE(test_aseprite_make_destroy)
{
	ase_t* ase = cute_aseprite_load_from_memory(girl_data, girl_sz, NULL);
	cute_aseprite_free(ase);
	return true;
}

TEST_SUITE(test_aseprite)
{
	RUN_TEST_CASE(test_aseprite_make_destroy);
}
