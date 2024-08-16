/*
	Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

#include "test_harness.h"
#include "internal/cute_app_internal.h"

#include <cute.h>
using namespace Cute;

/* Create an aabb tree and destroy it. */
TEST_CASE(test_aabb_tree_make_and_destroy)
{
	CHECK(cf_is_error(cf_make_app(NULL, 0, 0, 0, 0, 0, APP_OPTIONS_HIDDEN_BIT | APP_OPTIONS_NO_AUDIO_BIT | APP_OPTIONS_NO_GFX_BIT, NULL)));
	CF_AabbTree tree = cf_make_aabb_tree(0);
	REQUIRE(tree.id != 0);
	cf_destroy_aabb_tree(tree);
	cf_destroy_app();
	return true;
}

TEST_SUITE(test_aabb_tree)
{
	RUN_TEST_CASE(test_aabb_tree_make_and_destroy);
}
