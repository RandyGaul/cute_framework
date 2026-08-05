/*
	canvas_modes.c -- interactive tour of app-canvas sizing.

	The app's default canvas is recreated at window_size (in points) * cf_app_get_pixel_scale()
	on every canvas recreation event: a window resize, a display pixel-density change,
	cf_app_set_size(), or cf_app_set_msaa(). cf_app_set_canvas_size() is a ONE-SHOT override:
	it resizes the canvas now, and the custom size lasts only until the next recreation event.
	This sample holds a non-default size the intended way -- by re-applying it whenever the
	canvas has snapped back (see the reconcile block in the main loop).

	For a persistent fixed-resolution target (retro / pixel-art look) the right tool is your
	own canvas: render the scene into it with cf_render_to(), then scale it up onto the app
	canvas with cf_draw_canvas(). Mode 4 shows this -- note how its HUD text stays crisp,
	because the HUD is drawn onto the app canvas at native resolution.

	Press SPACE to cycle:
	  1. Default       -- app canvas = window * pixel_scale (crisp, HiDPI-native).
	  2. Custom 0.5x   -- app canvas at half resolution; the whole frame (HUD included) goes
	                      soft. The fill-rate-saving pattern.
	  3. Forced 1x     -- app canvas at window points; blurry on a HiDPI display. This is what
	                      every app looked like before HiDPI rendering. Identical to Default
	                      on a 1x display.
	  4. Fixed 320x180 -- scene rendered into an owned nearest-filtered canvas, letterboxed
	                      onto the default app canvas. Chunky pixels + bars, crisp HUD.

	The same scene is drawn in every mode so crispness differences pop. No external assets.
*/

#include <cute.h>
#include <stdio.h>

#define FIXED_W 320
#define FIXED_H 180

enum
{
	MODE_DEFAULT,
	MODE_HALF_RES,
	MODE_FORCED_1X,
	MODE_FIXED_CANVAS,
	MODE_COUNT,
};

static const char* mode_names[MODE_COUNT] = {
	"Default (window * pixel_scale)",
	"Custom scale 0.5x",
	"Forced 1x (window points)",
	"Fixed 320x180 canvas",
};

static const char* mode_hints[MODE_COUNT] = {
	"crisp text and shape edges (differs from Forced 1x only on a HiDPI display)",
	"the whole frame -- this text included -- goes soft; half the pixels are rendered",
	"the pre-HiDPI look: blurry on HiDPI displays, identical to Default on 1x displays",
	"chunky scene pixels and black bars, while this HUD text stays crisp",
};

// Aspect-fit (letterbox) destination size for scaling content_w x content_h up into
// window_w x window_h. See samples/window_resizing.cpp for crop/stretch variants.
static CF_V2 letterbox_dest(float content_w, float content_h, float window_w, float window_h)
{
	float content_aspect = content_w / content_h;
	float window_aspect = window_w / window_h;
	if (window_aspect > content_aspect) {
		return cf_v2(window_h * content_aspect, window_h); // Pillarbox (bars on the sides).
	} else {
		return cf_v2(window_w, window_w / content_aspect); // Letterbox (bars on top/bottom).
	}
}

static void draw_text_centered(const char* text, CF_V2 top_center)
{
	CF_V2 size = cf_text_size(text, -1);
	cf_draw_text(text, cf_v2(top_center.x - size.x * 0.5f, top_center.y), -1);
}

// One scene, drawn into a w x h logical space with a centered origin. Big shapes scale with
// the space; fine details (line thicknesses, the pixel ruler, font size) stay at absolute
// logical units so they probe the target's resolution.
static void draw_scene(CF_Sprite* sprite, float w, float h, float t)
{
	// Hairline border just inside the space's edge -- in mode 4 this marks the fixed
	// canvas bounds against the letterbox bars.
	cf_draw_push_color(cf_make_color_rgb(90, 100, 125));
	cf_draw_quad(cf_make_aabb(cf_v2(-w * 0.5f + 2, -h * 0.5f + 2), cf_v2(w * 0.5f - 2, h * 0.5f - 2)), 1.0f, 0);
	cf_draw_pop_color();

	cf_draw_push_color(cf_color_white());

	// -- Row of big shapes (SDF edge probes), sized relative to the space. --
	float row_y = h * 0.12f;
	cf_draw_circle_fill2(cf_v2(-w * 0.32f, row_y), h * 0.11f);
	cf_draw_circle2(cf_v2(-w * 0.12f, row_y), h * 0.11f, 2.0f);
	CF_Aabb box = cf_make_aabb_pos_w_h(cf_v2(w * 0.10f, row_y), w * 0.14f, h * 0.20f);
	cf_draw_box_rounded_fill(box, h * 0.03f);

	// A dot orbiting the scene -- a moving probe that crosses crisp and blurry regions.
	cf_draw_circle_fill2(cf_v2(cosf(t) * w * 0.42f, sinf(t) * h * 0.38f), 3.0f);

	cf_draw_pop_color();

	// CF's built-in pixel-art demo sprite -- intentionally chunky at any resolution.
	sprite->transform.p = cf_v2(w * 0.32f, row_y);
	cf_sprite_update(sprite);
	cf_draw_sprite(sprite);

	// -- Fine-detail probes at absolute logical units. --
	cf_draw_push_color(cf_color_white());

	// Lines of 1/3/6 unit thickness.
	float line_y = -h * 0.14f;
	cf_draw_line(cf_v2(-w * 0.40f, line_y + 12), cf_v2(-w * 0.12f, line_y + 12), 1.0f);
	cf_draw_line(cf_v2(-w * 0.40f, line_y), cf_v2(-w * 0.12f, line_y), 3.0f);
	cf_draw_line(cf_v2(-w * 0.40f, line_y - 14), cf_v2(-w * 0.12f, line_y - 14), 6.0f);

	// Pixel ruler: 1-unit lines at 3-unit spacing. Blur smears these grey immediately.
	for (int i = 0; i < 20; ++i) {
		float x = w * 0.08f + i * 3.0f;
		cf_draw_line(cf_v2(x, line_y - 12), cf_v2(x, line_y + 12), 1.0f);
	}

	// Small text as a glyph crispness probe.
	cf_push_font_size(13);
	draw_text_centered("The quick brown fox jumps over the lazy dog 0123456789", cf_v2(0, -h * 0.30f));
	cf_pop_font_size();

	cf_draw_pop_color();
}

int main(int argc, char* argv[])
{
	int options = CF_APP_OPTIONS_WINDOW_POS_CENTERED_BIT | CF_APP_OPTIONS_RESIZABLE_BIT;
	CF_Result result = cf_make_app("Canvas Modes", 0, 0, 0, 960, 540, options, argv[0]);
	if (cf_is_error(result)) {
		printf("Error: %s\n", result.details);
		return -1;
	}

	// Black window clear -- in mode 4 this is the letterbox bar color.
	cf_clear_color(0, 0, 0, 1.0f);

	// The persistent fixed-resolution canvas for mode 4. Nearest filtering keeps the
	// upscale blocky instead of smearing it.
	CF_CanvasParams params = cf_canvas_defaults(FIXED_W, FIXED_H);
	params.target.filter = CF_FILTER_NEAREST;
	CF_Canvas fixed_canvas = cf_make_canvas(params);
	cf_canvas_set_clear_color(fixed_canvas, cf_make_color_rgb_f(0.08f, 0.10f, 0.14f));

	CF_Sprite sprite = cf_make_demo_sprite();
	cf_sprite_play(&sprite, "idle");
	sprite.scale = cf_v2(2.0f, 2.0f);

	int mode = MODE_DEFAULT;
	float t = 0;

	while (cf_app_is_running()) {
		cf_app_update(NULL);
		t += CF_DELTA_TIME;

		if (cf_key_just_pressed(CF_KEY_SPACE)) {
			mode = (mode + 1) % MODE_COUNT;
		}

		int win_w, win_h;
		cf_app_get_size(&win_w, &win_h); // Logical points.
		float pixel_scale = cf_app_get_pixel_scale();

		// -- Reconcile the app canvas size with the current mode. --
		// cf_app_set_canvas_size is a one-shot override: any recreation event (window
		// resize, display density change, cf_app_set_size, cf_app_set_msaa) snaps the
		// canvas back to window * pixel_scale. So rather than reacting to resize events,
		// re-apply the desired size whenever the canvas no longer matches it.
		int desired_w, desired_h;
		switch (mode) {
		case MODE_HALF_RES:
			desired_w = (int)CF_ROUNDF(win_w * pixel_scale * 0.5f);
			desired_h = (int)CF_ROUNDF(win_h * pixel_scale * 0.5f);
			break;
		case MODE_FORCED_1X:
			desired_w = win_w;
			desired_h = win_h;
			break;
		default: // The automatic size -- only reached after leaving mode 2/3.
			desired_w = (int)CF_ROUNDF(win_w * pixel_scale);
			desired_h = (int)CF_ROUNDF(win_h * pixel_scale);
			break;
		}
		if (cf_app_get_canvas_width() != desired_w || cf_app_get_canvas_height() != desired_h) {
			cf_app_set_canvas_size(desired_w, desired_h);
		}

		// Linear filtering tells the blur story for the low-res app-canvas modes; nearest
		// (the default) would turn it into chunky pixels instead.
		bool low_res_app_canvas = (mode == MODE_HALF_RES || mode == MODE_FORCED_1X);
		cf_app_set_canvas_blit_filter(low_res_app_canvas ? CF_FILTER_LINEAR : CF_FILTER_NEAREST);

		cf_push_font("Calibri");

		// The default 2d projection follows the canvas, so set our own every frame: the
		// scene is authored in logical points (or fixed-canvas units in mode 4), never in
		// device pixels.
		if (mode == MODE_FIXED_CANVAS) {
			// Scene at fixed resolution, into the owned canvas.
			cf_draw_projection(cf_ortho_2d(0, 0, (float)FIXED_W, (float)FIXED_H));
			draw_scene(&sprite, (float)FIXED_W, (float)FIXED_H, t);
			cf_render_to(fixed_canvas, true);

			// Letterbox it onto the app canvas.
			cf_draw_projection(cf_ortho_2d(0, 0, (float)win_w, (float)win_h));
			CF_V2 dest = letterbox_dest((float)FIXED_W, (float)FIXED_H, (float)win_w, (float)win_h);
			cf_draw_canvas(fixed_canvas, cf_v2(0, 0), dest);
		} else {
			cf_draw_projection(cf_ortho_2d(0, 0, (float)win_w, (float)win_h));
			draw_scene(&sprite, (float)win_w, (float)win_h, t);
		}

		// -- HUD, drawn onto the app canvas in window points. --
		char buf[192];
		float hud_x = -win_w * 0.5f + 12.0f;
		float hud_y = win_h * 0.5f - 10.0f;
		cf_draw_push_color(cf_color_white());
		cf_push_font_size(16);
		snprintf(buf, sizeof(buf), "Mode %d/%d: %s  --  press SPACE to switch", mode + 1, MODE_COUNT, mode_names[mode]);
		cf_draw_text(buf, cf_v2(hud_x, hud_y), -1);
		cf_pop_font_size();
		cf_push_font_size(13);
		snprintf(buf, sizeof(buf), "window %dx%d pts  |  pixel_scale %.2f  |  app canvas %dx%d px%s",
			win_w, win_h, pixel_scale, cf_app_get_canvas_width(), cf_app_get_canvas_height(),
			mode == MODE_FIXED_CANVAS ? "  |  fixed canvas 320x180 px" : "");
		cf_draw_text(buf, cf_v2(hud_x, hud_y - 22), -1);
		snprintf(buf, sizeof(buf), "Look for: %s", mode_hints[mode]);
		cf_draw_text(buf, cf_v2(hud_x, hud_y - 40), -1);
		cf_pop_font_size();
		cf_draw_pop_color();

		cf_pop_font();

		cf_app_draw_onto_screen(true);
	}

	cf_destroy_canvas(fixed_canvas);
	cf_destroy_app();

	return 0;
}
