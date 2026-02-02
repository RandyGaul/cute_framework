#include <cute.h>
using namespace Cute;

enum ScaleMode
{
	SCALE_MODE_LETTERBOX,
	SCALE_MODE_CROP,
	SCALE_MODE_STRETCH,
	SCALE_MODE_COUNT,
};

const char* scale_mode_names[] = {
	"Letterbox",
	"Crop",
	"Stretch",
};

static CF_V2 calculate_dest_size(int game_w, int game_h, int window_w, int window_h, ScaleMode scale_mode)
{
	float game_aspect = (float)game_w / (float)game_h;
	float window_aspect = (float)window_w / (float)window_h;

	float dest_w, dest_h;

	switch (scale_mode) {
	case SCALE_MODE_STRETCH:
		dest_w = (float)window_w;
		dest_h = (float)window_h;
		break;

	case SCALE_MODE_LETTERBOX:
		if (window_aspect > game_aspect) {
			// Window is wider - pillarbox (black bars on sides).
			dest_h = (float)window_h;
			dest_w = dest_h * game_aspect;
		} else {
			// Window is taller - letterbox (black bars on top/bottom).
			dest_w = (float)window_w;
			dest_h = dest_w / game_aspect;
		}
		break;

	case SCALE_MODE_CROP:
		if (window_aspect > game_aspect) {
			// Window is wider - fill width, crop top/bottom.
			dest_w = (float)window_w;
			dest_h = dest_w / game_aspect;
		} else {
			// Window is taller - fill height, crop sides.
			dest_h = (float)window_h;
			dest_w = dest_h * game_aspect;
		}
		break;

	default:
		dest_w = (float)window_w;
		dest_h = (float)window_h;
		break;
	}

	return V2(dest_w, dest_h);
}

int main(int argc, char* argv[])
{
	// Fixed game resolution.
	int game_w = 320;
	int game_h = 240;

	// Initial window size (can be resized).
	int window_w = 640;
	int window_h = 480;

	make_app("Window Resizing", 0, 0, 0, window_w, window_h, CF_APP_OPTIONS_WINDOW_POS_CENTERED_BIT | CF_APP_OPTIONS_RESIZABLE_BIT, argv[0]);

	// Create a fixed-size canvas to draw to.
	CF_Canvas canvas = make_canvas(canvas_defaults(game_w, game_h));

	// Set up projection for the game canvas.
	draw_projection(ortho_2d(0, 0, (float)game_w, (float)game_h));

	ScaleMode scale_mode = SCALE_MODE_LETTERBOX;

	while (app_is_running()) {
		app_update();

		// Toggle scale mode with space bar.
		if (key_just_pressed(CF_KEY_SPACE)) {
			scale_mode = (ScaleMode)((scale_mode + 1) % SCALE_MODE_COUNT);
		}

		// Draw some shapes to visualize the game area (centered coordinates).
		float hw = game_w / 2.0f;
		float hh = game_h / 2.0f;
		draw_push_color(color_grey());
		draw_box(CF_Aabb { -hw, -hh, hw, hh }, 4.0f, 0);
		draw_pop_color();
		draw_circle(V2(0, 0), 50.0f, 5.0f);
		draw_line(V2(-hw, -hh), V2(hw, hh), 2.0f);
		draw_line(V2(hw, -hh), V2(-hw, hh), 2.0f);

		// Draw instructions.
		draw_push_color(color_white());
		char buf[128];
		snprintf(buf, sizeof(buf), "Mode: %s", scale_mode_names[scale_mode]);
		draw_text(buf, V2(-hw + 10, hh - 20.0f), -1);
		draw_text("Press SPACE to cycle modes", V2(-hw + 10, hh - 40.0f), -1);
		draw_pop_color();

		// Draw game content to the canvas.
		render_to(canvas, true);

		// Calculate scaled size based on current mode.
		app_get_size(&window_w, &window_h);
		app_set_canvas_size(window_w, window_h);
		CF_V2 dest = calculate_dest_size(game_w, game_h, window_w, window_h, scale_mode);

		// Draw the canvas scaled and centered in the window.
		draw_projection(ortho_2d(0, 0, (float)window_w, (float)window_h));
		draw_canvas(canvas, V2(0,0), dest);

		// Restore projection for next frame.
		draw_projection(ortho_2d(0, 0, (float)game_w, (float)game_h));

		app_draw_onto_screen(true);
	}

	destroy_canvas(canvas);
	destroy_app();

	return 0;
}
