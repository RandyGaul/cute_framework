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

#include <cute.h>
using namespace Cute;

CF_TEST_CASE(test_hashtable_macros, "Call the hashtable APIs through the h*** macros.");
int test_hashtable_macros()
{
	{
		const char** h = NULL;
		hset(h, 5, "yo");
		const char* val = hget(h, 5);
		CF_TEST_ASSERT(!CF_STRCMP(val, "yo"));
		hfree(h);
	}
	{
		v2* h = NULL;
		hset(h, 0, V2(1, 2));
		hset(h, 1, V2(4, 10));
		hset(h, 2, V2(-12, 13));
		v2 a = hget(h, 0);
		v2 b = hget(h, 1);
		v2 c = hget(h, 2);
		CF_TEST_ASSERT(a.x == 1 && a.y == 2);
		CF_TEST_ASSERT(b.x == 4 && b.y == 10);
		CF_TEST_ASSERT(c.x == -12 && c.y == 13);
		hdel(h, 0);
		hdel(h, 1);
		hdel(h, 2);
		a = hget(h, 0);
		b = hget(h, 1);
		c = hget(h, 2);
		CF_TEST_ASSERT(a.x == 0 && a.y == 0);
		CF_TEST_ASSERT(b.x == 0 && b.y == 0);
		CF_TEST_ASSERT(c.x == 0 && c.y == 0);
		hfree(h);
	}
	{
		v2* h = NULL;
		int iters = 100;
		for (int i = 0; i < iters; ++i) {
			v2 v = V2((float)i, (float)i);
			hset(h, i, v);
		}
		for (int i = 0; i < iters; ++i) {
			v2 v = hget(h, i);
			CF_TEST_ASSERT(v.x == (float)i && v.y == (float)i);
		}
		for (int i = 0; i < iters; ++i) {
			hdel(h, i);
		}
		hfree(h);
	}
	{
		int* h = NULL;
		hadd(h, 10, 10);
		hadd(h, 2, 2);
		hadd(h, 5, 5);
		hadd(h, 7, 7);
		hadd(h, 1, 1);
		hadd(h, 3, 3);
		hadd(h, 9, 9);
		hadd(h, 6, 6);
		hadd(h, 4, 4);
		hadd(h, 0, 0);
		hadd(h, 8, 8);
		hsort(h);
		for (int i = 0; i < hcount(h) - 1; ++i) {
			int a = hget(h, i);
			int b = hget(h, i + 1);
			CF_TEST_ASSERT(a < b);
		}
		hfree(h);
	}
	{
		int* h = NULL;
		hadd(h, sintern("eee"), 4);
		hadd(h, sintern("ddd"), 3);
		hadd(h, sintern("bbb"), 1);
		hadd(h, sintern("ccc"), 2);
		hadd(h, sintern("aaa"), 0);
		hssort(h);
		CF_TEST_ASSERT(hget(h, sintern("aaa")) == 0);
		CF_TEST_ASSERT(hget(h, sintern("bbb")) == 1);
		CF_TEST_ASSERT(hget(h, sintern("ccc")) == 2);
		CF_TEST_ASSERT(hget(h, sintern("ddd")) == 3);
		CF_TEST_ASSERT(hget(h, sintern("eee")) == 4);
		CF_TEST_ASSERT(hget(h, sintern("aaa")) < hget(h, sintern("bbb")));
		CF_TEST_ASSERT(hget(h, sintern("bbb")) < hget(h, sintern("ccc")));
		CF_TEST_ASSERT(hget(h, sintern("ccc")) < hget(h, sintern("ddd")));
		CF_TEST_ASSERT(hget(h, sintern("ddd")) < hget(h, sintern("eee")));
		const char* keys[] = {
			sintern("aaa"),
			sintern("bbb"),
			sintern("ccc"),
			sintern("ddd"),
			sintern("eee"),
		};
		for (int i = 0; i < 5 - 1; ++i) {
			int a = hget(h, keys[i]);
			int b = hget(h, keys[i + 1]);
			CF_TEST_ASSERT(a < b);
		}
		hfree(h);
	}
	{
		Map<int, int> m;
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
		m.sort_by_keys([](int a, int b) { return a < b; });
		for (int i = 0; i < m.count() - 1; ++i) {
			int a = m.get(i);
			int b = m.get(i + 1);
			CF_TEST_ASSERT(a < b);
		}
	}
	{
		Map<const char*, int> m;
		m.insert(sintern("eee"), 4);
		m.insert(sintern("ddd"), 3);
		m.insert(sintern("bbb"), 1);
		m.insert(sintern("ccc"), 2);
		m.insert(sintern("aaa"), 0);
		m.sort_by_keys([](const char* a, const char* b) { return sicmp(a, b); });
		CF_TEST_ASSERT(m.get(sintern("aaa")) == 0);
		CF_TEST_ASSERT(m.get(sintern("bbb")) == 1);
		CF_TEST_ASSERT(m.get(sintern("ccc")) == 2);
		CF_TEST_ASSERT(m.get(sintern("ddd")) == 3);
		CF_TEST_ASSERT(m.get(sintern("eee")) == 4);
		CF_TEST_ASSERT(m.get(sintern("aaa")) < m.get(sintern("bbb")));
		CF_TEST_ASSERT(m.get(sintern("bbb")) < m.get(sintern("ccc")));
		CF_TEST_ASSERT(m.get(sintern("ccc")) < m.get(sintern("ddd")));
		CF_TEST_ASSERT(m.get(sintern("ddd")) < m.get(sintern("eee")));
		const char* keys[] = {
			sintern("aaa"),
			sintern("bbb"),
			sintern("ccc"),
			sintern("ddd"),
			sintern("eee"),
		};
		for (int i = 0; i < 5 - 1; ++i) {
			int a = m.get(keys[i]);
			int b = m.get(keys[i + 1]);
			CF_TEST_ASSERT(a < b);
		}
	}
	return 0;
}
