/*
	Blend modes -- per-drawable blend modes via cf_draw_push_blend.

	Four identical panels, one per mode: the same backdrop (a dark-to-light gradient
	with colored shapes) and the same overlapping RGB circles on top. Only the blend
	mode differs, so every visual difference between panels is the mode itself:

	  NORMAL    alpha blends over everything.
	  ADD       brightens what's beneath; strongest over dark, washes out over light.
	  MULTIPLY  darkens; tints the light regions, crushes the dark ones.
	  SCREEN    brightens softly; strongest over dark, gentle over light.

	Modes record per draw call in one paint-ordered stream -- no batching or render
	state juggling. Run with `blend_modes screenshot out.png` for a one-frame png.
*/

#include <cute.h>
#include <stdio.h>

using namespace Cute;

static void s_panel(float cx, CF_DrawBlend mode, const char* label, float t)
{
	float w = 280;
	float h = 420;
	float x0 = cx - w * 0.5f;
	float y0 = -150;

	// Identical backdrop: vertical gradient strips from near-black to white, plus an
	// orange diamond and a teal bar so cross-color effects are visible.
	int strips = 24;
	for (int i = 0; i < strips; ++i) {
		float f = (float)i / (float)(strips - 1);
		float sy0 = y0 + h * (float)i / (float)strips;
		float sy1 = y0 + h * (float)(i + 1) / (float)strips;
		draw_push_color(make_color(f, f, f));
		draw_quad_fill(make_aabb(V2(x0, sy0), V2(x0 + w, sy1)), 0);
		draw_pop_color();
	}
	draw_push_color(make_color(0.95f, 0.55f, 0.1f));
	draw_push();
	draw_translate(cx, y0 + h * 0.72f);
	draw_rotate(CF_PI * 0.25f);
	draw_quad_fill(make_aabb(V2(-42, -42), V2(42, 42)), 6);
	draw_pop();
	draw_pop_color();
	draw_push_color(make_color(0.1f, 0.6f, 0.6f));
	draw_quad_fill(make_aabb(V2(x0 + 20, y0 + h * 0.22f), V2(x0 + w - 20, y0 + h * 0.32f)), 8);
	draw_pop_color();

	// The same RGB venn circles in this panel's blend mode, gently orbiting.
	float r = 62;
	float o = 34 + sinf(t) * 6.0f;
	v2 c = V2(cx, y0 + h * 0.5f);
	draw_push_blend(mode);
	draw_push_color(make_color(1.0f, 0.15f, 0.15f, 0.75f));
	draw_circle_fill(c + V2(0, o), r);
	draw_pop_color();
	draw_push_color(make_color(0.15f, 1.0f, 0.15f, 0.75f));
	draw_circle_fill(c + V2(-o * 0.87f, -o * 0.5f), r);
	draw_pop_color();
	draw_push_color(make_color(0.3f, 0.3f, 1.0f, 0.75f));
	draw_circle_fill(c + V2(o * 0.87f, -o * 0.5f), r);
	draw_pop_color();
	draw_pop_blend();

	// Label.
	push_font_size(24);
	draw_push_color(color_white());
	v2 size = text_size(label);
	draw_text(label, V2(cx - size.x * 0.5f, y0 - 16), -1);
	draw_pop_color();
	pop_font_size();
}

int main(int argc, char* argv[])
{
	bool screenshot = argc > 1 && CF_STRCMP(argv[1], "screenshot") == 0;
	const char* screenshot_path = argc > 2 ? argv[2] : "blend_modes.png";
	int options = CF_APP_OPTIONS_WINDOW_POS_CENTERED_BIT | (screenshot ? CF_APP_OPTIONS_HIDDEN_BIT : CF_APP_OPTIONS_RESIZABLE_BIT);
	CF_Result result = make_app("Blend Modes", 0, 0, 0, 1280, 720, options, argv[0]);
	if (is_error(result)) return -1;
	if (!screenshot) cf_app_set_present_mode(CF_PRESENT_MODE_IMMEDIATE);
	cf_clear_color(0.10f, 0.10f, 0.14f, 1.0f);

	float t = screenshot ? 0.8f : 0;

	do {
		app_update();
		t += CF_DELTA_TIME;

		s_panel(-480, CF_DRAW_BLEND_NORMAL, "NORMAL", t);
		s_panel(-160, CF_DRAW_BLEND_ADD, "ADD", t);
		s_panel(160, CF_DRAW_BLEND_MULTIPLY, "MULTIPLY", t);
		s_panel(480, CF_DRAW_BLEND_SCREEN, "SCREEN", t);

		draw_push_color(color_white());
		push_font_size(16);
		draw_text("cf_draw_push_blend: identical panels, only the circles' blend mode differs -- all one paint-ordered stream", V2(-620, 348), -1);
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
