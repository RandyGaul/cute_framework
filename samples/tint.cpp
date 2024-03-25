#include <cute.h>
using namespace Cute;

int main(int argc, char* argv[])
{
	int options = APP_OPTIONS_DEFAULT_GFX_CONTEXT | APP_OPTIONS_WINDOW_POS_CENTERED;
	Result result = make_app("Tint Demo", 0, 0, 640, 480, options, argv[0]);
	if (is_error(result)) return -1;

	Sprite s = cf_make_demo_sprite();
	s.play("idle");
	float t = 0;

	while (app_is_running()) {
		app_update();

		camera_dimensions(640/4, 480/4);
		draw_push_antialias(true);

		static int which = 0;
		if (key_just_pressed(KEY_1))
			which = 1;
		if (key_just_pressed(KEY_2)) which = 2;
		if (key_just_pressed(KEY_3)) which = 3;
		if (key_just_pressed(KEY_4)) which = 4;
		if (key_just_pressed(KEY_5)) which = 5;
		if (key_just_pressed(KEY_6)) which = 6;
		if (key_just_pressed(KEY_7)) which = 7;
		if (key_just_pressed(KEY_8)) which = 8;
		switch (which) {
		case 1: draw_push_tint(color_grey()); break;
		case 2: draw_push_tint(color_red()); break;
		case 3: draw_push_tint(color_purple()); break;
		case 4: draw_push_tint(color_orange()); break;
		case 5: draw_push_tint(color_green()); break;
		case 6: draw_push_tint(color_white()); break;
		case 7: draw_push_tint(color_black()); break;
		case 8: draw_push_tint(color_blue()); break;
		}

		s.update();
		s.draw();
		t += DELTA_TIME;
		float radius = 10.0f;
		float motion = (sinf(t) + 1.0f) * 0.5f * 10.0f;
		draw_push_layer(-1);
		draw_circle(V2(0,10), radius + motion, 1.0f + motion / 4);
		draw_pop_layer();
		app_draw_onto_screen();
	}

	destroy_app();

	return 0;
}
