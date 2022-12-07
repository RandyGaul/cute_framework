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

	while (app_is_running()) {
		float dt = calc_dt();
		app_update(dt);
		s.update(dt);
		s.draw();
		draw_circle(V2(0,30),10,10,5);
		if (key_was_pressed(KEY_ANY)) draw_pop_tint();
		if (key_was_pressed(KEY_1)) draw_push_tint(color_red());
		if (key_was_pressed(KEY_2)) draw_push_tint(color_purple());
		if (key_was_pressed(KEY_3)) draw_push_tint(color_orange());
		if (key_was_pressed(KEY_4)) draw_push_tint(color_green());
		if (key_was_pressed(KEY_5)) draw_push_tint(color_white());
		if (key_was_pressed(KEY_6)) draw_push_tint(color_black());
		if (key_was_pressed(KEY_7)) draw_push_tint(color_blue());
		app_present();
	}

	destroy_app();

	return 0;
}
