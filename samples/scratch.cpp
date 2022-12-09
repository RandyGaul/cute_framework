#include <cute.h>
using namespace Cute;

int main(int argc, const char** argv)
{
	int options = APP_OPTIONS_DEFAULT_GFX_CONTEXT | APP_OPTIONS_WINDOW_POS_CENTERED | APP_OPTIONS_RESIZABLE;
	Result result = make_app("Development Scratch", 0, 0, 640, 480, options, argv[0]);
	if (is_error(result)) return -1;

	float t = 0;
	float w = 640/4;
	float h = 480/4;
	camera_dimensions(w, h);
	draw_push_antialias(true);

	Sprite s = cf_make_sprite("test_data/girl.aseprite");
	s.play("spin");

	while (app_is_running()) {
		float dt = calc_dt();
		app_update(dt);

		if (app_was_resized()) {
			int x, y;
			app_get_size(&x, &y);
			cf_app_resize_canvas(x, y);
		}

		t += dt;
		camera_look_at((cosf(t)+1)*0.5f * w/8, (sinf(t)+1)*0.5f * h/8);
		camera_rotate(t*2);
		draw_circle(V2(0, -20), 7, 20, 0);
		s.update(dt);
		s.draw();

		app_present();
	}

	destroy_app();

	return 0;
}
