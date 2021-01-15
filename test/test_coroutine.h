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

using namespace cute;

void coroutine_func(coroutine_t* co)
{
	int a, b;
	coroutine_pop(co, &a, sizeof(a));
	coroutine_pop(co, &b, sizeof(b));
	coroutine_yield(co);

	int c = a * b;
	coroutine_push(co, &c, sizeof(c));
}

CUTE_TEST_CASE(test_coroutine, "Call some coroutine functions or whatever.");
int test_coroutine()
{
	coroutine_t* co = coroutine_make(coroutine_func);
	int a = 5;
	int b = 10;
	int c = 0;
	coroutine_push(co, &a, sizeof(a));
	coroutine_push(co, &b, sizeof(b));
	coroutine_resume(co);
	CUTE_TEST_ASSERT(c == 0);
	coroutine_resume(co);
	coroutine_pop(co, &c, sizeof(c));
	CUTE_TEST_ASSERT(c == 50);

	coroutine_destroy(co);

	return 0;
}
