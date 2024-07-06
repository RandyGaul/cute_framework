/*
	Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

#include "test_harness.h"

#include <cute.h>

using namespace Cute;

void coroutine_func(CF_Coroutine co)
{
	int a, b;
	cf_coroutine_pop(co, &a, sizeof(a));
	cf_coroutine_pop(co, &b, sizeof(b));
	cf_coroutine_yield(co);

	int c = a * b;
	cf_coroutine_push(co, &c, sizeof(c));
}

void coroutine_wait_func(CF_Coroutine co)
{
	coroutine_yield(co);

	int a = 3;
	cf_coroutine_push(co, &a, sizeof(a));
}

/* Call some coroutine functions or whatever. */
TEST_CASE(test_basic)
{
	CF_Coroutine co = cf_make_coroutine(coroutine_func, 0, NULL);
	int a = 5;
	int b = 10;
	int c = 0;
	cf_coroutine_push(co, &a, sizeof(a));
	cf_coroutine_push(co, &b, sizeof(b));
	cf_coroutine_resume(co);
	REQUIRE(c == 0);
	cf_coroutine_resume(co);
	cf_coroutine_pop(co, &c, sizeof(c));
	REQUIRE(c == 50);
	cf_destroy_coroutine(co);

	co = cf_make_coroutine(coroutine_func, 0, NULL);
	a = 5;
	b = 10;
	c = 0;
	cf_coroutine_push(co, &a, sizeof(a));
	cf_coroutine_push(co, &b, sizeof(b));
	cf_coroutine_resume(co);
	REQUIRE(c == 0);
	cf_coroutine_resume(co);
	cf_coroutine_pop(co, &c, sizeof(c));
	REQUIRE(c == 50);
	cf_destroy_coroutine(co);

	return true;
}

TEST_SUITE(test_coroutine)
{
	RUN_TEST_CASE(test_basic);
}
