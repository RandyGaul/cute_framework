/*
	Cute Framework
	Copyright (C) 2026 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

#include "test_harness.h"
#include "test_app_shared.h"

#include <cute.h>
#include <internal/cute_app_internal.h>
#include <SDL3/SDL.h>

using namespace Cute;

// The pixel scale (physical pixels per logical point) is a user-controlled value: it starts
// at the display scale the OS wants and afterwards changes only via cf_app_set_pixel_scale.
// Nothing -- not window resizes, not display density changes -- resizes the app canvas or
// touches the default 2d projection behind the user's back. CI and most dev machines run at
// density 1.0 where points and pixels agree, so these tests set a non-unity scale explicitly
// to make the HiDPI contract testable everywhere.

#define LOGICAL_W 320
#define LOGICAL_H 240

static bool s_px_near(CF_Pixel p, int r, int g, int b, int a, int tol)
{
	bool ok = cf_abs((int)p.colors.r - r) <= tol && cf_abs((int)p.colors.g - g) <= tol && cf_abs((int)p.colors.b - b) <= tol && cf_abs((int)p.colors.a - a) <= tol;
	if (!ok) printf("pixel (%d %d %d %d) expected (%d %d %d %d) +/-%d\n", p.colors.r, p.colors.g, p.colors.b, p.colors.a, r, g, b, a, tol);
	return ok;
}

// A failed REQUIRE returns out of the test case early; RAII keeps a modified pixel scale or
// canvas size from leaking into whichever test runs next (see also test_app_shared's sweep).
struct HidpiGuard
{
	~HidpiGuard()
	{
		test_destroy_app();
	}
};

static bool s_readback_canvas(CF_Canvas canvas, int w, int h, CF_Pixel* out)
{
	CF_Readback rb = cf_canvas_readback(canvas);
	REQUIRE(rb.id);
	while (!cf_readback_ready(rb)) {}
	int size = w * h * (int)sizeof(CF_Pixel);
	if (cf_readback_size(rb) != size) {
		printf("readback size mismatch: got %d expected %d (w=%d h=%d) app_canvas=%dx%d app_window=%dx%d pixel_scale=%f\n",
			cf_readback_size(rb), size, w, h,
			cf_app_get_canvas_width(), cf_app_get_canvas_height(),
			cf_app_get_width(), cf_app_get_height(), (double)cf_app_get_pixel_scale());
	}
	REQUIRE(cf_readback_size(rb) == size);
	cf_readback_data(rb, out, size);
	cf_destroy_readback(rb);
	return true;
}

// The manual 2x recipe in its packaged form: scale for AA/glyph density, canvas for
// resolution, projection rebuilt in logical points (the same value startup chose).
static void s_apply_2x()
{
	cf_app_update_display(2.0f);
}

// cf_app_set_pixel_scale changes only the scale value -- the canvas and window keep their
// sizes. Rendering at the new density is an explicit second step (cf_app_set_canvas_size).
TEST_CASE(test_hidpi_set_pixel_scale_is_value_only)
{
	if (!test_make_app(LOGICAL_W, LOGICAL_H)) return true; // Headless CI: no display/GPU.
	HidpiGuard guard;

	// 3.0f can't collide with any real display density, so this is a guaranteed change
	// even on a machine whose display scale is already 2.0.
	int canvas_w = cf_app_get_canvas_width();
	int canvas_h = cf_app_get_canvas_height();
	cf_app_set_pixel_scale(3.0f);
	REQUIRE(cf_app_get_pixel_scale() == 3.0f);
	REQUIRE(cf_app_get_canvas_width() == canvas_w);
	REQUIRE(cf_app_get_canvas_height() == canvas_h);
	REQUIRE(cf_app_get_width() == LOGICAL_W);
	REQUIRE(cf_app_get_height() == LOGICAL_H);

	// Zero/negative values are ignored, not applied.
	cf_app_set_pixel_scale(0);
	REQUIRE(cf_app_get_pixel_scale() == 3.0f);
	cf_app_set_pixel_scale(-1.0f);
	REQUIRE(cf_app_get_pixel_scale() == 3.0f);
	return true;
}

// NO_HIGH_DPI only pins the INITIAL scale at 1.0 (the window gets a 1x backbuffer, so the
// display scale is 1.0 too). The value stays user-controllable afterwards.
TEST_CASE(test_hidpi_no_high_dpi_initial_scale)
{
	if (!test_make_app(LOGICAL_W, LOGICAL_H, CF_APP_OPTIONS_NO_HIGH_DPI_BIT)) return true; // Headless CI: no display/GPU.
	HidpiGuard guard;

	REQUIRE(cf_app_get_pixel_scale() == 1.0f);
	REQUIRE(cf_app_get_display_scale() == 1.0f);
	REQUIRE(cf_app_get_canvas_width() == LOGICAL_W);
	REQUIRE(cf_app_get_canvas_height() == LOGICAL_H);
	return true;
}

// NO_GFX apps have no canvas; setting a scale is a plain value change and must not touch a
// NULL backend.
TEST_CASE(test_hidpi_set_scale_safe_without_gfx)
{
	if (!test_make_app(LOGICAL_W, LOGICAL_H, CF_APP_OPTIONS_NO_GFX_BIT)) return true;
	HidpiGuard guard;

	cf_app_set_pixel_scale(2.0f);
	REQUIRE(cf_app_get_pixel_scale() == 2.0f);

	// The packaged helper skips its canvas/projection half without gfx.
	cf_app_update_display(3.0f);
	REQUIRE(cf_app_get_pixel_scale() == 3.0f);
	return true;
}

// Startup is the one automatic step: canvas at logical size times the display scale, and
// the applied pixel scale starts equal to it.
TEST_CASE(test_hidpi_startup_defaults)
{
	if (!test_make_app(LOGICAL_W, LOGICAL_H)) return true; // Headless CI: no display/GPU.
	HidpiGuard guard;

	float display_scale = cf_app_get_display_scale();
	REQUIRE(display_scale > 0);
	REQUIRE(cf_app_get_pixel_scale() == display_scale);
	REQUIRE(cf_app_get_canvas_width() == (int)CF_ROUNDF(LOGICAL_W * display_scale));
	REQUIRE(cf_app_get_canvas_height() == (int)CF_ROUNDF(LOGICAL_H * display_scale));
	return true;
}

// cf_app_update_display is the packaged recipe: one call sets the scale AND resizes the
// canvas to window * scale (the projection half of its contract is readback-verified by the
// tests below, which all go through s_apply_2x). Invalid scales are ignored wholesale.
TEST_CASE(test_hidpi_update_display_helper)
{
	if (!test_make_app(LOGICAL_W, LOGICAL_H)) return true; // Headless CI: no display/GPU.
	HidpiGuard guard;

	cf_app_update_display(2.0f);
	REQUIRE(cf_app_get_pixel_scale() == 2.0f);
	REQUIRE(cf_app_get_canvas_width() == LOGICAL_W * 2);
	REQUIRE(cf_app_get_canvas_height() == LOGICAL_H * 2);
	REQUIRE(cf_app_get_width() == LOGICAL_W);
	REQUIRE(cf_app_get_height() == LOGICAL_H);

	// An invalid scale must not half-apply (no canvas resize either).
	cf_app_update_display(0);
	REQUIRE(cf_app_get_pixel_scale() == 2.0f);
	REQUIRE(cf_app_get_canvas_width() == LOGICAL_W * 2);
	return true;
}

// The default 2d projection spans logical points, so world coordinates land on the same
// spot of a 1:1 user canvas no matter the pixel scale. If the projection wrongly tracked
// canvas pixels, everything would shrink toward the center and the probe read background.
TEST_CASE(test_hidpi_default_projection_is_points)
{
	if (!test_make_app(LOGICAL_W, LOGICAL_H)) return true; // Headless CI: no display/GPU.
	HidpiGuard guard;

	s_apply_2x();

	int w = LOGICAL_W, h = LOGICAL_H;
	CF_Canvas canvas = cf_make_canvas(cf_canvas_defaults(w, h));
	CF_Pixel* px = (CF_Pixel*)cf_alloc(w * h * (int)sizeof(CF_Pixel));

	// A bar left of center, spanning y=0 so the probe row is insensitive to readback
	// row order: world x in [-140, -60].
	cf_draw_push_color(cf_make_color_rgb_f(1.0f, 0, 0));
	cf_draw_quad_fill(cf_make_aabb(cf_v2(-140, -20), cf_v2(-60, 20)), 0);
	cf_draw_pop_color();
	cf_render_to(canvas, true);
	cf_app_draw_onto_screen(false);

	REQUIRE(s_readback_canvas(canvas, w, h, px));
	// World (-100, 0) is pixel column w/2 - 100 when one world unit is one point.
	REQUIRE(s_px_near(px[(h / 2) * w + (w / 2 - 100)], 255, 0, 0, 255, 3));
	// Just outside the bar: untouched.
	REQUIRE(s_px_near(px[(h / 2) * w + (w / 2 - 150)], 0, 0, 0, 0, 0));

	cf_free(px);
	cf_destroy_canvas(canvas);
	return true;
}

// Through the real present path: a quad spanning the exact logical extent must cover every
// pixel of the (2x larger) app canvas.
TEST_CASE(test_hidpi_full_extent_covers_app_canvas)
{
	if (!test_make_app(LOGICAL_W, LOGICAL_H)) return true; // Headless CI: no display/GPU.
	HidpiGuard guard;

	s_apply_2x();

	int w = cf_app_get_canvas_width();
	int h = cf_app_get_canvas_height();
	REQUIRE(w == LOGICAL_W * 2 && h == LOGICAL_H * 2);
	CF_Pixel* px = (CF_Pixel*)cf_alloc(w * h * (int)sizeof(CF_Pixel));

	cf_draw_push_color(cf_make_color_rgb_f(1.0f, 0, 0));
	cf_draw_quad_fill(cf_make_aabb(cf_v2(-LOGICAL_W / 2.0f, -LOGICAL_H / 2.0f), cf_v2(LOGICAL_W / 2.0f, LOGICAL_H / 2.0f)), 0);
	cf_draw_pop_color();
	cf_app_draw_onto_screen(true); // Renders the remaining draw commands onto the (cleared) app canvas.

	REQUIRE(s_readback_canvas(cf_app_get_canvas(), w, h, px));
	// Probe 5px inside each corner (clear of the AA band) plus the center.
	REQUIRE(s_px_near(px[5 * w + 5], 255, 0, 0, 255, 3));
	REQUIRE(s_px_near(px[5 * w + (w - 6)], 255, 0, 0, 255, 3));
	REQUIRE(s_px_near(px[(h - 6) * w + 5], 255, 0, 0, 255, 3));
	REQUIRE(s_px_near(px[(h - 6) * w + (w - 6)], 255, 0, 0, 255, 3));
	REQUIRE(s_px_near(px[(h / 2) * w + (w / 2)], 255, 0, 0, 255, 3));

	cf_free(px);
	return true;
}

// Stickiness is now a guarantee: neither cf_app_set_canvas_size nor cf_app_set_msaa touches
// the projection. A custom cf_draw_projection survives every canvas recreation.
TEST_CASE(test_hidpi_projection_sticky_across_canvas_recreate)
{
	if (!test_make_app(LOGICAL_W, LOGICAL_H)) return true; // Headless CI: no display/GPU.
	HidpiGuard guard;

	// A custom projection spanning HALF the logical size: world content renders at 2x the
	// scale of the default projection, so probes can tell the two apart.
	cf_draw_projection(cf_ortho_2d(0, 0, LOGICAL_W / 2.0f, LOGICAL_H / 2.0f));

	// Both recreation paths, back to back.
	cf_app_set_canvas_size(LOGICAL_W, LOGICAL_H);
	cf_app_set_msaa(1);

	int w = LOGICAL_W, h = LOGICAL_H;
	CF_Pixel* px = (CF_Pixel*)cf_alloc(w * h * (int)sizeof(CF_Pixel));

	// An 80x60-world quad: under the custom projection it covers half the canvas each way;
	// under the (stomped) default it would cover only a quarter as much area.
	cf_draw_push_color(cf_make_color_rgb_f(1.0f, 0, 0));
	cf_draw_quad_fill(cf_make_aabb(cf_v2(-40, -30), cf_v2(40, 30)), 0);
	cf_draw_pop_color();
	cf_app_draw_onto_screen(true);

	REQUIRE(s_readback_canvas(cf_app_get_canvas(), w, h, px));
	// Inside the doubled quad but outside the default-projection footprint.
	REQUIRE(s_px_near(px[(h / 2) * w + (w / 2 + 70)], 255, 0, 0, 255, 3));
	REQUIRE(s_px_near(px[(h / 2 + 50) * w + (w / 2)], 255, 0, 0, 255, 3));
	// Outside the doubled quad: background.
	REQUIRE(s_px_near(px[(h / 2) * w + (w / 2 + 130)], 0, 0, 0, 0, 0));

	cf_free(px);
	// Restore the default projection for whichever test shares the app next.
	cf_draw_projection(cf_ortho_2d(0, 0, (float)LOGICAL_W, (float)LOGICAL_H));
	return true;
}

// The manual resize recipe from the docs/sample, in one frame: set_size, then canvas +
// projection by hand, then draw -- the very same frame must render correctly.
TEST_CASE(test_hidpi_manual_resize_recipe)
{
	if (!test_make_app(LOGICAL_W, LOGICAL_H)) return true; // Headless CI: no display/GPU.
	HidpiGuard guard;

	cf_app_set_pixel_scale(2.0f);
	cf_app_update(NULL);

	// The recipe.
	cf_app_set_size(400, 300);
	float scale = cf_app_get_pixel_scale();
	cf_app_set_canvas_size((int)CF_ROUNDF(400 * scale), (int)CF_ROUNDF(300 * scale));
	cf_draw_projection(cf_ortho_2d(0, 0, 400, 300));

	int w = cf_app_get_canvas_width();
	int h = cf_app_get_canvas_height();
	REQUIRE(w == 800 && h == 600);
	CF_Pixel* px = (CF_Pixel*)cf_alloc(w * h * (int)sizeof(CF_Pixel));

	cf_draw_push_color(cf_make_color_rgb_f(0, 1.0f, 0));
	cf_draw_quad_fill(cf_make_aabb(cf_v2(-200, -150), cf_v2(200, 150)), 0);
	cf_draw_pop_color();
	cf_app_draw_onto_screen(true);

	REQUIRE(s_readback_canvas(cf_app_get_canvas(), w, h, px));
	REQUIRE(s_px_near(px[5 * w + 5], 0, 255, 0, 255, 3));
	REQUIRE(s_px_near(px[(h - 6) * w + (w - 6)], 0, 255, 0, 255, 3));
	REQUIRE(s_px_near(px[(h / 2) * w + (w / 2)], 0, 255, 0, 255, 3));

	cf_free(px);
	return true;
}

// A window resize EVENT is bookkeeping only: cf_app_was_resized fires and app w/h update,
// but the canvas -- including a custom-sized one -- is untouched until the user reacts.
TEST_CASE(test_hidpi_resize_event_does_not_recreate_canvas)
{
	if (!test_make_app(LOGICAL_W, LOGICAL_H)) return true; // Headless CI: no display/GPU.
	HidpiGuard guard;

	// test_make_app's cf_app_set_size may still have an SDL_SetWindowSize confirmation in
	// flight (on X11 the ConfigureNotify can arrive late, as a stale-then-fresh burst).
	// Settle it now so a late real resize event can't land in the same pump as the
	// synthetic one below and stomp app->w after it.
	cf_app_draw_onto_screen(false);
	cf_app_update(NULL);

	cf_app_set_canvas_size(300, 200);
	REQUIRE(cf_app_get_canvas_width() == 300);
	REQUIRE(cf_app_get_canvas_height() == 200);

	// SDL resize events carry raw window coordinates, which CF converts to logical points
	// by the display's content scale (1.0 on macOS, but e.g. X11 can report otherwise) --
	// the synthetic event must speak raw coordinates like a real one.
	float cs = SDL_GetDisplayContentScale(SDL_GetDisplayForWindow(app->window));
	if (cs <= 0) cs = 1.0f;
	SDL_Event e = { };
	e.type = SDL_EVENT_WINDOW_RESIZED;
	e.window.windowID = SDL_GetWindowID(app->window);
	e.window.data1 = (int)CF_ROUNDF(500 * cs);
	e.window.data2 = (int)CF_ROUNDF(400 * cs);
	SDL_PushEvent(&e);
	cf_app_update(NULL);

	REQUIRE(cf_app_was_resized());
	// Compare against the exact raw->points round-trip the handler computes: for content
	// scales that don't divide integers cleanly the result can differ from 500 by one.
	REQUIRE(cf_app_get_width() == (int)CF_ROUNDF(e.window.data1 / cs));
	REQUIRE(cf_app_get_height() == (int)CF_ROUNDF(e.window.data2 / cs));
	REQUIRE(cf_app_get_canvas_width() == 300);
	REQUIRE(cf_app_get_canvas_height() == 200);
	return true;
}

// With no hidden refresh machinery, changing the scale mid-recording is harmless: the
// recording's identity space and the live projection are both left alone.
TEST_CASE(test_hidpi_draw_list_recorded_across_scale_change)
{
	if (!test_make_app(LOGICAL_W, LOGICAL_H)) return true; // Headless CI: no display/GPU.
	HidpiGuard guard;

	s_apply_2x();

	CF_DrawList list = cf_make_draw_list();
	cf_draw_list_begin(list);
	cf_app_set_pixel_scale(3.0f); // Mid-recording: a plain value change, nothing to defer.
	cf_draw_push_color(cf_make_color_rgb_f(1.0f, 0, 0));
	cf_draw_quad_fill(cf_make_aabb(cf_v2(-40, -40), cf_v2(40, 40)), 0);
	cf_draw_pop_color();
	cf_draw_list_end();
	cf_app_set_pixel_scale(2.0f);
	cf_app_draw_onto_screen(false);

	int w = cf_app_get_canvas_width();
	int h = cf_app_get_canvas_height();
	REQUIRE(w == LOGICAL_W * 2 && h == LOGICAL_H * 2);
	CF_Pixel* px = (CF_Pixel*)cf_alloc(w * h * (int)sizeof(CF_Pixel));

	cf_app_update(NULL);
	cf_draw_list(list);
	cf_app_draw_onto_screen(true);

	REQUIRE(s_readback_canvas(cf_app_get_canvas(), w, h, px));
	// The 80x80-logical quad replays centered at 160x160 device pixels on the 640x480 canvas.
	REQUIRE(s_px_near(px[(h / 2) * w + (w / 2)], 255, 0, 0, 255, 3));      // Center.
	REQUIRE(s_px_near(px[(h / 2) * w + (w / 2 + 70)], 255, 0, 0, 255, 3)); // Inside the quad.
	REQUIRE(s_px_near(px[(h / 2) * w + (w / 2 + 100)], 0, 0, 0, 0, 0));    // Outside the quad.

	cf_free(px);
	cf_destroy_draw_list(list);
	return true;
}

TEST_SUITE(test_hidpi)
{
	RUN_TEST_CASE(test_hidpi_set_pixel_scale_is_value_only);
	RUN_TEST_CASE(test_hidpi_no_high_dpi_initial_scale);
	RUN_TEST_CASE(test_hidpi_set_scale_safe_without_gfx);
	RUN_TEST_CASE(test_hidpi_startup_defaults);
	RUN_TEST_CASE(test_hidpi_update_display_helper);
	RUN_TEST_CASE(test_hidpi_default_projection_is_points);
	RUN_TEST_CASE(test_hidpi_full_extent_covers_app_canvas);
	RUN_TEST_CASE(test_hidpi_projection_sticky_across_canvas_recreate);
	RUN_TEST_CASE(test_hidpi_manual_resize_recipe);
	RUN_TEST_CASE(test_hidpi_resize_event_does_not_recreate_canvas);
	RUN_TEST_CASE(test_hidpi_draw_list_recorded_across_scale_change);
}
