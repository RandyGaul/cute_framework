#include <cute.h>
using namespace Cute;

int main(int argc, const char** argv)
{
	int options = APP_OPTIONS_DEFAULT_GFX_CONTEXT | APP_OPTIONS_WINDOW_POS_CENTERED | APP_OPTIONS_RESIZABLE;
	Result result = make_app("Development Scratch", 0, 0, 640, 480, options, argv[0]);
	if (is_error(result)) return -1;

	float w = 640/1;
	float h = 480/1;
	camera_dimensions(w, h);

	Font font = make_font("test_data/ProggyClean.ttf");
	font_add_codepoints(font, ascii_latin());
	font_build(font, 13);

	while (app_is_running()) {
		float dt = calc_dt();
		app_update(dt);

		cf_draw_text("The quick brown fox jumps over the lazy dog. 1234567890", font, V2(-100,0));

		app_present();
	}

	destroy_app();

	return 0;
}
