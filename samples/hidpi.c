/*
	hidpi.c -- HiDPI / Retina rendering visual verification.

	Cute Framework renders its default canvas at physical resolution (logical
	size scaled by `cf_app_get_pixel_scale()`), so text and shapes stay crisp
	on Retina/HiDPI displays without any extra work from the user. This sample
	is a quick visual check of that: run it on a HiDPI display and glyph edges
	and shape antialiasing should look sharp, not soft/blurry.

	See docs/topics/hidpi.md for the full point/pixel model and the retro
	pixel-art escape hatch (`cf_app_set_canvas_size`).

	What it draws:
	  - Text at three sizes (12px / 24px / 48px) to eyeball glyph
	    crispness at different scales.
	  - A row of basic SDF shapes (filled circle, outlined circle, lines
	    of varying thickness including a thin ~1px line, a filled rounded
	    box, and an outlined triangle) to eyeball shape edge antialiasing.
	  - A live readout of `cf_app_get_pixel_scale()` alongside the physical
	    canvas size, so the current display's HiDPI scale factor is visible
	    at a glance.

	No interactivity beyond closing the window; no external assets needed.
*/

#include <cute.h>
#include <stdio.h>

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

	while (cf_app_is_running()) {
		cf_app_update(NULL);

		if (cf_app_was_resized()) {
			// Nothing special to handle here -- part of the point of this
			// sample is to observe how resizing/HiDPI scaling affects
			// rendering crispness.
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
		char pixel_scale_buf[128];
		float pixel_scale = cf_app_get_pixel_scale();
		int physical_w = cf_app_get_canvas_width();
		int physical_h = cf_app_get_canvas_height();
		snprintf(
			pixel_scale_buf, sizeof(pixel_scale_buf),
			"pixel_scale: %.2fx (physical canvas: %dx%d)",
			pixel_scale, physical_w, physical_h
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
