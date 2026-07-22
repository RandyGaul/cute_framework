/*
	Draw lists -- record a static scene once, replay it every frame.

	A night skyline viewed from the horizon: thousands of lit windows across three
	parallax depth layers plus a starfield, all recorded into draw lists once at
	startup and replayed per frame under a camera that slowly zooms from street level
	waaay out to reveal some absurdly tall megatowers and a giant moon with a ring of
	orbiting debris. The moon, its orbiters, and the twinkling stars are live (drawn
	immediate-mode every frame) to show recorded and dynamic content mixing freely --
	blend modes included: additive glows and twinkles, multiply craters, screen halo.

	SPACE toggles between replaying the lists and re-recording all static layers
	immediate-mode every frame -- watch the record-time number collapse.

	Run with `draw_lists screenshot out.png` to render one frame to a png and exit.
	Run with `draw_lists bench` to print immediate vs replay record times and exit.
*/

#include <cute.h>
#include <stdio.h>

using namespace Cute;

#define GROUND_Y -350.0f

// Rolling terrain: layered sines give each depth layer its own hills to climb.
static float s_hill(float x, float amp, float wavelength, float phase)
{
	return amp * (sinf(x / wavelength + phase) * 0.6f + sinf(x / (wavelength * 0.37f) + phase * 1.7f) * 0.4f);
}

// One building: body, optional stepped crown and antenna, and a grid of lit windows.
static void s_building(CF_Rnd* rnd, float x, float ground, float w, float h, CF_Color body, bool windows, float lit_chance)
{
	float y0 = ground;
	float y1 = ground + h;
	draw_push_color(body);
	draw_quad_fill(make_aabb(V2(x - w, y0), V2(x + w, y1)), 0);
	// Stepped crown on taller buildings.
	if (h > 900 && cf_rnd_range_float(rnd, 0, 1) < 0.6f) {
		draw_quad_fill(make_aabb(V2(x - w * 0.6f, y1), V2(x + w * 0.6f, y1 + h * 0.06f)), 0);
		draw_quad_fill(make_aabb(V2(x - w * 0.3f, y1 + h * 0.06f), V2(x + w * 0.3f, y1 + h * 0.1f)), 0);
	}
	draw_pop_color();
	// Antenna with a warm beacon.
	if (h > 1200 && cf_rnd_range_float(rnd, 0, 1) < 0.5f) {
		draw_push_color(body);
		draw_quad_fill(make_aabb(V2(x - 6, y1), V2(x + 6, y1 + h * 0.16f)), 0);
		draw_pop_color();
		draw_push_blend(CF_DRAW_BLEND_ADD);
		draw_push_color(make_color(0.9f, 0.2f, 0.15f));
		draw_circle_fill(V2(x, y1 + h * 0.16f), 14);
		draw_pop_color();
		draw_pop_blend();
	}
	if (!windows) return;
	// Window grid.
	float wx = 60;
	float wy = 85;
	int cols = (int)((w * 2.0f - 30.0f) / wx);
	int rows = (int)((h - 60.0f) / wy);
	for (int r = 0; r < rows; ++r) {
		for (int c = 0; c < cols; ++c) {
			float roll = cf_rnd_range_float(rnd, 0, 1);
			if (roll > lit_chance) continue;
			CF_Color wc = roll < lit_chance * 0.8f ? make_color(0.95f, 0.85f, 0.5f) : make_color(0.5f, 0.85f, 0.95f);
			wc.a = cf_rnd_range_float(rnd, 0.5f, 1.0f);
			float cx = x - w + 25.0f + (c + 0.5f) * wx;
			float cy = ground + 40.0f + (r + 0.5f) * wy;
			draw_push_color(wc);
			draw_quad_fill(make_aabb(V2(cx - 14, cy - 20), V2(cx + 14, cy + 20)), 2);
			draw_pop_color();
		}
	}
}

// Layer 0: sky gradient + starfield.
static void s_layer_sky()
{
	int strips = 28;
	for (int i = 0; i < strips; ++i) {
		float f = (float)i / (float)(strips - 1);
		float y0 = GROUND_Y + f * 9000.0f;
		float y1 = GROUND_Y + (f + 1.0f / (strips - 1)) * 9000.0f;
		draw_push_color(make_color(0.03f + 0.05f * (1.0f - f), 0.03f + 0.04f * (1.0f - f), 0.10f + 0.06f * (1.0f - f)));
		draw_quad_fill(make_aabb(V2(-30000, y0), V2(30000, y1)), 0);
		draw_pop_color();
	}
	CF_Rnd rnd = cf_rnd_seed(11);
	for (int i = 0; i < 900; ++i) {
		float x = cf_rnd_range_float(&rnd, -28000, 28000);
		float y = cf_rnd_range_float(&rnd, GROUND_Y + 800, 9000);
		float b = cf_rnd_range_float(&rnd, 0.3f, 1.0f);
		draw_push_color(make_color(b, b, b * 0.95f));
		draw_circle_fill(V2(x, y), cf_rnd_range_float(&rnd, 2, 6));
		draw_pop_color();
	}
}

// Layer 1: far-away mountain ranges, two rows deep with faint snow caps.
static void s_layer_mountains()
{
	CF_Rnd rnd = cf_rnd_seed(88);
	for (int range = 0; range < 2; ++range) {
		CF_Color c = range == 0 ? make_color(0.095f, 0.10f, 0.185f) : make_color(0.075f, 0.08f, 0.155f);
		CF_Color cap = range == 0 ? make_color(0.155f, 0.165f, 0.25f) : make_color(0.125f, 0.135f, 0.215f);
		float x = -28000;
		while (x < 28000) {
			float w = cf_rnd_range_float(&rnd, 1800, 3600) * (range == 0 ? 1.5f : 1.0f);
			float h = cf_rnd_range_float(&rnd, 1400, range == 0 ? 4400 : 2900);
			v2 peak = V2(x + cf_rnd_range_float(&rnd, -0.25f, 0.25f) * w, GROUND_Y + h);
			v2 bl = V2(x - w, GROUND_Y - 300);
			v2 br = V2(x + w, GROUND_Y - 300);
			draw_push_color(c);
			cf_draw_tri_fill(bl, br, peak, 0);
			draw_pop_color();
			if (h > 2400) {
				float f = 0.24f; // Snow line: shrink the triangle toward the peak.
				v2 sl = V2(peak.x + (bl.x - peak.x) * f, peak.y + (bl.y - peak.y) * f);
				v2 sr = V2(peak.x + (br.x - peak.x) * f, peak.y + (br.y - peak.y) * f);
				draw_push_color(cap);
				cf_draw_tri_fill(sl, sr, peak, 0);
				draw_pop_color();
			}
			x += w * cf_rnd_range_float(&rnd, 0.7f, 1.1f);
		}
	}
}

// Layer 2: distant silhouettes, hazy blue, on gentle hills.
static void s_layer_far()
{
	CF_Rnd rnd = cf_rnd_seed(22);
	float x = -26000;
	while (x < 26000) {
		float w = cf_rnd_range_float(&rnd, 180, 520);
		float h = cf_rnd_range_float(&rnd, 400, 2600);
		s_building(&rnd, x, GROUND_Y + s_hill(x, 200, 6200, 1.3f), w, h, make_color(0.10f, 0.11f, 0.20f), false, 0);
		x += w * 2.0f + cf_rnd_range_float(&rnd, 20, 200);
	}
	// Haze band at the horizon.
	draw_push_blend(CF_DRAW_BLEND_SCREEN);
	draw_push_color(make_color(0.06f, 0.06f, 0.10f));
	draw_quad_fill(make_aabb(V2(-30000, GROUND_Y - 200), V2(30000, GROUND_Y + 900)), 0);
	draw_pop_color();
	draw_pop_blend();
}

// Layer 3: mid-distance towers with dim windows, rolling a bit deeper.
static void s_layer_mid()
{
	CF_Rnd rnd = cf_rnd_seed(33);
	float x = -22000;
	while (x < 22000) {
		float w = cf_rnd_range_float(&rnd, 140, 380);
		float h = cf_rnd_range_float(&rnd, 500, 3600);
		s_building(&rnd, x, GROUND_Y + s_hill(x, 300, 5200, 3.7f), w, h, make_color(0.085f, 0.09f, 0.15f), true, 0.22f);
		x += w * 2.0f + cf_rnd_range_float(&rnd, 40, 320);
	}
}

// Layer 4: foreground skyline with megatowers and bright windows, laid over hills.
static float s_near_hill(float x) { return s_hill(x, 380, 4600, 0.4f); }

static void s_layer_near()
{
	CF_Rnd rnd = cf_rnd_seed(44);
	float x = -20000;
	int i = 0;
	while (x < 20000) {
		float w = cf_rnd_range_float(&rnd, 120, 320);
		float h = cf_rnd_range_float(&rnd, 500, 2400);
		if (++i % 9 == 0) h = cf_rnd_range_float(&rnd, 4800, 7500); // Megatower.
		s_building(&rnd, x, GROUND_Y + s_near_hill(x), w, h, make_color(0.045f, 0.045f, 0.085f), true, 0.4f);
		x += w * 2.0f + cf_rnd_range_float(&rnd, 60, 420);
	}
	// Ground follows the hills: triangle strips from the terrain line down.
	float step = 240;
	for (float gx = -30000; gx < 30000; gx += step) {
		v2 t0 = V2(gx, GROUND_Y + s_near_hill(gx));
		v2 t1 = V2(gx + step, GROUND_Y + s_near_hill(gx + step));
		v2 b0 = V2(gx, GROUND_Y - 3300);
		v2 b1 = V2(gx + step, GROUND_Y - 3300);
		draw_push_color(make_color(0.03f, 0.03f, 0.05f));
		cf_draw_tri_fill(b0, t1, t0, 0);
		cf_draw_tri_fill(b0, b1, t1, 0);
		draw_pop_color();
	}
	// Streetlights ride the terrain.
	for (float lx = -20000; lx < 20000; lx += 420) {
		float ly = GROUND_Y + s_near_hill(lx);
		draw_push_color(make_color(0.35f, 0.33f, 0.28f));
		draw_quad_fill(make_aabb(V2(lx - 5, ly), V2(lx + 5, ly + 110)), 0);
		draw_pop_color();
		draw_push_blend(CF_DRAW_BLEND_ADD);
		draw_push_color(make_color(0.35f, 0.3f, 0.12f));
		draw_circle_fill(V2(lx, ly + 120), 34);
		draw_pop_color();
		draw_pop_blend();
	}
}

#define LAYER_COUNT 5
typedef void (*LayerFn)();
static LayerFn s_layers[LAYER_COUNT] = { s_layer_sky, s_layer_mountains, s_layer_far, s_layer_mid, s_layer_near };
static float s_parallax[LAYER_COUNT] = { 0.25f, 0.38f, 0.55f, 0.75f, 1.0f };

// Draws all static layers with per-layer parallax, from lists or immediate.
static void s_draw_static(CF_DrawList* lists, bool use_lists, float zoom, v2 cam)
{
	for (int i = 0; i < LAYER_COUNT; ++i) {
		draw_push();
		draw_scale(zoom, zoom);
		draw_translate(-cam.x * s_parallax[i], -cam.y * (0.6f + 0.4f * s_parallax[i]));
		if (use_lists) draw_list(lists[i]);
		else s_layers[i]();
		draw_pop();
	}
}

// The giant moon with craters, halo, and a ring of orbiting debris. Always live.
static void s_moon_and_orbiters(float t, float zoom, v2 cam)
{
	v2 moon = V2(3400, 4600);
	float R = 2400;
	draw_push();
	draw_scale(zoom, zoom);
	draw_translate(-cam.x * 0.4f, -cam.y * 0.75f);

	// Orbiters behind the moon (upper half of the tilted ring).
	CF_Rnd rnd = cf_rnd_seed(55);
	struct Orb { float a, r, size, speed; };
	for (int pass = 0; pass < 2; ++pass) {
		CF_Rnd orb_rnd = rnd;
		if (pass == 1) {
			// The moon itself, between the ring halves: screen halo, body, multiply craters.
			draw_push_blend(CF_DRAW_BLEND_SCREEN);
			for (int i = 0; i < 10; ++i) {
				float f = 0.055f - i * 0.005f;
				draw_push_color(make_color(f, f, f * 1.3f));
				draw_circle_fill(moon, R + 200.0f + i * 220.0f);
				draw_pop_color();
			}
			draw_pop_blend();
			draw_push_color(make_color(0.93f, 0.91f, 0.82f));
			draw_circle_fill(moon, R);
			draw_pop_color();
			draw_push_blend(CF_DRAW_BLEND_MULTIPLY);
			CF_Rnd crater_rnd = cf_rnd_seed(66);
			for (int i = 0; i < 14; ++i) {
				float ca = cf_rnd_range_float(&crater_rnd, 0, 6.283f);
				float cr = cf_rnd_range_float(&crater_rnd, 0, R * 0.8f);
				float cs = cf_rnd_range_float(&crater_rnd, R * 0.05f, R * 0.2f);
				draw_push_color(make_color(0.86f, 0.85f, 0.88f));
				draw_circle_fill(moon + V2(cosf(ca) * cr, sinf(ca) * cr * 0.9f), cs);
				draw_pop_color();
			}
			draw_pop_blend();
		}
		for (int i = 0; i < 56; ++i) {
			float a0 = cf_rnd_range_float(&orb_rnd, 0, 6.283f);
			float orbit = cf_rnd_range_float(&orb_rnd, R * 1.35f, R * 2.3f);
			float size = cf_rnd_range_float(&orb_rnd, 26, 80);
			float speed = cf_rnd_range_float(&orb_rnd, 0.06f, 0.3f);
			float a = a0 + t * speed;
			float z = sinf(a);
			if ((pass == 0) != (z > 0)) continue; // Back half first, front half after the moon.
			// Tilted elliptical ring.
			v2 ring = V2(cosf(a) * orbit, z * orbit * 0.30f);
			v2 p = moon + V2(ring.x * cosf(-0.3f) - ring.y * sinf(-0.3f), ring.x * sinf(-0.3f) + ring.y * cosf(-0.3f));
			float shade = z > 0 ? 0.7f : 0.45f;
			draw_push_color(make_color(0.55f * shade, 0.6f * shade, 0.75f * shade));
			draw_circle_fill(p, size);
			draw_pop_color();
			// Additive glint trailing the big ones.
			if (size > 60) {
				draw_push_blend(CF_DRAW_BLEND_ADD);
				draw_push_color(make_color(0.10f, 0.12f, 0.2f));
				draw_circle_fill(p, size * 2.2f);
				draw_pop_color();
				draw_pop_blend();
			}
		}
	}
	draw_pop();
}

// A few live twinkling stars over everything.
static void s_twinkles(float t, float zoom, v2 cam)
{
	draw_push();
	draw_scale(zoom, zoom);
	draw_translate(-cam.x * 0.25f, -cam.y * 0.7f);
	draw_push_blend(CF_DRAW_BLEND_ADD);
	CF_Rnd rnd = cf_rnd_seed(77);
	for (int i = 0; i < 50; ++i) {
		float x = cf_rnd_range_float(&rnd, -26000, 26000);
		float y = cf_rnd_range_float(&rnd, 2000, 8600);
		float phase = cf_rnd_range_float(&rnd, 0, 6.283f);
		float pulse = 0.5f + 0.5f * sinf(t * 2.0f + phase);
		draw_push_color(make_color(0.5f * pulse, 0.5f * pulse, 0.45f * pulse));
		draw_circle_fill(V2(x, y), 8.0f + pulse * 6.0f);
		draw_pop_color();
	}
	draw_pop_blend();
	draw_pop();
}

int main(int argc, char* argv[])
{
	bool screenshot = argc > 1 && CF_STRCMP(argv[1], "screenshot") == 0;
	bool bench = argc > 1 && CF_STRCMP(argv[1], "bench") == 0;
	const char* screenshot_path = argc > 2 ? argv[2] : "draw_lists.png";
	int options = CF_APP_OPTIONS_WINDOW_POS_CENTERED_BIT | (screenshot || bench ? CF_APP_OPTIONS_HIDDEN_BIT : CF_APP_OPTIONS_RESIZABLE_BIT);
	CF_Result result = make_app("Draw Lists -- SPACE toggles list replay vs immediate re-record", 0, 0, 0, 1280, 720, options, argv[0]);
	if (is_error(result)) return -1;
	if (!screenshot) cf_app_set_present_mode(CF_PRESENT_MODE_IMMEDIATE);
	cf_clear_color(0.02f, 0.025f, 0.06f, 1.0f);

	// Record each parallax layer into its own list, once.
	CF_DrawList lists[LAYER_COUNT];
	for (int i = 0; i < LAYER_COUNT; ++i) {
		lists[i] = make_draw_list();
		draw_list_begin(lists[i]);
		s_layers[i]();
		draw_list_end();
	}

	if (bench) {
		const int warmup = 30;
		const int frames = 200;
		for (int pass = 0; pass < 2; ++pass) {
			double record_ms = 0;
			for (int f = 0; f < warmup + frames; ++f) {
				app_update();
				uint64_t t0 = cf_get_ticks();
				s_draw_static(lists, pass == 1, 0.1f, V2(0, 1500));
				uint64_t t1 = cf_get_ticks();
				app_draw_onto_screen(true);
				if (f >= warmup) record_ms += (double)(t1 - t0) / (double)cf_get_tick_frequency() * 1000.0;
			}
			printf("%s: %.3f ms/frame record avg over %d frames\n", pass == 0 ? "IMMEDIATE " : "LIST REPLAY", record_ms / frames, frames);
		}
		for (int i = 0; i < 4; ++i) destroy_draw_list(lists[i]);
		destroy_app();
		return 0;
	}

	bool use_lists = true;
	float t = screenshot ? 19.0f : 0;
	double avg_ms = 0;

	do {
		app_update();
		t += CF_DELTA_TIME;
		if (key_just_pressed(CF_KEY_SPACE)) use_lists = !use_lists;

		// Slow breathing zoom: street level out to the whole skyline and back.
		float f = 0.5f - 0.5f * cosf(t * 0.15f);
		float fs = f * f * (3.0f - 2.0f * f); // Smoothstep the ends.
		float zoom = expf((1.0f - fs) * logf(0.85f) + fs * logf(0.052f));
		v2 cam = V2(sinf(t * 0.05f) * 5200.0f * (1.0f - fs * 0.7f), 500.0f + fs * 3600.0f);

		uint64_t t0 = cf_get_ticks();
		s_draw_static(lists, use_lists, zoom, cam);
		double ms = (double)(cf_get_ticks() - t0) / (double)cf_get_tick_frequency() * 1000.0;
		avg_ms = avg_ms * 0.95 + ms * 0.05;

		s_moon_and_orbiters(t, zoom, cam);
		s_twinkles(t, zoom, cam);

		draw_push_color(color_white());
		push_font_size(16);
		char buf[160];
		snprintf(buf, sizeof(buf), "%s  |  static record cost: %.3f ms/frame  |  SPACE toggles", use_lists ? "LIST REPLAY (recorded once)" : "IMMEDIATE (re-recorded every frame)", avg_ms);
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

	for (int i = 0; i < 4; ++i) destroy_draw_list(lists[i]);
	destroy_app();
	return 0;
}
