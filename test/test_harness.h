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

#include <cute_error.h>
#include <cute_string.h>

typedef int (test_fn)();

struct test_t
{
	const char* test_name;
	const char* description;
	test_fn* fn_ptr;
};

#ifndef CUTE_TEST_IO_STREAM
#	include <stdio.h>
#	define CUTE_TEST_IO_STREAM stderr
#endif

#ifdef _MSC_VER
#define _WINSOCKAPI_
#include <Windows.h>

// At the time of writing, this define requires fairly recent windows version, so it's
// safest to just define it ourselves... Should be harmless!
#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#	define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif

// https://docs.microsoft.com/en-us/windows/console/console-virtual-terminal-sequences
void windows_turn_on_console_color()
{
	HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
	DWORD flags = 0;
	GetConsoleMode(h, &flags);
	flags |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
	SetConsoleMode(h, flags);
}
#endif

int do_test(test_t* test, int i)
{
	const char* test_name = test->test_name;
	const char* description = test->description;
	fprintf(CUTE_TEST_IO_STREAM, "Running test #%d\n\tName:         %s\n\tDescription:  %s\n\t", i, test_name, description);
	int result = test->fn_ptr();
	const char* result_string = result ? "\033[31mFAILED\033[0m\n\n" : "\033[32mPASSED\033[0m\n\n";
	fprintf(CUTE_TEST_IO_STREAM, "Result:       %s", result_string);

	cute::string_nuke_static_pool();

#ifdef _MSC_VER
	//_CrtDumpMemoryLeaks();
#endif

	return result;
}

#define CUTE_TEST_PRINT_FILE_LINE(s) do { fprintf(CUTE_TEST_IO_STREAM, "Extra info:   %s\n\tLine number:  %d\n\tFile:         %s\n\t", s, __LINE__, __FILE__); } while (0)
#define CUTE_TEST_ASSERT(x) do { if (!(x)) { CUTE_TEST_PRINT_FILE_LINE("Assertion was false."); return -1; } } while (0)
#define CUTE_TEST_CHECK(x) do { if (x) { CUTE_TEST_PRINT_FILE_LINE("Return code failed check."); return -1; } } while (0)
#define CUTE_TEST_CHECK_POINTER(x) do { if (!(x)) { CUTE_TEST_PRINT_FILE_LINE("Pointer failed check."); return -1; } } while (0)

#define CUTE_TEST_CASE(function, description) int function(); test_t test_##function = { #function, description, function }
#define CUTE_TEST_CASE_ENTRY(function) test_##function
