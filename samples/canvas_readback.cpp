#include <cute.h>
#include <cute/cute_png.h>

#define NUM_CIRCLES 12

typedef struct Circle
{
	CF_V2 pos;
	CF_V2 vel;
	float radius;
	CF_Color color;
} Circle;

int main(int argc, char* argv[])
{
	int w = 640;
	int h = 480;
	int options = CF_APP_OPTIONS_WINDOW_POS_CENTERED_BIT;
	CF_Result result = cf_make_app("Canvas Readback", 0, 0, 0, w, h, options, argv[0]);
	if (cf_is_error(result)) return -1;

	CF_Canvas offscreen = cf_make_canvas(cf_canvas_defaults(w, h));

	// Seed bouncing circles with random positions, velocities, colors, radii.
	CF_Rnd rnd = cf_rnd_seed(0xCAFE);
	Circle circles[NUM_CIRCLES];
	CF_Color palette[] = {
		cf_color_red(),
		cf_color_green(),
		cf_color_blue(),
		cf_color_yellow(),
		cf_color_orange(),
		cf_color_purple(),
		cf_color_cyan(),
		cf_color_magenta(),
		cf_color_white(),
	};
	int palette_count = sizeof(palette) / sizeof(palette[0]);
	float hw = w * 0.5f;
	float hh = h * 0.5f;
	for (int i = 0; i < NUM_CIRCLES; i++) {
		circles[i].radius = cf_rnd_range_float(&rnd, 15.0f, 40.0f);
		circles[i].pos = cf_v2(cf_rnd_range_float(&rnd, -hw + circles[i].radius, hw - circles[i].radius),
		                       cf_rnd_range_float(&rnd, -hh + circles[i].radius, hh - circles[i].radius));
		circles[i].vel = cf_v2(cf_rnd_range_float(&rnd, -150.0f, 150.0f),
		                       cf_rnd_range_float(&rnd, -150.0f, 150.0f));
		circles[i].color = palette[cf_rnd_range_int(&rnd, 0, palette_count - 1)];
	}

	bool readback_pending = false;
	CF_Readback readback = { 0 };
	float flash_timer = 0.0f;

	while (cf_app_is_running()) {
		cf_app_update(NULL);
		float dt = CF_DELTA_TIME;

		// Animate circles (bounce off edges).
		for (int i = 0; i < NUM_CIRCLES; i++) {
			circles[i].pos.x += circles[i].vel.x * dt;
			circles[i].pos.y += circles[i].vel.y * dt;
			float r = circles[i].radius;
			if (circles[i].pos.x - r < -hw) { circles[i].pos.x = -hw + r; circles[i].vel.x = -circles[i].vel.x; }
			if (circles[i].pos.x + r >  hw) { circles[i].pos.x =  hw - r; circles[i].vel.x = -circles[i].vel.x; }
			if (circles[i].pos.y - r < -hh) { circles[i].pos.y = -hh + r; circles[i].vel.y = -circles[i].vel.y; }
			if (circles[i].pos.y + r >  hh) { circles[i].pos.y =  hh - r; circles[i].vel.y = -circles[i].vel.y; }
		}

		// Draw circles to offscreen canvas.
		for (int i = 0; i < NUM_CIRCLES; i++) {
			cf_draw_push_color(circles[i].color);
			cf_draw_circle_fill2(circles[i].pos, circles[i].radius);
			cf_draw_pop_color();
		}

		// Draw "Copied to clipboard!" text briefly after capture.
		cf_draw_push_color(cf_color_white());
		const char* text = NULL;
		if (flash_timer > 0.0f) {
			flash_timer -= dt;
			text = "Copied to clipboard!";
		} else {
			text = "Press space to capture a screenshot.";
		}
		cf_draw_text(text, cf_v2(-cf_text_width(text, -1)*0.5f, 0), -1);
		cf_draw_pop_color();

		// Perform actual render to canvas. This captures our prior drawable commands.
		cf_render_to(offscreen, true);

		// Draw offscreen canvas to screen.
		cf_draw_canvas(offscreen, cf_v2(0, 0), cf_v2((float)w, (float)h));

		// If space just pressed: initiate readback.
		if (cf_key_just_pressed(CF_KEY_SPACE) && !readback_pending) {
			readback = cf_canvas_readback(offscreen);
			readback_pending = true;
		}

		// If pending readback is ready, encode to PNG and copy to clipboard.
		if (readback_pending && cf_readback_ready(readback)) {
			int pixel_size = cf_readback_size(readback);
			void* pixels = cf_alloc(pixel_size);
			cf_readback_data(readback, pixels, pixel_size);
			cf_destroy_readback(readback);
			readback_pending = false;

			// Encode to PNG.
			cp_image_t img;
			img.w = w;
			img.h = h;
			img.pix = (cp_pixel_t*)pixels;
			cp_saved_png_t png = cp_save_png_to_memory(&img);
			cf_free(pixels);

			if (png.data) {
				// Copy PNG to clipboard.
				const char* types[] = { "image/png" };
				cf_clipboard_set_data(png.data, png.size, types, 1);
				free(png.data);
				flash_timer = 2.0f;
			}
		}

		cf_app_draw_onto_screen(true);
	}

	if (readback_pending) {
		cf_destroy_readback(readback);
	}
	cf_destroy_canvas(offscreen);
	cf_destroy_app();

	return 0;
}
