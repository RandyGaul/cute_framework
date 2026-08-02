/*
    Cute Framework
    Copyright (C) 2025 Randy Gaul https://randygaul.github.io/

    This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

// Shadow-map machinery end to end: render depth into a canvas, then sample that canvas's
// depth attachment through sampler2DShadow with a comparison sampler. This verifies both
// halves of the shadow story at once -- depth-attachment sampling and hardware comparison
// fetches -- through the real compiler and backend.
//
// SDL_GPU only for now: the GLES backend attaches depth as a renderbuffer, which cannot be
// sampled (cf_canvas_get_depth_stencil_target returns a zero handle there). Switching GLES
// to depth-texture attachments is the follow-up that would light this up on web.

#include "test_harness.h"

#include <cute.h>

using namespace Cute;

#define W 64
#define H 64

static const char* s_vs =
"layout (location = 0) in vec2 in_pos;\n"
"layout (set = 1, binding = 0) uniform uniform_block {\n"
"    vec4 u_z;\n"
"};\n"
"void main() { gl_Position = vec4(in_pos, u_z.x, 1); }\n";

static const char* s_depth_fs =
"layout (location = 0) out vec4 result;\n"
"void main() { result = vec4(1.0); }\n";

// Compares u_ref.x against the stored depth at the screen centerish; visibility is 1 where
// ref passes the comparison, 0 where it fails.
static const char* s_shadow_fs =
"layout (location = 0) out vec4 result;\n"
"layout (set = 2, binding = 0) uniform sampler2DShadow u_shadow;\n"
"layout (set = 3, binding = 0) uniform uniform_block {\n"
"    vec4 u_ref;\n"
"};\n"
"void main() {\n"
"    float vis = texture(u_shadow, vec3(0.5, 0.5, u_ref.x));\n"
"    result = vec4(vis, vis, vis, 1.0);\n"
"}\n";

static CF_Mesh s_make_fullscreen_quad()
{
	struct Vertex { float x, y; };
	Vertex verts[6] = { { -1, -1 }, { 1, -1 }, { 1, 1 }, { -1, -1 }, { 1, 1 }, { -1, 1 } };
	CF_VertexAttribute attrs[1] = { };
	attrs[0].name = "in_pos";
	attrs[0].format = CF_VERTEX_FORMAT_FLOAT2;
	attrs[0].offset = 0;
	CF_Mesh mesh = cf_make_mesh(sizeof(verts), attrs, 1, sizeof(Vertex));
	cf_mesh_update_vertex_data(mesh, verts, 6);
	return mesh;
}

TEST_CASE(test_shadow_compare_sampling)
{
	const char* gles = getenv("CF_TEST_GLES");
	if (gles && *gles == '1') return true; // GLES depth attachments are renderbuffers; see header comment.
	if (cf_is_error(cf_make_app(NULL, 0, 0, 0, W, H, CF_APP_OPTIONS_HIDDEN_BIT | CF_APP_OPTIONS_NO_AUDIO_BIT, NULL))) return true; // Headless CI: no display/GPU.

	// Depth canvas whose depth attachment carries a comparison sampler.
	CF_CanvasParams params = cf_canvas_defaults(W, H);
	params.depth_stencil_enable = true;
	params.depth_stencil_target.compare_enable = true;
	params.depth_stencil_target.compare_function = CF_COMPARE_FUNCTION_LESS_THAN_OR_EQUAL;
	params.depth_stencil_target.usage |= CF_TEXTURE_USAGE_SAMPLER_BIT;
	CF_Canvas depth_canvas = cf_make_canvas(params);
	REQUIRE(depth_canvas.id);

	CF_Mesh mesh = s_make_fullscreen_quad();
	CF_Shader depth_shader = cf_make_shader_from_source(s_vs, s_depth_fs);
	REQUIRE(depth_shader.id);
	CF_Material depth_material = cf_make_material();
	CF_RenderState rs = cf_render_state_3d_defaults();
	rs.cull_mode = CF_CULL_MODE_NONE;
	cf_material_set_render_state(depth_material, rs);

	// Write depth 0.5 across the whole canvas.
	float z[4] = { 0.5f, 0, 0, 0 };
	cf_material_set_uniform_vs(depth_material, "u_z", z, CF_UNIFORM_TYPE_FLOAT4, 1);
	cf_app_update(NULL);
	cf_apply_canvas(depth_canvas, true);
	cf_apply_mesh(mesh);
	cf_apply_shader(depth_shader, depth_material);
	cf_draw_elements();
	cf_app_draw_onto_screen(false);

	CF_Texture shadow_map = cf_canvas_get_depth_stencil_target(depth_canvas);
	REQUIRE(shadow_map.id);

	// Pass 2: comparison-sample the depth. ref 0.25 <= 0.5 passes (white), 0.75 fails (black).
	CF_Shader shadow_shader = cf_make_shader_from_source(s_vs, s_shadow_fs);
	REQUIRE(shadow_shader.id);
	CF_Material shadow_material = cf_make_material();
	CF_Canvas out_canvas = cf_make_canvas(cf_canvas_defaults(W, H));
	CF_Pixel* px = (CF_Pixel*)cf_alloc(W * H * (int)sizeof(CF_Pixel));
	cf_material_set_texture_fs(shadow_material, "u_shadow", shadow_map);
	float z0[4] = { 0.0f, 0, 0, 0 };
	cf_material_set_uniform_vs(shadow_material, "u_z", z0, CF_UNIFORM_TYPE_FLOAT4, 1);

	struct { float ref; bool lit; } cases[2] = { { 0.25f, true }, { 0.75f, false } };
	for (int i = 0; i < 2; ++i) {
		float ref[4] = { cases[i].ref, 0, 0, 0 };
		cf_material_set_uniform_fs(shadow_material, "u_ref", ref, CF_UNIFORM_TYPE_FLOAT4, 1);
		cf_app_update(NULL);
		cf_apply_canvas(out_canvas, true);
		cf_apply_mesh(mesh);
		cf_apply_shader(shadow_shader, shadow_material);
		cf_draw_elements();
		cf_app_draw_onto_screen(false);

		CF_Readback rb = cf_canvas_readback(out_canvas);
		REQUIRE(rb.id);
		while (!cf_readback_ready(rb)) {}
		cf_readback_data(rb, px, W * H * (int)sizeof(CF_Pixel));
		cf_destroy_readback(rb);
		CF_Pixel c = px[(H / 2) * W + (W / 2)];
		if (cases[i].lit) REQUIRE(c.colors.r > 200);
		else REQUIRE(c.colors.r < 60);
	}

	cf_free(px);
	cf_destroy_canvas(out_canvas);
	cf_destroy_material(shadow_material);
	cf_destroy_shader(shadow_shader);
	cf_destroy_material(depth_material);
	cf_destroy_shader(depth_shader);
	cf_destroy_mesh(mesh);
	cf_destroy_canvas(depth_canvas);
	cf_destroy_app();
	return true;
}

TEST_SUITE(test_shadow_sampling)
{
	RUN_TEST_CASE(test_shadow_compare_sampling);
}
