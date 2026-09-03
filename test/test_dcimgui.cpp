/*
	Cute Framework
	Copyright (C) 2026 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

// dcimgui.h is deliberately the first include. It wraps imconfig.h in extern "C", and imconfig.h
// pulls in cute_math.h, so this must compile before any CF header has been seen.
#include <dcimgui.h>
#include <dcimgui_internal.h>

#include <cute.h>
#include "test_harness.h"

TEST_CASE(test_dcimgui_headers_first_in_cpp)
{
	REQUIRE(sizeof(ImFontAtlasBuilder) > 0);
	REQUIRE(sizeof(ImVec2) == sizeof(CF_V2));
	return true;
}

TEST_SUITE(test_dcimgui)
{
	RUN_TEST_CASE(test_dcimgui_headers_first_in_cpp);
}
