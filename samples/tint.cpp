#include <cute.h>
using namespace Cute;

int main(int argc, const char** argv)
{
	int options = APP_OPTIONS_DEFAULT_GFX_CONTEXT | APP_OPTIONS_WINDOW_POS_CENTERED;
	Result result = make_app("Tint Demo", 0, 0, 640, 480, options, argv[0]);
	if (is_error(result)) return -1;

	Sprite s = cf_make_sprite("test_data/girl.aseprite");
	s.play("spin");

	camera_dimensions(640/4, 480/4);
	draw_push_color(make_color(0xeba48bff));
	draw_push_tint(color_purple());
	draw_push_antialias(true);
	float t = 0;

	while (app_is_running()) {
		app_update();
		s.update();
		s.draw();
		t += DELTA_TIME;
		float radius = 10.0f;
		float motion = (sinf(t) + 1.0f) * 0.5f * 10.0f;
		draw_push_layer(-1);
		draw_circle(V2(0,10), radius + motion, 50, 1.0f + motion / 4);
		draw_pop_layer();
		if (key_just_pressed(KEY_1)) draw_push_tint(color_grey());
		if (key_just_pressed(KEY_2)) draw_push_tint(color_red());
		if (key_just_pressed(KEY_3)) draw_push_tint(color_purple());
		if (key_just_pressed(KEY_4)) draw_push_tint(color_orange());
		if (key_just_pressed(KEY_5)) draw_push_tint(color_green());
		if (key_just_pressed(KEY_6)) draw_push_tint(color_white());
		if (key_just_pressed(KEY_7)) draw_push_tint(color_black());
		if (key_just_pressed(KEY_8)) draw_push_tint(color_blue());
		app_draw_onto_screen();
	}

	destroy_app();

	return 0;
}
