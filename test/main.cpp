/*
	Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

#ifndef _CRT_SECURE_NO_WARNINGS
#	define _CRT_SECURE_NO_WARNINGS
#endif

#ifndef _CRT_NONSTDC_NO_DEPRECATE
#	define _CRT_NONSTDC_NO_DEPRECATE
#endif

#ifdef _MSC_VER
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#include <SDL3/SDL.h>

#define PICO_UNIT_IMPLEMENTATION
#include <pico/pico_unit.h>

#include <cute.h>

TEST_SUITE(test_array);
TEST_SUITE(test_aseprite);
TEST_SUITE(test_audio);
TEST_SUITE(test_base64);
TEST_SUITE(test_color);
TEST_SUITE(test_coroutine);
TEST_SUITE(test_doubly_list);
TEST_SUITE(test_hashtable);
TEST_SUITE(test_path);
TEST_SUITE(test_png_cache);
TEST_SUITE(test_sprite);
TEST_SUITE(test_string);
TEST_SUITE(test_json);
TEST_SUITE(test_markups);
TEST_SUITE(test_math);
extern "C" {
TEST_SUITE(test_math_c);
TEST_SUITE(test_ckit);
}

#include <SDL3/SDL.h>

int main(int argc, char* argv[])
{
	cf_fs_init(argv[0]);
	printf("Tests are running from \"%s\"\n\n", cf_fs_get_base_directory());
	cf_fs_destroy();

#ifdef _MSC_VER
	_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG | _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	_CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE);
	_CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_FILE);
	_CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE);
	_CrtSetReportFile(_CRT_WARN, _CRTDBG_FILE_STDERR);
	_CrtSetReportFile(_CRT_ERROR, _CRTDBG_FILE_STDERR);
	_CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDERR);
#endif

	pu_display_colors(true);
	setvbuf(stdout, NULL, _IONBF, 0);
	setvbuf(stderr, NULL, _IONBF, 0);

#define RUN_TRACED(suite_fp) fprintf(stderr, ">>> " #suite_fp "\n"); RUN_TEST_SUITE(suite_fp); fprintf(stderr, "<<< " #suite_fp "\n");
	RUN_TRACED(test_array);
	RUN_TRACED(test_aseprite);
	RUN_TRACED(test_audio);
	RUN_TRACED(test_base64);
	RUN_TRACED(test_color);
	RUN_TRACED(test_coroutine);
	RUN_TRACED(test_doubly_list);
	RUN_TRACED(test_hashtable);
	RUN_TRACED(test_path);
	RUN_TRACED(test_png_cache);
	RUN_TRACED(test_sprite);
	RUN_TRACED(test_string);
	RUN_TRACED(test_json);
	RUN_TRACED(test_markups);
	RUN_TRACED(test_math);
	RUN_TRACED(test_math_c);
	RUN_TRACED(test_ckit);
#undef RUN_TRACED

	pu_print_stats();
	return pu_test_failed();
}
