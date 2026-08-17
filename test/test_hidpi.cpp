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

// On a HiDPI (Retina) display the default app canvas is logical_size * pixel_scale physical
// pixels while the public draw API stays in logical points. CI and most dev machines run at
// pixel_scale 1.0 where points and pixels are the same number, so a points/pixels mixup is
// invisible there -- these tests force a non-unity pixel_scale through the same code path a
// real display-density change takes, making the HiDPI contract testable everywhere.
//
// Regression context: 3f5a8283 set the default 2d projection to the canvas's *pixel*
// dimensions, which halved everything drawn in point space on 2x displays.

#define LOGICAL_W 320
#define LOGICAL_H 240

static bool s_px_near(CF_Pixel p, int r, int g, int b, int a, int tol)
{
	bool ok = cf_abs((int)p.colors.r - r) <= tol && cf_abs((int)p.colors.g - g) <= tol && cf_abs((int)p.colors.b - b) <= tol && cf_abs((int)p.colors.a - a) <= tol;
	if (!ok) printf("pixel (%d %d %d %d) expected (%d %d %d %d) +/-%d\n", p.colors.r, p.colors.g, p.colors.b, p.colors.a, r, g, b, a, tol);
	return ok;
}

// A failed REQUIRE returns out of the test case early; RAII keeps the forced pixel_scale
// from leaking into whichever test runs next (see also test_app_shared's own sweep).
struct HidpiGuard
{
	~HidpiGuard()
	{
		cf_app_force_pixel_scale(0);
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

// One empty frame: reset_cam at end of frame latches the DEFAULT projection into mvp, so
// tests observe the projection itself rather than whatever an earlier test left latched.
static void s_latch_default_projection()
{
	cf_app_update(NULL);
	cf_app_draw_onto_screen(false);
	cf_app_update(NULL);
}

// The force hook itself: forcing 2x must recreate the default canvas at 2x the logical
// window size, and forcing 0 must restore the true SDL-reported density.
TEST_CASE(test_hidpi_forced_scale_resizes_canvas)
{
	if (!test_make_app(LOGICAL_W, LOGICAL_H)) return true; // Headless CI: no display/GPU.
	HidpiGuard guard;

	cf_app_force_pixel_scale(2.0f);
	REQUIRE(cf_app_get_pixel_scale() == 2.0f);
	REQUIRE(cf_app_get_canvas_width() == LOGICAL_W * 2);
	REQUIRE(cf_app_get_canvas_height() == LOGICAL_H * 2);
	// The logical window size is unchanged -- only the rasterization resolution moved.
	REQUIRE(cf_app_get_width() == LOGICAL_W);
	REQUIRE(cf_app_get_height() == LOGICAL_H);

	cf_app_force_pixel_scale(0);
	float real = cf_app_get_pixel_scale();
	REQUIRE(real > 0);
	REQUIRE(cf_app_get_canvas_width() == (int)CF_ROUNDF(LOGICAL_W * real));
	return true;
}

// CF_APP_OPTIONS_NO_HIGH_DPI_BIT pins pixel_scale at 1.0 -- a forced value must not
// manufacture a state production can never reach.
TEST_CASE(test_hidpi_force_respects_no_high_dpi)
{
	if (!test_make_app(LOGICAL_W, LOGICAL_H, CF_APP_OPTIONS_NO_HIGH_DPI_BIT)) return true; // Headless CI: no display/GPU.
	HidpiGuard guard;

	cf_app_force_pixel_scale(2.0f);
	REQUIRE(cf_app_get_pixel_scale() == 1.0f);
	REQUIRE(cf_app_get_canvas_width() == LOGICAL_W);
	REQUIRE(cf_app_get_canvas_height() == LOGICAL_H);
	return true;
}

// NO_GFX apps never size a canvas, so the force hook must update pixel_scale without
// recreating one (that would dispatch into a NULL backend).
TEST_CASE(test_hidpi_force_is_safe_without_gfx)
{
	if (!test_make_app(LOGICAL_W, LOGICAL_H, CF_APP_OPTIONS_NO_GFX_BIT)) return true;
	HidpiGuard guard;

	cf_app_force_pixel_scale(2.0f);
	REQUIRE(cf_app_get_pixel_scale() == 2.0f);
	return true;
}

// The default 2d projection spans logical points, so world coordinates land on the same
// spot of a 1:1 user canvas no matter the display density. At the broken pixel-space
// projection everything shrinks toward the center and the probe reads background.
TEST_CASE(test_hidpi_default_projection_is_points)
{
	if (!test_make_app(LOGICAL_W, LOGICAL_H)) return true; // Headless CI: no display/GPU.
	HidpiGuard guard;

	cf_app_force_pixel_scale(2.0f);
	s_latch_default_projection();

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
// pixel of the (2x larger) default canvas. At the broken projection it covers only the
// central quarter and the corners read background.
TEST_CASE(test_hidpi_full_extent_covers_app_canvas)
{
	if (!test_make_app(LOGICAL_W, LOGICAL_H)) return true; // Headless CI: no display/GPU.
	HidpiGuard guard;

	cf_app_force_pixel_scale(2.0f);
	s_latch_default_projection();

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

// cf_app_set_size recreates the canvas and refreshes the projection immediately -- and the
// very next frame must draw with it. The projection's companion mvp is only re-latched at
// end of frame by reset_cam, so a projection refresh that skips mvp renders the first
// post-resize frame with the stale matrix.
TEST_CASE(test_hidpi_first_frame_after_resize)
{
	if (!test_make_app(LOGICAL_W, LOGICAL_H)) return true; // Headless CI: no display/GPU.
	HidpiGuard guard;

	cf_app_force_pixel_scale(2.0f);
	cf_app_update(NULL);

	cf_app_set_size(400, 300);
	int w = cf_app_get_canvas_width();
	int h = cf_app_get_canvas_height();
	REQUIRE(w == 800 && h == 600);
	CF_Pixel* px = (CF_Pixel*)cf_alloc(w * h * (int)sizeof(CF_Pixel));

	// Draw WITHOUT an intervening cf_app_update: this is the same frame the resize
	// happened on, exactly where a stale mvp would bite.
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

// cf_app_set_canvas_size is a one-shot override: the canvas takes an exact pixel size
// until the next recreation event snaps it back. Draw coordinates STAY in window points
// throughout -- the projection maps them onto whatever canvas is current.
TEST_CASE(test_hidpi_one_shot_canvas_keeps_points_projection)
{
	if (!test_make_app(LOGICAL_W, LOGICAL_H)) return true; // Headless CI: no display/GPU.
	HidpiGuard guard;

	// test_make_app's cf_app_set_size may still have an SDL_SetWindowSize in flight (on X11
	// the ConfigureNotify confirmation can arrive late, as a stale-then-fresh burst). Settle
	// it now so it can't recreate the canvas out from under the one-shot override below.
	cf_app_draw_onto_screen(false);
	cf_app_update(NULL);

	cf_app_force_pixel_scale(2.0f);
	// Exactly 300x200 pixels: distinct from the logical size, 2x of it, and the window's
	// aspect, so the probes can tell them apart.
	cf_app_set_canvas_size(300, 200);
	REQUIRE(cf_app_get_canvas_width() == 300);
	REQUIRE(cf_app_get_canvas_height() == 200);
	s_latch_default_projection();

	int w = 300, h = 200;
	CF_Pixel* px = (CF_Pixel*)cf_alloc(w * h * (int)sizeof(CF_Pixel));

	// The full LOGICAL extent (window points) covers the whole override canvas.
	cf_draw_push_color(cf_make_color_rgb_f(0, 0, 1.0f));
	cf_draw_quad_fill(cf_make_aabb(cf_v2(-LOGICAL_W / 2.0f, -LOGICAL_H / 2.0f), cf_v2(LOGICAL_W / 2.0f, LOGICAL_H / 2.0f)), 0);
	cf_draw_pop_color();
	cf_app_draw_onto_screen(true);

	REQUIRE(s_readback_canvas(cf_app_get_canvas(), w, h, px));
	REQUIRE(s_px_near(px[5 * w + 5], 0, 0, 255, 255, 3));
	REQUIRE(s_px_near(px[(h - 6) * w + (w - 6)], 0, 0, 255, 255, 3));
	REQUIRE(s_px_near(px[(h / 2) * w + (w / 2)], 0, 0, 255, 255, 3));

	// The next recreation event snaps the override back to window * pixel_scale.
	cf_app_set_size(LOGICAL_W, LOGICAL_H);
	REQUIRE(cf_app_get_canvas_width() == LOGICAL_W * 2);
	REQUIRE(cf_app_get_canvas_height() == LOGICAL_H * 2);

	cf_free(px);
	return true;
}

// A redundant SDL_EVENT_WINDOW_RESIZED reporting the window's CURRENT size (e.g. an
// X11/Xvfb ConfigureNotify fired on window map) is not a recreation event and must not
// stomp a one-shot cf_app_set_canvas_size override.
TEST_CASE(test_hidpi_noop_resize_event_does_not_recreate_canvas)
{
	if (!test_make_app(LOGICAL_W, LOGICAL_H)) return true; // Headless CI: no display/GPU.
	HidpiGuard guard;

	cf_app_force_pixel_scale(2.0f);
	cf_app_set_canvas_size(300, 200);
	REQUIRE(cf_app_get_canvas_width() == 300);
	REQUIRE(cf_app_get_canvas_height() == 200);

	SDL_Event e = { };
	e.type = SDL_EVENT_WINDOW_RESIZED;
	e.window.windowID = SDL_GetWindowID(app->window);
	e.window.data1 = LOGICAL_W; // Same size app->w/h already report -- nothing changed.
	e.window.data2 = LOGICAL_H;
	SDL_PushEvent(&e);
	cf_app_update(NULL);

	REQUIRE(cf_app_get_canvas_width() == 300);
	REQUIRE(cf_app_get_canvas_height() == 200);
	return true;
}

// A canvas recreation landing mid-recording must not corrupt the retained draw list:
// recording runs in identity space and replay composes the live projection on top, so a
// refresh stomped into the recording would bake the ortho in twice. Deferring it also
// makes post-recording frames render at the NEW size even though cf_draw_list_end's pop
// restored the pre-resize projection.
TEST_CASE(test_hidpi_draw_list_recorded_across_resize)
{
	if (!test_make_app(LOGICAL_W, LOGICAL_H)) return true; // Headless CI: no display/GPU.
	HidpiGuard guard;

	cf_app_force_pixel_scale(2.0f);
	s_latch_default_projection();

	CF_DrawList list = cf_make_draw_list();
	cf_draw_list_begin(list);
	cf_app_set_size(400, 300); // Recreates the default canvas MID-recording.
	cf_draw_push_color(cf_make_color_rgb_f(1.0f, 0, 0));
	cf_draw_quad_fill(cf_make_aabb(cf_v2(-40, -40), cf_v2(40, 40)), 0); // Recorded after the resize.
	cf_draw_pop_color();
	cf_draw_list_end();
	cf_app_draw_onto_screen(false); // Frame boundary: the deferred projection refresh applies here.

	int w = cf_app_get_canvas_width();
	int h = cf_app_get_canvas_height();
	REQUIRE(w == 800 && h == 600);
	CF_Pixel* px = (CF_Pixel*)cf_alloc(w * h * (int)sizeof(CF_Pixel));

	cf_app_update(NULL);
	cf_draw_list(list);
	cf_app_draw_onto_screen(true);

	REQUIRE(s_readback_canvas(cf_app_get_canvas(), w, h, px));
	// The 80x80-logical quad replays centered at 160x160 device pixels on the 800x600 canvas.
	REQUIRE(s_px_near(px[(h / 2) * w + (w / 2)], 255, 0, 0, 255, 3));      // Center.
	REQUIRE(s_px_near(px[(h / 2) * w + (w / 2 + 70)], 255, 0, 0, 255, 3)); // Inside the quad.
	REQUIRE(s_px_near(px[(h / 2) * w + (w / 2 + 100)], 0, 0, 0, 0, 0));    // Outside the quad.

	cf_free(px);
	cf_destroy_draw_list(list);
	return true;
}

TEST_SUITE(test_hidpi)
{
	RUN_TEST_CASE(test_hidpi_forced_scale_resizes_canvas);
	RUN_TEST_CASE(test_hidpi_force_respects_no_high_dpi);
	RUN_TEST_CASE(test_hidpi_force_is_safe_without_gfx);
	RUN_TEST_CASE(test_hidpi_default_projection_is_points);
	RUN_TEST_CASE(test_hidpi_full_extent_covers_app_canvas);
	RUN_TEST_CASE(test_hidpi_first_frame_after_resize);
	RUN_TEST_CASE(test_hidpi_one_shot_canvas_keeps_points_projection);
	RUN_TEST_CASE(test_hidpi_noop_resize_event_does_not_recreate_canvas);
	RUN_TEST_CASE(test_hidpi_draw_list_recorded_across_resize);
}
