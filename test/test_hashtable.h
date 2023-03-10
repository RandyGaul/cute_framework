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
		CF_ASSERT(a.x == 1 && a.y == 2);
		CF_ASSERT(b.x == 4 && b.y == 10);
		CF_ASSERT(c.x == -12 && c.y == 13);
		hdel(h, 0);
		hdel(h, 1);
		hdel(h, 2);
		a = hget(h, 0);
		b = hget(h, 1);
		c = hget(h, 2);
		CF_ASSERT(a.x == 0 && a.y == 0);
		CF_ASSERT(b.x == 0 && b.y == 0);
		CF_ASSERT(c.x == 0 && c.y == 0);
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
			CF_ASSERT(v.x == (float)i && v.y == (float)i);
		}
		for (int i = 0; i < iters; ++i) {
			hdel(h, i);
		}
		hfree(h);
	}
	return 0;
}
