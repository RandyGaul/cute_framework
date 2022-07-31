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

#include <cute_lru_cache.h>

CUTE_TEST_CASE(test_lru_cache, "Use all methods on lru cache and assert correctness.");
int test_lru_cache()
{
	cf_lru_cache<int, int> cache(3, NULL);

	*cache.insert(1) = 1;
	*cache.insert(2) = 2;
	*cache.insert(3) = 3;

	CUTE_TEST_ASSERT(*cache.lru() == 1);
	CUTE_TEST_ASSERT(*cache.mru() == 3);

	int val;
	CUTE_TEST_ASSERT(!cf_is_error(&cache.find(2, &val)));
	CUTE_TEST_ASSERT(*cache.lru() == 1);
	CUTE_TEST_ASSERT(*cache.mru() == 2);

	int* val_ptr = cache.find(1);
	CUTE_TEST_CHECK_POINTER(val_ptr);
	CUTE_TEST_ASSERT(*val_ptr == 1);
	CUTE_TEST_ASSERT(*cache.lru() == 3);
	CUTE_TEST_ASSERT(*cache.mru() == 1);

	cache.insert(4, 4);
	CUTE_TEST_ASSERT(*cache.lru() == 2);
	CUTE_TEST_ASSERT(*cache.mru() == 4);

	cache.remove(2);
	CUTE_TEST_ASSERT(*cache.lru() == 1);
	CUTE_TEST_ASSERT(*cache.mru() == 4);

	CUTE_TEST_ASSERT(cache.count() == 2);

	return 0;
}
