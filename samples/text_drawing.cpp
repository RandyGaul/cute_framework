#include "cute_draw.h"
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

// Text on a curved baseline: one glyph at a time, each rotated to the arc's tangent at
// its position. Glyphs render from Bezier outlines by default, so the per-glyph
// rotation stays crisp -- no atlas resampling. center_angle points at the middle of the
// string; the arc reads left-to-right along the circle's top.
static void draw_text_arc(const char* text, v2 center, float radius, float center_angle)
{
	float total = text_width(text);
	float a = center_angle + (total * 0.5f) / radius;
	float ascent = text_height("X"); // Baseline sits on the arc; glyph tops lean inward.
	const char* p = text;
	while (*p) {
		int cp;
		const char* next = cf_string_decode_UTF8(p, &cp);
		char glyph[8] = { 0 };
		CF_MEMCPY(glyph, p, (size_t)(next - p));
		p = next;
		float w = text_width(glyph);
		float half = (w * 0.5f) / radius;
		a -= half;
		draw_push();
		draw_translate(center + V2(cosf(a), sinf(a)) * radius);
		draw_rotate(a - CF_PI * 0.5f);
		draw_text(glyph, V2(-w * 0.5f, ascent), -1);
		draw_pop();
		a -= half;
	}
}

int main(int argc, char* argv[])
{
	make_app("Text Drawing", 0, 0, 0, 1280, 900, CF_APP_OPTIONS_WINDOW_POS_CENTERED_BIT | CF_APP_OPTIONS_RESIZABLE_BIT, argv[0]);

	draw_push_shape_aa(1.5f);
	make_font_from_memory(proggy_data, proggy_sz, "ProggyClean");
	// Map <b> to a second face so style tags are visible without a real bold TTF.
	// In a game you would load "MyFont-Bold.ttf" / "MyFont-Italic.ttf" and map those.
	text_effect_set_font("b", "ProggyClean");
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
		// Left column, above the style-tag demos and clear of the right-side gradients.
		push_font_size(13 * 5);
		push_font_blur(10);
		draw_text_boxed("glowing~", V2(-500-10, -50+10));
		pop_font_blur();
		draw_text_boxed("<fade>glowing~</fade>", V2(-500, -50));

		// Using font blurring for a shadow effect.
		push_font_size(13 * 5);
		push_font_blur(10);
		draw_push_color(color_black());
		draw_text_boxed("shadow", V2(-470-10-2.5f, -120+5));
		draw_pop_color();
		pop_font_blur();
		draw_text_boxed("shadow", V2(-470, -120));

		// Drawing a formatted string.
		String draws = String::fmt("Draw calls: %d", draw_calls);
		push_font_size(13);
		draw_text_boxed(draws.c_str(), V2(-620.f, 400.f));

		push_font_size(26);
		draw_text_boxed("Half-rendered effect <wave>groovy</wave>", V2(-230.f, 180.f), 25);
		pop_font_size();

		// Text shake effect.
		// For moving text it helps to round positions.
		push_font_size(30);
		draw_text_boxed("Some <shake freq=35 x=2 y=2>shaking</shake> text.", round(V2(sinf((float)CF_SECONDS*0.25f)*100,cosf((float)CF_SECONDS*0.25f)*100)));


		float gx = 130;
		float gy = -60;
		float step = 30;

		// Horizontal: left-to-right red to blue.
		push_font_size(26);
		draw_text_boxed("<gradient left=#ff0000 right=#0055ff>Left-to-right gradient!</gradient>", V2(gx, gy));

		// Vertical: top gold to bottom purple.
		draw_text_boxed("<gradient top=#ffd700 bottom=#8b00ff>Top-to-bottom gradient!</gradient>", V2(gx, gy - step));

		// Corners: direct per-corner control.
		draw_text_boxed("<gradient topleft=#ff0000 topright=#00ff00 bottomright=#0000ff bottomleft=#ffff00>Per-corner colors</gradient>", V2(gx, gy - step*2));

		// Mix: edge + corner override.
		draw_text_boxed("<gradient left=#ff0000 right=#0000ff topleft=#00ff00>Edge + corner override</gradient>", V2(gx, gy - step*3));

		// Single edge: fades from red to the glyph's current color.
		draw_text_boxed("<gradient left=#ff0000>Fade from one color</gradient>", V2(gx, gy - step*4));

		// Short strings: gradient should still work on 1-2 glyphs.
		push_font_size(30);
		draw_text_boxed("<gradient left=#ff0000 right=#0000ff>AB</gradient>", V2(gx, gy - step*5));
		draw_text_boxed("<gradient top=#ff0000 bottom=#0000ff>X</gradient>", V2(gx + 80, gy - step*5));

		// Composited text effects.
		push_font_size(26);
		draw_text_boxed("<wave><fade>wave + fade</fade></wave>", V2(gx, gy - step*6));
		draw_text_boxed("<shake freq=35 x=2 y=2><gradient left=#ff0000 right=#0000ff>shake + gradient</gradient></shake>", V2(gx, gy - step*7));

        cf_push_text_id(42);
		push_font_blur(10);
		draw_push_color(color_black());
		draw_text_boxed("<wave><shake freq=30 x=1 y=1><fade><strike>every effect at once!</strike></fade></shake></wave>", V2(gx, gy - step*8));
		draw_pop_color();
		pop_font_blur();
        push_font_blur(5);
		draw_text_boxed("<wave><shake freq=30 x=1 y=1><fade><strike><gradient left=#ff0000 right=#0000ff>every effect at once!</gradient></strike></fade></shake></wave>", V2(gx, gy - step*8));
		draw_pop_color();
		pop_font_blur();
		draw_text_boxed("<wave><shake freq=30 x=1 y=1><fade><strike><gradient left=#ff0000 right=#0000ff>every effect at once!</gradient></strike></fade></shake></wave>", V2(gx, gy - step*8));
        cf_pop_text_id();

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

		// Strikethrough on proportional font (tests contiguous segments).
		push_font("Calibri");
		push_font_size(30);
		draw_text_boxed("<strike>Strikethrough on proportional font</strike>", V2(gx, gy - step*9));
		draw_text_boxed("<wave><strike>wavy strike on proportional font</strike></wave>", V2(gx, gy - step*10));
		pop_font_size();

		// Underline markup: default thickness, explicit thickness, and riding a wave.
		push_font_size(26);
		draw_text_boxed("<underline>Underlined</underline>, <underline underline=3>thick</underline>, and <wave><underline>wavy</underline></wave>.", V2(gx, gy - step*11));
		pop_font_size();

		// Outlined text via cf_push_text_stroke (curve glyphs, no pre-baked outline font
		// needed), plus a filled+outline combo by drawing the same string twice.
		push_font_size(30);
		push_text_stroke(0.8f);
		draw_push_color(make_color(0.45f, 0.85f, 1.0f));
		draw_text_boxed("Outline only", V2(gx, gy - step*12));
		draw_pop_color();
		draw_push_color(color_black());
		draw_text_boxed("filled + outline", V2(gx + 210, gy - step*12));
		draw_pop_color();
		pop_text_stroke();
		draw_push_color(make_color(1.0f, 0.8f, 0.2f));
		draw_text_boxed("filled + outline", V2(gx + 210, gy - step*12));
		draw_pop_color();
		pop_font_size();

		// Crisp directional drop shadow: the same string twice with an animated offset.
		// No blur involved, so the shadow copy stays as sharp as the fill at any scale.
		{
			push_font_size(36);
			v2 shadow_pos = V2(350, 150);
			v2 shadow_dir = V2(cosf((float)CF_SECONDS * 0.7f), sinf((float)CF_SECONDS * 0.7f)) * 7.0f;
			draw_push_color(make_color(0.0f, 0.0f, 0.0f, 0.85f));
			draw_text("directional shadow", shadow_pos + shadow_dir, -1);
			draw_pop_color();
			draw_push_color(make_color(1.0f, 0.85f, 0.3f));
			draw_text_boxed("directional shadow", shadow_pos);
			draw_pop_color();
			pop_font_size();
		}

		// Text on a curved baseline, swaying. Per-glyph rotation stays crisp on the
		// curve path.
		push_font_size(30);
		draw_push_color(make_color(0.6f, 1.0f, 0.6f));
		draw_text_arc("~ curved baseline text ~", V2(460, 210), 130.0f, CF_PI * 0.5f + sinf((float)CF_SECONDS * 0.6f) * 0.25f);
		draw_pop_color();
		pop_font_size();

		// Font style tags via cf_text_effect_set_font / built-in <font> markup (#349).
		// <b> is mapped to ProggyClean above; real bold/italic faces work the same way.
		// Bottom-left column, below glow/shadow so the two demos never overlap.
		push_font("Calibri");
		push_font_size(26);
		const float style_x = -600.f;
		const float style_y = -230.f;
		const float style_step = 32.f;
		draw_text_boxed("Style tags: normal, <b>bold-mapped</b>, normal again.", V2(style_x, style_y));
		draw_text_boxed("Inline <font name=\"ProggyClean\">monospace span</font> mid-sentence.", V2(style_x, style_y - style_step));
		draw_text_boxed("<font name=\"ProggyClean\" size=36>Big mono</font> then <b>mapped bold</b>.", V2(style_x, style_y - style_step * 2.2f));
		draw_text_boxed("<font name=\"ProggyClean\" size=22>nested <b>mapped</b> inside font</font>", V2(style_x, style_y - style_step * 3.5f));
		draw_text_boxed("<wave><b>styled + wave</b></wave> and <b><color=#ffcc00>styled + color</color></b>", V2(style_x, style_y - style_step * 4.6f));
		pop_font_size();

		// Instructions
		const char* instructions = "Press Space to toggle bounding boxes";
		v2 size = text_size(instructions);
		draw_text_boxed(instructions, V2(-size.x * 0.5f, 0.f));

		draw_calls = app_draw_onto_screen(true);
	}

	destroy_app();

	return 0;
}
