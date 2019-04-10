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

#include <cute_doubly_list.h>

using namespace cute;

CUTE_TEST_CASE(test_doubly_list, "Make list of three elements, perform all operations on it, assert correctness.");
int test_doubly_list()
{
	list_t list;

	list_node_t a;
	list_node_t b;
	list_node_t c;

	list_init(&list);
	list_init_node(&a);
	list_init_node(&b);
	list_init_node(&c);

	CUTE_TEST_ASSERT(list.nodes.next == &list.nodes);
	CUTE_TEST_ASSERT(list.nodes.prev == &list.nodes);
	CUTE_TEST_ASSERT(a.next == &a);
	CUTE_TEST_ASSERT(a.prev == &a);
	CUTE_TEST_ASSERT(list_empty(&list));

	list_push_front(&list, &a);
	CUTE_TEST_ASSERT(!list_empty(&list));
	CUTE_TEST_ASSERT(list.nodes.next == &a);
	CUTE_TEST_ASSERT(list.nodes.prev == &a);
	CUTE_TEST_ASSERT(list.nodes.next->next == &list.nodes);
	CUTE_TEST_ASSERT(list.nodes.prev->prev == &list.nodes);

	list_push_front(&list, &b);
	list_push_front(&list, &c);

	list_node_t* nodes[3] = { &c, &b, &a };
	int index = 0;
	for (list_node_t* n = list_begin(&list); n != list_end(&list); n = n->next)
	{
		CUTE_TEST_ASSERT(n == nodes[index++]);
	}

	CUTE_TEST_ASSERT(list_pop_front(&list) == &c);
	CUTE_TEST_ASSERT(list_pop_front(&list) == &b);
	CUTE_TEST_ASSERT(list_pop_back(&list) == &a);

	CUTE_TEST_ASSERT(list_empty(&list));

	return 0;
}
