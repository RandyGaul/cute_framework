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

#include <cute_c_runtime.h>
#include <test_harness.h>

#include <test_handle.h>
#include <test_circular_buffer.h>
#include <test_nonce_buffer.h>

int main(int argc, const char** argv)
{
#ifdef _MSC_VER
	windows_turn_on_console_color();
#endif

	test_t tests[] = {
		CUTE_TEST_CASE_ENTRY(test_handle_basic),
		CUTE_TEST_CASE_ENTRY(test_handle_large_loop),
		CUTE_TEST_CASE_ENTRY(test_handle_large_loop_and_free),
		CUTE_TEST_CASE_ENTRY(test_handle_alloc_too_many),
		CUTE_TEST_CASE_ENTRY(test_circular_buffer_basic),
		CUTE_TEST_CASE_ENTRY(test_circular_buffer_fill_up_and_empty),
		CUTE_TEST_CASE_ENTRY(test_circular_buffer_overflow),
		CUTE_TEST_CASE_ENTRY(test_circular_buffer_underflow),
		CUTE_TEST_CASE_ENTRY(test_circular_buffer_two_threads),
		CUTE_TEST_CASE_ENTRY(test_nonce_buffer_valid_packets),
		CUTE_TEST_CASE_ENTRY(test_nonce_buffer_old_packet_out_of_range),
		CUTE_TEST_CASE_ENTRY(test_nonce_buffer_duplicate),
	};
	int test_count = sizeof(tests) / sizeof(*tests);
	int fail_count = 0;

	// TODO: Make cute_mem_leak.h/.cpp to check for memory leaks, and use it in the test harness for each test.

	// Run all tests.
	if (argc == 1) {
		for (int i = 0; i < test_count; ++i)
		{
			test_t* test = tests + i;
			if (do_test(test, i + 1)) fail_count++;
		}
		if (fail_count) {
			fprintf(CUTE_TEST_IO_STREAM, "\033[31mFAILED\033[0m %d test case%s.\n\n", fail_count, fail_count > 1 ? "s" : "");
		} else {
			fprintf(CUTE_TEST_IO_STREAM, "All tests \033[32mPASSED\033[0m.\n\n");
		}
	} else if (argc == 2) {
		const char* soak = argv[1];
		if (CUTE_STRCMP(soak, "soak") == 0) {
			while (1)
			{
				for (int i = 0; i < test_count; ++i)
				{
					test_t* test = tests + i;
					do_test(test, i + 1);
				}
			}
		}

		// Run a specific test function.
		const char* test_name = argv[1];
		int found = 0;
		for (int i = 0; i < test_count; ++i)
		{
			test_t* test = tests + i;
			if (CUTE_STRCMP(test_name, test->test_name) == 0) {
				do_test(test, 1);
				found = 1;
				break;
			}
		}

		if (!found) {
			fprintf(CUTE_TEST_IO_STREAM, "Unable to find test %s.\n", test_name);
			return -1;
		}
	} else {
		fprintf(CUTE_TEST_IO_STREAM, "Invalid number of parameters. Please pass in either no parameters to run all tests, or just the test function name.\nYou may also pass in \"soak\" to run tests in an infinite loop.\n");
		return -1;
	}

	return 0;
}
