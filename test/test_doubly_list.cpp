/*
	Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

#include "test_harness.h"

#include <cute_doubly_list.h>

using namespace Cute;

/* Make list of three elements, perform all operations on it, assert correctness. */
TEST_CASE(test_doubly_list_operations)
{
	CF_List list;

	CF_ListNode a;
	CF_ListNode b;
	CF_ListNode c;

	cf_list_init(&list);
	cf_list_init_node(&a);
	cf_list_init_node(&b);
	cf_list_init_node(&c);

	REQUIRE(list.nodes.next == &list.nodes);
	REQUIRE(list.nodes.prev == &list.nodes);
	REQUIRE(a.next == &a);
	REQUIRE(a.prev == &a);
	REQUIRE(cf_list_empty(&list));

	cf_list_push_front(&list, &a);
	REQUIRE(!cf_list_empty(&list));
	REQUIRE(list.nodes.next == &a);
	REQUIRE(list.nodes.prev == &a);
	REQUIRE(list.nodes.next->next == &list.nodes);
	REQUIRE(list.nodes.prev->prev == &list.nodes);
	REQUIRE(cf_list_front(&list) == &a);
	REQUIRE(cf_list_back(&list) == &a);

	cf_list_push_front(&list, &b);
	REQUIRE(cf_list_front(&list) == &b);
	REQUIRE(cf_list_back(&list) == &a);

	cf_list_push_back(&list, &c);
	REQUIRE(cf_list_front(&list) == &b);
	REQUIRE(cf_list_back(&list) == &c);

	CF_ListNode* nodes[3] = { &b, &a, &c };
	int index = 0;
	for (CF_ListNode* n = cf_list_begin(&list); n != cf_list_end(&list); n = n->next)
	{
		REQUIRE(n == nodes[index++]);
	}

	REQUIRE(cf_list_pop_front(&list) == &b);
	REQUIRE(cf_list_pop_back(&list) == &c);
	REQUIRE(cf_list_pop_back(&list) == &a);

	REQUIRE(cf_list_empty(&list));

	return true;
}

TEST_SUITE(test_doubly_list)
{
	RUN_TEST_CASE(test_doubly_list_operations);
}
