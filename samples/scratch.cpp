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
	int draw_calls = 0;

	make_font("sample_data/ProggyClean.ttf", "ProggyClean");
	cf_draw_push_font("ProggyClean");

	while (app_is_running()) {
		float dt = calc_dt();
		app_update(dt);

		cf_draw_push_font_size(13);
		cf_draw_text("The quick brown fox jumps over the lazy dog. 1234567890", V2(-100,0));
		cf_draw_pop_font_size();

		cf_draw_push_font_size(26);
		cf_draw_push_color(make_color(0x55b6f2ff));
		cf_draw_text("Some bigger and blue text.", V2(-100,-30));
		cf_draw_pop_color();
		cf_draw_pop_font_size();

		cf_draw_push_font_size(13 * 5);
		cf_draw_push_font_blur(10);
		cf_draw_text("glowing~", V2(-150-10,-90+10));
		cf_draw_pop_font_blur();
		cf_draw_text("glowing~", V2(-150,-90));
		cf_draw_pop_font_size();

		cf_draw_push_font_size(13 * 10);
		cf_draw_push_font_blur(10);
		cf_draw_push_color(color_black());
		cf_draw_text("shadow", V2(-150-10-2.5f,-200+5));
		cf_draw_pop_color();
		cf_draw_pop_font_blur();
		cf_draw_text("shadow", V2(-150,-200));
		cf_draw_pop_font_size();

		String draws;
		draws.fmt("Draw calls: %d", draw_calls);
		cf_draw_push_font_size(13);
		cf_draw_text(draws.c_str(), V2(-w/2 + 10,-h/2 + 10));
		cf_draw_pop_font_size();

		draw_calls = app_present();
	}

	destroy_app();

	return 0;
}
