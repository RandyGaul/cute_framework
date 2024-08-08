#include <cute.h>
using namespace Cute;

#include "proggy.h"
#include "sample_text.h"

int main(int argc, char* argv[])
{
	int options = APP_OPTIONS_WINDOW_POS_CENTERED | APP_OPTIONS_RESIZABLE;
	Result result = make_app("Text Drawing", 0, 0, 0, 640, 480, options, argv[0]);
	if (is_error(result)) return -1;

	draw_push_antialias(true);
	make_font_from_memory(proggy_data, proggy_sz, "ProggyClean");
	set_fixed_timestep();
	int draw_calls = 0;

	const char* sample = (const char*)sample_text_data;

	while (app_is_running()) {
		app_update();

		static float t = 0;
		t += DELTA_TIME;

		push_font("ProggyClean");

		// Clip text within a box.
		v2 o = V2(cosf(t),sinf(t)) * 25.0f;
		Aabb clip = make_aabb(V2(-75,-75) + o, V2(75,50) + o);
		draw_quad(clip, 0);
		push_text_clip_box(clip);
		push_font_size(13);
		draw_text("Clip this text within a box.", V2(-100,0));
		pop_text_clip_box();

		// Draw text with a limited width.
		push_font_size(13);
		push_text_wrap_width(100.0f + cosf(t) * 75.0f);
		o = V2(-200, 150);
		cf_draw_line(V2(cosf(t) * 75.0f,0) + o, V2(cosf(t) * 75.0f,-75) + o, 0);
		draw_text("This text width is animating over time and wrapping the words dynamically.", V2(-100,0) + o);
		pop_text_wrap_width();

		// Draw utf8 encoded text loaded from a text file.
		push_font("Calibri");
		push_font_size(20);
		draw_text(sample, V2(-300,200));
		pop_font_size();

		// Coloring text.
		push_font_size(26);
		draw_push_color(make_color(0x55b6f2));
		draw_text("Some bigger and blue text.", V2(-100,150));
		draw_pop_color();

		// Using font blurring for a glowing effect.
		push_font_size(13 * 5);
		push_font_blur(10);
		draw_text("glowing~", V2(-200-10,-90+10));
		pop_font_blur();
		draw_text("<fade>glowing~</fade>", V2(-200,-90));

		// Using font blurring for a shadow effect.
		push_font_size(13 * 5);
		push_font_blur(10);
		draw_push_color(color_black());
		draw_text("shadow", V2(-150-10-2.5f,-150+5));
		draw_pop_color();
		pop_font_blur();
		draw_text("shadow", V2(-150,-150));

		// Drawing a formatted string.
		String draws = String::fmt("Draw calls: %d", draw_calls);
		push_font_size(13);
		draw_text(draws.c_str(), V2(-640/2.0f + 10,-480/2.0f + 20));

		// Text shake effect.
		// For moving text it helps to round positions.
		push_font_size(30);
		draw_text("Some <shake freq=35 x=2 y=2>shaking</shake> text.", round(V2(sinf(t*0.25f)*100,cosf(t*0.25f)*100)));

		draw_calls = app_draw_onto_screen();
	}

	destroy_app();

	return 0;
}
