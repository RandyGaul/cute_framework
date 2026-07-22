/*
	Vector text -- curve glyph rendering demo (cf_push_text_curves).

	Glyphs render per-pixel straight from their quadratic Bezier outlines instead of
	rasterized atlas bitmaps: resolution-independent, so text stays crisp under any
	camera zoom or rotation, and cf_push_text_stroke draws outlined text. The top line
	uses the regular atlas path for comparison -- zoom with the mouse wheel and watch
	the difference.

	Run with `vector_text screenshot out.png` to render one frame to a png and exit.
	Run with `vector_text bench` to fill the window with text at several font sizes and
	print average frame times for the atlas path vs the curve path.
*/

#include <cute.h>
#include <stdio.h>
#include <stdlib.h>

using namespace Cute;

// Fills the window with lines of a pangram at `font_size`, returns glyphs drawn.
static int s_wall_of_text(float font_size)
{
	const char* line = "Sphinx of black quartz, judge my vow! 0123456789 the quick brown fox jumps over the lazy dog. ";
	int chars_per_line = (int)CF_STRLEN(line);
	push_font_size(font_size);
	draw_push_color(color_white());
	float line_h = font_size * 1.2f;
	int glyphs = 0;
	for (float y = 348; y > -360; y -= line_h) {
		draw_text(line, V2(-630, y), -1);
		glyphs += chars_per_line;
	}
	draw_pop_color();
	pop_font_size();
	return glyphs;
}

static void s_bench()
{
	const int warmup = 30;
	const int frames = 200;
	float sizes[3] = { 13, 26, 64 };
	for (int si = 0; si < 3; ++si) {
		for (int pass = 0; pass < 2; ++pass) {
			push_text_curves(pass == 1);
			double total_ms = 0, record_ms = 0;
			int glyphs = 0;
			for (int f = 0; f < warmup + frames; ++f) {
				app_update();
				uint64_t t0 = cf_get_ticks();
				glyphs = s_wall_of_text(sizes[si]);
				uint64_t t1 = cf_get_ticks();
				app_draw_onto_screen(true);
				double ms = (double)(cf_get_ticks() - t0) / (double)cf_get_tick_frequency() * 1000.0;
				double rms = (double)(t1 - t0) / (double)cf_get_tick_frequency() * 1000.0;
				if (f >= warmup) { total_ms += ms; record_ms += rms; }
			}
			pop_text_curves();
			printf("%s %2.0fpx: %d glyphs, %.3f ms/frame avg over %d frames (record %.3f ms)\n",
				pass == 1 ? "CURVES" : "ATLAS ", sizes[si], glyphs, total_ms / frames, frames, record_ms / frames);
		}
	}
}

int main(int argc, char* argv[])
{
	bool screenshot = argc > 1 && CF_STRCMP(argv[1], "screenshot") == 0;
	bool bench = argc > 1 && CF_STRCMP(argv[1], "bench") == 0;
	const char* screenshot_path = argc > 2 ? argv[2] : "vector_text.png";
	int options = CF_APP_OPTIONS_WINDOW_POS_CENTERED_BIT | (screenshot || bench ? CF_APP_OPTIONS_HIDDEN_BIT : CF_APP_OPTIONS_RESIZABLE_BIT);
	CF_Result result = make_app("Vector Text -- mouse wheel zooms", 0, 0, 0, 1280, 720, options, argv[0]);
	if (is_error(result)) return -1;
	if (!screenshot) cf_app_set_present_mode(CF_PRESENT_MODE_IMMEDIATE);
	cf_clear_color(0.08f, 0.09f, 0.12f, 1.0f);

	if (bench) {
		s_bench();
		destroy_app();
		return 0;
	}

	float zoom = screenshot ? (argc > 3 ? (float)atof(argv[3]) : 6.0f) : 1.0f;
	float t = screenshot ? 0.6f : 0;

	do {
		app_update();
		t += CF_DELTA_TIME;
		int wheel = cf_mouse_wheel_motion();
		if (wheel) zoom = clamp(zoom * powf(1.25f, (float)wheel), 0.25f, 200.0f);

		// Zoomed world: atlas path on top for comparison, curve path below.
		draw_push();
		draw_scale(zoom, zoom);
		push_font_size(26);

		draw_push_color(color_white());
		push_text_curves(false); // Force the rasterized atlas path (curves are the default).
		draw_text("Atlas:  Sphinx of black quartz, judge my vow.", V2(-300, 52), -1);
		pop_text_curves();
		push_text_curves(true);
		draw_text("Curves: Sphinx of black quartz, judge my vow.", V2(-300, 20), -1);
		draw_pop_color();

		// Stroked outline text -- only possible on the curve path.
		push_font_size(48);
		push_text_stroke(0.5f);
		draw_push_color(make_color(0.45f, 0.85f, 1.0f));
		draw_text("Outlined text!", V2(-300, -16), -1);
		draw_pop_color();
		pop_text_stroke();
		pop_font_size();

		// Rotating text stays crisp at any angle.
		draw_push();
		draw_translate(160, -130);
		draw_rotate(sinf(t) * 0.35f);
		push_font_size(32);
		draw_push_color(make_color(1.0f, 0.75f, 0.35f));
		draw_text("spin me", V2(-58, 12), -1);
		draw_pop_color();
		pop_font_size();
		draw_pop();

		// Small sizes for eyeballing AA quality against the raster path.
		draw_push_color(color_white());
		push_font_size(13);
		draw_text("13px curves: the quick brown fox jumps over the lazy dog", V2(-300, -80), -1);
		pop_font_size();
		pop_text_curves();
		push_text_curves(false);
		push_font_size(13);
		draw_text("13px atlas:  the quick brown fox jumps over the lazy dog", V2(-300, -100), -1);
		pop_font_size();
		pop_text_curves();
		draw_pop_color();

		pop_font_size();
		draw_pop();

		// Fixed-scale overlay.
		draw_push_color(color_white());
		push_font_size(16);
		char buf[128];
		snprintf(buf, sizeof(buf), "zoom %.2fx  |  mouse wheel to zoom", zoom);
		draw_text(buf, V2(-620, 348), -1);
		pop_font_size();
		draw_pop_color();

		if (screenshot) {
			CF_Canvas canvas = make_canvas(canvas_defaults(1280, 720));
			render_to(canvas, true);
			app_draw_onto_screen(false);
			CF_Readback rb = cf_canvas_readback(canvas);
			while (!cf_readback_ready(rb)) {}
			CF_Image img;
			img.w = 1280;
			img.h = 720;
			img.pix = (CF_Pixel*)cf_alloc(1280 * 720 * sizeof(CF_Pixel));
			cf_readback_data(rb, img.pix, 1280 * 720 * (int)sizeof(CF_Pixel));
			cf_destroy_readback(rb);
			cf_destroy_canvas(canvas);
			fs_set_write_directory(fs_get_base_directory());
			cf_image_save_png(screenshot_path, &img);
			cf_free(img.pix);
			break;
		}

		app_draw_onto_screen(true);
	} while (app_is_running());

	destroy_app();
	return 0;
}
