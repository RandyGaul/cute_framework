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
	push_font("ProggyClean");
	draw_push_antialias(true);

	while (app_is_running()) {
		float dt = calc_dt();
		app_update(dt);

		static float t = 0;
		t += dt;

		v2 o = V2(cosf(t),sinf(t)) * 25.0f;
		Aabb clip = make_aabb(V2(-100,-75) + o, V2(75,75) + o);
		draw_quad(clip, 0);
		push_text_clip_box(clip);
		push_font_size(13);
		push_text_wrap_width(100.0f + cosf(t) * 75.0f);
		cf_draw_line(V2(cosf(t) * 75.0f,0), V2(cosf(t) * 75.0f,-75), 0);
		draw_text("The quick brown fox jumps over the lazy dog. 1234567890", V2(-100,0));
		pop_text_wrap_width();
		pop_font_size();
		pop_text_clip_box();

		push_font_size(26);
		draw_push_color(make_color(0x55b6f2ff));
		draw_text("Some bigger and blue text.", V2(-100,-30));
		draw_pop_color();
		pop_font_size();

		push_font_size(13 * 5);
		push_font_blur(10);
		draw_text("glowing~", V2(-150-10,-90+10));
		pop_font_blur();
		draw_text("glowing~", V2(-150,-90));
		pop_font_size();

		push_font_size(13 * 10);
		push_font_blur(10);
		draw_push_color(color_black());
		draw_text("shadow", V2(-150-10-2.5f,-200+5));
		draw_pop_color();
		pop_font_blur();
		draw_text("shadow", V2(-150,-200));
		pop_font_size();

		String draws;
		draws.fmt("Draw calls: %d", draw_calls);
		push_font_size(13);
		draw_text(draws.c_str(), V2(-w/2 + 10,-h/2 + 10));
		pop_font_size();

		draw_calls = app_present();
	}

	destroy_app();

	return 0;
}
