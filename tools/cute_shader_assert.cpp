/*
	Cute Framework
	Copyright (C) 2026 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info

	Default CF_ASSERT handler for standalone shader tools (cute-shaderc), needed
	because cute_alloc.cpp's CF_ASSERT calls reference g_assert_fn. CF apps get
	g_assert_fn from src/cute_app.cpp instead; this must only be linked into
	executables that do not link the cute library.
*/

#include <stdio.h>

#include <cute_defines.h>
#include <scottt/debugbreak.h>

static void s_default_assert(bool expr, const char* message, const char* file, int line)
{
	if (!expr) {
		fprintf(stderr, "CF_ASSERT(%s) : %s, line %d\n", message, file, line);
		debug_break();
	}
}

cf_assert_fn* g_assert_fn = s_default_assert;
