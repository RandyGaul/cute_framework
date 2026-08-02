/*
    Cute Framework
    Copyright (C) 2025 Randy Gaul https://randygaul.github.io/

    This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

// Reload coverage for the two-file cf_make_shader path. Draw and compute shaders could already
// reload; the low-level path -- the only one a custom 3d vertex stage can use -- could not.

#include "test_harness.h"

#include <cute.h>

using namespace Cute;

#define W 64
#define H 64

static const char* s_vs =
"layout (location = 0) in vec2 in_pos;\n"
"void main() { gl_Position = vec4(in_pos, 0, 1); }\n";

// Parameterized so the test can rewrite the file and check the change took effect.
static const char* s_fs_fmt =
"layout(location = 0) out vec4 result;\n"
"void main() { result = vec4(%s); }\n";

static CF_Mesh s_make_fullscreen_quad()
{
	struct Vertex { float x, y; };
	Vertex verts[4] = { { -1, -1 }, { 1, -1 }, { 1, 1 }, { -1, 1 } };
	uint16_t indices[6] = { 0, 1, 2, 0, 2, 3 };
	CF_VertexAttribute attrs[1] = { };
	attrs[0].name = "in_pos";
	attrs[0].format = CF_VERTEX_FORMAT_FLOAT2;
	attrs[0].offset = 0;
	CF_Mesh mesh = cf_make_mesh(sizeof(verts), attrs, 1, sizeof(Vertex));
	cf_mesh_update_vertex_data(mesh, verts, 4);
	cf_mesh_set_index_buffer(mesh, sizeof(indices), 16);
	cf_mesh_update_index_data(mesh, indices, 6);
	return mesh;
}

static CF_Pixel s_render_probe(CF_Mesh mesh, CF_Shader shader, CF_Material material, CF_Canvas canvas, CF_Pixel* px)
{
	cf_app_update(NULL);
	cf_apply_canvas(canvas, true);
	cf_apply_mesh(mesh);
	cf_apply_shader(shader, material);
	cf_draw_elements();
	cf_app_draw_onto_screen(false);

	CF_Readback rb = cf_canvas_readback(canvas);
	while (!cf_readback_ready(rb)) {}
	cf_readback_data(rb, px, W * H * (int)sizeof(CF_Pixel));
	cf_destroy_readback(rb);
	return px[(H / 2) * W + (W / 2)];
}

TEST_CASE(test_two_file_shader_reload)
{
	if (cf_is_error(cf_make_app(NULL, 0, 0, 0, W, H, CF_APP_OPTIONS_HIDDEN_BIT | CF_APP_OPTIONS_NO_AUDIO_BIT, NULL))) return true; // Headless CI: no display/GPU.

	const char* base = cf_fs_get_base_directory();
	cf_fs_set_write_directory(base);
	cf_fs_create_directory("/shader_reload_test");

	char fs_src[256];
	snprintf(fs_src, sizeof(fs_src), s_fs_fmt, "1.0, 0.0, 0.0, 1.0"); // Red.
	REQUIRE(!cf_is_error(cf_fs_write_string_to_file("/shader_reload_test/t.vs", s_vs)));
	REQUIRE(!cf_is_error(cf_fs_write_string_to_file("/shader_reload_test/t.fs", fs_src)));

	CF_Shader shader = cf_make_shader("/shader_reload_test/t.vs", "/shader_reload_test/t.fs");
	REQUIRE(shader.id);

	CF_Mesh mesh = s_make_fullscreen_quad();
	CF_Material material = cf_make_material();
	CF_Canvas canvas = cf_make_canvas(cf_canvas_defaults(W, H));
	CF_Pixel* px = (CF_Pixel*)cf_alloc(W * H * (int)sizeof(CF_Pixel));

	CF_Pixel before = s_render_probe(mesh, shader, material, canvas, px);
	REQUIRE(before.colors.r > 200 && before.colors.g < 60);

	// Rewrite the fragment file and reload through the same handle.
	snprintf(fs_src, sizeof(fs_src), s_fs_fmt, "0.0, 1.0, 0.0, 1.0"); // Green.
	REQUIRE(!cf_is_error(cf_fs_write_string_to_file("/shader_reload_test/t.fs", fs_src)));

	uint64_t id_before = shader.id;
	REQUIRE(cf_shader_reload_from_files(&shader));
	REQUIRE(shader.id == id_before); // Contents swap in place; the handle stays valid.

	CF_Pixel after = s_render_probe(mesh, shader, material, canvas, px);
	REQUIRE(after.colors.g > 200 && after.colors.r < 60);

	// A shader that does not come from cf_make_shader cannot be reloaded this way.
	CF_Shader from_source = cf_make_shader_from_source(s_vs, fs_src);
	REQUIRE(from_source.id);
	REQUIRE(!cf_shader_reload_from_files(&from_source));
	cf_destroy_shader(from_source);

	// A broken edit must leave the working shader alone rather than blanking the screen.
	REQUIRE(!cf_is_error(cf_fs_write_string_to_file("/shader_reload_test/t.fs", "this is not glsl\n")));
	REQUIRE(!cf_shader_reload_from_files(&shader));
	CF_Pixel after_bad = s_render_probe(mesh, shader, material, canvas, px);
	REQUIRE(after_bad.colors.g > 200 && after_bad.colors.r < 60); // Still green.

	cf_free(px);
	cf_destroy_canvas(canvas);
	cf_destroy_material(material);
	cf_destroy_mesh(mesh);
	cf_destroy_shader(shader);
	cf_destroy_app();
	return true;
}

// cf_make_shader used to CF_ASSERT on a missing file, which meant hot reload catching a file
// mid-write took the app down instead of keeping the last good shader.
TEST_CASE(test_make_shader_missing_file_is_not_fatal)
{
	if (cf_is_error(cf_make_app(NULL, 0, 0, 0, W, H, CF_APP_OPTIONS_HIDDEN_BIT | CF_APP_OPTIONS_NO_AUDIO_BIT, NULL))) return true; // Headless CI: no display/GPU.

	CF_Shader shader = cf_make_shader("/definitely_not_here.vs", "/definitely_not_here.fs");
	REQUIRE(shader.id == 0);
	REQUIRE(cf_shader_compile_error() != NULL); // And it says why.

	cf_destroy_app();
	return true;
}

TEST_SUITE(test_shader_reload)
{
	RUN_TEST_CASE(test_two_file_shader_reload);
	RUN_TEST_CASE(test_make_shader_missing_file_is_not_fatal);
}
