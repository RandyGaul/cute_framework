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
#include <stdlib.h>
#include <string.h>

#include "test_app_shared.h"
#include "test_leak.h"

TEST_SUITE(test_alloc);
TEST_SUITE(test_app);
TEST_SUITE(test_array);
TEST_SUITE(test_aseprite);
TEST_SUITE(test_video);
TEST_SUITE(test_audio);
TEST_SUITE(test_base64);
TEST_SUITE(test_color);
TEST_SUITE(test_coroutine);
TEST_SUITE(test_doubly_list);
TEST_SUITE(test_hashtable);
TEST_SUITE(test_path);
TEST_SUITE(test_custom_sprite);
TEST_SUITE(test_sprite);
TEST_SUITE(test_string);
TEST_SUITE(test_json);
TEST_SUITE(test_markups);
TEST_SUITE(test_draw_tiled);
TEST_SUITE(test_draw_z);
TEST_SUITE(test_atlas_uvs);
TEST_SUITE(test_graphics_3d);
TEST_SUITE(test_shader_reload);
TEST_SUITE(test_shader_directory);
TEST_SUITE(test_canvas_clear);
TEST_SUITE(test_gpu_submit);
TEST_SUITE(test_mrt);
TEST_SUITE(test_texture_types);
TEST_SUITE(test_shadow_sampling);
TEST_SUITE(test_canvas_copy_depth);
TEST_SUITE(test_instancing);
TEST_SUITE(test_buffer_updates);
TEST_SUITE(test_draw3d);
TEST_SUITE(test_uniform_arrays);
TEST_SUITE(test_gles);
TEST_SUITE(test_compute);
TEST_SUITE(test_math);
TEST_SUITE(test_math3d);
TEST_SUITE(test_model);
TEST_SUITE(test_physics);
TEST_SUITE(test_networking);
TEST_SUITE(test_arith);
extern "C" {
TEST_SUITE(test_math_c);
TEST_SUITE(test_math3d_c);
TEST_SUITE(test_ckit);
}
TEST_SUITE(test_jpg);
TEST_SUITE(test_dds);
TEST_SUITE(test_sym);
TEST_SUITE(test_crash);
TEST_SUITE(test_imgui);
TEST_SUITE(test_device_loss);

#include <SDL3/SDL.h>

#if defined(_MSC_VER) && defined(_DEBUG)
// SDL's heap is tagged _CLIENT_BLOCK so the leak counts (normal blocks) measure CF alone.
static void* SDLCALL s_sdl_malloc(size_t size) { return _malloc_dbg(size, _CLIENT_BLOCK, NULL, 0); }
static void* SDLCALL s_sdl_calloc(size_t count, size_t size) { return _calloc_dbg(count, size, _CLIENT_BLOCK, NULL, 0); }
static void* SDLCALL s_sdl_realloc(void* ptr, size_t size) { return _realloc_dbg(ptr, size, _CLIENT_BLOCK, NULL, 0); }
static void SDLCALL s_sdl_free(void* ptr) { _free_dbg(ptr, _CLIENT_BLOCK); }
#endif

int main(int argc, char* argv[])
{
#if defined(_MSC_VER) && defined(_DEBUG)
	SDL_SetMemoryFunctions(s_sdl_malloc, s_sdl_calloc, s_sdl_realloc, s_sdl_free);
#endif
	TestLeakCheck leak;
	test_leak_begin(&leak);

	cf_fs_init(argv[0]);
	printf("Tests are running from \"%s\"\n\n", cf_fs_get_base_directory());
	cf_fs_destroy();

#ifdef _MSC_VER
	_CrtSetDbgFlag(_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG) | _CRTDBG_ALLOC_MEM_DF);
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

	// Suite names on the command line select which suites run; no arguments runs everything.
	//     tests.exe test_draw3d test_mrt
	auto suite_enabled = [&](const char* name) {
		if (argc <= 1) return true;
		for (int i = 1; i < argc; ++i) {
			if (!strcmp(argv[i], name)) return true;
		}
		return false;
	};

	// Every GPU test skips when no app comes up, so a WebGPU run that silently lands on another
	// backend, or on none, would pass. Refuse to run instead.
	const char* webgpu = getenv("CF_TEST_WEBGPU");
	if (webgpu && *webgpu == '1') {
		CF_Result result = cf_make_app(NULL, 0, 0, 0, 64, 64, test_app_options(0), NULL);
		CF_BackendType backend = cf_is_error(result) ? CF_BACKEND_TYPE_INVALID : cf_query_backend();
		cf_destroy_app();
		if (cf_is_error(result)) {
			fprintf(stderr, "FATAL: CF_TEST_WEBGPU=1 but cf_make_app failed: %s\n", result.details ? result.details : "no details");
			return 1;
		}
		if (backend != CF_BACKEND_TYPE_WEBGPU) {
			fprintf(stderr, "FATAL: CF_TEST_WEBGPU=1 but the app came up on %s, not CF_BACKEND_TYPE_WEBGPU.\n", cf_backend_type_to_string(backend));
			return 1;
		}
	}

#define RUN_TRACED(suite_fp) if (suite_enabled(#suite_fp)) { fprintf(stderr, ">>> " #suite_fp "\n"); RUN_TEST_SUITE(suite_fp); fprintf(stderr, "<<< " #suite_fp "\n"); }
	RUN_TRACED(test_alloc);
	RUN_TRACED(test_app);
	RUN_TRACED(test_array);
	RUN_TRACED(test_aseprite);
	RUN_TRACED(test_audio);
	RUN_TRACED(test_base64);
	RUN_TRACED(test_color);
	RUN_TRACED(test_coroutine);
	RUN_TRACED(test_doubly_list);
	RUN_TRACED(test_hashtable);
	RUN_TRACED(test_path);
	RUN_TRACED(test_custom_sprite);
	RUN_TRACED(test_sprite);
	RUN_TRACED(test_string);
	RUN_TRACED(test_json);
	RUN_TRACED(test_markups);
	RUN_TRACED(test_draw_tiled);
	RUN_TRACED(test_draw_z);
	RUN_TRACED(test_atlas_uvs);
	RUN_TRACED(test_graphics_3d);
	RUN_TRACED(test_shader_reload);
	RUN_TRACED(test_shader_directory);
	RUN_TRACED(test_canvas_clear);
	RUN_TRACED(test_gpu_submit);
	RUN_TRACED(test_mrt);
	RUN_TRACED(test_texture_types);
	RUN_TRACED(test_shadow_sampling);
	RUN_TRACED(test_canvas_copy_depth);
	RUN_TRACED(test_instancing);
	RUN_TRACED(test_buffer_updates);
	RUN_TRACED(test_draw3d);
	RUN_TRACED(test_uniform_arrays);
	RUN_TRACED(test_gles);
	RUN_TRACED(test_compute);
	RUN_TRACED(test_math);
	RUN_TRACED(test_math_c);
	RUN_TRACED(test_math3d);
	RUN_TRACED(test_math3d_c);
	RUN_TRACED(test_model);
	RUN_TRACED(test_physics);
	RUN_TRACED(test_networking);
	RUN_TRACED(test_arith);
	// test_ckit calls sintern_nuke(), which invalidates every interned pointer a live
	// engine holds as map keys (cf_sinuke's documented contract: not while an app
	// exists). Kill the shared app first; the next GPU suite boots a fresh one whose
	// interns are all post-nuke.
	test_shutdown_shared_app();
	RUN_TRACED(test_ckit);
	RUN_TRACED(test_jpg);
	RUN_TRACED(test_dds);
	RUN_TRACED(test_sym);
	RUN_TRACED(test_crash);
	RUN_TRACED(test_video);
	// Private apps from here on: each suite below leaves its app unfit to share.
	RUN_TRACED(test_imgui);
	RUN_TRACED(test_device_loss);
#undef RUN_TRACED

	test_shutdown_shared_app(); // The shared GPU app dies here so the leak checker sees a clean exit.
	pu_print_stats();

	// Interned strings and cute_spirv's global tables outlive the run by design, so this is a
	// report rather than a pass/fail. CF_TEST_LEAK_DUMP=1 lists every surviving block.
	if (test_leak_check_enabled()) {
		fprintf(stderr, "Leak check: %lld bytes in %lld blocks still allocated at exit (plus %lld bytes held by SDL).\n", (long long)test_leak_bytes(&leak), (long long)test_leak_blocks(&leak), (long long)test_leak_sdl_bytes(&leak));
		if (test_leak_dump_requested()) test_leak_dump(&leak);
	}
	return pu_test_failed();
}
