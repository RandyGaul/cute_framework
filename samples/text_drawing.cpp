#include <cute.h>
using namespace Cute;

#include "proggy.h"
#include "sample_text.h"

static bool draw_text_bound = false;
static bool change_text = false;
static int tab_press_count = 0;

static void draw_text_boxed(const char* text, v2 pos, int len = -1)
{
	draw_text(text, pos, len);

	if (draw_text_bound)
	{
		v2 size = text_size(text, len);
		draw_box(cf_make_aabb_from_top_left(pos, size.x, size.y), 0.f);
	}
}

int main(int argc, char* argv[])
{
	make_app("Text Drawing", 0, 0, 0, 640, 480, CF_APP_OPTIONS_WINDOW_POS_CENTERED_BIT | CF_APP_OPTIONS_RESIZABLE_BIT, argv[0]);

	draw_push_shape_aa(true);
	make_font_from_memory(proggy_data, proggy_sz, "ProggyClean");
	set_fixed_timestep();
	int draw_calls = 0;

	const char* sample = (const char*)sample_text_data;
	clear_color(0.5f, 0.5f, 0.5f, 1.0f);

	while (app_is_running()) {
		app_update();

		if (key_just_pressed(CF_KEY_SPACE)) {
			draw_text_bound = !draw_text_bound;
		}

		if (key_just_pressed(CF_KEY_TAB)) {
			++tab_press_count;
			change_text = !change_text;
		}

		push_font("ProggyClean");

		// Clip text within a box.
		v2 o = V2(cosf((float)CF_SECONDS),sinf((float)CF_SECONDS)) * 25.0f;

		// @TODO -- Fix + refactor scissor/viewport coordinate frame.
		//Aabb clip = make_aabb(V2(-75,-75) + o, V2(75,50) + o);
		//draw_quad(clip, 0);
		//push_text_clip_box(clip);
		//push_font_size(13);
		//draw_text("Clip this text within a box.", V2(-100,0));
		//pop_text_clip_box();

		// Draw text with a limited width.
		push_font_size(13);
		push_text_wrap_width(100.0f + cosf((float)CF_SECONDS) * 75.0f);
		o = V2(-200, 150);
		cf_draw_line(V2(cosf((float)CF_SECONDS) * 75.0f,0) + o, V2(cosf((float)CF_SECONDS) * 75.0f,-75) + o, 0);
		draw_text_boxed("This text width is animating over time and wrapping the words dynamically.", V2(-100,0) + o);
		pop_text_wrap_width();

		// Draw utf8 encoded text loaded from a text file.
		push_font("Calibri");
		push_font_size(20);
		draw_text_boxed(sample, V2(-300,200));
		pop_font_size();

		// Coloring text.
		push_font_size(26);
		draw_push_color(make_color(0x55b6f2));
		draw_text_boxed("Some bigger and blue text.", V2(-100,150));
		draw_pop_color();

		// Using font blurring for a glowing effect.
		push_font_size(13 * 5);
		push_font_blur(10);
		draw_text_boxed("glowing~", V2(-200-10,-90+10));
		pop_font_blur();
		draw_text_boxed("<fade>glowing~</fade>", V2(-200,-90));

		// Using font blurring for a shadow effect.
		push_font_size(13 * 5);
		push_font_blur(10);
		draw_push_color(color_black());
		draw_text_boxed("shadow", V2(-150-10-2.5f,-150+5));
		draw_pop_color();
		pop_font_blur();
		draw_text_boxed("shadow", V2(-150,-150));

		// Drawing a formatted string.
		String draws = String::fmt("Draw calls: %d", draw_calls);
		push_font_size(13);
		draw_text_boxed(draws.c_str(), V2(-640/2.0f + 10,-480/2.0f + 20));

		push_font_size(26);
		draw_text_boxed("Half-rendered effect <wave>groovy</wave>", V2(-230.f, 180.f), 25);
		pop_font_size();

		// Text shake effect.
		// For moving text it helps to round positions.
		push_font_size(30);
		draw_text_boxed("Some <shake freq=35 x=2 y=2>shaking</shake> text.", round(V2(sinf((float)CF_SECONDS*0.25f)*100,cosf((float)CF_SECONDS*0.25f)*100)));


		cf_push_text_id(1);
		{
			char text_buf[512];
			snprintf(
				text_buf, sizeof(text_buf),
				"<wave height=30>You have pressed tab %d times</wave>",
				tab_press_count
			);
			draw_text_boxed(text_buf, V2(-230.f, 60.f));

			const char* switchable_text = change_text
				? "<wave height=30>Press Tab to change this text</wave>"
				: "<wave height=30>This is a new text, press Tab to change</wave>";
			draw_text_boxed(switchable_text, V2(-230.f, 30.f));
		}
		cf_pop_text_id();

		// Instructions
		const char* instructions = "Press Space to toggle bounding boxes";
		v2 size = text_size(instructions);
		draw_text_boxed(instructions, V2(-size.x * 0.5f, 0.f));

		draw_calls = app_draw_onto_screen(true);
	}

	destroy_app();

	return 0;
}
