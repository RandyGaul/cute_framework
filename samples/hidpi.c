/*
	hidpi.c -- HiDPI / Retina rendering, the manual way.

	The pixel scale (physical pixels per logical point) is a plain user-controlled
	value, like the window or canvas size. At startup CF creates the app canvas
	once at window_points * the display's natural density and sets the default 2d
	projection once from the logical window size -- and never touches either again.
	Reacting to window resizes and display-density changes is YOUR code, and this
	sample is the copy-paste recipe:

	    // React to a window resize (and/or a density change while tracking the
	    // display): re-apply scale, canvas, and projection in one place.
	    static void apply_pixel_scale(float scale)
	    {
	        int w = cf_app_get_width();
	        int h = cf_app_get_height();
	        cf_app_set_pixel_scale(scale);
	        cf_app_set_canvas_size((int)(w * scale + 0.5f), (int)(h * scale + 0.5f));
	        cf_draw_projection(cf_ortho_2d(0, 0, (float)w, (float)h));
	    }

	    // In the main loop:
	    if (cf_app_was_resized()) apply_pixel_scale(cf_app_get_pixel_scale());
	    if (cf_app_dpi_scale_was_changed() && tracking_the_display) {
	        apply_pixel_scale(cf_app_get_natural_pixel_scale());
	    }

	A fixed-size, non-resizable window on one display needs NONE of this -- the
	startup defaults are already correct.

	Interactivity: press N to track the display's natural density (the default),
	or 1 / 2 / 4 to force a 1x / 2x / 4x pixel scale -- forcing a value is also
	how you test HiDPI behavior on a non-HiDPI monitor. Resize the window to
	watch the recipe keep everything crisp.

	What it draws:
	  - Text at three sizes (12px / 24px / 48px) to eyeball glyph
	    crispness at different scales.
	  - A row of basic SDF shapes (filled circle, outlined circle, lines
	    of varying thickness including a thin ~1px line, a filled rounded
	    box, and an outlined triangle) to eyeball shape edge antialiasing.
	  - A live readout of the applied and natural pixel scales alongside
	    the physical canvas size.
*/

#include <cute.h>
#include <stdio.h>

// The whole "automatic HiDPI" replacement, in one function: apply a pixel scale and
// rebuild the canvas and projection to match the current window size.
static void apply_pixel_scale(float scale)
{
	int w = cf_app_get_width();
	int h = cf_app_get_height();
	cf_app_set_pixel_scale(scale);
	cf_app_set_canvas_size((int)(w * scale + 0.5f), (int)(h * scale + 0.5f));
	cf_draw_projection(cf_ortho_2d(0, 0, (float)w, (float)h));
}

// Draws `text` such that it's horizontally centered underneath/at `top_center`,
// with `top_center.y` acting as the top of the text (matches cf_draw_text's
// top-left-origin convention).
static void draw_text_centered(const char* text, CF_V2 top_center)
{
	CF_V2 size = cf_text_size(text, -1);
	cf_draw_text(text, cf_v2(top_center.x - size.x * 0.5f, top_center.y), -1);
}

int main(int argc, char* argv[])
{
	int options = CF_APP_OPTIONS_WINDOW_POS_CENTERED_BIT | CF_APP_OPTIONS_RESIZABLE_BIT;
	CF_Result result = cf_make_app("HiDPI Rendering Test", 0, 0, 0, 1280, 800, options, argv[0]);
	if (cf_is_error(result)) {
		printf("Error: %s\n", result.details);
		return -1;
	}

	// Solid, dark background so both white shape/text edges are easy to eyeball.
	cf_clear_color(0.08f, 0.10f, 0.14f, 1.0f);

	// CF's built-in pixel-art demo sprite, resized up -- contrasts a chunky,
	// intentionally low-res sprite against the crisp vector text/shapes above.
	CF_Sprite sprite = cf_make_demo_sprite();
	cf_sprite_play(&sprite, "idle");
	sprite.scale = cf_v2(3.0f, 3.0f);

	// true = follow the display's natural density; false = a forced 1x/2x/4x scale.
	bool track_natural = true;

	while (cf_app_is_running()) {
		cf_app_update(NULL);

		// Scale mode switching. Forcing a scale on purpose is exactly the same call the
		// engine-side recipe uses -- there is no separate "override" concept.
		if (cf_key_just_pressed(CF_KEY_N)) { track_natural = true;  apply_pixel_scale(cf_app_get_natural_pixel_scale()); }
		if (cf_key_just_pressed(CF_KEY_1)) { track_natural = false; apply_pixel_scale(1.0f); }
		if (cf_key_just_pressed(CF_KEY_2)) { track_natural = false; apply_pixel_scale(2.0f); }
		if (cf_key_just_pressed(CF_KEY_4)) { track_natural = false; apply_pixel_scale(4.0f); }

		// The manual-model recipe: window resized -> rebuild canvas + projection at the
		// current scale. Density changed (moved to another monitor) -> re-apply the new
		// natural scale, but only when tracking it.
		if (cf_app_was_resized()) {
			apply_pixel_scale(cf_app_get_pixel_scale());
		}
		if (cf_app_dpi_scale_was_changed() && track_natural) {
			apply_pixel_scale(cf_app_get_natural_pixel_scale());
		}

		cf_push_font("Calibri");
		cf_draw_push_color(cf_color_white());

		// -- Title --
		cf_push_font_size(32);
		draw_text_centered("HiDPI Rendering Test Harness", cf_v2(0, 380));
		cf_pop_font_size();

		cf_push_font_size(16);
		draw_text_centered(
			"Compare glyph and shape edge crispness before/after the physical-resolution rendering fix.",
			cf_v2(0, 335)
		);
		cf_pop_font_size();

		// -- Live pixel-scale readout --
		char pixel_scale_buf[192];
		float pixel_scale = cf_app_get_pixel_scale();
		int physical_w = cf_app_get_canvas_width();
		int physical_h = cf_app_get_canvas_height();
		snprintf(
			pixel_scale_buf, sizeof(pixel_scale_buf),
			"pixel_scale: %.2fx %s (natural: %.2fx, physical canvas: %dx%d) -- press N/1/2/4",
			pixel_scale, track_natural ? "[natural]" : "[forced]",
			cf_app_get_natural_pixel_scale(), physical_w, physical_h
		);
		cf_push_font_size(12);
		draw_text_centered(pixel_scale_buf, cf_v2(0, 300));
		cf_pop_font_size();

		cf_draw_pop_color();

		cf_draw_push_color(cf_make_color_rgb(120, 130, 150));
		cf_draw_line(cf_v2(-620, 305), cf_v2(620, 305), 1.0f);
		cf_draw_pop_color();

		// -- Text at several font sizes --
		const char* sample_small = "The quick brown fox jumps over the lazy dog -- AaBbCcGgQqRrSs 0123456789";
		const char* sample_medium = "AaBbCcGgQq -- The quick brown fox -- 0123456789";
		const char* sample_large = "AaBbCcGgQq 0123456789";

		cf_draw_push_color(cf_color_white());

		cf_push_font_size(14);
		cf_draw_text("12px:", cf_v2(-600, 275), -1);
		cf_pop_font_size();
		cf_push_font_size(12);
		cf_draw_text(sample_small, cf_v2(-600, 255), -1);
		cf_pop_font_size();

		cf_push_font_size(14);
		cf_draw_text("24px:", cf_v2(-600, 220), -1);
		cf_pop_font_size();
		cf_push_font_size(24);
		cf_draw_text(sample_medium, cf_v2(-600, 195), -1);
		cf_pop_font_size();

		cf_push_font_size(14);
		cf_draw_text("48px:", cf_v2(-600, 145), -1);
		cf_pop_font_size();
		cf_push_font_size(48);
		cf_draw_text(sample_large, cf_v2(-600, 115), -1);
		cf_pop_font_size();

		cf_draw_pop_color();

		cf_draw_push_color(cf_make_color_rgb(120, 130, 150));
		cf_draw_line(cf_v2(-620, -100), cf_v2(620, -100), 1.0f);
		cf_draw_pop_color();

		// -- Row of assorted SDF shapes (exercise aaf-based antialiasing) --
		float shapes_y = -180.0f;
		float label_y = -235.0f;

		cf_draw_push_color(cf_color_white());

		// Filled circle.
		cf_draw_circle_fill2(cf_v2(-500, shapes_y), 40.0f);

		// Hollow (outlined) circle.
		cf_draw_circle2(cf_v2(-360, shapes_y), 40.0f, 3.0f);

		// Lines of varying thickness, including a thin ~1px line.
		cf_draw_line(cf_v2(-260, shapes_y + 20), cf_v2(-120, shapes_y + 20), 1.0f);
		cf_draw_line(cf_v2(-260, shapes_y), cf_v2(-120, shapes_y), 3.0f);
		cf_draw_line(cf_v2(-260, shapes_y - 25), cf_v2(-120, shapes_y - 25), 6.0f);

		// Filled rounded box.
		CF_Aabb box = cf_make_aabb_pos_w_h(cf_v2(40, shapes_y), 100.0f, 70.0f);
		cf_draw_box_rounded_fill(box, 14.0f);

		// Outlined triangle.
		cf_draw_tri(cf_v2(220, shapes_y + 40), cf_v2(180, shapes_y - 40), cf_v2(260, shapes_y - 40), 2.0f, 0.0f);

		cf_draw_pop_color();

		// Resized pixel-art demo sprite -- deliberately chunky, unlike the crisp
		// vector shapes to its left.
		sprite.transform.p = cf_v2(400, shapes_y + 20);
		cf_sprite_update(&sprite);
		cf_draw_sprite(&sprite);

		cf_push_font_size(12);
		draw_text_centered("Filled circle", cf_v2(-500, label_y));
		draw_text_centered("Hollow circle", cf_v2(-360, label_y));
		draw_text_centered("Lines 1/3/6px", cf_v2(-190, label_y));
		draw_text_centered("Rounded box", cf_v2(40, label_y));
		draw_text_centered("Triangle", cf_v2(220, label_y));
		draw_text_centered("Demo sprite (3x)", cf_v2(400, label_y));
		cf_pop_font_size();

		cf_pop_font();

		cf_app_draw_onto_screen(true);
	}

	cf_destroy_app();

	return 0;
}
