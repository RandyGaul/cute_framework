/*
    Cute Framework
    Copyright (C) 2025 Randy Gaul https://randygaul.github.io/

    This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

#include "test_harness.h"
#include "test_app_shared.h"

#include <cute.h>

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

TEST_SUITE(test_shader_directory)
{
	RUN_TEST_CASE(test_shader_directory_survives_extensionless_files);
}
