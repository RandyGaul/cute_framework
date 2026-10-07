/*
    Cute Framework
    Copyright (C) 2026 Randy Gaul https://randygaul.github.io/

    This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

// 2d depth (cf_draw_push_z): 2d against 3d meshes through the shared depth buffer, 2d against
// 2d, halo-free opaque edges, unchanged output without Z, path selection, draw lists, and
// gl_FragDepth from a draw shader.

#include "test_harness.h"
#include "test_app_shared.h"

#include <cute.h>
#include <internal/cute_draw_internal.h>

using namespace Cute;

#define W 64
#define H 64

static const char* s_mesh_vs =
"layout (location = 0) in vec3 in_pos;\n"
"layout (location = 8) in vec4 in_model0;\n"
"layout (location = 9) in vec4 in_model1;\n"
"layout (location = 10) in vec4 in_model2;\n"
"layout (location = 15) in vec4 in_mesh_attributes;\n"
"layout (location = 0) out vec4 v_color;\n"
"layout (set = 1, binding = 0) uniform uniform_block {\n"
"    mat4 u_view_projection;\n"
"};\n"
"void main() {\n"
"    vec4 p = vec4(in_pos, 1.0);\n"
"    vec3 world = vec3(dot(in_model0, p), dot(in_model1, p), dot(in_model2, p));\n"
"    v_color = in_mesh_attributes;\n"
"    gl_Position = u_view_projection * vec4(world, 1.0);\n"
"}\n";

static const char* s_mesh_fs =
"layout (location = 0) in vec4 v_color;\n"
"layout (location = 0) out vec4 result;\n"
"void main() { result = v_color; }\n";

// Writes depth relative to the Z depth: 0.1 nearer than the draw's own Z.
static const char* s_frag_depth_shd =
"vec4 shader(vec4 color, ShaderParams params)\n"
"{\n"
"	gl_FragDepth = gl_FragCoord.z - 0.1;\n"
"	return color;\n"
"}\n";

static CF_Mesh s_make_quad(float half)
{
	struct Vertex { float x, y, z; };
	Vertex verts[6] = {
		{ -half, -half, 0 }, { half, -half, 0 }, { half, half, 0 },
		{ -half, -half, 0 }, { half, half, 0 }, { -half, half, 0 },
	};
	CF_VertexAttribute attrs[1] = { };
	attrs[0].name = "in_pos";
	attrs[0].format = CF_VERTEX_FORMAT_FLOAT3;
	attrs[0].offset = 0;
	CF_Mesh mesh = cf_make_mesh(sizeof(verts), attrs, 1, sizeof(Vertex));
	cf_mesh_update_vertex_data(mesh, verts, 6);
	return mesh;
}

static CF_Canvas s_make_canvas(bool depth)
{
	CF_CanvasParams params = cf_canvas_defaults(W, H);
	params.depth_stencil_enable = depth;
	return cf_make_canvas(params);
}

static void s_readback(CF_Canvas canvas, CF_Pixel* px)
{
	CF_Readback rb = cf_canvas_readback(canvas);
	while (!cf_readback_ready(rb)) {}
	cf_readback_data(rb, px, W * H * (int)sizeof(CF_Pixel));
	cf_destroy_readback(rb);
}

static bool s_is(CF_Pixel* px, int x, int y, int r, int g, int b)
{
	CF_Pixel p = px[y * W + x];
	return cf_abs((int)p.colors.r - r) < 40 && cf_abs((int)p.colors.g - g) < 40 && cf_abs((int)p.colors.b - b) < 40;
}

// Filled box over a pixel rect [x0, x1) x [y0, y1) (top-left origin). The default 2d camera puts
// the world origin at the canvas center with +y up.
static void s_box(int x0, int y0, int x1, int y1, CF_Color color)
{
	cf_draw_push_color(color);
	cf_draw_box_fill(cf_make_aabb(cf_v2((float)(x0 - W / 2), (float)(H / 2 - y1)), cf_v2((float)(x1 - W / 2), (float)(H / 2 - y0))), 0);
	cf_draw_pop_color();
}

// A 3d quad at world z = 3 against 2d boxes at z = 2 (left) and z = 4 (right), under an ortho and
// a perspective camera, meshes first and 2d first. The z = 2 box hides wherever the mesh covers
// it and the z = 4 box always wins -- submission order doesn't matter, depth does.
TEST_CASE(test_draw_z_vs_3d)
{
	if (!test_make_app(W, H)) return true; // Headless CI: no display/GPU.

	CF_Mesh mesh = s_make_quad(0.5f);
	CF_Shader shader = cf_make_shader_from_source(s_mesh_vs, s_mesh_fs);
	REQUIRE(shader.id);
	CF_Canvas canvas = s_make_canvas(true);
	CF_Pixel* px = (CF_Pixel*)cf_alloc(W * H * (int)sizeof(CF_Pixel));

	for (int round = 0; round < 4; ++round) {
		bool perspective = round >= 2;
		bool mesh_first = (round & 1) == 0;
		cf_app_update(NULL);
		// The eye sits at z = 10, so the mesh is 7 units away. The perspective fov makes the
		// half-unit quad span ndc +-0.5 there, matching the ortho round's footprint.
		CF_M4x4 proj = perspective ? cf_perspective(2.0f * atanf(1.0f / 7.0f), 1.0f, 0.1f, 100.0f) : cf_ortho(-1, 1, -1, 1, 0.1f, 100.0f);
		cf_draw3d_push_projection(proj);
		cf_draw3d_push_view(cf_look_at(cf_v3(0, 0, 10), cf_v3(0, 0, 0), cf_v3(0, 1, 0)));
		cf_draw3d_push_shader(shader);
		cf_draw3d_push_mesh_attributes(cf_v4(0, 1, 0, 1)); // Green.

		for (int pass = 0; pass < 2; ++pass) {
			if ((pass == 0) == mesh_first) {
				cf_draw3d_push();
				cf_draw3d_translate(cf_v3(0, 0, 3));
				cf_draw3d_mesh(mesh);
				cf_draw3d_pop();
			} else {
				cf_draw_push_z(2);
				s_box(4, 24, 32, 40, cf_color_red());
				cf_draw_pop_z();
				cf_draw_push_z(4);
				s_box(32, 24, 60, 40, cf_color_blue());
				cf_draw_pop_z();
			}
		}
		cf_render_to(canvas, true);
		cf_app_draw_onto_screen(false);
		s_readback(canvas, px);

		REQUIRE(s_is(px, 24, 32, 0, 255, 0));  // Mesh over the z = 2 box.
		REQUIRE(s_is(px, 40, 32, 0, 0, 255));  // z = 4 box over the mesh.
		REQUIRE(s_is(px, 8, 32, 255, 0, 0));   // z = 2 box clear of the mesh.
		REQUIRE(s_is(px, 56, 32, 0, 0, 255));  // z = 4 box clear of the mesh.
		REQUIRE(s_is(px, 32, 20, 0, 255, 0));  // Mesh alone.

		cf_draw3d_pop_mesh_attributes();
		cf_draw3d_pop_shader();
		cf_draw3d_pop_view();
		cf_draw3d_pop_projection();
	}

	cf_free(px);
	cf_destroy_canvas(canvas);
	cf_destroy_shader(shader);
	cf_destroy_mesh(mesh);
	test_destroy_app();
	return true;
}

// Without a 3d camera, Z still orders 2d against 2d: later-drawn lower-Z shapes and sprites hide
// behind earlier higher-Z ones, higher-Z ones draw over them, and a draw without Z ignores depth.
TEST_CASE(test_draw_z_2d_order)
{
	if (!test_make_app(W, H)) return true; // Headless CI: no display/GPU.

	CF_Canvas canvas = s_make_canvas(true);
	CF_Pixel* px = (CF_Pixel*)cf_alloc(W * H * (int)sizeof(CF_Pixel));
	CF_Pixel white[16 * 16];
	for (int i = 0; i < 16 * 16; ++i) white[i] = cf_make_pixel_rgba(255, 255, 255, 255);
	CF_Sprite sprite = cf_make_easy_sprite_from_pixels(white, 16, 16);

	cf_app_update(NULL);
	cf_draw_push_z(5);
	s_box(8, 8, 40, 40, cf_color_red());
	cf_draw_pop_z();
	cf_draw_push_z(1);
	s_box(24, 24, 56, 56, cf_color_blue()); // Behind red where they overlap.
	sprite.transform.p = cf_v2(0, 0);         // Pixels 24..40: inside red, behind it.
	cf_draw_sprite(&sprite);
	cf_draw_pop_z();
	cf_draw_push_z(9);
	s_box(4, 44, 20, 60, cf_color_green());
	cf_draw_pop_z();
	cf_draw_push_z(0);
	s_box(10, 50, 30, 54, cf_color_red()); // Behind green.
	cf_draw_pop_z();
	s_box(52, 4, 60, 12, cf_color_green()); // No Z: no depth test.
	cf_render_to(canvas, true);
	cf_app_draw_onto_screen(false);
	s_readback(canvas, px);

	REQUIRE(s_is(px, 16, 16, 255, 0, 0)); // Red alone.
	REQUIRE(s_is(px, 32, 32, 255, 0, 0)); // Red over the later blue box and white sprite.
	REQUIRE(s_is(px, 48, 48, 0, 0, 255)); // Blue alone.
	REQUIRE(s_is(px, 12, 52, 0, 255, 0)); // Green over the later red at z = 0.
	REQUIRE(s_is(px, 22, 52, 255, 0, 0)); // That red where green doesn't reach.
	REQUIRE(s_is(px, 56, 8, 0, 255, 0));

	// Higher Z drawn later wins.
	cf_app_update(NULL);
	cf_draw_push_z(1);
	s_box(8, 8, 40, 40, cf_color_red());
	cf_draw_pop_z();
	cf_draw_push_z(2);
	s_box(24, 24, 56, 56, cf_color_blue());
	cf_draw_pop_z();
	cf_render_to(canvas, true);
	cf_app_draw_onto_screen(false);
	s_readback(canvas, px);
	REQUIRE(s_is(px, 32, 32, 0, 0, 255));

	cf_easy_sprite_unload(&sprite);
	cf_free(px);
	cf_destroy_canvas(canvas);
	test_destroy_app();
	return true;
}

// An opaque anti-aliased edge writes depth only where it is at least half covered, so a shape
// drawn behind it later shows through the outer fringe instead of meeting a dark halo.
TEST_CASE(test_draw_z_no_halo)
{
	if (!test_make_app(W, H)) return true; // Headless CI: no display/GPU.

	CF_Canvas canvas = s_make_canvas(true);
	CF_Pixel* px = (CF_Pixel*)cf_alloc(W * H * (int)sizeof(CF_Pixel));

	cf_app_update(NULL);
	cf_clear_color(0, 0, 0, 1);
	cf_draw_push_z(5);
	cf_draw_circle_fill2(cf_v2(0.5f, 0.25f), 13.3f);
	cf_draw_pop_z();
	cf_draw_push_z(1);
	s_box(0, 0, W, H, cf_color_red());
	cf_draw_pop_z();
	cf_render_to(canvas, true);
	cf_app_draw_onto_screen(false);
	s_readback(canvas, px);

	REQUIRE(s_is(px, 32, 32, 255, 255, 255));
	REQUIRE(s_is(px, 2, 2, 255, 0, 0));
	// Every pixel is red (behind) or at least half-covered white over the black clear. A fringe
	// that owned depth would leave dark pixels where red belongs.
	int dark = 0;
	for (int i = 0; i < W * H; ++i) dark += px[i].colors.r < 100;
	REQUIRE(dark == 0);

	cf_clear_color(0, 0, 0, 0);
	cf_free(px);
	cf_destroy_canvas(canvas);
	test_destroy_app();
	return true;
}

// The same scene of shapes, sprites, and text renders bit-identically: without Z on a canvas
// without depth, with Z on a canvas without depth (Z means nothing there), and without Z on a
// canvas with depth (no Z means no depth test).
TEST_CASE(test_draw_z_unchanged_without_depth)
{
	if (!test_make_app(W, H)) return true; // Headless CI: no display/GPU.

	CF_Canvas plain = s_make_canvas(false);
	CF_Canvas deep = s_make_canvas(true);
	CF_Pixel* ref = (CF_Pixel*)cf_alloc(W * H * (int)sizeof(CF_Pixel));
	CF_Pixel* px = (CF_Pixel*)cf_alloc(W * H * (int)sizeof(CF_Pixel));
	CF_Pixel pix[8 * 8];
	for (int i = 0; i < 8 * 8; ++i) pix[i] = cf_make_pixel_rgba(200, 100, (uint8_t)(i * 4), 255);
	CF_Sprite sprite = cf_make_easy_sprite_from_pixels(pix, 8, 8);

	// Round -1 only warms the atlas: an image first seen after the frame's defrag samples from
	// its own texture until the next frame packs it, which shifts sampling slightly.
	for (int round = -1; round < 3; ++round) {
		bool with_z = round == 1;
		cf_app_update(NULL);
		if (with_z) cf_draw_push_z(3);
		cf_draw_push_color(cf_make_color_rgba_f(0.2f, 0.6f, 0.9f, 0.7f));
		cf_draw_circle_fill2(cf_v2(-10, 8), 11.5f);
		cf_draw_pop_color();
		cf_draw_capsule2(cf_v2(-20, -20), cf_v2(20, -12), 3.3f, 1.5f);
		sprite.transform.p = cf_v2(12, 10);
		cf_draw_sprite(&sprite);
		cf_draw_text("Zq", cf_v2(-28, 30), -1);
		if (with_z) cf_draw_pop_z();
		cf_render_to(round == 2 ? deep : plain, true);
		cf_app_draw_onto_screen(false);
		s_readback(round == 2 ? deep : plain, round <= 0 ? ref : px);
		if (round > 0) REQUIRE(!CF_MEMCMP(ref, px, W * H * (int)sizeof(CF_Pixel)));
	}

	cf_easy_sprite_unload(&sprite);
	cf_free(ref);
	cf_free(px);
	cf_destroy_canvas(plain);
	cf_destroy_canvas(deep);
	test_destroy_app();
	return true;
}

// The tiled path can't depth test: batches with Z route instanced even with tiled forced on,
// while the same draws without Z still take the tile walk.
TEST_CASE(test_draw_z_path_selection)
{
	if (!test_make_app(W, H)) return true; // Headless CI: no display/GPU.
	if (!cf_draw_tiled_available()) { test_destroy_app(); return true; } // GLES: instanced only.

	CF_Canvas canvas = s_make_canvas(true);
	cf_draw_set_tiled_enabled(true);
	for (int round = 0; round < 2; ++round) {
		bool with_z = round == 1;
		cf_app_update(NULL);
		cf_draw_tiled_stats(NULL, NULL, NULL); // Reset.
		if (with_z) cf_draw_push_z(1);
		s_box(4, 4, 60, 60, cf_color_red());
		s_box(8, 8, 30, 30, cf_color_blue());
		if (with_z) cf_draw_pop_z();
		cf_render_to(canvas, true);
		int tiled = 0, instanced = 0;
		cf_draw_tiled_stats(&tiled, &instanced, NULL);
		cf_app_draw_onto_screen(false);
		if (with_z) {
			REQUIRE(tiled == 0 && instanced > 0);
		} else {
			REQUIRE(tiled > 0 && instanced == 0);
		}
	}
	cf_draw_set_tiled_auto();
	cf_destroy_canvas(canvas);
	test_destroy_app();
	return true;
}

// Draw lists record Z relative to the Z at cf_draw_list_begin and replay it on top of the Z then
// current, like layers -- a list recorded without Z still depth-tests when replayed under one.
TEST_CASE(test_draw_z_draw_list)
{
	if (!test_make_app(W, H)) return true; // Headless CI: no display/GPU.

	CF_Canvas canvas = s_make_canvas(true);
	CF_Pixel* px = (CF_Pixel*)cf_alloc(W * H * (int)sizeof(CF_Pixel));
	CF_DrawList list = cf_make_draw_list();

	cf_app_update(NULL);
	cf_draw_list_begin(list);
	s_box(16, 16, 48, 48, cf_color_blue()); // No Z of its own.
	cf_draw_push_z(4);
	s_box(4, 28, 12, 36, cf_color_green()); // Z 4 above the replay's Z.
	cf_draw_pop_z();
	cf_draw_list_end();

	cf_draw_push_z(5);
	s_box(0, 0, 32, 64, cf_color_red());  // Left half at z = 5.
	cf_draw_pop_z();
	cf_draw_push_z(3);
	cf_draw_list(list); // Blue at z = 3: behind red. Green at z = 7: over red.
	cf_draw_pop_z();
	cf_render_to(canvas, true);
	cf_app_draw_onto_screen(false);
	s_readback(canvas, px);

	REQUIRE(s_is(px, 24, 32, 255, 0, 0)); // Red over the replayed blue.
	REQUIRE(s_is(px, 40, 32, 0, 0, 255)); // Replayed blue clear of red.
	REQUIRE(s_is(px, 8, 32, 0, 255, 0));  // Replayed green over red.

	cf_destroy_draw_list(list);
	cf_free(px);
	cf_destroy_canvas(canvas);
	test_destroy_app();
	return true;
}

// A draw shader that writes gl_FragDepth replaces the Z depth, and can write it relative to the
// Z depth through gl_FragCoord.z. Here the shader pulls a z = 0 box 0.1 nearer in depth -- in the
// default range, as near as z = 2000 -- so a later box at z = 1000 hides behind it while one at
// z = 3000 doesn't.
TEST_CASE(test_draw_z_frag_depth)
{
	if (!test_make_app(W, H)) return true; // Headless CI: no display/GPU.

	CF_Shader shd = cf_make_draw_shader_from_source(s_frag_depth_shd);
	REQUIRE(shd.id);
	CF_Canvas canvas = s_make_canvas(true);
	CF_Pixel* px = (CF_Pixel*)cf_alloc(W * H * (int)sizeof(CF_Pixel));

	cf_app_update(NULL);
	cf_draw_push_z(0);
	cf_draw_push_shader(shd);
	s_box(8, 8, 56, 56, cf_color_red());
	cf_draw_pop_shader();
	cf_draw_pop_z();
	cf_draw_push_z(1000);
	s_box(0, 24, 32, 40, cf_color_blue());
	cf_draw_pop_z();
	cf_draw_push_z(3000);
	s_box(32, 24, 64, 40, cf_color_green());
	cf_draw_pop_z();
	cf_render_to(canvas, true);
	cf_app_draw_onto_screen(false);
	s_readback(canvas, px);

	REQUIRE(s_is(px, 20, 32, 255, 0, 0)); // Red's written depth beats z = 1000.
	REQUIRE(s_is(px, 4, 32, 0, 0, 255));  // Blue clear of red.
	REQUIRE(s_is(px, 44, 32, 0, 255, 0)); // z = 3000 beats red's written depth.

	cf_free(px);
	cf_destroy_canvas(canvas);
	cf_destroy_shader(shd);
	test_destroy_app();
	return true;
}

TEST_SUITE(test_draw_z)
{
	RUN_TEST_CASE(test_draw_z_vs_3d);
	RUN_TEST_CASE(test_draw_z_2d_order);
	RUN_TEST_CASE(test_draw_z_no_halo);
	RUN_TEST_CASE(test_draw_z_unchanged_without_depth);
	RUN_TEST_CASE(test_draw_z_path_selection);
	RUN_TEST_CASE(test_draw_z_draw_list);
	RUN_TEST_CASE(test_draw_z_frag_depth);
}
