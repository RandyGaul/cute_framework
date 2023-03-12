#include <cute.h>
using namespace Cute;

int main(int argc, char* argv[])
{
	int options = APP_OPTIONS_DEFAULT_GFX_CONTEXT | APP_OPTIONS_WINDOW_POS_CENTERED;
	Result result = make_app("Basic Sprite", 0, 0, 640, 480, options, argv[0]);
	if (is_error(result)) return -1;

	Sprite girl_sprite = cf_make_sprite("test_data/girl.aseprite");
	girl_sprite.play("spin");
	girl_sprite.scale = V2(4,4);

	while (app_is_running()) {
		app_update();

		girl_sprite.update();
		girl_sprite.draw();

		app_draw_onto_screen();
	}

	destroy_app();

	return 0;
}
