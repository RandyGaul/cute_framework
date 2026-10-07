/*
    Cute Framework
    Copyright (C) 2026 Randy Gaul https://randygaul.github.io/

    This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

// cf_canvas_copy_depth end to end: the copied depth reads back through a sampler, an effect
// pass can sample the copy while depth-testing against the original, and misuse is rejected.
//
// The scene canvas keeps the default (non-sampleable) depth target on purpose: on GLES that is
// a renderbuffer, which only a canvas-level copy can reach.

#include "test_harness.h"
#include "test_app_shared.h"

#include <cute.h>

using namespace Cute;

#define W 64
#define H 64
#define SCENE_Z 0.375f

static const char* s_vs =
"layout (location = 0) in vec2 in_pos;\n"
"layout (set = 1, binding = 0) uniform uniform_block {\n"
"    vec4 u_z;\n"
"};\n"
"void main() { gl_Position = vec4(in_pos, u_z.x, 1); }\n";

static const char* s_scene_fs =
"layout (location = 0) out vec4 result;\n"
"void main() { result = vec4(0.0, 0.0, 1.0, 1.0); }\n";

// Writes the sampled depth into red, and green as a marker that this pass wrote the pixel.
static const char* s_sample_fs =
"layout (location = 0) out vec4 result;\n"
"layout (set = 2, binding = 0) uniform sampler2D u_depth;\n"
"layout (set = 3, binding = 0) uniform uniform_block {\n"
"    vec4 u_inv_size;\n"
"};\n"
"void main() {\n"
"    float d = texture(u_depth, gl_FragCoord.xy * u_inv_size.xy).r;\n"
"    result = vec4(d, 1.0, 0.0, 1.0);\n"
"}\n";

static CF_Mesh s_make_quad(float x0, float x1)
{
	struct Vertex { float x, y; };
	Vertex verts[6] = { { x0, -1 }, { x1, -1 }, { x1, 1 }, { x0, -1 }, { x1, 1 }, { x0, 1 } };
	CF_VertexAttribute attrs[1] = { };
	attrs[0].name = "in_pos";
	attrs[0].format = CF_VERTEX_FORMAT_FLOAT2;
	attrs[0].offset = 0;
	CF_Mesh mesh = cf_make_mesh(sizeof(verts), attrs, 1, sizeof(Vertex));
	cf_mesh_update_vertex_data(mesh, verts, 6);
	return mesh;
}

static CF_Canvas s_make_sampleable_depth_canvas()
{
	CF_CanvasParams params = cf_canvas_defaults(W, H);
	params.depth_stencil_enable = true;
	params.depth_stencil_target.usage |= CF_TEXTURE_USAGE_SAMPLER_BIT;
	params.depth_stencil_target.filter = CF_FILTER_NEAREST;
	return cf_make_canvas(params);
}

static CF_Canvas s_make_scene_canvas()
{
	CF_CanvasParams params = cf_canvas_defaults(W, H);
	params.depth_stencil_enable = true;
	return cf_make_canvas(params);
}

static void s_readback(CF_Canvas canvas, CF_Pixel* px)
{
	CF_Readback rb = cf_canvas_readback(canvas);
	while (!cf_readback_ready(rb)) {}
	cf_readback_data(rb, px, W * H * (int)sizeof(CF_Pixel));
	cf_destroy_readback(rb);
}

static CF_Pixel s_left(CF_Pixel* px) { return px[(H / 2) * W + W / 4]; }
static CF_Pixel s_right(CF_Pixel* px) { return px[(H / 2) * W + 3 * W / 4]; }

static bool s_near(int value, float expected)
{
	int e = (int)(expected * 255.0f + 0.5f);
	return value >= e - 2 && value <= e + 2;
}

struct DepthCopyScene
{
	CF_Mesh left_quad;
	CF_Mesh full_quad;
	CF_Shader scene_shader;
	CF_Shader sample_shader;
	CF_Material scene_material;
	CF_Material sample_material;
	CF_Canvas scene;
	CF_Canvas copy;
};

// Scene depth: SCENE_Z over the left half, the 1.0 clear over the right half. The copy is
// cleared to 0 first so a missed copy can't pass for a correct one.
static void s_scene_begin(DepthCopyScene* s)
{
	s->left_quad = s_make_quad(-1, 0);
	s->full_quad = s_make_quad(-1, 1);
	s->scene_shader = cf_make_shader_from_source(s_vs, s_scene_fs);
	s->sample_shader = cf_make_shader_from_source(s_vs, s_sample_fs);
	s->scene_material = cf_make_material();
	s->sample_material = cf_make_material();
	s->scene = s_make_scene_canvas();
	s->copy = s_make_sampleable_depth_canvas();

	CF_RenderState rs = cf_render_state_3d_defaults();
	rs.cull_mode = CF_CULL_MODE_NONE;
	cf_material_set_render_state(s->scene_material, rs);
	float z[4] = { SCENE_Z, 0, 0, 0 };
	cf_material_set_uniform_vs(s->scene_material, "u_z", z, CF_UNIFORM_TYPE_FLOAT4, 1);
	cf_canvas_set_clear_depth_stencil(s->copy, 0.0f, 0);

	cf_app_update(NULL);
	cf_apply_canvas(s->copy, true);
	cf_apply_canvas(s->scene, true);
	cf_apply_mesh(s->left_quad);
	cf_apply_shader(s->scene_shader, s->scene_material);
	cf_draw_elements();
}

static void s_scene_end(DepthCopyScene* s)
{
	cf_destroy_canvas(s->copy);
	cf_destroy_canvas(s->scene);
	cf_destroy_material(s->sample_material);
	cf_destroy_material(s->scene_material);
	cf_destroy_shader(s->sample_shader);
	cf_destroy_shader(s->scene_shader);
	cf_destroy_mesh(s->full_quad);
	cf_destroy_mesh(s->left_quad);
}

static void s_bind_copy(DepthCopyScene* s, float effect_z, bool depth_test)
{
	CF_RenderState rs = cf_render_state_3d_defaults();
	rs.cull_mode = CF_CULL_MODE_NONE;
	rs.depth_write_enabled = false;
	rs.depth_compare = depth_test ? CF_COMPARE_FUNCTION_LESS_THAN : CF_COMPARE_FUNCTION_ALWAYS;
	cf_material_set_render_state(s->sample_material, rs);
	float z[4] = { effect_z, 0, 0, 0 };
	float inv_size[4] = { 1.0f / W, 1.0f / H, 0, 0 };
	cf_material_set_uniform_vs(s->sample_material, "u_z", z, CF_UNIFORM_TYPE_FLOAT4, 1);
	cf_material_set_uniform_fs(s->sample_material, "u_inv_size", inv_size, CF_UNIFORM_TYPE_FLOAT4, 1);
	cf_material_set_texture_fs(s->sample_material, "u_depth", cf_canvas_get_depth_stencil_target(s->copy));
}

TEST_CASE(test_copy_depth_then_sample)
{
	if (!test_make_app(W, H)) return true; // Headless CI: no display/GPU.

	DepthCopyScene s;
	s_scene_begin(&s);
	REQUIRE(s.scene.id && s.copy.id && s.scene_shader.id && s.sample_shader.id);
	REQUIRE(cf_canvas_get_depth_stencil_target(s.copy).id);

	cf_canvas_copy_depth(s.copy, s.scene);

	CF_Canvas out = cf_make_canvas(cf_canvas_defaults(W, H));
	s_bind_copy(&s, 0.0f, false);
	cf_apply_canvas(out, true);
	cf_apply_mesh(s.full_quad);
	cf_apply_shader(s.sample_shader, s.sample_material);
	cf_draw_elements();
	cf_app_draw_onto_screen(false);

	CF_Pixel* px = (CF_Pixel*)cf_alloc(W * H * (int)sizeof(CF_Pixel));
	s_readback(out, px);
	CF_Pixel l = s_left(px), r = s_right(px);
	cf_free(px);
	cf_destroy_canvas(out);
	s_scene_end(&s);
	test_destroy_app();

	REQUIRE(s_near(l.colors.r, SCENE_Z));
	REQUIRE(s_near(r.colors.r, 1.0f));
	return true;
}

// A depth-only copy target: a depth texture attached with attach_target. The getter must hand
// back that texture so the copy can be sampled through it like any other canvas depth.
TEST_CASE(test_copy_depth_into_attached_depth)
{
	if (!test_make_app(W, H)) return true;

	DepthCopyScene s;
	s_scene_begin(&s);
	REQUIRE(s.scene.id && s.copy.id && s.scene_shader.id && s.sample_shader.id);

	CF_TextureParams depth_params = cf_canvas_defaults(W, H).depth_stencil_target;
	depth_params.usage = CF_TEXTURE_USAGE_DEPTH_STENCIL_TARGET_BIT | CF_TEXTURE_USAGE_SAMPLER_BIT;
	depth_params.filter = CF_FILTER_NEAREST;
	CF_Texture depth_tex = cf_make_texture(depth_params);
	CF_CanvasParams attach_params = cf_canvas_defaults(0, 0);
	attach_params.attach_target = depth_tex;
	CF_Canvas depth_only = cf_make_canvas(attach_params);
	REQUIRE(depth_tex.id && depth_only.id);
	CF_Texture got = cf_canvas_get_depth_stencil_target(depth_only);
	REQUIRE(got.id == depth_tex.id);

	cf_canvas_set_clear_depth_stencil(depth_only, 0.0f, 0);
	cf_clear_canvas(depth_only);
	cf_canvas_copy_depth(depth_only, s.scene);

	CF_Canvas out = cf_make_canvas(cf_canvas_defaults(W, H));
	s_bind_copy(&s, 0.0f, false);
	cf_material_set_texture_fs(s.sample_material, "u_depth", got);
	cf_apply_canvas(out, true);
	cf_apply_mesh(s.full_quad);
	cf_apply_shader(s.sample_shader, s.sample_material);
	cf_draw_elements();
	cf_app_draw_onto_screen(false);

	CF_Pixel* px = (CF_Pixel*)cf_alloc(W * H * (int)sizeof(CF_Pixel));
	s_readback(out, px);
	CF_Pixel l = s_left(px), r = s_right(px);
	cf_free(px);
	cf_destroy_canvas(out);
	cf_destroy_canvas(depth_only);
	cf_destroy_texture(depth_tex);
	s_scene_end(&s);
	test_destroy_app();

	REQUIRE(s_near(l.colors.r, SCENE_Z));
	REQUIRE(s_near(r.colors.r, 1.0f));
	return true;
}

// Soft particles / water / decals: the effect samples the copy while the original depth
// still occludes it. The left half (scene in front of the effect) must keep the scene color;
// the right half shows the effect, carrying the copied depth it sampled.
TEST_CASE(test_copy_depth_effect_pass)
{
	if (!test_make_app(W, H)) return true;

	DepthCopyScene s;
	s_scene_begin(&s);
	REQUIRE(s.scene.id && s.copy.id && s.scene_shader.id && s.sample_shader.id);

	cf_canvas_copy_depth(s.copy, s.scene);

	s_bind_copy(&s, 0.5f, true);
	cf_apply_canvas(s.scene, false);
	cf_apply_mesh(s.full_quad);
	cf_apply_shader(s.sample_shader, s.sample_material);
	cf_draw_elements();
	cf_app_draw_onto_screen(false);

	CF_Pixel* px = (CF_Pixel*)cf_alloc(W * H * (int)sizeof(CF_Pixel));
	s_readback(s.scene, px);
	CF_Pixel l = s_left(px), r = s_right(px);
	cf_free(px);
	s_scene_end(&s);
	test_destroy_app();

	REQUIRE(l.colors.b > 200 && l.colors.g < 60);
	REQUIRE(r.colors.g > 200 && s_near(r.colors.r, 1.0f));
	return true;
}

static int s_assert_failures;

static void s_count_asserts(bool expr, const char* message, const char* file, int line)
{
	CF_UNUSED(message);
	CF_UNUSED(file);
	CF_UNUSED(line);
	if (!expr) ++s_assert_failures;
}

// Rejected copies assert; the default handler would break into the debugger, so count instead.
TEST_CASE(test_copy_depth_misuse)
{
	if (!test_make_app(W, H)) return true;

	CF_Canvas a = s_make_scene_canvas();
	CF_Canvas b = s_make_sampleable_depth_canvas();
	CF_CanvasParams small_params = cf_canvas_defaults(W / 2, H / 2);
	small_params.depth_stencil_enable = true;
	CF_Canvas small = cf_make_canvas(small_params);
	CF_Canvas no_depth = cf_make_canvas(cf_canvas_defaults(W, H));
	REQUIRE(a.id && b.id && small.id && no_depth.id);

	cf_app_update(NULL);
	s_assert_failures = 0;
	cf_assert_fn* prev_assert = g_assert_fn;
	cf_set_assert_handler(s_count_asserts);
	cf_canvas_copy_depth(b, small);
	int after_size = s_assert_failures;
	cf_canvas_copy_depth(small, a);
	int after_size_reversed = s_assert_failures;
	cf_canvas_copy_depth(b, no_depth);
	int after_no_src_depth = s_assert_failures;
	cf_canvas_copy_depth(no_depth, a);
	int after_no_dst_depth = s_assert_failures;
	cf_canvas_copy_depth(a, a);
	int after_same = s_assert_failures;
	cf_canvas_copy_depth(b, a);
	int after_valid = s_assert_failures;
	cf_set_assert_handler(prev_assert);

	int after_format = after_valid;
	CF_PixelFormat other = CF_PIXEL_FORMAT_D16_UNORM;
	CF_CanvasParams format_params = cf_canvas_defaults(W, H);
	format_params.depth_stencil_enable = true;
	if (format_params.depth_stencil_target.pixel_format == other) other = CF_PIXEL_FORMAT_D32_FLOAT;
	bool format_supported = cf_texture_supports_format(other, CF_TEXTURE_USAGE_DEPTH_STENCIL_TARGET_BIT);
	CF_Canvas other_format = { };
	if (format_supported) {
		format_params.depth_stencil_target.pixel_format = other;
		other_format = cf_make_canvas(format_params);
		cf_set_assert_handler(s_count_asserts);
		cf_canvas_copy_depth(b, other_format);
		after_format = s_assert_failures;
		cf_set_assert_handler(prev_assert);
	}
	cf_app_draw_onto_screen(false);

	if (other_format.id) cf_destroy_canvas(other_format);
	cf_destroy_canvas(no_depth);
	cf_destroy_canvas(small);
	cf_destroy_canvas(b);
	cf_destroy_canvas(a);
	test_destroy_app();

	REQUIRE(after_size == 1);
	REQUIRE(after_size_reversed == 2);
	REQUIRE(after_no_src_depth == 3);
	REQUIRE(after_no_dst_depth == 4);
	REQUIRE(after_same == 5);
	REQUIRE(after_valid == 5);
	if (format_supported) REQUIRE(after_format == 6);
	return true;
}

TEST_SUITE(test_canvas_copy_depth)
{
	RUN_TEST_CASE(test_copy_depth_then_sample);
	RUN_TEST_CASE(test_copy_depth_into_attached_depth);
	RUN_TEST_CASE(test_copy_depth_effect_pass);
	RUN_TEST_CASE(test_copy_depth_misuse);
}
