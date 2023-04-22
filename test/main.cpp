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

#include <SDL.h>

#define PICO_UNIT_IMPLEMENTATION
#include <pico/pico_unit.h>

#include <internal/cute_file_system_internal.h> // fs_init / fs_destroy
#include <cute.h>


TEST_SUITE(test_array);
TEST_SUITE(test_aseprite);
TEST_SUITE(test_audio);
TEST_SUITE(test_base64);
TEST_SUITE(test_circular_buffer);
TEST_SUITE(test_coroutine);
TEST_SUITE(test_doubly_list);
TEST_SUITE(test_ecs);
TEST_SUITE(test_font);
TEST_SUITE(test_handle);
TEST_SUITE(test_hashtable);
TEST_SUITE(test_kv);
TEST_SUITE(test_path);
TEST_SUITE(test_png_cache);
TEST_SUITE(test_sprite);
TEST_SUITE(test_string);

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

	RUN_TEST_SUITE(test_array);	
	RUN_TEST_SUITE(test_aseprite);	
	RUN_TEST_SUITE(test_audio);	
	RUN_TEST_SUITE(test_base64);	
	RUN_TEST_SUITE(test_circular_buffer);
	RUN_TEST_SUITE(test_coroutine);
	RUN_TEST_SUITE(test_doubly_list);
	RUN_TEST_SUITE(test_ecs);
	RUN_TEST_SUITE(test_font);
	RUN_TEST_SUITE(test_handle);
	RUN_TEST_SUITE(test_hashtable);
	RUN_TEST_SUITE(test_kv);
	RUN_TEST_SUITE(test_path);
	RUN_TEST_SUITE(test_png_cache);
	RUN_TEST_SUITE(test_sprite);
	RUN_TEST_SUITE(test_string);

	pu_print_stats();
	return pu_test_failed();
}