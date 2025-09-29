#include <cute.h>
using namespace Cute;

int main(int argc, char* argv[])
{
	CF_Result result = make_app("Basic Sprite", 0, 0, 0, 640, 480, CF_APP_OPTIONS_WINDOW_POS_CENTERED_BIT, argv[0]);
	if (is_error(result)) return -1;

	CF_Sprite girl = cf_make_demo_sprite();

	sprite_play(girl, "spin");
	girl.scale = V2(4,4);

	while (app_is_running()) {
		app_update();

		sprite_update(girl);
		sprite_draw(girl);

		app_draw_onto_screen(true);
	}

	destroy_app();

	return 0;
}
