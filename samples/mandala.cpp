/*
	Mandala -- a shape/sprite stress test that's nice to look at.

	Thousands of translucent shapes arranged on a golden-angle phyllotaxis spiral, three
	counter-rotating capsule rings layering moire overdraw on top, an orbit of animated
	sprites, and a CSG blob centerpiece. Everything batches into a handful of draw calls
	through the command renderer.

	SPACE cycles the renderer mode (auto/instanced/tiled), 1/2 halves/doubles the shape
	count. The overlay shows frame time and per-frame batch stats.

	Run with `mandala screenshot out.png` to render one frame to a png and exit.
*/

#include <cute.h>
#include <stdio.h>
#include <internal/cute_draw_internal.h> // Renderer mode toggles + batch stats.

using namespace Cute;

int main(int argc, char* argv[])
{
	bool screenshot = argc > 1 && CF_STRCMP(argv[1], "screenshot") == 0;
	const char* screenshot_path = argc > 2 ? argv[2] : "mandala.png";
	int options = CF_APP_OPTIONS_WINDOW_POS_CENTERED_BIT | (screenshot ? CF_APP_OPTIONS_HIDDEN_BIT : CF_APP_OPTIONS_RESIZABLE_BIT);
	CF_Result result = make_app("Mandala -- SPACE cycles renderer, 1/2 shape count", 0, 0, 0, 1280, 720, options, argv[0]);
	if (is_error(result)) return -1;
	if (!screenshot) cf_app_set_present_mode(CF_PRESENT_MODE_IMMEDIATE);

	CF_Sprite girl = cf_make_demo_sprite();
	sprite_play(girl, "spin");
	girl.scale = V2(2, 2);

	int count = 3000;
	int mode = 0; // 0 = auto, 1 = instanced, 2 = tiled.
	const char* mode_names[3] = { "AUTO", "INSTANCED", "TILED" };

	double avg_ms = 0;
	float t = screenshot ? 5.0f : 0;
	do {
		app_update();
		t += CF_DELTA_TIME;
		avg_ms = avg_ms * 0.95 + (double)CF_DELTA_TIME * 1000.0 * 0.05;

		if (key_just_pressed(CF_KEY_SPACE)) mode = (mode + 1) % 3;
		if (key_just_pressed(CF_KEY_1)) count = max(count / 2, 100);
		if (key_just_pressed(CF_KEY_2)) count = min(count * 2, 48000);
		if (mode == 0) cf_draw_set_tiled_auto();
		else cf_draw_set_tiled_enabled(mode == 2);

		// Deep-space backdrop.
		draw_push_color(make_color(0x0b0e1d));
		draw_box_fill(make_aabb(V2(-640, -360), V2(640, 360)));
		draw_pop_color();

		// Phyllotaxis spiral: each item sits at the golden angle from its neighbor, so
		// the field self-organizes into interleaved clockwise/counterclockwise arms.
		// Everything is translucent -- the pattern's look IS blended overdraw.
		const float golden = 2.399963f;
		float breathe = 1.0f + 0.06f * sinf(t * 0.7f);
		for (int i = 0; i < count; ++i) {
			float a = (float)i * golden + t * 0.1f;
			float r = 7.5f * sqrtf((float)i) * breathe;
			if (r > 700.0f) break;
			v2 p = V2(cosf(a), sinf(a)) * r;
			float s = (4.0f + 9.0f * (0.5f + 0.5f * sinf(t * 2.0f + (float)i * 0.015f))) * (0.35f + r / 700.0f);
			float hue = fmodf((float)i * 0.61803f + t * 0.02f, 1.0f);
			CF_Color c = hsv_to_rgb(make_color(hue, 0.75f, 1.0f));
			c.a = 0.35f;
			draw_push_color(c);
			switch (i % 4) {
			case 0: cf_draw_circle_fill2(p, s); break;
			case 1: {
				// Box rotated with the spiral arm -- pinwheel instead of grid-aligned.
				v2 u = V2(cosf(a * 0.5f + t * 0.3f), sinf(a * 0.5f + t * 0.3f)) * s;
				v2 v = skew(u);
				cf_draw_quad_fill2(p - u - v, p + u - v, p + u + v, p - u + v, s * 0.4f);
			}	break;
			case 2: {
				// Capsule along the spiral tangent.
				v2 tangent = norm(V2(-sinf(a), cosf(a))) * s;
				cf_draw_capsule_fill2(p - tangent, p + tangent, s * 0.45f);
			}	break;
			case 3: {
				v2 up = V2(cosf(a + t), sinf(a + t)) * s;
				v2 side = skew(up);
				cf_draw_tri_fill(p + up, p - up * 0.5f + side, p - up * 0.5f - side, 2.0f);
			}	break;
			}
			draw_pop_color();
		}

		// Three counter-rotating capsule rings: thin translucent spokes crossing each
		// other, layering moire interference over the spiral.
		for (int ring = 0; ring < 3; ++ring) {
			float rr = 180.0f + 140.0f * (float)ring;
			float spin = t * 0.12f * ((ring & 1) ? -1.0f : 1.0f) + (float)ring;
			int spokes = 48 + ring * 16;
			CF_Color rc = hsv_to_rgb(make_color(fmodf(0.55f + 0.13f * (float)ring, 1.0f), 0.5f, 1.0f));
			rc.a = 0.22f;
			draw_push_color(rc);
			for (int k = 0; k < spokes; ++k) {
				float ka = spin + (float)k * (2.0f * CF_PI / (float)spokes);
				v2 dir = V2(cosf(ka), sinf(ka));
				cf_draw_capsule_fill2(dir * (rr - 55.0f), dir * (rr + 55.0f), 3.0f);
			}
			draw_pop_color();
		}

		// A ring of animated sprites weaving through the shapes.
		sprite_update(girl);
		for (int k = 0; k < 24; ++k) {
			float ka = t * 0.25f + (float)k * (2.0f * CF_PI / 24.0f);
			float kr = 260.0f + 40.0f * sinf(t + (float)k * 0.7f);
			girl.transform.p = V2(cosf(ka), sinf(ka)) * kr;
			cf_draw_sprite(&girl);
		}

		// Centerpiece: a CSG metaball trio melted into one shape with a composite outline.
		v2 b0 = V2(cosf(t * 0.9f), sinf(t * 1.2f)) * 44.0f;
		v2 b1 = V2(cosf(t * 0.6f + 2.1f), sinf(t * 0.8f + 1.0f)) * 52.0f;
		v2 b2 = V2(cosf(t * 1.4f + 4.2f), sinf(t * 0.5f + 3.0f)) * 40.0f;
		for (int pass = 0; pass < 2; ++pass) {
			draw_push_color(pass == 0 ? make_color(0.1f, 0.9f, 0.75f, 0.9f) : make_color(1.0f, 1.0f, 1.0f, 0.9f));
			draw_shape_group_begin();
			cf_draw_circle_fill2(b0, 30);
			draw_shape_group_op(CF_SHAPE_OP_UNION, 22.0f);
			cf_draw_circle_fill2(b1, 22);
			cf_draw_circle_fill2(b2, 18);
			if (pass == 0) draw_shape_group_end();
			else draw_shape_group_end_stroked(2.5f);
			draw_pop_color();
		}

		// Stats overlay.
		{
			int tiled_batches, instanced_batches;
			uint64_t upload;
			cf_draw_tiled_stats(&tiled_batches, &instanced_batches, &upload);
			char buf[256];
			snprintf(buf, sizeof(buf), "%s  |  %d shapes + 24 sprites  |  %.2f ms  |  batches tiled %d / instanced %d  |  upload %d KB",
				mode_names[mode], count, avg_ms, tiled_batches, instanced_batches, (int)(upload / 1024));
			draw_push_color(color_white());
			push_font_size(16);
			draw_text(buf, V2(-620, 348), -1);
			pop_font_size();
			draw_pop_color();
		}

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
