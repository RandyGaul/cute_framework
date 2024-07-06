/*
	Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

#include "test_harness.h"

#include <cute_c_runtime.h>
#include <cute_circular_buffer.h>
#include <cute_multithreading.h>
using namespace Cute;

/* Typical use-case example, push and pull some data. */
TEST_CASE(test_circular_buffer_basic)
{
	CF_CircularBuffer buffer = cf_make_circular_buffer(1024);
	CHECK_POINTER(buffer.data);

	const char* the_data = "Here's some data.";
	int the_data_size = (int)CF_STRLEN(the_data) + 1;

	CHECK(cf_circular_buffer_push(&buffer, the_data, the_data_size));
	void* pull_data = malloc(the_data_size);
	CHECK(cf_circular_buffer_pull(&buffer, pull_data, the_data_size));
	REQUIRE(memcmp(the_data, pull_data, the_data_size) == 0);
	free(pull_data);

	cf_destroy_circular_buffer(&buffer);

	return true;
}

/* Fill up the buffer and empty it a few times. */
TEST_CASE(test_circular_buffer_fill_up_and_empty)
{
	int bytes = 10;
	CF_CircularBuffer buffer = cf_make_circular_buffer(bytes);
	CHECK_POINTER(buffer.data);

	for (int iters = 0; iters < 5; ++iters) {
		for (int i = 0; i < bytes; ++i) {
			uint8_t byte = (uint8_t)i;
			CHECK(cf_circular_buffer_push(&buffer, &byte, 1));
		}

		for (int i = 0; i < bytes; ++i) {
			uint8_t byte;
			CHECK(cf_circular_buffer_pull(&buffer, &byte, 1));
			REQUIRE(byte == i);
		}
	}

	cf_destroy_circular_buffer(&buffer);

	return true;
}

/* Attempt to push too much data to the buffer. */
TEST_CASE(test_circular_buffer_overflow)
{
	int bytes = 10;
	CF_CircularBuffer buffer = cf_make_circular_buffer(bytes);
	CHECK_POINTER(buffer.data);

	for (int i = 0; i < bytes; ++i) {
		uint8_t byte = (uint8_t)i;
		CHECK(cf_circular_buffer_push(&buffer, &byte, 1));
	}

	uint8_t byte = 0;
	CHECK(!cf_circular_buffer_push(&buffer, &byte, 1));

	cf_destroy_circular_buffer(&buffer);

	return true;
}

/* Attempt to pull too many bytes from the buffer. */
TEST_CASE(test_circular_buffer_underflow)
{
	int bytes = 10;
	CF_CircularBuffer buffer = cf_make_circular_buffer(bytes);
	CHECK_POINTER(buffer.data);

	uint8_t byte = 0;
	CHECK(!cf_circular_buffer_pull(&buffer, &byte, 1));

	for (int i = 0; i < bytes; ++i) {
		uint8_t byte = (uint8_t)i;
		CHECK(cf_circular_buffer_push(&buffer, &byte, 1));
	}

	uint8_t bytes_11[11];
	CHECK(!cf_circular_buffer_pull(&buffer, bytes_11, 11));

	cf_destroy_circular_buffer(&buffer);

	return true;
}

int test_circular_buffer_running = 1;
int test_CircularBufferwo_threads_push(void* data)
{
	CF_CircularBuffer* buffer = (CF_CircularBuffer*)data;

	// Push incrementing integers into the buffer.
	int iters = 100;
	int val = 0;
	while (iters && test_circular_buffer_running) {
		int result = cf_circular_buffer_push(buffer, &val, sizeof(int));
		if (result) continue;
		++val;
		--iters;
	}

	return 0;
}

int test_CircularBufferwo_threads_pull(void* data)
{
	CF_CircularBuffer* buffer = (CF_CircularBuffer*)data;

	// Pull integers out of the buffer, make sure they are incrementing.
	int iters = 100;
	int expected_val = 0;
	int got_val;
	while (iters) {
		int result = cf_circular_buffer_pull(buffer, &got_val, sizeof(int));
		if (result) continue;
		REQUIRE(expected_val == got_val);
		++expected_val;
		--iters;
	}

	return 0;
}

/* Run a producer and a consumer thread, and validate input/output of the buffer. */
TEST_CASE(test_CircularBufferwo_threads)
{
	for (int iters = 0; iters < 10; ++iters) {
		CF_CircularBuffer buffer = cf_make_circular_buffer(sizeof(int) * 32);
		CHECK_POINTER(buffer.data);
		CF_Thread* push = cf_thread_create(test_CircularBufferwo_threads_push, "thread push", &buffer);
		CF_Thread* pull = cf_thread_create(test_CircularBufferwo_threads_pull, "thread pull", &buffer);

		REQUIRE(!cf_is_error(cf_thread_wait(pull)));
		test_circular_buffer_running = 0; // Let push thread know the pull thread early-exited, in case it fails.
		REQUIRE(!cf_is_error(cf_thread_wait(push)));
		test_circular_buffer_running = 1; // Reset state for next time.

		cf_destroy_circular_buffer(&buffer);
	}

	return true;
}

TEST_SUITE(test_circular_buffer)
{
	RUN_TEST_CASE(test_circular_buffer_basic);
	RUN_TEST_CASE(test_circular_buffer_fill_up_and_empty);
	RUN_TEST_CASE(test_circular_buffer_overflow);
	RUN_TEST_CASE(test_circular_buffer_underflow);
	RUN_TEST_CASE(test_CircularBufferwo_threads);
}