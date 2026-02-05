/*
	Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

#include "test_harness.h"

#include <cute.h>
using namespace Cute;

/* Test the CF_MAP C macros and the C++ Map<T> wrapper. */
TEST_CASE(test_hashtable_macros)
{
	{
		CF_MAP(const char*) h = { 0 };
		map_set(h, 5, "yo");
		const char* val = map_get(h, 5);
		REQUIRE(!CF_STRCMP(val, "yo"));
		map_free(h);
	}
	{
		CF_MAP(v2) h = { 0 };
		map_set(h, 0, V2(1, 2));
		map_set(h, 1, V2(4, 10));
		map_set(h, 2, V2(-12, 13));
		v2 a = map_get(h, 0);
		v2 b = map_get(h, 1);
		v2 c = map_get(h, 2);
		REQUIRE(a.x == 1 && a.y == 2);
		REQUIRE(b.x == 4 && b.y == 10);
		REQUIRE(c.x == -12 && c.y == 13);
		map_del(h, 0);
		map_del(h, 1);
		map_del(h, 2);
		a = map_get(h, 0);
		b = map_get(h, 1);
		c = map_get(h, 2);
		REQUIRE(a.x == 0 && a.y == 0);
		REQUIRE(b.x == 0 && b.y == 0);
		REQUIRE(c.x == 0 && c.y == 0);
		map_free(h);
	}
	{
		CF_MAP(v2) h = { 0 };
		map_set(h, 0, V2(1, 2));
		map_set(h, 1, V2(4, 10));
		map_set(h, 2, V2(-12, 13));
		v2 *a = map_get_ptr(h, 0);
		v2 *b = map_get_ptr(h, 1);
		v2 *c = map_get_ptr(h, 2);
		REQUIRE(a->x == 1 && a->y == 2);
		REQUIRE(b->x == 4 && b->y == 10);
		REQUIRE(c->x == -12 && c->y == 13);
		map_del(h, 0);
		map_del(h, 1);
		map_del(h, 2);
		a = map_get_ptr(h, 0);
		b = map_get_ptr(h, 1);
		c = map_get_ptr(h, 2);
		REQUIRE(a == NULL);
		REQUIRE(b == NULL);
		REQUIRE(c == NULL);
		map_free(h);
	}
	{
		CF_MAP(v2) h = { 0 };
		int iters = 100;
		for (int i = 0; i < iters; ++i) {
			v2 v = V2((float)i, (float)i);
			map_set(h, i, v);
		}
		for (int i = 0; i < iters; ++i) {
			v2 v = map_get(h, i);
			REQUIRE(v.x == (float)i && v.y == (float)i);
		}
		for (int i = 0; i < iters; ++i) {
			map_del(h, i);
		}
		map_free(h);
	}
	{
		CF_MAP(int) h = { 0 };
		map_set(h, 10, 10);
		map_set(h, 2, 2);
		map_set(h, 5, 5);
		map_set(h, 7, 7);
		map_set(h, 1, 1);
		map_set(h, 3, 3);
		map_set(h, 9, 9);
		map_set(h, 6, 6);
		map_set(h, 4, 4);
		map_set(h, 0, 0);
		map_set(h, 8, 8);
		for (int i = 0; i < 11; ++i) {
			int a = map_get(h, i);
			REQUIRE(a == i);
		}
		map_free(h);
	}
	{
		CF_MAP(int) h = { 0 };
		map_set(h, sintern("eee"), 4);
		map_set(h, sintern("ddd"), 3);
		map_set(h, sintern("bbb"), 1);
		map_set(h, sintern("ccc"), 2);
		map_set(h, sintern("aaa"), 0);
		REQUIRE(map_get(h, sintern("aaa")) == 0);
		REQUIRE(map_get(h, sintern("bbb")) == 1);
		REQUIRE(map_get(h, sintern("ccc")) == 2);
		REQUIRE(map_get(h, sintern("ddd")) == 3);
		REQUIRE(map_get(h, sintern("eee")) == 4);
		map_free(h);
	}
	{
		Map<int> m;
		m.insert(10, 10);
		m.insert(2, 2);
		m.insert(5, 5);
		m.insert(7, 7);
		m.insert(1, 1);
		m.insert(3, 3);
		m.insert(9, 9);
		m.insert(6, 6);
		m.insert(4, 4);
		m.insert(0, 0);
		m.insert(8, 8);
		for (int i = 0; i < 11; ++i) {
			int a = m.get(i);
			REQUIRE(a == i);
		}
	}
	{
		Map<int> m;
		m.insert(sintern("eee"), 4);
		m.insert(sintern("ddd"), 3);
		m.insert(sintern("bbb"), 1);
		m.insert(sintern("ccc"), 2);
		m.insert(sintern("aaa"), 0);
		REQUIRE(m.get(sintern("aaa")) == 0);
		REQUIRE(m.get(sintern("bbb")) == 1);
		REQUIRE(m.get(sintern("ccc")) == 2);
		REQUIRE(m.get(sintern("ddd")) == 3);
		REQUIRE(m.get(sintern("eee")) == 4);
	}
	return true;
}

TEST_CASE(test_hashtable_has)
{
	CF_MAP(v2) h = { 0 };
	map_set(h, 0, V2(1, 2));
	map_set(h, 1, V2(4, 10));
	map_set(h, 2, V2(-12, 13));

	REQUIRE(!map_get_ptr(h, 42));
	REQUIRE(map_get_ptr(h, 0));

	map_del(h, 1);

	REQUIRE(!map_get_ptr(h, 1));
	REQUIRE(map_get_ptr(h, 2));

    map_free(h);

    return true;
}

TEST_SUITE(test_hashtable)
{
	RUN_TEST_CASE(test_hashtable_macros);
}
