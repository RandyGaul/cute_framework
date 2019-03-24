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

#include <cute_circular_buffer.h>
#include <cute_concurrency.h>
using namespace cute;

CUTE_TEST_CASE(test_circular_buffer_basic, "Typical use-case example, push and pull some data.");
int test_circular_buffer_basic()
{
	circular_buffer_t buffer = circular_buffer_make(1024);
	CUTE_TEST_CHECK_POINTER(buffer.data);

	const char* the_data = "Here's some data.";
	int the_data_size = (int)CUTE_STRLEN(the_data) + 1;

	CUTE_TEST_CHECK(circular_buffer_push(&buffer, the_data, the_data_size));
	void* pull_data = malloc(the_data_size);
	CUTE_TEST_CHECK(circular_buffer_pull(&buffer, pull_data, the_data_size));
	CUTE_TEST_ASSERT(memcmp(the_data, pull_data, the_data_size) == 0);
	free(pull_data);

	circular_buffer_free(&buffer);

	return 0;
}

CUTE_TEST_CASE(test_circular_buffer_fill_up_and_empty, "Fill up the buffer and empty it a few times.");
int test_circular_buffer_fill_up_and_empty()
{
	int bytes = 10;
	circular_buffer_t buffer = circular_buffer_make(bytes);
	CUTE_TEST_CHECK_POINTER(buffer.data);

	for (int iters = 0; iters < 5; ++iters)
	{
		for (int i = 0; i < bytes; ++i)
		{
			uint8_t byte = (uint8_t)i;
			CUTE_TEST_CHECK(circular_buffer_push(&buffer, &byte, 1));
		}

		for (int i = 0; i < bytes; ++i)
		{
			uint8_t byte;
			CUTE_TEST_CHECK(circular_buffer_pull(&buffer, &byte, 1));
			CUTE_TEST_ASSERT(byte == i);
		}
	}

	circular_buffer_free(&buffer);

	return 0;
}

CUTE_TEST_CASE(test_circular_buffer_overflow, "Attempt to push too much data to the buffer.");
int test_circular_buffer_overflow()
{
	int bytes = 10;
	circular_buffer_t buffer = circular_buffer_make(bytes);
	CUTE_TEST_CHECK_POINTER(buffer.data);

	for (int i = 0; i < bytes; ++i)
	{
		uint8_t byte = (uint8_t)i;
		CUTE_TEST_CHECK(circular_buffer_push(&buffer, &byte, 1));
	}

	uint8_t byte = 0;
	CUTE_TEST_CHECK(!circular_buffer_push(&buffer, &byte, 1));

	circular_buffer_free(&buffer);

	return 0;
}

CUTE_TEST_CASE(test_circular_buffer_underflow, "Attempt to pull too many bytes from the buffer.");
int test_circular_buffer_underflow()
{
	int bytes = 10;
	circular_buffer_t buffer = circular_buffer_make(bytes);
	CUTE_TEST_CHECK_POINTER(buffer.data);

	uint8_t byte = 0;
	CUTE_TEST_CHECK(!circular_buffer_pull(&buffer, &byte, 1));

	for (int i = 0; i < bytes; ++i)
	{
		uint8_t byte = (uint8_t)i;
		CUTE_TEST_CHECK(circular_buffer_push(&buffer, &byte, 1));
	}

	uint8_t bytes_11[11];
	CUTE_TEST_CHECK(!circular_buffer_pull(&buffer, bytes_11, 11));

	circular_buffer_free(&buffer);

	return 0;
}

int test_circular_buffer_running = 1;
int test_circular_buffer_two_threads_push(void *data)
{
	circular_buffer_t* buffer = (circular_buffer_t*)data;

	// Push incrementing integers into the buffer.
	int iters = 100;
	int val = 0;
	while (iters && test_circular_buffer_running)
	{
		int result = circular_buffer_push(buffer, &val, sizeof(int));
		if (result) continue;
		++val;
		--iters;
	}

	return 0;
}

int test_circular_buffer_two_threads_pull(void *data)
{
	circular_buffer_t* buffer = (circular_buffer_t*)data;

	// Pull integers out of the buffer, make sure they are incrementing.
	int iters = 100;
	int expected_val = 0;
	int got_val;
	while (iters)
	{
		int result = circular_buffer_pull(buffer, &got_val, sizeof(int));
		if (result) continue;
		CUTE_TEST_ASSERT(expected_val == got_val);
		++expected_val;
		--iters;
	}

	return 0;
}

CUTE_TEST_CASE(test_circular_buffer_two_threads, "Run a producer and a consumer thread, and validate input/output of the buffer.");
int test_circular_buffer_two_threads()
{
	for (int iters = 0; iters < 100; ++iters)
	{
		circular_buffer_t buffer = circular_buffer_make(sizeof(int) * 32);
		CUTE_TEST_CHECK_POINTER(buffer.data);

		thread_t* push = thread_create(test_circular_buffer_two_threads_push, "thread push", &buffer);
		thread_t* pull = thread_create(test_circular_buffer_two_threads_pull, "thread pull", &buffer);

		CUTE_TEST_CHECK(thread_wait(pull));
		test_circular_buffer_running = 0; // Let push thread know the pull thread early-exited, in case it fails.
		CUTE_TEST_CHECK(thread_wait(push));
		test_circular_buffer_running = 1; // Reset state for next time.

		circular_buffer_free(&buffer);
	}

	error_set(NULL);

	return 0;
}
