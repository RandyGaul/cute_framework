#include <cute.h>
using namespace Cute;

int main(int argc, char* argv[])
{
	Result result = make_app("Basic Sprite", 0, 0, 0, 640, 480, APP_OPTIONS_WINDOW_POS_CENTERED_BIT, argv[0]);
	if (is_error(result)) return -1;

	Sprite girl = cf_make_demo_sprite();
	girl.play("spin");
	girl.scale = V2(4,4);

	while (app_is_running()) {
		app_update();

		girl.update();
		girl.draw();

		app_draw_onto_screen(true);
	}

	destroy_app();

	return 0;
}
