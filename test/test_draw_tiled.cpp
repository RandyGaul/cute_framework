/*
	Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

#include "test_harness.h"

#include <cute.h>
#include <internal/cute_draw_internal.h>
#include <internal/cute_app_internal.h>

using namespace Cute;

// -------------------------------------------------------------------------------------------------
// Pure binning helper tests (no app required).

TEST_CASE(test_tile_range_basics)
{
	int x0, y0, x1, y1;

	// A box spanning pixels [10..40]x[10..40] with 16px tiles covers tiles [0..2]x[0..2].
	REQUIRE(cf_tile_range(10, 10, 40, 40, 10, 10, &x0, &y0, &x1, &y1));
	REQUIRE(x0 == 0 && y0 == 0 && x1 == 2 && y1 == 2);

	// Clamped at grid edges.
	REQUIRE(cf_tile_range(-100, -100, 1000000, 5, 4, 4, &x0, &y0, &x1, &y1));
	REQUIRE(x0 == 0 && y0 == 0 && x1 == 3 && y1 == 0);

	// Fully outside.
	REQUIRE(!cf_tile_range(-50, -50, -1, -1, 4, 4, &x0, &y0, &x1, &y1));
	REQUIRE(!cf_tile_range(16 * 4 + 1, 0, 16 * 4 + 10, 10, 4, 4, &x0, &y0, &x1, &y1));

	return true;
}




// -------------------------------------------------------------------------------------------------
// Mesh path vs tiled path pixel diff.

static void s_scene(float t)
{
	// Overlapping translucent shapes of every type, some crossing tile boundaries,
	// plus camera transforms mid-frame to exercise the matrix palette.
	cf_draw_push_color(cf_make_color_rgba_f(0.9f, 0.2f, 0.2f, 0.8f));
	cf_draw_circle_fill2(cf_v2(-100, 50), 60);
	cf_draw_pop_color();

	cf_draw_push_color(cf_make_color_rgba_f(0.2f, 0.9f, 0.3f, 0.6f));
	cf_draw_circle_fill2(cf_v2(-70, 30), 60);
	cf_draw_pop_color();

	cf_draw_push_color(cf_make_color_rgba_f(0.2f, 0.3f, 0.9f, 0.7f));
	cf_draw_quad_fill(cf_make_aabb(cf_v2(-40, -80), cf_v2(80, 20)), 8.0f);
	cf_draw_pop_color();

	// Rotated box via camera.
	cf_draw_push();
	cf_draw_rotate(0.5f + t);
	cf_draw_push_color(cf_make_color_rgba_f(0.9f, 0.8f, 0.1f, 0.5f));
	cf_draw_quad_fill(cf_make_aabb(cf_v2(-30, -30), cf_v2(50, 40)), 0);
	cf_draw_pop_color();
	cf_draw_pop();

	// Long skinny diagonal line (bin-raster case).
	cf_draw_push_color(cf_make_color_rgba_f(0.1f, 0.9f, 0.9f, 1.0f));
	cf_draw_line(cf_v2(-300, -200), cf_v2(300, 220), 3.0f);
	cf_draw_pop_color();

	// Capsule + stroked circle.
	cf_draw_push_color(cf_make_color_rgba_f(0.9f, 0.1f, 0.9f, 0.9f));
	cf_draw_capsule_fill2(cf_v2(120, -60), cf_v2(220, 60), 25);
	cf_draw_circle2(cf_v2(180, 120), 45, 6);
	cf_draw_pop_color();

	// Triangles: filled SDF + polyline polygon.
	cf_draw_push_color(cf_make_color_rgba_f(0.4f, 0.6f, 0.2f, 0.75f));
	cf_draw_tri_fill(cf_v2(-250, -150), cf_v2(-150, -150), cf_v2(-200, -40), 5.0f);
	cf_draw_pop_color();

	CF_V2 poly[5] = { cf_v2(60, 140), cf_v2(120, 160), cf_v2(150, 210), cf_v2(80, 230), cf_v2(40, 190) };
	cf_draw_push_color(cf_make_color_rgba_f(0.7f, 0.4f, 0.9f, 0.85f));
	cf_draw_polygon_fill(poly, 5, 4.0f);
	cf_draw_pop_color();

	// Camera scale for another palette entry.
	cf_draw_push();
	cf_draw_scale(1.5f, 1.5f);
	cf_draw_push_color(cf_make_color_rgba_f(0.9f, 0.5f, 0.2f, 0.4f));
	cf_draw_circle_fill2(cf_v2(50, -100), 30);
	cf_draw_pop_color();
	cf_draw_pop();

	// Text (glyph sprites with per-corner colors).
	cf_push_font_size(26);
	cf_draw_push_color(cf_color_white());
	cf_draw_text("tile binning renderer", cf_v2(-200, 180), -1);
	cf_draw_pop_color();
	cf_pop_font_size();

	// Polylines: translucent zig-zag (bisector-clipped bodies must not double-blend)
	// and an opaque closed loop (wraparound joints).
	CF_V2 zig[5] = { cf_v2(-300, 60), cf_v2(-250, 120), cf_v2(-200, 60), cf_v2(-150, 130), cf_v2(-100, 70) };
	cf_draw_push_color(cf_make_color_rgba_f(0.9f, 0.6f, 0.1f, 0.5f));
	cf_draw_polyline(zig, 5, 9.0f, false);
	cf_draw_pop_color();
	CF_V2 ring[4] = { cf_v2(220, -160), cf_v2(290, -150), cf_v2(280, -90), cf_v2(210, -100) };
	cf_draw_push_color(cf_make_color_rgba_f(0.2f, 0.8f, 0.9f, 1.0f));
	cf_draw_polyline(ring, 4, 7.0f, true);
	cf_draw_pop_color();

	// Opaque stacked shapes: exercises the opaque-cover cull (the big top circle hides
	// large parts of everything drawn above/underneath it in covered tiles).
	cf_draw_push_color(cf_make_color_rgba_f(0.15f, 0.2f, 0.5f, 1.0f));
	cf_draw_circle_fill2(cf_v2(-180, -120), 90);
	cf_draw_pop_color();
	cf_draw_push_color(cf_make_color_rgba_f(0.5f, 0.2f, 0.15f, 1.0f));
	cf_draw_circle_fill2(cf_v2(-160, -100), 70);
	cf_draw_pop_color();
}

// mode: 0 = instanced command-fed mesh (reference), 1 = tiled (GPU binning).
typedef void (*TestSceneFn)();
static bool s_readback(TestSceneFn fn, int mode, int w, int h, CF_Pixel* out)
{
	cf_app_update(NULL); // Acquire the frame's GPU command buffer.
	cf_draw_set_tiled_enabled(mode == 1);
	CF_Canvas canvas = cf_make_canvas(cf_canvas_defaults(w, h));
	fn();
	cf_render_to(canvas, true);
	cf_app_draw_onto_screen(false); // Submits all recorded GPU work (hidden window).
	CF_Readback rb = cf_canvas_readback(canvas);
	REQUIRE(rb.id);
	while (!cf_readback_ready(rb)) {}
	int size = w * h * (int)sizeof(CF_Pixel);
	REQUIRE(cf_readback_size(rb) == size);
	cf_readback_data(rb, out, size);
	cf_destroy_readback(rb);
	cf_destroy_canvas(canvas);
	if (getenv("CF_TEST_DUMP")) { // Debug aid: dump every readback for eyeballing.
		static int dump_counter = 0;
		char path[64];
		snprintf(path, sizeof(path), "/dump_%03d.png", dump_counter++);
		cf_fs_set_write_directory(cf_fs_get_base_directory());
		CF_Image img = { w, h, out };
		cf_image_save_png(path, &img);
	}
	return true;
}

static void s_scene0() { s_scene(0); }
static bool s_readback_scene(int mode, int w, int h, CF_Pixel* out) { return s_readback(s_scene0, mode, w, h, out); }

// The tiled path composites in fp32 registers while the mesh path round-trips the
// framebuffer at 8 bits per blend, so results are near-identical but not bit-equal.
// Allow small per-channel deltas everywhere and larger ones on a small fraction of
// pixels (AA fringes and shape edges).
static bool s_diff_ok(const CF_Pixel* a, const CF_Pixel* b, int total, const char* label)
{
	auto absi = [](int v) { return v < 0 ? -v : v; };
	int big_diffs = 0;
	double sum_diff = 0;
	for (int i = 0; i < total; ++i) {
		int dr = absi((int)a[i].colors.r - (int)b[i].colors.r);
		int dg = absi((int)a[i].colors.g - (int)b[i].colors.g);
		int db = absi((int)a[i].colors.b - (int)b[i].colors.b);
		int da = absi((int)a[i].colors.a - (int)b[i].colors.a);
		int d = cf_max(cf_max(dr, dg), cf_max(db, da));
		sum_diff += d;
		if (d > 8) big_diffs++;
	}
	double mean = sum_diff / (double)total;
	// Require: mean under 1 LSB, and less than 0.5% of pixels differing noticeably.
	bool ok = mean < 1.0 && big_diffs <= total / 200;
	if (!ok) printf("%s diff too large: mean=%f big_diffs=%d/%d\n", label, mean, big_diffs, total);
	return ok;
}

// Samples the pixel at world position (x_world, 0). All probe scenes keep their
// geometry x-separated and spanning y=0 so the result is insensitive to whether
// readback row 0 is the top or bottom of the canvas.
static CF_Pixel s_probe(const CF_Pixel* px, int w, int h, int x_world)
{
	return px[(h / 2) * w + (w / 2 + x_world)];
}

static bool s_px_near(CF_Pixel p, int r, int g, int b, int a, int tol)
{
	auto absi = [](int v) { return v < 0 ? -v : v; };
	bool ok = absi((int)p.colors.r - r) <= tol && absi((int)p.colors.g - g) <= tol && absi((int)p.colors.b - b) <= tol && absi((int)p.colors.a - a) <= tol;
	if (!ok) printf("pixel (%d %d %d %d) expected (%d %d %d %d) +/-%d\n", p.colors.r, p.colors.g, p.colors.b, p.colors.a, r, g, b, a, tol);
	return ok;
}

// Set CF_TEST_GLES=1 to run the draw tests against the GLES3 backend (instanced-only:
// forcing tiled on falls back per-batch, so both "modes" render instanced there).
static int s_app_options()
{
	int options = CF_APP_OPTIONS_HIDDEN_BIT | CF_APP_OPTIONS_NO_AUDIO_BIT;
	const char* gles = getenv("CF_TEST_GLES");
	if (gles && *gles == '1') options |= CF_APP_OPTIONS_GFX_OPENGL_BIT | CF_APP_OPTIONS_GFX_DEBUG_BIT;
	return options;
}

TEST_CASE(test_tiled_matches_mesh)
{
	if (cf_is_error(cf_make_app(NULL, 0, 0, 0, 640, 480, s_app_options(), NULL))) return true; // Headless CI: no display/GPU.

	if (cf_query_backend() == CF_BACKEND_TYPE_GLES3) {
		// No SSBOs on GLES -- the tiled path never exists there.
		cf_destroy_app();
		return true;
	}
	// On SDL_GPU backends the tiled path must be available; a tile-shader compile
	// failure would otherwise silently disable it and this test would pass vacuously.
	REQUIRE(cf_draw_tiled_available());

	int w = 640, h = 480;
	CF_Pixel* mesh_px = (CF_Pixel*)cf_alloc(w * h * sizeof(CF_Pixel));
	CF_Pixel* tile_px = (CF_Pixel*)cf_alloc(w * h * sizeof(CF_Pixel));

	REQUIRE(s_readback_scene(0, w, h, mesh_px)); // Instanced path is the reference.
	REQUIRE(s_readback_scene(1, w, h, tile_px));
	REQUIRE(s_diff_ok(mesh_px, tile_px, w * h, "tiled-vs-mesh"));

	cf_free(mesh_px);
	cf_free(tile_px);
	cf_destroy_app();
	return true;
}

// -------------------------------------------------------------------------------------------------
// Custom draw shaders ride the instanced command path (never tiled). Verify a user
// `shader()` stub end-to-end: attributes plumbing, uniform blocks, SDF coverage in
// `color.a`, and that forcing the tiled path safely falls back to instanced.

#ifdef CF_RUNTIME_SHADER_COMPILATION
static const char* s_attr_shd_src = R"(
layout (set = 3, binding = 1) uniform shd_uniforms {
	vec4 u_tint;
};

vec4 shader(vec4 color, ShaderParams params)
{
	// Replace the shape's color with attributes * uniform, keeping SDF coverage (color.a).
	return vec4(params.attributes.rgb * u_tint.rgb, 1.0) * color.a;
}
)";
static CF_Shader s_attr_shd;

static void s_scene_custom_shader()
{
	cf_draw_push_shader(s_attr_shd);
	cf_draw_set_uniform_color("u_tint", cf_make_color_rgba_f(1.0f, 0.5f, 1.0f, 1.0f));
	cf_draw_push_vertex_attributes(0.25f, 0.5f, 0.75f, 1.0f);
	cf_draw_quad_fill(cf_make_aabb(cf_v2(-100, -40), cf_v2(-20, 40)), 0);
	cf_draw_pop_vertex_attributes();
	cf_draw_push_vertex_attributes(1.0f, 1.0f, 0.0f, 1.0f);
	cf_draw_quad_fill(cf_make_aabb(cf_v2(20, -40), cf_v2(100, 40)), 0);
	cf_draw_pop_vertex_attributes();
	cf_draw_pop_shader();
}
#endif

TEST_CASE(test_draw_custom_shader)
{
#ifdef CF_RUNTIME_SHADER_COMPILATION
	if (cf_is_error(cf_make_app(NULL, 0, 0, 0, 640, 480, s_app_options(), NULL))) return true; // Headless CI: no display/GPU.

	s_attr_shd = cf_make_draw_shader_from_source(s_attr_shd_src);
	REQUIRE(s_attr_shd.id);

	int w = 640, h = 480;
	CF_Pixel* px = (CF_Pixel*)cf_alloc(w * h * sizeof(CF_Pixel));
	// mode 1 forces tiled on: custom-shader batches must detect ineligibility and
	// fall back to the instanced path with identical output.
	for (int mode = 0; mode <= 1; ++mode) {
		REQUIRE(s_readback(s_scene_custom_shader, mode, w, h, px));
		REQUIRE(s_px_near(s_probe(px, w, h, -60), 64, 64, 191, 255, 3));  // attributes(.25,.5,.75) * tint(1,.5,1)
		REQUIRE(s_px_near(s_probe(px, w, h, 60), 255, 128, 0, 255, 3));   // attributes(1,1,0) * tint(1,.5,1)
		REQUIRE(s_px_near(s_probe(px, w, h, 0), 0, 0, 0, 0, 0));          // Gap between the boxes: untouched.
	}

	cf_free(px);
	cf_destroy_shader(s_attr_shd);
	cf_destroy_app();
#endif
	return true;
}

// -------------------------------------------------------------------------------------------------
// Multiple textures in one batch: a premade atlas (its own texture), shapes (no texture),
// and glyphs (dynamic atlas texture) interleaved in paint order. Exercises the seq-merge
// logic that splits a batch into per-texture draws without reordering.

static CF_Sprite s_premade;

static void s_scene_multi_atlas()
{
	// Bottom: opaque green box across the left+center.
	cf_draw_push_color(cf_make_color_rgba_f(0, 1, 0, 1));
	cf_draw_quad_fill(cf_make_aabb(cf_v2(-100, -50), cf_v2(10, 50)), 0);
	cf_draw_pop_color();

	// Middle: premade sprite (own texture) covering [-32,32] on both axes.
	s_premade.transform.p = cf_v2(0, 0);
	s_premade.scale = cf_v2(8, 8);
	cf_draw_sprite(&s_premade);

	// Top: opaque blue box over the sprite's right half and beyond.
	cf_draw_push_color(cf_make_color_rgba_f(0, 0, 1, 1));
	cf_draw_quad_fill(cf_make_aabb(cf_v2(16, -50), cf_v2(100, 50)), 0);
	cf_draw_pop_color();

	// Glyphs pull in the dynamic atlas texture as a third texture in the batch.
	cf_draw_push_color(cf_color_white());
	cf_push_font_size(18);
	cf_draw_text("atlas", cf_v2(-100, 60), -1);
	cf_pop_font_size();
	cf_draw_pop_color();

	// Glyphs UNDER a later shape, with other textures in the same flush: paint order
	// must hold across atlas texture splits (a regression here draws whichever
	// texture's sprites render last on top of everything).
	cf_draw_push_color(cf_color_white());
	cf_push_font_size(30);
	cf_draw_text("@@@@", cf_v2(-266, 16), -1); // Dense ink crossing y=0 near x=-250..-180.
	cf_pop_font_size();
	cf_draw_pop_color();
	cf_draw_push_color(cf_make_color_rgba_f(1, 0, 1, 1));
	cf_draw_quad_fill(cf_make_aabb(cf_v2(-276, -30), cf_v2(-170, 30)), 0);
	cf_draw_pop_color();
}

TEST_CASE(test_draw_multi_atlas_interleave)
{
	if (cf_is_error(cf_make_app(NULL, 0, 0, 0, 640, 480, s_app_options(), NULL))) return true; // Headless CI: no display/GPU.

	// Bake a solid 8x8 png on the fly and register it as a premade atlas.
	CF_Pixel solid[64];
	for (int i = 0; i < 64; ++i) solid[i] = cf_make_pixel_rgba(200, 40, 220, 255);
	CF_Image img;
	img.w = 8;
	img.h = 8;
	img.pix = solid;
	cf_fs_set_write_directory(cf_fs_get_base_directory());
	REQUIRE(!cf_is_error(cf_image_save_png("/tile_test_premade.png", &img)));
	CF_AtlasSubImage sub = { 0, 8, 8, 0.0f, 0.0f, 1.0f, 1.0f };
	CF_Texture premade_tex = cf_register_premade_atlas("/tile_test_premade.png", 1, &sub);
	REQUIRE(premade_tex.id);
	s_premade = cf_make_premade_sprite(0);

	int w = 640, h = 480;
	CF_Pixel* a = (CF_Pixel*)cf_alloc(w * h * sizeof(CF_Pixel));
	CF_Pixel* b = (CF_Pixel*)cf_alloc(w * h * sizeof(CF_Pixel));
	CF_Pixel* px[2] = { a, b };
	for (int mode = 0; mode <= 1; ++mode) {
		REQUIRE(s_readback(s_scene_multi_atlas, mode, w, h, px[mode]));
		REQUIRE(s_px_near(s_probe(px[mode], w, h, -60), 0, 255, 0, 255, 3));    // Green box only.
		REQUIRE(s_px_near(s_probe(px[mode], w, h, -20), 200, 40, 220, 255, 6)); // Sprite over green box.
		REQUIRE(s_px_near(s_probe(px[mode], w, h, 24), 0, 0, 255, 255, 3));     // Blue box over sprite.
		REQUIRE(s_px_near(s_probe(px[mode], w, h, 60), 0, 0, 255, 255, 3));     // Blue box only.
		REQUIRE(s_px_near(s_probe(px[mode], w, h, -220), 255, 0, 255, 255, 3)); // Box over glyphs (cross-texture paint order).
	}
	REQUIRE(s_diff_ok(a, b, w * h, "multi-atlas tiled-vs-mesh"));

	cf_free(a);
	cf_free(b);
	cf_destroy_texture(premade_tex);
	cf_destroy_app();
	return true;
}

// -------------------------------------------------------------------------------------------------
// Custom render states route to the instanced path (in-register composition is only valid
// for the default premultiplied src-over state). Verify additive blending and a two-pass
// stencil mask still work, including when the tiled path is forced on.

static void s_scene_additive()
{
	CF_RenderState add = cf_render_state_defaults();
	add.blend.rgb_src_blend_factor = CF_BLENDFACTOR_ONE;
	add.blend.rgb_dst_blend_factor = CF_BLENDFACTOR_ONE;
	add.blend.alpha_src_blend_factor = CF_BLENDFACTOR_ONE;
	add.blend.alpha_dst_blend_factor = CF_BLENDFACTOR_ONE;
	cf_draw_push_render_state(add);
	cf_draw_push_color(cf_make_color_rgba_f(1, 0, 0, 1));
	cf_draw_quad_fill(cf_make_aabb(cf_v2(-80, -40), cf_v2(20, 40)), 0);
	cf_draw_pop_color();
	cf_draw_push_color(cf_make_color_rgba_f(0, 1, 0, 1));
	cf_draw_quad_fill(cf_make_aabb(cf_v2(-20, -40), cf_v2(80, 40)), 0);
	cf_draw_pop_color();
	cf_draw_pop_render_state();
}

TEST_CASE(test_draw_render_states)
{
	if (cf_is_error(cf_make_app(NULL, 0, 0, 0, 640, 480, s_app_options(), NULL))) return true; // Headless CI: no display/GPU.

	int w = 640, h = 480;
	CF_Pixel* px = (CF_Pixel*)cf_alloc(w * h * sizeof(CF_Pixel));

	// Additive blend; identical results whether tiled is forced on (falls back) or off.
	for (int mode = 0; mode <= 1; ++mode) {
		REQUIRE(s_readback(s_scene_additive, mode, w, h, px));
		REQUIRE(s_px_near(s_probe(px, w, h, -50), 255, 0, 0, 255, 3)); // Red only.
		REQUIRE(s_px_near(s_probe(px, w, h, 0), 255, 255, 0, 255, 3)); // Overlap: red + green.
		REQUIRE(s_px_near(s_probe(px, w, h, 50), 0, 255, 0, 255, 3));  // Green only.
		REQUIRE(s_px_near(s_probe(px, w, h, 110), 0, 0, 0, 0, 0));     // Untouched.
	}

	// Two-pass stencil mask on a stencil-capable canvas: pass 1 writes ref=1 under a
	// left-half red box, pass 2 draws a fullscreen blue box clipped to stencil==1.
	cf_app_update(NULL);
	cf_draw_set_tiled_enabled(true); // Must fall back per-batch; stencil states are never tiled.
	CF_CanvasParams params = cf_canvas_defaults(w, h);
	params.depth_stencil_enable = true;
	CF_Canvas canvas = cf_make_canvas(params);

	CF_RenderState ws = cf_render_state_defaults();
	ws.stencil.enabled = true;
	ws.stencil.front.compare = CF_COMPARE_FUNCTION_ALWAYS;
	ws.stencil.front.pass_op = CF_STENCIL_OP_REPLACE;
	ws.stencil.back = ws.stencil.front;
	ws.stencil.read_mask = 0;
	ws.stencil.write_mask = 0xFF;
	ws.stencil.reference = 1;
	cf_draw_push_render_state(ws);
	cf_draw_push_color(cf_make_color_rgba_f(1, 0, 0, 1));
	cf_draw_quad_fill(cf_make_aabb(cf_v2(-(float)w, -(float)h), cf_v2(0, (float)h)), 0);
	cf_draw_pop_color();
	cf_draw_pop_render_state();
	cf_render_to(canvas, true);

	CF_RenderState ts = cf_render_state_defaults();
	ts.stencil.enabled = true;
	ts.stencil.front.compare = CF_COMPARE_FUNCTION_EQUAL;
	ts.stencil.front.pass_op = CF_STENCIL_OP_KEEP;
	ts.stencil.back = ts.stencil.front;
	ts.stencil.read_mask = 0xFF;
	ts.stencil.write_mask = 0;
	ts.stencil.reference = 1;
	cf_draw_push_render_state(ts);
	cf_draw_push_color(cf_make_color_rgba_f(0, 0, 1, 1));
	cf_draw_quad_fill(cf_make_aabb(cf_v2(-(float)w, -(float)h), cf_v2((float)w, (float)h)), 0);
	cf_draw_pop_color();
	cf_draw_pop_render_state();
	cf_render_to(canvas, false);

	cf_app_draw_onto_screen(false);
	CF_Readback rb = cf_canvas_readback(canvas);
	REQUIRE(rb.id);
	while (!cf_readback_ready(rb)) {}
	REQUIRE(cf_readback_size(rb) == w * h * (int)sizeof(CF_Pixel));
	cf_readback_data(rb, px, w * h * (int)sizeof(CF_Pixel));
	cf_destroy_readback(rb);
	cf_destroy_canvas(canvas);

	REQUIRE(s_px_near(s_probe(px, w, h, -40), 0, 0, 255, 255, 3)); // Stencil==1: blue drew over red.
	REQUIRE(s_px_near(s_probe(px, w, h, 40), 0, 0, 0, 0, 0));      // Stencil==0: blue clipped, untouched.

	cf_free(px);
	cf_destroy_app();
	return true;
}

// -------------------------------------------------------------------------------------------------
// Layers sort commands before rendering: higher layers draw on top regardless of
// submission order, in both the instanced and tiled paths.

static void s_scene_layers()
{
	cf_draw_push_layer(2);
	cf_draw_push_color(cf_make_color_rgba_f(1, 0, 0, 1));
	cf_draw_quad_fill(cf_make_aabb(cf_v2(-60, -40), cf_v2(20, 40)), 0);
	cf_draw_pop_color();
	cf_draw_pop_layer();

	// Submitted later but on a lower layer: must render underneath.
	cf_draw_push_layer(1);
	cf_draw_push_color(cf_make_color_rgba_f(0, 1, 0, 1));
	cf_draw_quad_fill(cf_make_aabb(cf_v2(-20, -40), cf_v2(60, 40)), 0);
	cf_draw_pop_color();
	cf_draw_pop_layer();
}

TEST_CASE(test_draw_layers)
{
	if (cf_is_error(cf_make_app(NULL, 0, 0, 0, 640, 480, s_app_options(), NULL))) return true; // Headless CI: no display/GPU.

	int w = 640, h = 480;
	CF_Pixel* a = (CF_Pixel*)cf_alloc(w * h * sizeof(CF_Pixel));
	CF_Pixel* b = (CF_Pixel*)cf_alloc(w * h * sizeof(CF_Pixel));
	CF_Pixel* px[2] = { a, b };
	for (int mode = 0; mode <= 1; ++mode) {
		REQUIRE(s_readback(s_scene_layers, mode, w, h, px[mode]));
		REQUIRE(s_px_near(s_probe(px[mode], w, h, -40), 255, 0, 0, 255, 3)); // Red only.
		REQUIRE(s_px_near(s_probe(px[mode], w, h, 0), 255, 0, 0, 255, 3));   // Overlap: layer 2 beats layer 1.
		REQUIRE(s_px_near(s_probe(px[mode], w, h, 40), 0, 255, 0, 255, 3));  // Green only.
	}
	REQUIRE(s_diff_ok(a, b, w * h, "layers tiled-vs-mesh"));

	cf_free(a);
	cf_free(b);
	cf_destroy_app();
	return true;
}

// -------------------------------------------------------------------------------------------------
// Polylines record one clipped-capsule command PER SEGMENT; probe every segment (a
// regression here dropped all but the last segment, which path-vs-path diffs cannot
// catch since both paths render the same wrong thing).

static void s_scene_polyline_segments()
{
	// Zig with three segments crossing y=0 at x=-100, 0, and +100 (their midpoints).
	CF_V2 pts[4] = { cf_v2(-150, -40), cf_v2(-50, 40), cf_v2(50, -40), cf_v2(150, 40) };
	cf_draw_push_color(cf_make_color_rgba_f(1, 0, 0, 1));
	cf_draw_polyline(pts, 4, 10.0f, false);
	cf_draw_pop_color();
}

TEST_CASE(test_draw_polyline_segments)
{
	if (cf_is_error(cf_make_app(NULL, 0, 0, 0, 640, 480, s_app_options(), NULL))) return true; // Headless CI: no display/GPU.

	int w = 640, h = 480;
	CF_Pixel* a = (CF_Pixel*)cf_alloc(w * h * sizeof(CF_Pixel));
	CF_Pixel* b = (CF_Pixel*)cf_alloc(w * h * sizeof(CF_Pixel));
	CF_Pixel* px[2] = { a, b };
	for (int mode = 0; mode <= 1; ++mode) {
		REQUIRE(s_readback(s_scene_polyline_segments, mode, w, h, px[mode]));
		REQUIRE(s_px_near(s_probe(px[mode], w, h, -100), 255, 0, 0, 255, 3)); // First segment.
		REQUIRE(s_px_near(s_probe(px[mode], w, h, 0), 255, 0, 0, 255, 3));    // Middle segment.
		REQUIRE(s_px_near(s_probe(px[mode], w, h, 100), 255, 0, 0, 255, 3));  // Last segment.
		REQUIRE(s_px_near(s_probe(px[mode], w, h, -200), 0, 0, 0, 0, 0));     // Outside.
	}
	REQUIRE(s_diff_ok(a, b, w * h, "polyline-segments tiled-vs-mesh"));

	cf_free(a);
	cf_free(b);
	cf_destroy_app();
	return true;
}

// -------------------------------------------------------------------------------------------------
// The builtin arrow renders as a single SDF command (capsule shaft unioned with the
// triangular head). A translucent arrow must therefore blend exactly once everywhere --
// in particular at the shaft/head seam, where the old two-command arrow double-blended.

static void s_scene_arrow()
{
	cf_draw_push_color(cf_make_color_rgba_f(0.0f, 0.5f, 1.0f, 0.5f));
	cf_draw_arrow(cf_v2(-100, 0), cf_v2(100, 0), 12.0f, 30.0f);
	cf_draw_pop_color();
}

TEST_CASE(test_draw_arrow_no_overdraw)
{
	if (cf_is_error(cf_make_app(NULL, 0, 0, 0, 640, 480, s_app_options(), NULL))) return true; // Headless CI: no display/GPU.

	int w = 640, h = 480;
	CF_Pixel* a = (CF_Pixel*)cf_alloc(w * h * sizeof(CF_Pixel));
	CF_Pixel* b = (CF_Pixel*)cf_alloc(w * h * sizeof(CF_Pixel));
	CF_Pixel* px[2] = { a, b };
	for (int mode = 0; mode <= 1; ++mode) {
		REQUIRE(s_readback(s_scene_arrow, mode, w, h, px[mode]));
		// Head base sits at x=70 (tip 100 - head length 30). Sample the shaft interior,
		// the seam region, and the head interior: alpha 0.5 over clear must give the
		// identical single-blend value everywhere, with no darker double-blend band.
		CF_Pixel shaft = s_probe(px[mode], w, h, -50);
		CF_Pixel seam = s_probe(px[mode], w, h, 68);
		CF_Pixel head = s_probe(px[mode], w, h, 80);
		REQUIRE(s_px_near(shaft, 0, 64, 128, 128, 3)); // premultiplied (0, .5, 1) * 0.5
		REQUIRE(s_px_near(seam, shaft.colors.r, shaft.colors.g, shaft.colors.b, shaft.colors.a, 2));
		REQUIRE(s_px_near(head, shaft.colors.r, shaft.colors.g, shaft.colors.b, shaft.colors.a, 2));
		REQUIRE(s_px_near(s_probe(px[mode], w, h, 140), 0, 0, 0, 0, 0)); // Past the tip: untouched.
	}
	REQUIRE(s_diff_ok(a, b, w * h, "arrow tiled-vs-mesh"));

	cf_free(a);
	cf_free(b);
	cf_destroy_app();
	return true;
}

// -------------------------------------------------------------------------------------------------
// User-registered custom SDF shapes: registry dispatch, exact match against the builtin
// circle SDF, stroked variant, and both renderer paths (including SDF-based tile culling
// which trusts the user's distance function).

#ifdef CF_RUNTIME_SHADER_COMPILATION
static const char* s_circle_sdf_src = R"(
// params: a = center, b.x = radius
float sdf(vec2 p, ShapeParams s)
{
	return length(p - s.a) - s.b.x;
}
)";

static const char* s_arrowish_sdf_src = R"(
// params: a = tail, b = tip, c.x = shaft radius, c.y = head length/half-width
float sdf(vec2 p, ShapeParams s)
{
	return distance_arrow(p, s.a, s.b, s.c.x, s.c.y);
}
)";

static CF_CustomShape s_circle_shape;
static CF_CustomShape s_arrowish_shape;

static void s_scene_custom_shapes()
{
	// Left: filled custom circle (registry id 1).
	float circle_params[3] = { -150, 0, 40 };
	cf_draw_push_color(cf_make_color_rgba_f(1, 0, 0, 1));
	cf_draw_custom_shape_fill(s_circle_shape, cf_make_aabb(cf_v2(-190, -40), cf_v2(-110, 40)), circle_params, 3);
	cf_draw_pop_color();

	// Middle: stroked custom circle (exercises the automatic outline machinery).
	float ring_params[3] = { -20, 0, 30 };
	cf_draw_push_color(cf_make_color_rgba_f(0, 1, 0, 1));
	cf_draw_custom_shape(s_circle_shape, cf_make_aabb(cf_v2(-50, -30), cf_v2(10, 30)), 8.0f, ring_params, 3);
	cf_draw_pop_color();

	// Right: second registered shape (registry id 2 -- dispatcher must pick the right fn).
	float arrow_params[6] = { 60, 0, 180, 0, 6, 24 };
	cf_draw_push_color(cf_make_color_rgba_f(0, 0, 1, 1));
	cf_draw_custom_shape_fill(s_arrowish_shape, cf_make_aabb(cf_v2(50, -30), cf_v2(190, 30)), arrow_params, 6);
	cf_draw_pop_color();
}

static void s_scene_builtin_circle_ref()
{
	// The builtin circle the custom sdf must match exactly.
	cf_draw_push_color(cf_make_color_rgba_f(1, 0, 0, 1));
	cf_draw_circle_fill2(cf_v2(-150, 0), 40);
	cf_draw_pop_color();
}

static void s_scene_custom_circle_only()
{
	float circle_params[3] = { -150, 0, 40 };
	cf_draw_push_color(cf_make_color_rgba_f(1, 0, 0, 1));
	cf_draw_custom_shape_fill(s_circle_shape, cf_make_aabb(cf_v2(-190, -40), cf_v2(-110, 40)), circle_params, 3);
	cf_draw_pop_color();
}
#endif

TEST_CASE(test_draw_custom_shapes)
{
#ifdef CF_RUNTIME_SHADER_COMPILATION
	if (cf_is_error(cf_make_app(NULL, 0, 0, 0, 640, 480, s_app_options(), NULL))) return true; // Headless CI: no display/GPU.

	s_circle_shape = cf_make_custom_shape(s_circle_sdf_src);
	REQUIRE(s_circle_shape.id);
	s_arrowish_shape = cf_make_custom_shape(s_arrowish_sdf_src);
	REQUIRE(s_arrowish_shape.id);

	int w = 640, h = 480;
	CF_Pixel* a = (CF_Pixel*)cf_alloc(w * h * sizeof(CF_Pixel));
	CF_Pixel* b = (CF_Pixel*)cf_alloc(w * h * sizeof(CF_Pixel));
	CF_Pixel* px[2] = { a, b };

	for (int mode = 0; mode <= 1; ++mode) {
		REQUIRE(s_readback(s_scene_custom_shapes, mode, w, h, px[mode]));
		REQUIRE(s_px_near(s_probe(px[mode], w, h, -150), 255, 0, 0, 255, 3)); // Custom circle interior.
		REQUIRE(s_px_near(s_probe(px[mode], w, h, -210), 0, 0, 0, 0, 0));     // Outside everything.
		REQUIRE(s_px_near(s_probe(px[mode], w, h, -50), 0, 255, 0, 255, 3));  // On the ring stroke (|d|=0 at r=30, probe at 30 px left of center).
		REQUIRE(s_px_near(s_probe(px[mode], w, h, -20), 0, 0, 0, 0, 0));      // Ring center: hollow.
		REQUIRE(s_px_near(s_probe(px[mode], w, h, 100), 0, 0, 255, 255, 3));  // Arrowish shaft.
		REQUIRE(s_px_near(s_probe(px[mode], w, h, 165), 0, 0, 255, 255, 3));  // Arrowish head.
	}
	REQUIRE(s_diff_ok(a, b, w * h, "custom-shapes tiled-vs-mesh"));

	// A true-SDF custom circle must render identically to the builtin circle: same
	// distance function through the same machinery. Instanced path, exact scene diff.
	REQUIRE(s_readback(s_scene_builtin_circle_ref, 0, w, h, a));
	REQUIRE(s_readback(s_scene_custom_circle_only, 0, w, h, b));
	REQUIRE(s_diff_ok(a, b, w * h, "custom-circle-vs-builtin"));

	cf_free(a);
	cf_free(b);
	cf_destroy_app();
#endif
	return true;
}

// -------------------------------------------------------------------------------------------------
// Deeper custom-shape coverage: attributes plumbing into ShapeParams, camera transforms,
// opaque-cover occlusion through a user SDF, failed-registration recovery, and
// invalid-handle draws.

#ifdef CF_RUNTIME_SHADER_COMPILATION
static const char* s_attr_circle_sdf_src = R"(
// params: a = center, b.x = base radius, scaled by attributes.x at draw time.
float sdf(vec2 p, ShapeParams s)
{
	return length(p - s.a) - s.b.x * s.attributes.x;
}
)";

static CF_CustomShape s_attr_circle_shape;
static CF_CustomShape s_plain_circle_shape;

static void s_scene_attr_shape()
{
	// Base radius 60 scaled to 30 via attributes: pixels between r=30 and r=60 stay
	// empty only if attributes actually reach the sdf.
	float params[3] = { -100, 0, 60 };
	cf_draw_push_color(cf_make_color_rgba_f(1, 0, 0, 1));
	cf_draw_push_vertex_attributes(0.5f, 0, 0, 0);
	cf_draw_custom_shape_fill(s_attr_circle_shape, cf_make_aabb(cf_v2(-160, -60), cf_v2(-40, 60)), params, 3);
	cf_draw_pop_vertex_attributes();
	cf_draw_pop_color();
}

static void s_scene_xform_builtin()
{
	cf_draw_push();
	cf_draw_translate(30, 15);
	cf_draw_rotate(0.7f);
	cf_draw_scale(1.4f, 1.4f);
	cf_draw_push_color(cf_make_color_rgba_f(0.2f, 0.8f, 0.4f, 1));
	cf_draw_circle_fill2(cf_v2(60, 10), 30);
	cf_draw_pop_color();
	cf_draw_pop();
}

static void s_scene_xform_custom()
{
	// Identical camera stack; a true-SDF custom circle must land on the same pixels.
	cf_draw_push();
	cf_draw_translate(30, 15);
	cf_draw_rotate(0.7f);
	cf_draw_scale(1.4f, 1.4f);
	float params[3] = { 60, 10, 30 };
	cf_draw_push_color(cf_make_color_rgba_f(0.2f, 0.8f, 0.4f, 1));
	cf_draw_custom_shape_fill(s_plain_circle_shape, cf_make_aabb(cf_v2(30, -20), cf_v2(90, 40)), params, 3);
	cf_draw_pop_color();
	cf_draw_pop();
}

static void s_scene_custom_occlusion()
{
	// Translucent clutter beneath...
	for (int i = 0; i < 8; ++i) {
		cf_draw_push_color(cf_make_color_rgba_f(0.1f * i, 0.9f - 0.1f * i, 0.5f, 0.4f));
		cf_draw_circle_fill2(cf_v2(-140.0f + 40.0f * i, (i & 1) ? 40.0f : -40.0f), 50);
		cf_draw_pop_color();
	}
	// ...hidden by a big opaque custom shape (covers well over 64 tiles: the tiled
	// path's opaque-cover cull must engage through the user sdf without artifacts)...
	float params[3] = { 0, 0, 200 };
	cf_draw_push_color(cf_make_color_rgba_f(0.15f, 0.15f, 0.3f, 1));
	cf_draw_custom_shape_fill(s_plain_circle_shape, cf_make_aabb(cf_v2(-200, -200), cf_v2(200, 200)), params, 3);
	cf_draw_pop_color();
	// ...with a translucent triangle composited on top.
	cf_draw_push_color(cf_make_color_rgba_f(1, 0.8f, 0.1f, 0.5f));
	cf_draw_tri_fill(cf_v2(-60, -50), cf_v2(60, -50), cf_v2(0, 70), 3.0f);
	cf_draw_pop_color();
}

static void s_scene_invalid_handles()
{
	float params[3] = { 0, 0, 100 };
	CF_CustomShape none = { 0 };
	CF_CustomShape out_of_range = { 99 };
	cf_draw_push_color(cf_make_color_rgba_f(1, 0, 0, 1));
	cf_draw_custom_shape_fill(none, cf_make_aabb(cf_v2(-100, -100), cf_v2(100, 100)), params, 3);
	cf_draw_custom_shape_fill(out_of_range, cf_make_aabb(cf_v2(-100, -100), cf_v2(100, 100)), params, 3);
	cf_draw_pop_color();
}
#endif

TEST_CASE(test_draw_custom_shapes_advanced)
{
#ifdef CF_RUNTIME_SHADER_COMPILATION
	if (cf_is_error(cf_make_app(NULL, 0, 0, 0, 640, 480, s_app_options(), NULL))) return true; // Headless CI: no display/GPU.

	s_attr_circle_shape = cf_make_custom_shape(s_attr_circle_sdf_src);
	REQUIRE(s_attr_circle_shape.id);
	s_plain_circle_shape = cf_make_custom_shape(s_circle_sdf_src);
	REQUIRE(s_plain_circle_shape.id);

	// A bad snippet must fail cleanly (compile errors print to stderr -- expected here)
	// and leave the working pipelines and registry intact.
	CF_CustomShape junk = cf_make_custom_shape("this is not glsl");
	REQUIRE(junk.id == 0);

	int w = 640, h = 480;
	CF_Pixel* a = (CF_Pixel*)cf_alloc(w * h * sizeof(CF_Pixel));
	CF_Pixel* b = (CF_Pixel*)cf_alloc(w * h * sizeof(CF_Pixel));
	CF_Pixel* px[2] = { a, b };

	// Attributes reach ShapeParams in both paths (and shapes still render after the
	// failed registration above).
	for (int mode = 0; mode <= 1; ++mode) {
		REQUIRE(s_readback(s_scene_attr_shape, mode, w, h, px[mode]));
		REQUIRE(s_px_near(s_probe(px[mode], w, h, -100), 255, 0, 0, 255, 3)); // Center.
		REQUIRE(s_px_near(s_probe(px[mode], w, h, -80), 255, 0, 0, 255, 3));  // r=20 < 30: inside.
		REQUIRE(s_px_near(s_probe(px[mode], w, h, -140), 0, 0, 0, 0, 0));     // r=40: outside scaled radius, inside base.
	}
	REQUIRE(s_diff_ok(a, b, w * h, "attr-shape tiled-vs-mesh"));

	// Under a translate/rotate/scale camera stack a custom circle must land on the
	// same pixels as the builtin circle.
	REQUIRE(s_readback(s_scene_xform_builtin, 0, w, h, a));
	REQUIRE(s_readback(s_scene_xform_custom, 0, w, h, b));
	REQUIRE(s_diff_ok(a, b, w * h, "xform custom-vs-builtin"));
	REQUIRE(s_readback(s_scene_xform_custom, 1, w, h, a));
	REQUIRE(s_diff_ok(a, b, w * h, "xform custom tiled-vs-mesh"));

	// Opaque-cover occlusion driven by a user sdf: both paths must composite
	// identically, and the covered interior must be exactly the opaque color.
	for (int mode = 0; mode <= 1; ++mode) {
		REQUIRE(s_readback(s_scene_custom_occlusion, mode, w, h, px[mode]));
		REQUIRE(s_px_near(s_probe(px[mode], w, h, -150), 38, 38, 77, 255, 3)); // Only the opaque cover: (0.15, 0.15, 0.3).
	}
	REQUIRE(s_diff_ok(a, b, w * h, "custom-occlusion tiled-vs-mesh"));

	// Invalid handles draw nothing and must not crash.
	REQUIRE(s_readback(s_scene_invalid_handles, 0, w, h, a));
	REQUIRE(s_px_near(s_probe(a, w, h, 0), 0, 0, 0, 0, 0));
	REQUIRE(s_px_near(s_probe(a, w, h, -50), 0, 0, 0, 0, 0));

	cf_free(a);
	cf_free(b);
	cf_destroy_app();
#endif
	return true;
}

// -------------------------------------------------------------------------------------------------
// CSG shape groups: boolean ops over existing shape calls composited as ONE command.
// Covers hard subtract, stroked composite outlines, smooth union bridging, translucent
// single-blend, and equivalence with the same CSG written as a custom shape sdf.

static void s_scene_shape_groups()
{
	// Left: crescent (circle minus circle), filled opaque red.
	cf_draw_push_color(cf_make_color_rgba_f(1, 0, 0, 1));
	cf_draw_shape_group_begin();
	cf_draw_circle_fill2(cf_v2(-180, 0), 60);
	cf_draw_shape_group_op(CF_SHAPE_OP_SUBTRACT, 0);
	cf_draw_circle_fill2(cf_v2(-210, 20), 50);
	cf_draw_shape_group_end();
	cf_draw_pop_color();

	// Center-left: the same crescent as a stroked outline of the composite.
	cf_draw_push_color(cf_make_color_rgba_f(0, 1, 0, 1));
	cf_draw_shape_group_begin();
	cf_draw_circle_fill2(cf_v2(-40, 0), 60);
	cf_draw_shape_group_op(CF_SHAPE_OP_SUBTRACT, 0);
	cf_draw_circle_fill2(cf_v2(-70, 20), 50);
	cf_draw_shape_group_end_stroked(6.0f);
	cf_draw_pop_color();

	// Right: smooth union -- two separated circles bridged by k.
	cf_draw_push_color(cf_make_color_rgba_f(0, 0, 1, 1));
	cf_draw_shape_group_begin();
	cf_draw_circle_fill2(cf_v2(120, 0), 30);
	cf_draw_shape_group_op(CF_SHAPE_OP_UNION, 40.0f);
	cf_draw_circle_fill2(cf_v2(190, 0), 30);
	cf_draw_shape_group_end();
	cf_draw_pop_color();

	// Far right: translucent union must blend exactly once across the overlap.
	cf_draw_push_color(cf_make_color_rgba_f(1, 1, 1, 0.5f));
	cf_draw_shape_group_begin();
	cf_draw_circle_fill2(cf_v2(260, 0), 30);
	cf_draw_circle_fill2(cf_v2(285, 0), 30);
	cf_draw_shape_group_end();
	cf_draw_pop_color();
}

#ifdef CF_RUNTIME_SHADER_COMPILATION
static const char* s_moon_sdf_src = R"(
// params: a = center, b.x = radius, c = bite offset, d.x = bite radius
float sdf(vec2 p, ShapeParams s)
{
	return max(length(p - s.a) - s.b.x, -(length(p - s.a - s.c) - s.d.x));
}
)";
static CF_CustomShape s_moon_shape;

static void s_scene_group_crescent_only()
{
	cf_draw_push_color(cf_make_color_rgba_f(1, 0, 0, 1));
	cf_draw_shape_group_begin();
	cf_draw_circle_fill2(cf_v2(-180, 0), 60);
	cf_draw_shape_group_op(CF_SHAPE_OP_SUBTRACT, 0);
	cf_draw_circle_fill2(cf_v2(-210, 20), 50);
	cf_draw_shape_group_end();
	cf_draw_pop_color();
}

static void s_scene_custom_crescent_only()
{
	// The identical distance field written as a custom shape sdf.
	float params[8] = { -180, 0, 60, 0, -30, 20, 50, 0 };
	cf_draw_push_color(cf_make_color_rgba_f(1, 0, 0, 1));
	cf_draw_custom_shape_fill(s_moon_shape, cf_make_aabb(cf_v2(-260, -70), cf_v2(-110, 70)), params, 8);
	cf_draw_pop_color();
}
#endif

// -------------------------------------------------------------------------------------------------
// Tile-list budget: a batch whose summed tile footprint exceeds the budget must route to
// the instanced path even when tiled is forced -- the bin list buffer would otherwise
// grow unboundedly (footprint x 4 bytes) and the gather goes O(tiles x cmds).

static void s_scene_over_budget()
{
	// 4 near-fullscreen translucent boxes: ~1200 tiles each at 640x480 = ~4.8k entries.
	// (Few layers on purpose: deep translucent stacks accumulate fp32-vs-8bit blending
	// divergence between the two paths, which is not what this test measures.)
	for (int i = 0; i < 4; ++i) {
		cf_draw_push_color(cf_make_color_rgba_f(0.5f, 0.2f * i, 0.1f, 0.4f));
		cf_draw_quad_fill(cf_make_aabb(cf_v2(-310.0f + i * 4.0f, -230.0f), cf_v2(310.0f, 230.0f)), 0);
		cf_draw_pop_color();
	}
}

TEST_CASE(test_draw_tiled_budget_fallback)
{
	if (cf_is_error(cf_make_app(NULL, 0, 0, 0, 640, 480, s_app_options(), NULL))) return true; // Headless CI: no display/GPU.
	if (cf_query_backend() == CF_BACKEND_TYPE_GLES3) { cf_destroy_app(); return true; }

	int w = 640, h = 480;
	CF_Pixel* a = (CF_Pixel*)cf_alloc(w * h * sizeof(CF_Pixel));
	CF_Pixel* b = (CF_Pixel*)cf_alloc(w * h * sizeof(CF_Pixel));

	// Under the default budget the forced-tiled path takes the batch...
	int tiled0, instanced0;
	cf_draw_tiled_stats(&tiled0, &instanced0, NULL);
	REQUIRE(s_readback(s_scene_over_budget, 1, w, h, a));
	int tiled1, instanced1;
	cf_draw_tiled_stats(&tiled1, &instanced1, NULL);
	REQUIRE(tiled1 >= 1);

	// ...and over budget it must fall back to instanced with identical pixels.
	cf_draw_set_tiled_list_budget(1000);
	REQUIRE(s_readback(s_scene_over_budget, 1, w, h, b));
	cf_draw_set_tiled_list_budget(8 * 1024 * 1024);
	int tiled2, instanced2;
	cf_draw_tiled_stats(&tiled2, &instanced2, NULL);
	REQUIRE(tiled2 == 0);
	REQUIRE(instanced2 >= 1);
	REQUIRE(s_diff_ok(a, b, w * h, "budget-fallback tiled-vs-instanced"));

	cf_free(a);
	cf_free(b);
	cf_destroy_app();
	return true;
}

TEST_CASE(test_draw_shape_groups)
{
	if (cf_is_error(cf_make_app(NULL, 0, 0, 0, 640, 480, s_app_options(), NULL))) return true; // Headless CI: no display/GPU.

	int w = 640, h = 480;
	CF_Pixel* a = (CF_Pixel*)cf_alloc(w * h * sizeof(CF_Pixel));
	CF_Pixel* b = (CF_Pixel*)cf_alloc(w * h * sizeof(CF_Pixel));
	CF_Pixel* px[2] = { a, b };
	for (int mode = 0; mode <= 1; ++mode) {
		REQUIRE(s_readback(s_scene_shape_groups, mode, w, h, px[mode]));
		// Crescent: body away from the bite is filled, inside the bite is carved out.
		REQUIRE(s_px_near(s_probe(px[mode], w, h, -150), 255, 0, 0, 255, 3));
		REQUIRE(s_px_near(s_probe(px[mode], w, h, -200), 0, 0, 0, 0, 0));
		// Stroked composite: on the body's boundary (far from the bite) the outline lands;
		// the interior stays hollow.
		REQUIRE(s_px_near(s_probe(px[mode], w, h, 20), 0, 255, 0, 255, 3));
		REQUIRE(s_px_near(s_probe(px[mode], w, h, -10), 0, 0, 0, 0, 0));
		// Smooth union: the midpoint between the circles is bridged by k=40 (a hard union
		// leaves it empty -- both circles are 5 units away).
		REQUIRE(s_px_near(s_probe(px[mode], w, h, 155), 0, 0, 255, 255, 3));
		// Translucent union: overlap and single-covered regions blend identically (one command).
		CF_Pixel overlap = s_probe(px[mode], w, h, 272);
		CF_Pixel single = s_probe(px[mode], w, h, 240);
		REQUIRE(s_px_near(overlap, 128, 128, 128, 128, 3));
		REQUIRE(s_px_near(single, overlap.colors.r, overlap.colors.g, overlap.colors.b, overlap.colors.a, 2));
	}
	REQUIRE(s_diff_ok(a, b, w * h, "shape-groups tiled-vs-mesh"));

#ifdef CF_RUNTIME_SHADER_COMPILATION
	// A shape group and a hand-written custom sdf of the same CSG must produce the same
	// distance field, hence the same pixels.
	s_moon_shape = cf_make_custom_shape(s_moon_sdf_src);
	REQUIRE(s_moon_shape.id);
	REQUIRE(s_readback(s_scene_group_crescent_only, 0, w, h, a));
	REQUIRE(s_readback(s_scene_custom_crescent_only, 0, w, h, b));
	REQUIRE(s_diff_ok(a, b, w * h, "group-vs-custom-sdf crescent"));
#endif

	cf_free(a);
	cf_free(b);
	cf_destroy_app();
	return true;
}

// -------------------------------------------------------------------------------------------------
// Curve text (cf_push_text_curves): glyphs rendered per-pixel from Bezier outlines.
// Topology probes are font-agnostic: a scanline through an 'O' ring's center crosses
// 2 ink runs filled and 4 stroked (both outline edges), and winding must leave the
// counter (hole) empty.

static void s_scene_curve_O_fill()
{
	cf_push_text_curves(true);
	cf_push_font_size(220);
	cf_draw_push_color(cf_color_white());
	cf_draw_text("O", cf_v2(-80, 110), -1);
	cf_draw_pop_color();
	cf_pop_font_size();
	cf_pop_text_curves();
}

static void s_scene_curve_O_stroke()
{
	cf_push_text_curves(true);
	cf_push_text_stroke(3.0f);
	cf_push_font_size(220);
	cf_draw_push_color(cf_color_white());
	cf_draw_text("O", cf_v2(-80, 110), -1);
	cf_draw_pop_color();
	cf_pop_font_size();
	cf_pop_text_stroke();
	cf_pop_text_curves();
}

static void s_scene_atlas_O_fill()
{
	cf_push_text_curves(false); // Force the rasterized atlas path (curves are the default).
	cf_push_font_size(220);
	cf_draw_push_color(cf_color_white());
	cf_draw_text("O", cf_v2(-80, 110), -1);
	cf_draw_pop_color();
	cf_pop_font_size();
	cf_pop_text_curves();
}

// Mixed scene for the tiled-vs-instanced pixel diff: filled + stroked + small curve text.
static void s_scene_curve_text_mixed()
{
	s_scene_curve_O_fill();
	cf_push_text_curves(true);
	cf_push_text_stroke(2.0f);
	cf_push_font_size(64);
	cf_draw_push_color(cf_make_color_rgba_f(0.3f, 0.8f, 1.0f, 1.0f));
	cf_draw_text("S", cf_v2(120, 60), -1);
	cf_draw_pop_color();
	cf_pop_font_size();
	cf_pop_text_stroke();
	cf_push_font_size(18);
	cf_draw_push_color(cf_color_white());
	cf_draw_text("small curve text", cf_v2(60, -80), -1);
	cf_draw_pop_color();
	cf_pop_font_size();
	cf_pop_text_curves();
}

// Ink bbox (alpha > 128) and the count of ink runs along the bbox's center row.
static void s_ink_stats(const CF_Pixel* px, int w, int h, int* ink_out, int* runs_out, int* bbox_w_out)
{
	int x0 = w, y0 = h, x1 = -1, y1 = -1, ink = 0;
	for (int y = 0; y < h; ++y) {
		for (int x = 0; x < w; ++x) {
			if (px[y * w + x].colors.a > 128) {
				++ink;
				x0 = cf_min(x0, x); x1 = cf_max(x1, x);
				y0 = cf_min(y0, y); y1 = cf_max(y1, y);
			}
		}
	}
	int runs = 0;
	if (y1 >= 0) {
		int yc = (y0 + y1) / 2;
		bool in = false;
		for (int x = 0; x < w; ++x) {
			bool on = px[yc * w + x].colors.a > 128;
			if (on && !in) ++runs;
			in = on;
		}
	}
	*ink_out = ink;
	*runs_out = runs;
	*bbox_w_out = y1 >= 0 ? x1 - x0 + 1 : 0;
}

// -------------------------------------------------------------------------------------------------
// Per-command blend modes (cf_draw_push_blend): exact fixed-function math on both
// paths, paint order preserved across mode changes.

static void s_scene_blend_modes()
{
	// Opaque 50% gray base strip.
	cf_draw_push_color(cf_make_color_rgba_f(0.5f, 0.5f, 0.5f, 1.0f));
	cf_draw_quad_fill(cf_make_aabb(cf_v2(-130, -40), cf_v2(80, 40)), 0);
	cf_draw_pop_color();

	// Additive red over the base: 0.5 + 0.25 = 0.75 red.
	cf_draw_push_blend(CF_DRAW_BLEND_ADD);
	cf_draw_push_color(cf_make_color_rgba_f(0.5f, 0, 0, 0.5f));
	cf_draw_quad_fill(cf_make_aabb(cf_v2(-70, -30), cf_v2(-30, 30)), 0);
	cf_draw_pop_color();
	cf_draw_pop_blend();

	// Multiply 50% gray over the base: half brightness.
	cf_draw_push_blend(CF_DRAW_BLEND_MULTIPLY);
	cf_draw_push_color(cf_make_color_rgba_f(0.5f, 0.5f, 0.5f, 1.0f));
	cf_draw_quad_fill(cf_make_aabb(cf_v2(-20, -30), cf_v2(20, 30)), 0);
	cf_draw_pop_color();
	cf_draw_pop_blend();

	// Screen 50% gray over the base: 0.5 + 0.5 * (1 - 0.5) = 0.75.
	cf_draw_push_blend(CF_DRAW_BLEND_SCREEN);
	cf_draw_push_color(cf_make_color_rgba_f(0.5f, 0.5f, 0.5f, 1.0f));
	cf_draw_quad_fill(cf_make_aabb(cf_v2(30, -30), cf_v2(70, 30)), 0);
	cf_draw_pop_color();
	cf_draw_pop_blend();

	// Additive over the transparent canvas: rgb lands, canvas alpha stays 0.
	cf_draw_push_blend(CF_DRAW_BLEND_ADD);
	cf_draw_push_color(cf_make_color_rgba_f(0.5f, 0, 0, 0.5f));
	cf_draw_quad_fill(cf_make_aabb(cf_v2(90, -30), cf_v2(130, 30)), 0);
	cf_draw_pop_color();
	cf_draw_pop_blend();

	// Paint order across mode changes: a normal opaque green box drawn after an
	// additive shape at the same spot must fully cover it.
	cf_draw_push_blend(CF_DRAW_BLEND_ADD);
	cf_draw_push_color(cf_make_color_rgba_f(0.9f, 0.9f, 0, 1.0f));
	cf_draw_circle_fill2(cf_v2(160, 0), 25);
	cf_draw_pop_color();
	cf_draw_pop_blend();
	cf_draw_push_color(cf_make_color_rgba_f(0, 1.0f, 0, 1.0f));
	cf_draw_quad_fill(cf_make_aabb(cf_v2(140, -20), cf_v2(180, 20)), 0);
	cf_draw_pop_color();
}

// -------------------------------------------------------------------------------------------------
// Vector paths (cf_draw_path_begin/end): nonzero winding fills with holes, strokes, and
// multi-row curve blocks, on both renderer paths.

static CF_DrawPath s_donut_path;
static CF_DrawPath s_blob_path;

static void s_scene_path_fill()
{
	cf_draw_push_color(cf_color_white());
	cf_draw_path_fill(s_donut_path);
	cf_draw_pop_color();
}

static void s_scene_path_stroke()
{
	cf_draw_push_color(cf_color_white());
	cf_draw_path(s_donut_path, 4.0f);
	cf_draw_pop_color();
}

static void s_scene_path_mixed()
{
	cf_draw_push_color(cf_make_color_rgba_f(0.9f, 0.5f, 0.2f, 1.0f));
	cf_draw_path_fill(s_donut_path);
	cf_draw_pop_color();
	cf_draw_push_color(cf_make_color_rgba_f(0.2f, 0.7f, 0.9f, 0.8f));
	cf_draw_push();
	cf_draw_translate(150, 0);
	cf_draw_path_fill(s_blob_path);
	cf_draw_path(s_blob_path, 2.0f);
	cf_draw_pop();
	cf_draw_pop_color();
}

TEST_CASE(test_draw_paths)
{
	if (cf_is_error(cf_make_app(NULL, 0, 0, 0, 640, 480, s_app_options(), NULL))) return true; // Headless CI: no display/GPU.

	// Donut: outer CCW square, inner CW square -- nonzero winding leaves the hole empty.
	cf_draw_path_begin();
	cf_draw_path_move_to(cf_v2(-100, -100));
	cf_draw_path_line_to(cf_v2(100, -100));
	cf_draw_path_line_to(cf_v2(100, 100));
	cf_draw_path_line_to(cf_v2(-100, 100));
	cf_draw_path_close();
	cf_draw_path_move_to(cf_v2(-40, -40));
	cf_draw_path_line_to(cf_v2(-40, 40));
	cf_draw_path_line_to(cf_v2(40, 40));
	cf_draw_path_line_to(cf_v2(40, -40));
	cf_draw_path_close();
	s_donut_path = cf_draw_path_end();
	REQUIRE(s_donut_path.id);

	// Curvy blob: quads and cubics.
	cf_draw_path_begin();
	cf_draw_path_move_to(cf_v2(0, -60));
	cf_draw_path_quad_to(cf_v2(70, -60), cf_v2(70, 0));
	cf_draw_path_cubic_to(cf_v2(70, 50), cf_v2(20, 70), cf_v2(0, 60));
	cf_draw_path_quad_to(cf_v2(-70, 40), cf_v2(-60, -20));
	cf_draw_path_close();
	s_blob_path = cf_draw_path_end();
	REQUIRE(s_blob_path.id);

	int w = 640, h = 480;
	CF_Pixel* a = (CF_Pixel*)cf_alloc(w * h * sizeof(CF_Pixel));
	CF_Pixel* b = (CF_Pixel*)cf_alloc(w * h * sizeof(CF_Pixel));
	int ink, runs, bbox_w;

	// Filled donut: ring solid, hole empty (2 ink runs on the center scanline).
	REQUIRE(s_readback(s_scene_path_fill, 0, w, h, a));
	s_ink_stats(a, w, h, &ink, &runs, &bbox_w);
	REQUIRE(ink > 0);
	REQUIRE(runs == 2);
	REQUIRE(bbox_w >= 198 && bbox_w <= 204);

	// Stroked donut: both edges of both squares (4 runs).
	REQUIRE(s_readback(s_scene_path_stroke, 0, w, h, a));
	s_ink_stats(a, w, h, &ink, &runs, &bbox_w);
	REQUIRE(ink > 0);
	REQUIRE(runs == 4);

	// Tiled walk matches the instanced path.
	if (cf_query_backend() != CF_BACKEND_TYPE_GLES3) {
		REQUIRE(s_readback(s_scene_path_mixed, 0, w, h, a));
		REQUIRE(s_readback(s_scene_path_mixed, 1, w, h, b));
		REQUIRE(s_diff_ok(a, b, w * h, "paths tiled-vs-mesh"));
	}

	cf_destroy_path(s_donut_path);
	cf_destroy_path(s_blob_path);
	cf_free(a);
	cf_free(b);
	cf_destroy_app();
	return true;
}

TEST_CASE(test_draw_blend_modes)
{
	if (cf_is_error(cf_make_app(NULL, 0, 0, 0, 640, 480, s_app_options(), NULL))) return true; // Headless CI: no display/GPU.

	int w = 640, h = 480;
	CF_Pixel* a = (CF_Pixel*)cf_alloc(w * h * sizeof(CF_Pixel));
	CF_Pixel* b = (CF_Pixel*)cf_alloc(w * h * sizeof(CF_Pixel));

	int mode_count = cf_query_backend() == CF_BACKEND_TYPE_GLES3 ? 1 : 2;
	for (int mode = 0; mode < mode_count; ++mode) {
		CF_Pixel* px = mode == 0 ? a : b;
		REQUIRE(s_readback(s_scene_blend_modes, mode, w, h, px));
		REQUIRE(s_px_near(s_probe(px, w, h, -100), 128, 128, 128, 255, 3)); // Base only.
		REQUIRE(s_px_near(s_probe(px, w, h, -50), 191, 128, 128, 255, 3));  // Additive red.
		REQUIRE(s_px_near(s_probe(px, w, h, 0), 64, 64, 64, 255, 3));       // Multiply.
		REQUIRE(s_px_near(s_probe(px, w, h, 50), 191, 191, 191, 255, 3));   // Screen.
		REQUIRE(s_px_near(s_probe(px, w, h, 110), 64, 0, 0, 0, 3));         // Additive over transparent.
		REQUIRE(s_px_near(s_probe(px, w, h, 160), 0, 255, 0, 255, 3));      // Normal covers additive (order).
	}
	if (mode_count == 2) {
		REQUIRE(s_diff_ok(a, b, w * h, "blend-modes tiled-vs-mesh"));
	}

	cf_free(a);
	cf_free(b);
	cf_destroy_app();
	return true;
}

// -------------------------------------------------------------------------------------------------
// Retained draw lists: replay must pixel-match immediate drawing, compose with the
// current transform, and support multiple replays per frame.

static CF_DrawList s_test_list;

static void s_scene_list_content()
{
	cf_draw_push_color(cf_make_color_rgba_f(1, 0, 0, 1));
	cf_draw_circle_fill2(cf_v2(-60, 0), 30);
	cf_draw_pop_color();
	cf_draw_push_color(cf_make_color_rgba_f(0.2f, 0.4f, 0.9f, 0.6f));
	cf_draw_quad_fill(cf_make_aabb(cf_v2(-20, -25), cf_v2(40, 25)), 4.0f);
	cf_draw_pop_color();
	// Text rides the atlas-entry flow: replays must re-resolve uvs against the live atlas.
	cf_push_font_size(26);
	cf_draw_push_color(cf_color_white());
	cf_draw_text("list", cf_v2(60, 13), -1);
	cf_draw_pop_color();
	cf_pop_font_size();
}

static void s_scene_list_immediate() { s_scene_list_content(); }
static void s_scene_list_replay() { cf_draw_list(s_test_list); }

// A starfield of small circles: subpixel once the camera zooms out, so nearly
// all of each star's energy lives in the AA fringe. Replay must preserve that
// fringe under a replay camera scale (regression: the recorded coverage quads
// kept their identity-camera AA inflation, clipping the widened fringe --
// dim, popping stars).
static void s_star_field()
{
	CF_Rnd rnd = cf_rnd_seed(7);
	for (int i = 0; i < 12; ++i) {
		for (int j = 0; j < 8; ++j) {
			float x = -2900.0f + i * 520.0f + cf_rnd_range_float(&rnd, -100, 100);
			float y = -1850.0f + j * 470.0f + cf_rnd_range_float(&rnd, -100, 100);
			cf_draw_push_color(cf_make_color_rgba_f(1, 1, 1, 1));
			cf_draw_circle_fill2(cf_v2(x, y), cf_rnd_range_float(&rnd, 2, 4));
			cf_draw_pop_color();
		}
	}
}

static CF_DrawList s_star_list;
static void s_scene_stars_immediate()
{
	cf_draw_push();
	cf_draw_scale(0.06f, 0.06f);
	s_star_field();
	cf_draw_pop();
}
static void s_scene_stars_replay()
{
	cf_draw_push();
	cf_draw_scale(0.06f, 0.06f);
	cf_draw_list(s_star_list);
	cf_draw_pop();
}

// Total luminance, for comparing star energy between render paths.
static double s_ink_sum(const CF_Pixel* px, int total)
{
	double sum = 0;
	for (int i = 0; i < total; ++i) {
		sum += px[i].colors.r + px[i].colors.g + px[i].colors.b;
	}
	return sum;
}
static void s_scene_list_replay_multi()
{
	cf_draw_list(s_test_list);
	cf_draw_push();
	cf_draw_translate(160, 0);
	cf_draw_list(s_test_list);
	cf_draw_pop();
	// Paint order after a replay: this box must cover the first replay's circle.
	cf_draw_push_color(cf_make_color_rgba_f(0, 1, 0, 1));
	cf_draw_quad_fill(cf_make_aabb(cf_v2(-75, -10), cf_v2(-45, 10)), 0);
	cf_draw_pop_color();
}

TEST_CASE(test_draw_lists)
{
	if (cf_is_error(cf_make_app(NULL, 0, 0, 0, 640, 480, s_app_options(), NULL))) return true; // Headless CI: no display/GPU.

	s_test_list = cf_make_draw_list();
	cf_draw_list_begin(s_test_list);
	s_scene_list_content();
	cf_draw_list_end();

	int w = 640, h = 480;
	CF_Pixel* a = (CF_Pixel*)cf_alloc(w * h * sizeof(CF_Pixel));
	CF_Pixel* b = (CF_Pixel*)cf_alloc(w * h * sizeof(CF_Pixel));

	// Replay must match drawing the same content immediately.
	REQUIRE(s_readback(s_scene_list_immediate, 0, w, h, a));
	REQUIRE(s_readback(s_scene_list_replay, 0, w, h, b));
	REQUIRE(s_diff_ok(a, b, w * h, "list-vs-immediate"));

	// Two replays (one transformed) plus paint order across a replay boundary.
	REQUIRE(s_readback(s_scene_list_replay_multi, 0, w, h, b));
	REQUIRE(s_px_near(s_probe(b, w, h, -60), 0, 255, 0, 255, 3));   // Green box over replayed circle.
	REQUIRE(s_px_near(s_probe(b, w, h, 100), 255, 0, 0, 255, 3));   // Second replay's circle at -60+160.
	REQUIRE(s_px_near(s_probe(b, w, h, 10), 31, 61, 138, 153, 6));  // First replay's translucent quad.

	// Subpixel stars under a zoomed-out replay camera must keep their AA-fringe
	// energy: total luminance of replayed stars must match immediate drawing, on
	// both the instanced and tiled paths (the tiled walk rasterizes from the
	// recorded coverage quads, where a stale AA inflation clips the fringe).
	s_star_list = cf_make_draw_list();
	cf_draw_list_begin(s_star_list);
	s_star_field();
	cf_draw_list_end();
	for (int mode = 0; mode <= 1; ++mode) {
		REQUIRE(s_readback(s_scene_stars_immediate, mode, w, h, a));
		REQUIRE(s_readback(s_scene_stars_replay, mode, w, h, b));
		double ink_immediate = s_ink_sum(a, w * h);
		double ink_replay = s_ink_sum(b, w * h);
		REQUIRE(ink_immediate > 0);
		if (!(ink_replay > ink_immediate * 0.85 && ink_replay < ink_immediate * 1.15)) {
			printf("star ink mismatch (mode %d): immediate=%f replay=%f\n", mode, ink_immediate, ink_replay);
			REQUIRE(false);
		}
	}
	cf_destroy_draw_list(s_star_list);

	cf_destroy_draw_list(s_test_list);
	cf_free(a);
	cf_free(b);
	cf_destroy_app();
	return true;
}

TEST_CASE(test_draw_text_curves)
{
	if (cf_is_error(cf_make_app(NULL, 0, 0, 0, 640, 480, s_app_options(), NULL))) return true; // Headless CI: no display/GPU.

	int w = 640, h = 480;
	CF_Pixel* a = (CF_Pixel*)cf_alloc(w * h * sizeof(CF_Pixel));
	CF_Pixel* b = (CF_Pixel*)cf_alloc(w * h * sizeof(CF_Pixel));
	int ink, runs, bbox_w;

	// Filled ring: solid band, empty counter (nonzero winding handles the hole).
	REQUIRE(s_readback(s_scene_curve_O_fill, 0, w, h, a));
	s_ink_stats(a, w, h, &ink, &runs, &bbox_w);
	REQUIRE(ink > 0);
	REQUIRE(runs == 2);

	// Same glyph through the atlas path: identical layout, so ink coverage and bbox
	// must land close (paths differ only in AA character).
	REQUIRE(s_readback(s_scene_atlas_O_fill, 0, w, h, b));
	int atlas_ink, atlas_runs, atlas_bbox_w;
	s_ink_stats(b, w, h, &atlas_ink, &atlas_runs, &atlas_bbox_w);
	auto absi = [](int v) { return v < 0 ? -v : v; };
	REQUIRE(atlas_runs == 2);
	REQUIRE(absi(ink - atlas_ink) < atlas_ink / 8);
	REQUIRE(absi(bbox_w - atlas_bbox_w) <= 4);

	// Stroked ring: both edges of the band stroke, band interior stays empty.
	REQUIRE(s_readback(s_scene_curve_O_stroke, 0, w, h, a));
	s_ink_stats(a, w, h, &ink, &runs, &bbox_w);
	REQUIRE(ink > 0);
	REQUIRE(runs == 4);

	// Tiled walk must match the instanced path pixel-for-pixel (within blend precision).
	if (cf_query_backend() != CF_BACKEND_TYPE_GLES3) {
		REQUIRE(cf_draw_tiled_available());
		REQUIRE(s_readback(s_scene_curve_text_mixed, 0, w, h, a));
		REQUIRE(s_readback(s_scene_curve_text_mixed, 1, w, h, b));
		REQUIRE(s_diff_ok(a, b, w * h, "curve-text tiled-vs-mesh"));
	}

	cf_free(a);
	cf_free(b);
	cf_destroy_app();
	return true;
}

TEST_SUITE(test_draw_tiled)
{
	// CF_TEST_ONLY=<case name> runs a single case, useful when isolating one scene.
	// (Cycling GL contexts across app lifetimes used to break the third context in a
	// process -- a stale render-state diff cache eliding glViewport on the fresh
	// context -- fixed in cf_gles_attach, so the full suite runs fine on GLES now.)
	const char* only = getenv("CF_TEST_ONLY");
#define RUN_TEST_CASE_IF(t) do { if (!only || CF_STRCMP(only, #t) == 0) { RUN_TEST_CASE(t); } } while (0)
	RUN_TEST_CASE_IF(test_tile_range_basics);
	RUN_TEST_CASE_IF(test_tiled_matches_mesh);
	RUN_TEST_CASE_IF(test_draw_custom_shader);
	RUN_TEST_CASE_IF(test_draw_multi_atlas_interleave);
	RUN_TEST_CASE_IF(test_draw_render_states);
	RUN_TEST_CASE_IF(test_draw_layers);
	RUN_TEST_CASE_IF(test_draw_polyline_segments);
	RUN_TEST_CASE_IF(test_draw_arrow_no_overdraw);
	RUN_TEST_CASE_IF(test_draw_custom_shapes);
	RUN_TEST_CASE_IF(test_draw_custom_shapes_advanced);
	RUN_TEST_CASE_IF(test_draw_shape_groups);
	RUN_TEST_CASE_IF(test_draw_tiled_budget_fallback);
	RUN_TEST_CASE_IF(test_draw_text_curves);
	RUN_TEST_CASE_IF(test_draw_blend_modes);
	RUN_TEST_CASE_IF(test_draw_paths);
	RUN_TEST_CASE_IF(test_draw_lists);
}
