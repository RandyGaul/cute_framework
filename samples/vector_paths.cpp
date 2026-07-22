/*
	Vector paths -- cf_draw_path_begin/end + fill/stroke.

	User Bezier paths baked once and rendered per-pixel by the renderer's winding
	machinery (the same engine behind curve text): a heart from cubics, a star with a
	nonzero-winding hole, and a stroked spline. Everything stays perfectly crisp under
	the animated zoom and rotation -- no tessellation, no resolution to pick.

	Run with `vector_paths screenshot out.png` to render one frame to a png and exit.
*/

#include <cute.h>
#include <stdio.h>

using namespace Cute;

int main(int argc, char* argv[])
{
	bool screenshot = argc > 1 && CF_STRCMP(argv[1], "screenshot") == 0;
	const char* screenshot_path = argc > 2 ? argv[2] : "vector_paths.png";
	int options = CF_APP_OPTIONS_WINDOW_POS_CENTERED_BIT | (screenshot ? CF_APP_OPTIONS_HIDDEN_BIT : CF_APP_OPTIONS_RESIZABLE_BIT);
	CF_Result result = make_app("Vector Paths", 0, 0, 0, 1280, 720, options, argv[0]);
	if (is_error(result)) return -1;
	if (!screenshot) cf_app_set_present_mode(CF_PRESENT_MODE_IMMEDIATE);
	cf_clear_color(0.09f, 0.10f, 0.13f, 1.0f);

	// Heart: two cubics.
	draw_path_begin();
	draw_path_move_to(V2(0, -70));
	draw_path_cubic_to(V2(-110, 20), V2(-70, 90), V2(0, 40));
	draw_path_cubic_to(V2(70, 90), V2(110, 20), V2(0, -70));
	draw_path_close();
	CF_DrawPath heart = draw_path_end();

	// Five-point star with a pentagonal hole: outer contour wound one way, inner the
	// other -- the nonzero winding rule cuts the hole.
	draw_path_begin();
	for (int i = 0; i < 10; ++i) {
		float a = (float)i * (CF_PI / 5.0f) + CF_PI * 0.5f;
		float r = (i & 1) ? 34.0f : 85.0f;
		v2 p = V2(cosf(a) * r, sinf(a) * r);
		if (i == 0) draw_path_move_to(p);
		else draw_path_line_to(p);
	}
	draw_path_close();
	for (int i = 4; i >= 0; --i) { // Reverse order flips the winding.
		float a = (float)i * (CF_PI * 2.0f / 5.0f) + CF_PI * 0.5f;
		v2 p = V2(cosf(a) * 18.0f, sinf(a) * 18.0f);
		if (i == 4) draw_path_move_to(p);
		else draw_path_line_to(p);
	}
	draw_path_close();
	CF_DrawPath star = draw_path_end();

	// Loopy spline for stroking.
	draw_path_begin();
	draw_path_move_to(V2(-90, -40));
	draw_path_quad_to(V2(-90, 60), V2(0, 55));
	draw_path_quad_to(V2(80, 50), V2(40, -10));
	draw_path_quad_to(V2(0, -60), V2(-20, 0));
	draw_path_quad_to(V2(-32, 38), V2(30, 30));
	CF_DrawPath spline = draw_path_end();

	float t = screenshot ? 1.2f : 0;

	do {
		app_update();
		t += CF_DELTA_TIME;
		float zoom = 1.6f + sinf(t * 0.5f) * 0.9f;

		// Heart: filled + stroked outline, breathing zoom.
		draw_push();
		draw_translate(-330, 40);
		draw_scale(zoom, zoom);
		draw_push_color(make_color(0.95f, 0.3f, 0.4f));
		draw_path_fill(heart);
		draw_pop_color();
		draw_push_color(make_color(1.0f, 0.75f, 0.8f));
		draw_path(heart, 3.0f / zoom); // Constant on-screen stroke width.
		draw_pop_color();
		draw_pop();

		// Star: rotating, with its nonzero-winding hole.
		draw_push();
		draw_translate(0, 40);
		draw_rotate(t * 0.4f);
		draw_scale(1.8f, 1.8f);
		draw_push_color(make_color(1.0f, 0.85f, 0.3f));
		draw_path_fill(star);
		draw_pop_color();
		draw_pop();

		// Spline: stroke only, open contours close automatically for the fill rule but
		// read as a drawn line when stroked.
		draw_push();
		draw_translate(330, 40);
		draw_scale(1.7f, 1.7f);
		draw_push_color(make_color(0.4f, 0.85f, 1.0f));
		draw_path(spline, 4.0f);
		draw_pop_color();
		draw_pop();

		draw_push_color(color_white());
		push_font_size(16);
		draw_text("cf_draw_path: cubics (heart), nonzero-winding hole (star), stroked spline -- crisp at any zoom/rotation", V2(-620, 348), -1);
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

	destroy_path(heart);
	destroy_path(star);
	destroy_path(spline);
	destroy_app();
	return 0;
}
