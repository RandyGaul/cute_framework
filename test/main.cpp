#include <cute_c_runtime.h>
#include <test_harness.h>

#include <test_handle.h>

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
	};
	int test_count = sizeof(tests) / sizeof(*tests);

	// Run all tests.
	if (argc == 1) {
		for (int i = 0; i < test_count; ++i)
		{
			test_t* test = tests + i;
			do_test(test, i + 1);
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
