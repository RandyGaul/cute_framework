/*
    Cute Framework
    Copyright (C) 2025 Randy Gaul https://randygaul.github.io/

    This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

// Instancing via attributes appended after mesh creation: a mesh built with only its vertex
// layout gains per-instance attributes through cf_mesh_append_attributes, then draws N copies
// in one call. This is the exact mechanism the draw3d layer uses to attach its reserved
// instance attributes (model rows, uv rect, mesh attributes) to arbitrary user meshes.

#include "test_harness.h"

#include <cute.h>

using namespace Cute;

#define W 64
#define H 64

static const char* s_vs =
"layout (location = 0) in vec2 in_pos;\n"
"layout (location = 1) in vec4 in_inst; // xy = offset, zw unused\n"
"layout (location = 2) in vec4 in_tint;\n"
"layout (location = 0) out vec4 v_tint;\n"
"void main() {\n"
"    v_tint = in_tint;\n"
"    gl_Position = vec4(in_pos * 0.4 + in_inst.xy, 0, 1);\n"
"}\n";

static const char* s_fs =
"layout (location = 0) in vec4 v_tint;\n"
"layout (location = 0) out vec4 result;\n"
"void main() { result = v_tint; }\n";

TEST_CASE(test_instancing_appended_attributes)
{
	int options = CF_APP_OPTIONS_HIDDEN_BIT | CF_APP_OPTIONS_NO_AUDIO_BIT;
	const char* gles = getenv("CF_TEST_GLES");
	if (gles && *gles == '1') options |= CF_APP_OPTIONS_GFX_OPENGL_BIT | CF_APP_OPTIONS_GFX_DEBUG_BIT;
	if (cf_is_error(cf_make_app(NULL, 0, 0, 0, W, H, options, NULL))) return true; // Headless CI: no display/GPU.

	// The mesh starts life knowing nothing about instancing.
	struct Vertex { float x, y; };
	Vertex verts[6] = { { -1, -1 }, { 1, -1 }, { 1, 1 }, { -1, -1 }, { 1, 1 }, { -1, 1 } };
	CF_VertexAttribute attrs[1] = { };
	attrs[0].name = "in_pos";
	attrs[0].format = CF_VERTEX_FORMAT_FLOAT2;
	attrs[0].offset = 0;
	CF_Mesh mesh = cf_make_mesh(sizeof(verts), attrs, 1, sizeof(Vertex));
	cf_mesh_update_vertex_data(mesh, verts, 6);

	// Instancing arrives later: two per-instance vec4s appended, then a buffer.
	struct Instance { float off[4]; float tint[4]; };
	CF_VertexAttribute inst_attrs[2] = { };
	inst_attrs[0].name = "in_inst";
	inst_attrs[0].format = CF_VERTEX_FORMAT_FLOAT4;
	inst_attrs[0].offset = CF_OFFSET_OF(Instance, off);
	inst_attrs[0].per_instance = true;
	inst_attrs[1].name = "in_tint";
	inst_attrs[1].format = CF_VERTEX_FORMAT_FLOAT4;
	inst_attrs[1].offset = CF_OFFSET_OF(Instance, tint);
	inst_attrs[1].per_instance = true;
	cf_mesh_append_attributes(mesh, inst_attrs, 2);
	cf_mesh_set_instance_buffer(mesh, sizeof(Instance) * 2, sizeof(Instance));

	Instance instances[2] = {
		{ { -0.5f, 0, 0, 0 }, { 1.0f, 0, 0, 1.0f } }, // left, red
		{ {  0.5f, 0, 0, 0 }, { 0, 1.0f, 0, 1.0f } }, // right, green
	};
	cf_mesh_update_instance_data(mesh, instances, 2);

	CF_Shader shader = cf_make_shader_from_source(s_vs, s_fs);
	REQUIRE(shader.id);
	CF_Material material = cf_make_material();
	CF_Canvas canvas = cf_make_canvas(cf_canvas_defaults(W, H));
	CF_Pixel* px = (CF_Pixel*)cf_alloc(W * H * (int)sizeof(CF_Pixel));

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

	// One draw call, two instances: left quarter red, right quarter green.
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


// Indexed + instanced together (the combination the kelp sample exposed on GLES): the same
// two-instance draw as above, but through an index buffer.
TEST_CASE(test_instancing_indexed)
{
	int options = CF_APP_OPTIONS_HIDDEN_BIT | CF_APP_OPTIONS_NO_AUDIO_BIT;
	const char* gles = getenv("CF_TEST_GLES");
	if (gles && *gles == '1') options |= CF_APP_OPTIONS_GFX_OPENGL_BIT | CF_APP_OPTIONS_GFX_DEBUG_BIT;
	if (cf_is_error(cf_make_app(NULL, 0, 0, 0, W, H, options, NULL))) return true; // Headless CI: no display/GPU.

	struct Vertex { float x, y; };
	Vertex verts[4] = { { -1, -1 }, { 1, -1 }, { 1, 1 }, { -1, 1 } };
	uint32_t indices[6] = { 0, 1, 2, 0, 2, 3 };
	CF_VertexAttribute attrs[1] = { };
	attrs[0].name = "in_pos";
	attrs[0].format = CF_VERTEX_FORMAT_FLOAT2;
	attrs[0].offset = 0;
	CF_Mesh mesh = cf_make_mesh(sizeof(verts), attrs, 1, sizeof(Vertex));
	cf_mesh_update_vertex_data(mesh, verts, 4);
	cf_mesh_set_index_buffer(mesh, sizeof(indices), 32);
	cf_mesh_update_index_data(mesh, indices, 6);

	struct Instance { float off[4]; float tint[4]; };
	CF_VertexAttribute inst_attrs[2] = { };
	inst_attrs[0].name = "in_inst";
	inst_attrs[0].format = CF_VERTEX_FORMAT_FLOAT4;
	inst_attrs[0].offset = CF_OFFSET_OF(Instance, off);
	inst_attrs[0].per_instance = true;
	inst_attrs[1].name = "in_tint";
	inst_attrs[1].format = CF_VERTEX_FORMAT_FLOAT4;
	inst_attrs[1].offset = CF_OFFSET_OF(Instance, tint);
	inst_attrs[1].per_instance = true;
	cf_mesh_append_attributes(mesh, inst_attrs, 2);
	cf_mesh_set_instance_buffer(mesh, sizeof(Instance) * 2, sizeof(Instance));
	Instance instances[2] = {
		{ { -0.5f, 0, 0, 0 }, { 1.0f, 0, 0, 1.0f } },
		{ {  0.5f, 0, 0, 0 }, { 0, 1.0f, 0, 1.0f } },
	};
	cf_mesh_update_instance_data(mesh, instances, 2);

	CF_Shader shader = cf_make_shader_from_source(s_vs, s_fs);
	REQUIRE(shader.id);
	CF_Material material = cf_make_material();
	CF_Canvas canvas = cf_make_canvas(cf_canvas_defaults(W, H));
	CF_Pixel* px = (CF_Pixel*)cf_alloc(W * H * (int)sizeof(CF_Pixel));

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

TEST_SUITE(test_instancing)
{
	RUN_TEST_CASE(test_instancing_appended_attributes);
	RUN_TEST_CASE(test_instancing_indexed);
}
