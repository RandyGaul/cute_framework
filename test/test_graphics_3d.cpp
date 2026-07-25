/*
    Cute Framework
    Copyright (C) 2025 Randy Gaul https://randygaul.github.io/

    This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

// Coverage for the two graphics paths a 3d renderer depends on and nothing else in the tree
// exercised: uploading a mat4 uniform, and actually depth-testing against a canvas depth buffer.
// CF_UNIFORM_TYPE_MAT4 had no callers at all before this, so its memory layout was unverified
// against a real backend.

#include "test_harness.h"

#include <cute.h>
#include <stdlib.h>

using namespace Cute;

#define W 256
#define H 256

static int s_app_options()
{
	int options = CF_APP_OPTIONS_HIDDEN_BIT | CF_APP_OPTIONS_NO_AUDIO_BIT;
	const char* gles = getenv("CF_TEST_GLES");
	if (gles && *gles == '1') options |= CF_APP_OPTIONS_GFX_OPENGL_BIT | CF_APP_OPTIONS_GFX_DEBUG_BIT;
	return options;
}

// Samples the canvas at a normalized-device coordinate. CF renders row 0 at the top, so +y in
// NDC is a low row index.
static CF_Pixel s_probe_ndc(const CF_Pixel* px, float nx, float ny)
{
	int col = (int)((nx * 0.5f + 0.5f) * W);
	int row = (int)((1.0f - (ny * 0.5f + 0.5f)) * H);
	if (col < 0) col = 0;
	if (col >= W) col = W - 1;
	if (row < 0) row = 0;
	if (row >= H) row = H - 1;
	return px[row * W + col];
}

// Spelled out rather than via abs(): `using namespace Cute` makes that name ambiguous here.
static bool s_near(int a, int b) { int d = a - b; return (d < 0 ? -d : d) < 24; }
static bool s_is(CF_Pixel p, int r, int g, int b) { return s_near(p.colors.r, r) && s_near(p.colors.g, g) && s_near(p.colors.b, b); }

static const char* s_vs =
"layout (location = 0) in vec2 in_pos;\n"
"layout (set = 1, binding = 0) uniform uniform_block { mat4 u_mvp; };\n"
"void main() { gl_Position = u_mvp * vec4(in_pos, 0, 1); }\n";

static const char* s_fs =
"layout(location = 0) out vec4 result;\n"
"layout (set = 3, binding = 0) uniform uniform_block { vec4 u_color; };\n"
"void main() { result = u_color; }\n";

static CF_Mesh s_make_quad(float x0, float y0, float x1, float y1)
{
	struct Vertex { float x, y; };
	Vertex verts[4] = { { x0, y0 }, { x1, y0 }, { x1, y1 }, { x0, y1 } };
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

// Matrices are built as raw float[16] rather than with cute_math3d.h's CF_M4x4 on purpose: what is
// under test here is the graphics-layer contract that CF_UNIFORM_TYPE_MAT4 uploads 16 contiguous
// floats which the shader reads column-major. Any CPU-side matrix type is a separate concern.
static void s_m4_identity(float* e)
{
	for (int i = 0; i < 16; ++i) e[i] = 0;
	e[0] = e[5] = e[10] = e[15] = 1.0f;
}

// Column-major, so element (row, col) is at e[col * 4 + row].
static void s_m4_rot_z(float* e, float radians)
{
	for (int i = 0; i < 16; ++i) e[i] = 0;
	float c = cosf(radians), s = sinf(radians);
	e[0] = c; e[1] = s;
	e[4] = -s; e[5] = c;
	e[10] = 1.0f; e[15] = 1.0f;
}

static void s_m4_translate_z(float* e, float z)
{
	s_m4_identity(e);
	e[14] = z;
}

static void s_readback(CF_Canvas canvas, CF_Pixel* px)
{
	CF_Readback rb = cf_canvas_readback(canvas);
	while (!cf_readback_ready(rb)) {}
	cf_readback_data(rb, px, W * H * (int)sizeof(CF_Pixel));
	cf_destroy_readback(rb);
}

// CF_M4x4 is column-major and uploaded by a straight memcpy, so the shader must read it back
// column-major too. A 90-degree z-rotation is the discriminator: transposed, a rotation becomes
// its inverse and the quad lands on the opposite side. A symmetric matrix would prove nothing.
TEST_CASE(test_mat4_uniform_is_column_major)
{
	if (cf_is_error(cf_make_app(NULL, 0, 0, 0, W, H, s_app_options(), NULL))) return true; // Headless CI: no display/GPU.

	CF_Mesh mesh = s_make_quad(0.3f, -0.2f, 0.7f, 0.2f); // Centered on +x.
	CF_Material material = cf_make_material();
	CF_Shader shader = cf_make_shader_from_source(s_vs, s_fs);
	REQUIRE(shader.id);

	CF_Canvas canvas = cf_make_canvas(cf_canvas_defaults(W, H));
	CF_Pixel* px = (CF_Pixel*)cf_alloc(W * H * (int)sizeof(CF_Pixel));
	cf_clear_color(0, 0, 0, 1);

	float red[4] = { 1, 0, 0, 1 };
	cf_material_set_uniform_fs(material, "u_color", red, CF_UNIFORM_TYPE_FLOAT4, 1);

	// Identity first: proves the uniform arrives at all. A block whose name does not match
	// reflects to nothing, uploads nothing, reads as zeros, and renders nothing -- which looks
	// exactly like a broken mat4, so separate the two failures.
	float m[16];
	s_m4_identity(m);
	cf_app_update(NULL);
	cf_material_set_uniform_vs(material, "u_mvp", m, CF_UNIFORM_TYPE_MAT4, 1);
	cf_apply_canvas(canvas, true);
	cf_apply_mesh(mesh);
	cf_apply_shader(shader, material);
	cf_draw_elements();
	cf_app_draw_onto_screen(false);
	s_readback(canvas, px);
	REQUIRE(s_is(s_probe_ndc(px, 0.5f, 0.0f), 255, 0, 0));

	// Now rotate 90 degrees about z: +x must travel to +y, not -y.
	s_m4_rot_z(m, CF_PI * 0.5f);
	cf_app_update(NULL);
	cf_material_set_uniform_vs(material, "u_mvp", m, CF_UNIFORM_TYPE_MAT4, 1);
	cf_apply_canvas(canvas, true);
	cf_apply_mesh(mesh);
	cf_apply_shader(shader, material);
	cf_draw_elements();
	cf_app_draw_onto_screen(false);
	s_readback(canvas, px);
	REQUIRE(s_is(s_probe_ndc(px, 0.0f, 0.5f), 255, 0, 0));   // Rotated the right way.
	REQUIRE(!s_is(s_probe_ndc(px, 0.0f, -0.5f), 255, 0, 0)); // Not the inverse rotation.

	cf_free(px);
	cf_destroy_canvas(canvas);
	cf_destroy_shader(shader);
	cf_destroy_material(material);
	cf_destroy_mesh(mesh);
	cf_destroy_app();
	return true;
}

// Draws a NEAR quad, then a FAR quad on top of it. With depth testing the far one is rejected;
// without it the later draw wins. The depth-off pass is a control: it proves the probe can tell
// the difference, so a passing depth-on result is not just "the second draw never happened".
TEST_CASE(test_render_state_3d_defaults_depth_tests)
{
	if (cf_is_error(cf_make_app(NULL, 0, 0, 0, W, H, s_app_options(), NULL))) return true; // Headless CI: no display/GPU.

	CF_RenderState rs3d = cf_render_state_3d_defaults();
	REQUIRE(rs3d.depth_write_enabled);
	REQUIRE(rs3d.depth_compare == CF_COMPARE_FUNCTION_LESS_THAN);
	REQUIRE(rs3d.cull_mode == CF_CULL_MODE_BACK);

	CF_Mesh mesh = s_make_quad(-0.6f, -0.6f, 0.6f, 0.6f);
	CF_Shader shader = cf_make_shader_from_source(s_vs, s_fs);
	REQUIRE(shader.id);

	CF_CanvasParams params = cf_canvas_defaults(W, H);
	params.depth_stencil_enable = true; // Without this every depth field below is ignored.
	CF_Canvas canvas = cf_make_canvas(params);

	CF_Material near_mat = cf_make_material();
	CF_Material far_mat = cf_make_material();
	CF_Pixel* px = (CF_Pixel*)cf_alloc(W * H * (int)sizeof(CF_Pixel));
	cf_clear_color(0, 0, 0, 1);

	for (int depth_on = 1; depth_on >= 0; --depth_on) {
		// Culling stays off here: the quad's winding is not the thing under test.
		CF_RenderState rs = depth_on ? cf_render_state_3d_defaults() : cf_render_state_defaults();
		rs.cull_mode = CF_CULL_MODE_NONE;
		cf_material_set_render_state(near_mat, rs);
		cf_material_set_render_state(far_mat, rs);

		float red[4] = { 1, 0, 0, 1 }, blue[4] = { 0, 0, 1, 1 };
		cf_material_set_uniform_fs(near_mat, "u_color", red, CF_UNIFORM_TYPE_FLOAT4, 1);
		cf_material_set_uniform_fs(far_mat, "u_color", blue, CF_UNIFORM_TYPE_FLOAT4, 1);

		// Identity with a z translation: gl_Position.z is the NDC z directly, since w stays 1.
		// 0.2 is nearer than 0.8 under the [0, 1] clip-space convention SDL_GPU normalizes to.
		float near_m[16], far_m[16];
		s_m4_translate_z(near_m, 0.2f);
		s_m4_translate_z(far_m, 0.8f);

		cf_app_update(NULL);
		cf_apply_canvas(canvas, true);

		cf_material_set_uniform_vs(near_mat, "u_mvp", near_m, CF_UNIFORM_TYPE_MAT4, 1);
		cf_apply_mesh(mesh);
		cf_apply_shader(shader, near_mat);
		cf_draw_elements();

		cf_material_set_uniform_vs(far_mat, "u_mvp", far_m, CF_UNIFORM_TYPE_MAT4, 1);
		cf_apply_mesh(mesh);
		cf_apply_shader(shader, far_mat);
		cf_draw_elements();

		cf_app_draw_onto_screen(false);
		s_readback(canvas, px);

		CF_Pixel c = s_probe_ndc(px, 0.0f, 0.0f);
		if (depth_on) {
			REQUIRE(s_is(c, 255, 0, 0)); // Near quad survived; the far one was depth-rejected.
		} else {
			REQUIRE(s_is(c, 0, 0, 255)); // Control: without depth the later draw wins.
		}
	}

	cf_free(px);
	cf_destroy_canvas(canvas);
	cf_destroy_shader(shader);
	cf_destroy_material(near_mat);
	cf_destroy_material(far_mat);
	cf_destroy_mesh(mesh);
	cf_destroy_app();
	return true;
}

// The built-in lit shader is the one piece of GLSL CF ships for 3d, so a compile failure would
// only surface in a user's app. Renders a full-screen quad facing the camera under a light aimed
// straight at it, where the Lambert term is exactly 1 and the result is predictable.
TEST_CASE(test_lit_shader_3d)
{
	if (cf_is_error(cf_make_app(NULL, 0, 0, 0, W, H, s_app_options(), NULL))) return true; // Headless CI: no display/GPU.

	CF_Shader shader = cf_make_lit_shader_3d();
	REQUIRE(shader.id); // Compiles at all -- the whole point of this case.

	// A quad in the z = 0 plane facing +z, already in clip space so no projection is needed.
	CF_Vertex3D verts[4] = {
		{ cf_v3(-0.8f, -0.8f, 0.0f), cf_v3(0, 0, 1.0f), cf_v2(0, 0) },
		{ cf_v3( 0.8f, -0.8f, 0.0f), cf_v3(0, 0, 1.0f), cf_v2(1.0f, 0) },
		{ cf_v3( 0.8f,  0.8f, 0.0f), cf_v3(0, 0, 1.0f), cf_v2(1.0f, 1.0f) },
		{ cf_v3(-0.8f,  0.8f, 0.0f), cf_v3(0, 0, 1.0f), cf_v2(0, 1.0f) },
	};
	uint32_t indices[6] = { 0, 1, 2, 0, 2, 3 };
	CF_Mesh mesh = cf_make_mesh_3d(verts, 4, indices, 6);

	// 1x1 white albedo, so the output is the lighting term alone.
	CF_TextureParams tp = cf_texture_defaults(1, 1);
	CF_Texture white = cf_make_texture(tp);
	CF_Pixel wpx = cf_pixel_white();
	cf_texture_update(white, &wpx, (int)sizeof(CF_Pixel));

	CF_Material material = cf_make_material();
	CF_RenderState rs = cf_render_state_3d_defaults();
	rs.cull_mode = CF_CULL_MODE_NONE; // Winding is not what this case is testing.
	cf_material_set_render_state(material, rs);

	// Light travels along -z, straight into a surface whose normal is +z, so N.L is 1.
	cf_lit_shader_3d_set_transform(material, cf_m4_identity(), cf_m4_identity());
	cf_lit_shader_3d_set_light(material, cf_v3(0, 0, -1.0f), cf_make_color_rgb_f(1.0f, 0, 0), cf_make_color_rgb_f(0, 0, 0));
	cf_lit_shader_3d_set_albedo(material, white);

	CF_CanvasParams params = cf_canvas_defaults(W, H);
	params.depth_stencil_enable = true;
	CF_Canvas canvas = cf_make_canvas(params);
	CF_Pixel* px = (CF_Pixel*)cf_alloc(W * H * (int)sizeof(CF_Pixel));
	cf_clear_color(0, 0, 0, 1);

	cf_app_update(NULL);
	cf_apply_canvas(canvas, true);
	cf_apply_mesh(mesh);
	cf_apply_shader(shader, material);
	cf_draw_elements();
	cf_app_draw_onto_screen(false);
	s_readback(canvas, px);

	// Fully lit by a red light: red channel saturated, nothing else.
	REQUIRE(s_is(s_probe_ndc(px, 0.0f, 0.0f), 255, 0, 0));
	// Outside the quad stays cleared, proving the quad did not simply cover everything.
	REQUIRE(s_is(s_probe_ndc(px, 0.95f, 0.95f), 0, 0, 0));

	cf_free(px);
	cf_destroy_canvas(canvas);
	cf_destroy_texture(white);
	cf_destroy_shader(shader);
	cf_destroy_material(material);
	cf_destroy_mesh(mesh);
	cf_destroy_app();
	return true;
}

TEST_SUITE(test_graphics_3d)
{
	RUN_TEST_CASE(test_mat4_uniform_is_column_major);
	RUN_TEST_CASE(test_render_state_3d_defaults_depth_tests);
	RUN_TEST_CASE(test_lit_shader_3d);
}
