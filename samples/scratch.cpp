#include <cute.h>
using namespace Cute;

#include <imgui.h>

int main(int argc, const char** argv)
{
	int w = 640/1;
	int h = 480/1;
	int options = APP_OPTIONS_DEFAULT_GFX_CONTEXT | APP_OPTIONS_WINDOW_POS_CENTERED | APP_OPTIONS_RESIZABLE;
	Result result = make_app("Development Scratch", 0, 0, w, h, options, argv[0]);
	if (is_error(result)) return -1;

	app_init_imgui();

	camera_dimensions((float)w, (float)h);
	int draw_calls = 0;

	Sprite s = cf_make_sprite("test_data/girl.aseprite");
	s.scale.x = 2.0f;
	s.scale.y = 2.0f;
	s.play("spin");

	float fps = 0;
	bool pause = false;
	float pause_t = 0;

	while (app_is_running()) {
		app_update();
	}

	destroy_app();

	return 0;
}
