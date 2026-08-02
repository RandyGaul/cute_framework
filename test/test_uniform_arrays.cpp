/*
    Cute Framework
    Copyright (C) 2025 Randy Gaul https://randygaul.github.io/

    This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

// Sized uniform-block arrays end to end: vec4 and mat4 array members set through
// cf_material_set_uniform, rendered back out as colors and verified by readback. This is the
// runtime half of the compiler's block-array support (see test_block_arrays in
// tools/cute_spirv_test.c) -- offsets, strides, and the upload path on both backends.

#include "test_harness.h"

#include <cute.h>

using namespace Cute;

#define W 64
#define H 64

static const char* s_vs =
"layout (location = 0) in vec2 in_pos;\n"
"void main() { gl_Position = vec4(in_pos, 0, 1); }\n";

// Reads one vec4 straight from each array: u_vals[2] in rgb terms, and u_mats[1]'s third
// column via multiplication with a unit basis vector.
static const char* s_fs =
"layout (location = 0) out vec4 result;\n"
"layout (set = 3, binding = 0) uniform uniform_block {\n"
"    mat4 u_mats[2];\n"
"    vec4 u_vals[3];\n"
"};\n"
"void main() {\n"
"    vec4 col = u_mats[1] * vec4(0, 0, 1, 0); // Column 2 of u_mats[1].\n"
"    result = vec4(u_vals[2].r, col.g, u_vals[0].b, 1.0);\n"
"}\n";

TEST_CASE(test_uniform_array_roundtrip)
{
	int options = CF_APP_OPTIONS_HIDDEN_BIT | CF_APP_OPTIONS_NO_AUDIO_BIT;
	const char* gles = getenv("CF_TEST_GLES");
	if (gles && *gles == '1') options |= CF_APP_OPTIONS_GFX_OPENGL_BIT | CF_APP_OPTIONS_GFX_DEBUG_BIT;
	if (cf_is_error(cf_make_app(NULL, 0, 0, 0, W, H, options, NULL))) return true; // Headless CI: no display/GPU.

	struct Vertex { float x, y; };
	Vertex verts[6] = { { -1, -1 }, { 1, -1 }, { 1, 1 }, { -1, -1 }, { 1, 1 }, { -1, 1 } };
	CF_VertexAttribute attrs[1] = { };
	attrs[0].name = "in_pos";
	attrs[0].format = CF_VERTEX_FORMAT_FLOAT2;
	attrs[0].offset = 0;
	CF_Mesh mesh = cf_make_mesh(sizeof(verts), attrs, 1, sizeof(Vertex));
	cf_mesh_update_vertex_data(mesh, verts, 6);

	CF_Shader shader = cf_make_shader_from_source(s_vs, s_fs);
	REQUIRE(shader.id);
	CF_Material material = cf_make_material();
	CF_Canvas canvas = cf_make_canvas(cf_canvas_defaults(W, H));
	CF_Pixel* px = (CF_Pixel*)cf_alloc(W * H * (int)sizeof(CF_Pixel));

	// u_vals[0].b = 0.75, u_vals[2].r = 0.25; u_mats[1] column 2's y = 0.5.
	float vals[3][4] = {
		{ 0.1f, 0.1f, 0.75f, 1 },
		{ 0.9f, 0.9f, 0.9f, 1 },
		{ 0.25f, 0.6f, 0.6f, 1 },
	};
	CF_M4x4 mats[2] = { cf_m4_identity(), cf_m4_identity() };
	mats[1].elements[2 * 4 + 1] = 0.5f; // (row 1, col 2), the value col.g reads.
	cf_material_set_uniform_fs(material, "u_vals", vals, CF_UNIFORM_TYPE_FLOAT4, 3);
	cf_material_set_uniform_fs(material, "u_mats", mats, CF_UNIFORM_TYPE_MAT4, 2);

	cf_app_update(NULL);
	cf_apply_canvas(canvas, true);
	cf_apply_mesh(mesh);
	cf_apply_shader(shader, material);
	cf_draw_elements();
	cf_app_draw_onto_screen(false);

	CF_Readback rb = cf_canvas_readback(canvas);
	REQUIRE(rb.id);
	while (!cf_readback_ready(rb)) {}
	cf_readback_data(rb, px, W * H * (int)sizeof(CF_Pixel));
	cf_destroy_readback(rb);

	CF_Pixel c = px[(H / 2) * W + (W / 2)];
	REQUIRE(c.colors.r > 54 && c.colors.r < 74);    // 0.25 -> ~64
	REQUIRE(c.colors.g > 118 && c.colors.g < 138);  // 0.5  -> ~128
	REQUIRE(c.colors.b > 181 && c.colors.b < 201);  // 0.75 -> ~191

	cf_free(px);
	cf_destroy_canvas(canvas);
	cf_destroy_material(material);
	cf_destroy_shader(shader);
	cf_destroy_mesh(mesh);
	cf_destroy_app();
	return true;
}


// The vertex-stage flavor, matching the skinning idiom: a bone palette in the VS block,
// dynamically indexed per vertex, with the result driving both position and a varying.
static const char* s_skin_vs =
"layout (location = 0) in vec2 in_pos;\n"
"layout (location = 1) in vec2 in_joints;\n"
"layout (location = 2) in vec2 in_weights;\n"
"layout (location = 0) out vec4 v_color;\n"
"layout (set = 1, binding = 0) uniform uniform_block {\n"
"    mat4 u_vp;\n"
"    mat4 u_bones[4];\n"
"    vec4 u_colors[4];\n"
"};\n"
"void main() {\n"
"    vec4 p0 = vec4(in_pos, 0, 1);\n"
"    vec4 p = u_bones[int(in_joints.x)] * p0 * in_weights.x + u_bones[int(in_joints.y)] * p0 * in_weights.y;\n"
"    v_color = u_colors[int(in_joints.x)];\n"
"    gl_Position = u_vp * p;\n"
"}\n";

static const char* s_skin_fs =
"layout (location = 0) in vec4 v_color;\n"
"layout (location = 0) out vec4 result;\n"
"void main() { result = v_color; }\n";

TEST_CASE(test_uniform_array_vertex_stage)
{
	int options = CF_APP_OPTIONS_HIDDEN_BIT | CF_APP_OPTIONS_NO_AUDIO_BIT;
	const char* gles = getenv("CF_TEST_GLES");
	if (gles && *gles == '1') options |= CF_APP_OPTIONS_GFX_OPENGL_BIT | CF_APP_OPTIONS_GFX_DEBUG_BIT;
	if (cf_is_error(cf_make_app(NULL, 0, 0, 0, W, H, options, NULL))) return true; // Headless CI: no display/GPU.

	// A quad on the left half bound to bone 1, right half bone 2, hard weights.
	struct Vertex { float x, y, j[2], w[2]; };
	Vertex verts[12] = {
		{ -1, -1, { 1, 1 }, { 1, 0 } }, { 0, -1, { 1, 1 }, { 1, 0 } }, { 0, 1, { 1, 1 }, { 1, 0 } },
		{ -1, -1, { 1, 1 }, { 1, 0 } }, { 0, 1, { 1, 1 }, { 1, 0 } }, { -1, 1, { 1, 1 }, { 1, 0 } },
		{ 0, -1, { 2, 2 }, { 1, 0 } }, { 1, -1, { 2, 2 }, { 1, 0 } }, { 1, 1, { 2, 2 }, { 1, 0 } },
		{ 0, -1, { 2, 2 }, { 1, 0 } }, { 1, 1, { 2, 2 }, { 1, 0 } }, { 0, 1, { 2, 2 }, { 1, 0 } },
	};
	CF_VertexAttribute attrs[3] = { };
	attrs[0].name = "in_pos";
	attrs[0].format = CF_VERTEX_FORMAT_FLOAT2;
	attrs[0].offset = CF_OFFSET_OF(Vertex, x);
	attrs[1].name = "in_joints";
	attrs[1].format = CF_VERTEX_FORMAT_FLOAT2;
	attrs[1].offset = CF_OFFSET_OF(Vertex, j);
	attrs[2].name = "in_weights";
	attrs[2].format = CF_VERTEX_FORMAT_FLOAT2;
	attrs[2].offset = CF_OFFSET_OF(Vertex, w);
	CF_Mesh mesh = cf_make_mesh(sizeof(verts), attrs, 3, sizeof(Vertex));
	cf_mesh_update_vertex_data(mesh, verts, 12);

	CF_Shader shader = cf_make_shader_from_source(s_skin_vs, s_skin_fs);
	REQUIRE(shader.id);
	CF_Material material = cf_make_material();
	CF_Canvas canvas = cf_make_canvas(cf_canvas_defaults(W, H));
	CF_Pixel* px = (CF_Pixel*)cf_alloc(W * H * (int)sizeof(CF_Pixel));

	// Bones 1 and 2 are identity; the others are garbage sentinels that must never be read.
	CF_M4x4 bones[4];
	for (int i = 0; i < 4; ++i) {
		bones[i] = cf_m4_identity();
		if (i != 1 && i != 2) for (int k = 0; k < 16; ++k) bones[i].elements[k] = 777.0f;
	}
	float colors[4][4] = {
		{ 1, 1, 1, 1 },
		{ 1, 0, 0, 1 },     // Bone 1: red.
		{ 0, 1, 0, 1 },     // Bone 2: green.
		{ 1, 1, 1, 1 },
	};
	CF_M4x4 vp = cf_m4_identity();
	cf_material_set_uniform_vs(material, "u_vp", &vp, CF_UNIFORM_TYPE_MAT4, 1);
	cf_material_set_uniform_vs(material, "u_bones", bones, CF_UNIFORM_TYPE_MAT4, 4);
	cf_material_set_uniform_vs(material, "u_colors", colors, CF_UNIFORM_TYPE_FLOAT4, 4);

	cf_app_update(NULL);
	cf_apply_canvas(canvas, true);
	cf_apply_mesh(mesh);
	cf_apply_shader(shader, material);
	cf_draw_elements();
	cf_app_draw_onto_screen(false);

	CF_Readback rb = cf_canvas_readback(canvas);
	REQUIRE(rb.id);
	while (!cf_readback_ready(rb)) {}
	cf_readback_data(rb, px, W * H * (int)sizeof(CF_Pixel));
	cf_destroy_readback(rb);

	CF_Pixel left = px[(H / 2) * W + W / 4];
	CF_Pixel right = px[(H / 2) * W + (3 * W) / 4];
	REQUIRE(left.colors.r > 200 && left.colors.g < 60);
	REQUIRE(right.colors.g > 200 && right.colors.r < 60);

	cf_free(px);
	cf_destroy_canvas(canvas);
	cf_destroy_material(material);
	cf_destroy_shader(shader);
	cf_destroy_mesh(mesh);
	cf_destroy_app();
	return true;
}

TEST_SUITE(test_uniform_arrays)
{
	RUN_TEST_CASE(test_uniform_array_roundtrip);
	RUN_TEST_CASE(test_uniform_array_vertex_stage);
}
