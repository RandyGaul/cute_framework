/*
    Cute Framework
    Copyright (C) 2025 Randy Gaul https://randygaul.github.io/

    This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

#include "test_harness.h"
#include "test_app_shared.h"

#include <cute.h>
#include <internal/cute_app_internal.h>

using namespace Cute;

// cf_shader_directory walks the tree and checks each file's extension. cf_path_get_ext returns
// NULL for a file that has no extension, and comparing that through CF_Path::ext() strcmp'd the
// null pointer -- so a shader directory containing a plain "README", a "Makefile", or (very
// easily) a built executable took the whole app down before it drew a frame.
//
// A directory with a shader next to an extensionless file is the whole repro.
TEST_CASE(test_shader_directory_survives_extensionless_files)
{
	if (!test_make_app(64, 64)) return true; // Headless CI: no display/GPU.

	// Build the directory under the write directory so the test owns it outright.
	const char* base = cf_fs_get_base_directory();
	cf_fs_set_write_directory(base);
	cf_fs_create_directory("/shader_dir_test");

	const char* shader_src = "vec4 shader(vec4 color, ShaderParams params) { return color; }\n";
	REQUIRE(!cf_is_error(cf_fs_write_string_to_file("/shader_dir_test/ok.shd", shader_src)));
	// No dot anywhere in the name: cf_path_get_ext returns NULL for this one.
	REQUIRE(!cf_is_error(cf_fs_write_string_to_file("/shader_dir_test/README", "not a shader\n")));

	// Pre-fix this call segfaults inside s_shader_directory_recursive.
	cf_shader_directory("/shader_dir_test");

	// The real shader beside it must still have been picked up.
	CF_Shader shd = cf_make_draw_shader("ok.shd");
	REQUIRE(shd.id);
	cf_destroy_shader(shd);

	test_destroy_app();
	return true;
}

// The watcher reports only through this callback, and registering one also drops the frame
// throttle -- so with it installed one cf_app_update is exactly one scan.
#define MAX_CHANGED 32
static const char* s_changed[MAX_CHANGED];
static int s_changed_count;

static void s_on_changed(const char* path, void* udata)
{
	CF_UNUSED(udata);
	if (s_changed_count < MAX_CHANGED) s_changed[s_changed_count++] = path;
}

static bool s_saw(const char* key)
{
	for (int i = 0; i < s_changed_count; ++i) {
		if (!CF_STRCMP(s_changed[i], key)) return true;
	}
	return false;
}

static void s_scan(int times)
{
	for (int i = 0; i < times; ++i) cf_app_update(NULL);
}

// Shared between the watch cases. With the shared app the directory is already set by whichever
// case ran first; with CF_TEST_FRESH_APP=1 each case sets it on its own fresh app.
static void s_use_shader_dir()
{
	cf_fs_set_write_directory(cf_fs_get_base_directory());
	cf_fs_create_directory("/shader_dir_test");
	// The cases below all test what happens when a file *appears*, so they must start from a
	// directory that does not contain it. Each case removes its own file when it finishes; this
	// sweeps up after a run that failed before it got there.
	cf_fs_remove("/shader_dir_test/w.vert");
	cf_fs_remove("/shader_dir_test/w.frag");
	cf_fs_remove("/shader_dir_test/late.shd");
	cf_fs_remove("/shader_dir_test/gone.shd");
	if (!app->shader_directory_set) cf_shader_directory("/shader_dir_test");
}

// .vert and .frag are the extensions a two-file cf_make_shader pair uses, and they were missing
// from the watched set -- the reload machinery existed but nothing ever reached it.
//
// The sleep is not padding: mtimes have one-second resolution, so a rewrite inside the same second
// as the original write is genuinely indistinguishable from no change at all.
TEST_CASE(test_shader_watch_reports_vert_and_frag_edits)
{
	if (!test_make_app(64, 64)) return true; // Headless CI: no display/GPU.
	s_use_shader_dir();

	REQUIRE(!cf_is_error(cf_fs_write_string_to_file("/shader_dir_test/w.vert", "// one\n")));
	REQUIRE(!cf_is_error(cf_fs_write_string_to_file("/shader_dir_test/w.frag", "// one\n")));

	cf_shader_on_changed(s_on_changed, NULL);
	s_scan(4);                // Let both files land in the watch map.
	s_changed_count = 0;
	cf_sleep(1100);

	REQUIRE(!cf_is_error(cf_fs_write_string_to_file("/shader_dir_test/w.vert", "// two\n")));
	REQUIRE(!cf_is_error(cf_fs_write_string_to_file("/shader_dir_test/w.frag", "// two\n")));
	s_scan(4);

	REQUIRE(s_saw("/w.vert"));
	REQUIRE(s_saw("/w.frag"));

	cf_fs_remove("/shader_dir_test/w.vert");
	cf_fs_remove("/shader_dir_test/w.frag");
	cf_shader_on_changed(NULL, NULL);
	test_destroy_app();
	return true;
}

// A file created after cf_shader_directory has no entry in the watch map, and the watch pass used
// to look one up unconditionally -- Map::get asserts and then dereferences null. It also has to
// actually become watched: the directory listing is cached between scans, so a cache that never
// noticed the new name would leave the file invisible forever rather than crash.
TEST_CASE(test_shader_watch_adopts_files_created_after_startup)
{
	if (!test_make_app(64, 64)) return true; // Headless CI: no display/GPU.
	s_use_shader_dir();

	cf_shader_on_changed(s_on_changed, NULL);
	s_scan(4);

	REQUIRE(!cf_is_error(cf_fs_write_string_to_file("/shader_dir_test/late.shd", "// late\n")));
	s_scan(4); // Pre-fix this asserts inside the watch pass.

	REQUIRE(app->shader_file_infos.try_find(sintern("/late.shd")) != NULL);

	cf_fs_remove("/shader_dir_test/late.shd");
	cf_shader_on_changed(NULL, NULL);
	test_destroy_app();
	return true;
}

// The listing is cached, so a removed file stays in it until the cache notices. Every lookup for
// it then fails, which has to read as "nothing to compare" rather than as a change or a crash.
TEST_CASE(test_shader_watch_survives_removed_files)
{
	if (!test_make_app(64, 64)) return true; // Headless CI: no display/GPU.
	s_use_shader_dir();

	REQUIRE(!cf_is_error(cf_fs_write_string_to_file("/shader_dir_test/gone.shd", "// here\n")));
	cf_shader_on_changed(s_on_changed, NULL);
	s_scan(4);
	REQUIRE(app->shader_file_infos.try_find(sintern("/gone.shd")) != NULL);

	REQUIRE(!cf_is_error(cf_fs_remove("/shader_dir_test/gone.shd")));
	s_changed_count = 0;
	s_scan(4);
	REQUIRE(!s_saw("/gone.shd")); // A deletion is not a modification.

	cf_shader_on_changed(NULL, NULL);
	test_destroy_app();
	return true;
}

// A second mount serving the same directory adds entries no walk of the first one can see.
TEST_CASE(test_shader_watch_sees_files_from_a_later_mount)
{
	if (!test_make_app(64, 64)) return true; // Headless CI: no display/GPU.
	s_use_shader_dir();

	cf_shader_on_changed(s_on_changed, NULL);
	s_scan(4);

	// A second real directory, mounted so that it also provides /shader_dir_test.
	CF_Path extra = CF_Path(cf_fs_get_base_directory()) + "shader_dir_mount";
	cf_fs_set_write_directory(cf_fs_get_base_directory());
	cf_fs_create_directory("/shader_dir_mount");
	cf_fs_create_directory("/shader_dir_mount/shader_dir_test");
	REQUIRE(!cf_is_error(cf_fs_write_string_to_file("/shader_dir_mount/shader_dir_test/mounted.shd", "// mounted\n")));
	REQUIRE(!cf_is_error(cf_fs_mount(extra.c_str(), "", true)));

	s_scan(4);
	REQUIRE(app->shader_file_infos.try_find(sintern("/mounted.shd")) != NULL);

	cf_fs_dismount(extra.c_str());
	cf_shader_on_changed(NULL, NULL);
	test_destroy_app();
	return true;
}

// A mount point *below* the shader directory makes its first component appear there as a virtual
// subdirectory. Nothing on disk changes, and no walk can ever produce that name, so no directory
// signature moves -- the mount counter is the only thing that can notice.
TEST_CASE(test_shader_watch_sees_a_mount_point_below_the_shader_directory)
{
	if (!test_make_app(64, 64)) return true; // Headless CI: no display/GPU.
	s_use_shader_dir();

	cf_shader_on_changed(s_on_changed, NULL);
	s_scan(4);

	CF_Path under = CF_Path(cf_fs_get_base_directory()) + "shader_dir_under";
	cf_fs_set_write_directory(cf_fs_get_base_directory());
	cf_fs_create_directory("/shader_dir_under");
	REQUIRE(!cf_is_error(cf_fs_write_string_to_file("/shader_dir_under/inner.shd", "// inner\n")));
	REQUIRE(!cf_is_error(cf_fs_mount(under.c_str(), "/shader_dir_test/sub", true)));

	s_scan(4);
	REQUIRE(app->shader_file_infos.try_find(sintern("/sub/inner.shd")) != NULL);

	cf_fs_dismount(under.c_str());
	cf_shader_on_changed(NULL, NULL);
	test_destroy_app();
	return true;
}

TEST_SUITE(test_shader_directory)
{
	RUN_TEST_CASE(test_shader_directory_survives_extensionless_files);
	RUN_TEST_CASE(test_shader_watch_reports_vert_and_frag_edits);
	RUN_TEST_CASE(test_shader_watch_adopts_files_created_after_startup);
	RUN_TEST_CASE(test_shader_watch_survives_removed_files);
	RUN_TEST_CASE(test_shader_watch_sees_files_from_a_later_mount);
	RUN_TEST_CASE(test_shader_watch_sees_a_mount_point_below_the_shader_directory);
}
