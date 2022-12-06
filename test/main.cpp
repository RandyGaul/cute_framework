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

#include <cute_c_runtime.h>
#include <cute_file_system.h>
#include <test_harness.h>
#include <internal/cute_file_system_internal.h>

#define CUTE_RETURN_IF_ERROR(x) do { CF_Result err = (x); if (cf_is_error(err)) return err; } while (0)
#define CUTE_RETURN_IF_FALSE(x) do { bool err = (x); if (!err) return err; } while (0)

#include <test_handle.h>
#include <test_circular_buffer.h>
#include <test_doubly_list.h>
#include <test_base64.h>
#include <test_kv.h>
#include <test_audio.h>
#include <test_ecs.h>
#include <test_array.h>
#include <test_aseprite.h>
#include <test_png_cache.h>
#include <test_sprite.h>
#include <test_coroutine.h>
#include <test_string.h>
#include <test_hashtable.h>
#include <test_path.h>
#include <test_font.h>

#include <imgui/imgui.h>
#include <sokol/sokol_gfx_imgui.h>

int main(int argc, const char** argv)
{
#if 0
	uint32_t options = APP_OPTIONS_DEFAULT_GFX_CONTEXT | APP_OPTIONS_WINDOW_POS_CENTERED | APP_OPTIONS_RESIZABLE;
	Result result = make_app("Fancy Window Title", 0, 0, 640, 480, options, argv[0]);
	if (is_error(result)) return -1;

	Sprite s = cf_make_sprite("test_data/girl.aseprite");
	s.play("spin");

	auto imgui = cf_app_init_imgui(false);
	sg_imgui_t* sg_imgui = app_get_sokol_imgui();
	camera_dimensions(640/2, 480/2);

	while (app_is_running()) {
		float dt = calc_dt();
		app_update(dt);
		s.update(dt);
		s.draw();
		draw_push_layer(-1);
		draw_line(V2(0,0), V2(100,10), 5, color_red(), true);
		draw_pop_layer();
		if (ImGui::BeginMainMenuBar()) {
			if (ImGui::BeginMenu("sokol-gfx")) {
				ImGui::MenuItem("Buffers", 0, &sg_imgui->buffers.open);
				ImGui::MenuItem("Images", 0, &sg_imgui->images.open);
				ImGui::MenuItem("Shaders", 0, &sg_imgui->shaders.open);
				ImGui::MenuItem("Pipelines", 0, &sg_imgui->pipelines.open);
				ImGui::MenuItem("Passes", 0, &sg_imgui->passes.open);
				ImGui::MenuItem("Calls", 0, &sg_imgui->capture.open);
				ImGui::EndMenu();
			}
			ImGui::EndMainMenuBar();
		}
		app_present();
	}

	destroy_app();

	return 0;
#endif

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
	windows_turn_on_console_color();
#endif

	test_t tests[] = {
		CUTE_TEST_CASE_ENTRY(test_font_wip),
		CUTE_TEST_CASE_ENTRY(test_path),
		CUTE_TEST_CASE_ENTRY(test_array_macros_simple),
		CUTE_TEST_CASE_ENTRY(test_string_macros_simple),
		CUTE_TEST_CASE_ENTRY(test_string_macros_advanced),
		CUTE_TEST_CASE_ENTRY(test_hashtable_macros),
		CUTE_TEST_CASE_ENTRY(test_string_interning),
		CUTE_TEST_CASE_ENTRY(test_dictionary_and_interning),
		CUTE_TEST_CASE_ENTRY(test_handle_basic),
		CUTE_TEST_CASE_ENTRY(test_handle_large_loop),
		CUTE_TEST_CASE_ENTRY(test_handle_large_loop_and_free),
		CUTE_TEST_CASE_ENTRY(test_handle_alloc_too_many),
		CUTE_TEST_CASE_ENTRY(test_circular_buffer_basic),
		CUTE_TEST_CASE_ENTRY(test_circular_buffer_fill_up_and_empty),
		CUTE_TEST_CASE_ENTRY(test_circular_buffer_overflow),
		CUTE_TEST_CASE_ENTRY(test_circular_buffer_underflow),
		CUTE_TEST_CASE_ENTRY(test_CircularBufferwo_threads),
		CUTE_TEST_CASE_ENTRY(test_doubly_list),
		CUTE_TEST_CASE_ENTRY(test_base64_encode),
		CUTE_TEST_CASE_ENTRY(test_kv_basic),
		CUTE_TEST_CASE_ENTRY(test_kv_std_string_to_disk),
		CUTE_TEST_CASE_ENTRY(test_kv_std_string_from_disk),
		CUTE_TEST_CASE_ENTRY(test_kv_std_vector),
		CUTE_TEST_CASE_ENTRY(test_kv_write_delta_basic),
		CUTE_TEST_CASE_ENTRY(test_kv_read_delta_basic),
		CUTE_TEST_CASE_ENTRY(test_kv_write_delta_deep),
		CUTE_TEST_CASE_ENTRY(test_kv_read_delta_deep),
		CUTE_TEST_CASE_ENTRY(test_kv_read_delta_array),
		CUTE_TEST_CASE_ENTRY(test_kv_read_and_write_delta_blob),
		CUTE_TEST_CASE_ENTRY(test_kv_read_delta_string),
		CUTE_TEST_CASE_ENTRY(test_kv_read_delta_object),
		CUTE_TEST_CASE_ENTRY(test_audio_load_synchronous),
		CUTE_TEST_CASE_ENTRY(test_ecs_octorok),
		CUTE_TEST_CASE_ENTRY(test_ecs_no_kv),
		CUTE_TEST_CASE_ENTRY(test_array_list_init),
		CUTE_TEST_CASE_ENTRY(test_aseprite_make_destroy),
		CUTE_TEST_CASE_ENTRY(test_png_cache),
		CUTE_TEST_CASE_ENTRY(test_make_sprite),
		CUTE_TEST_CASE_ENTRY(test_coroutine),
	};
	int test_count = sizeof(tests) / sizeof(*tests);
	int fail_count = 0;

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
			fprintf(CUTE_TEST_IO_STREAM, "All %d tests \033[32mPASSED\033[0m.\n\n", test_count);
		}
	} else if (argc == 2) {
		const char* soak = argv[1];
		if (CUTE_STRCMP(soak, "soak") == 0) {
			while (1)
			{
				for (int i = 0; i < test_count; ++i)
				{
					test_t* test = tests + i;
					int result = do_test(test, i + 1);
					if (result) goto break_soak;
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

	return fail_count ? -1 : 0;

break_soak:
	return -1;
}
