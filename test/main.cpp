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
TEST_SUITE(test_coroutine);
TEST_SUITE(test_doubly_list);
TEST_SUITE(test_hashtable);
TEST_SUITE(test_path);
TEST_SUITE(test_png_cache);
TEST_SUITE(test_sprite);
TEST_SUITE(test_string);
TEST_SUITE(test_json);
TEST_SUITE(test_markups);

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
	#undef RUN_TEST_SUITE
	#define RUN_TEST_SUITE(suite_fp) pu_run_suite(#suite_fp, suite_fp); sinuke(); SDL_Quit(); _CrtDumpMemoryLeaks();
#endif

	pu_display_colors(true);

	RUN_TEST_SUITE(test_array);
	RUN_TEST_SUITE(test_aseprite);
	RUN_TEST_SUITE(test_audio);
	RUN_TEST_SUITE(test_base64);
	RUN_TEST_SUITE(test_coroutine);
	RUN_TEST_SUITE(test_doubly_list);
	RUN_TEST_SUITE(test_hashtable);
	RUN_TEST_SUITE(test_path);
	RUN_TEST_SUITE(test_png_cache);
	RUN_TEST_SUITE(test_sprite);
	RUN_TEST_SUITE(test_string);
	RUN_TEST_SUITE(test_json);
	RUN_TEST_SUITE(test_markups);

	pu_print_stats();
	return pu_test_failed();
}
