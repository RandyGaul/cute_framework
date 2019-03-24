#include <cute_handle_table.h>
using namespace cute;

CUTE_TEST_CASE(test_handle_basic, "Typical use-case example, alloc and free some handles.");
int test_handle_basic()
{
	handle_table_t table;
	CUTE_TEST_CHECK(handle_table_init(&table, 1024, NULL));

	cute::handle_t h0 = handle_table_alloc(&table, 7);
	cute::handle_t h1 = handle_table_alloc(&table, 13);
	CUTE_TEST_ASSERT(h0 != CUTE_INVALID_HANDLE);
	CUTE_TEST_ASSERT(h1 != CUTE_INVALID_HANDLE);
	int index0 = CUTE_HANDLE_INDEX(h0);
	int index1 = CUTE_HANDLE_INDEX(h1);
	CUTE_TEST_ASSERT(index0 == 7);
	CUTE_TEST_ASSERT(index1 == 13);

	handle_table_free(&table, h0);
	handle_table_free(&table, h1);

	h0 = handle_table_alloc(&table, 4);
	h1 = handle_table_alloc(&table, 267);
	CUTE_TEST_ASSERT(h0 != CUTE_INVALID_HANDLE);
	CUTE_TEST_ASSERT(h1 != CUTE_INVALID_HANDLE);
	index0 = CUTE_HANDLE_INDEX(h0);
	index1 = CUTE_HANDLE_INDEX(h1);
	CUTE_TEST_ASSERT(index0 == 4);
	CUTE_TEST_ASSERT(index1 == 267);

	h1 = handle_table_update_index(&table, h1, 9);
	CUTE_TEST_ASSERT(h1 != CUTE_INVALID_HANDLE);
	index1 = CUTE_HANDLE_INDEX(h1);
	CUTE_TEST_ASSERT(index1 == 9);

	handle_table_clean_up(&table);
	CUTE_TEST_ASSERT(table.handles == NULL);

	return 0;
}

CUTE_TEST_CASE(test_handle_large_loop, "Allocate right up the maximum size possible for `handle_table_t`, which is 65535.");
int test_handle_large_loop()
{
	handle_table_t table;
	CUTE_TEST_CHECK(handle_table_init(&table, 65535, NULL));

	for (int i = 0; i < 65535; ++i)
	{
		cute::handle_t h = handle_table_alloc(&table, i);
		CUTE_TEST_ASSERT(h != CUTE_INVALID_HANDLE);
		CUTE_ASSERT(CUTE_HANDLE_INDEX(h) == i);
	}

	handle_table_clean_up(&table);
	CUTE_TEST_ASSERT(table.handles == NULL);

	return 0;
}

CUTE_TEST_CASE(test_handle_large_loop_and_free, "\"Soak test\" to fill up the handle buffer and empty it a few times.");
int test_handle_large_loop_and_free()
{
	handle_table_t table;
	CUTE_TEST_CHECK(handle_table_init(&table, 65535, NULL));

	for (int iters = 0; iters < 5; ++iters)
	{
		for (int i = 0; i < 65535; ++i)
		{
			cute::handle_t h = handle_table_alloc(&table, i);
			CUTE_TEST_ASSERT(h != CUTE_INVALID_HANDLE);
			CUTE_ASSERT(CUTE_HANDLE_INDEX(h) == i);
		}

		CUTE_TEST_ASSERT(table.freelist == 65535);

		for (int i = 0; i < 65535; ++i)
		{
			handle_table_free(&table, i);
		}
	}

	handle_table_clean_up(&table);
	CUTE_TEST_ASSERT(table.handles == NULL);

	return 0;
}

CUTE_TEST_CASE(test_handle_alloc_too_many, "Allocating over 65535 entries should result in failure.");
int test_handle_alloc_too_many()
{
	handle_table_t table;
	CUTE_TEST_CHECK(handle_table_init(&table, 65535, NULL));

	for (int i = 0; i < 65535; ++i)
	{
		cute::handle_t h = handle_table_alloc(&table, i);
		CUTE_TEST_ASSERT(h != CUTE_INVALID_HANDLE);
		CUTE_ASSERT(CUTE_HANDLE_INDEX(h) == i);
	}

	cute::handle_t h = handle_table_alloc(&table, 0);
	CUTE_TEST_ASSERT(h == CUTE_INVALID_HANDLE);

	handle_table_clean_up(&table);
	CUTE_TEST_ASSERT(table.handles == NULL);

	return 0;
}
