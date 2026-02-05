#include <cute.h>
#include <SDL3/SDL_hints.h>

// A structure with 2 text representations
typedef struct TextBoxData {
	// utf32 codepoint array for editting.
	dyna int* utf32;
	// null-terminated utf8 string for rendering.
	// TODO: This gets decoded back into utf32 anyway which is inefficient.
	char* utf8;
} TextBoxData;

static inline int utf8_bytes_required(int codepoint) {
	if (codepoint <= 0x7F) {
		return 1;
	} else if (codepoint <= 0x7FF) {
		return 2;
	} else if (codepoint <= 0xFFFF) {
		return 3;
	} else if (codepoint <= 0x10FFFF) {
		return 4;
	} else {
		return 0;
	}
}

void text_append_input(TextBoxData* data, CF_InputTextBuffer buffer) {
	for (int i = 0; i < buffer.len; ++i) {
		int codepoint = buffer.codepoints[i];
		sappend_UTF8(data->utf8, codepoint);
		apush(data->utf32, codepoint);
	}
}

void text_pop(TextBoxData* data) {
	if (data->utf32 && asize(data->utf32) > 0) {
		int codepoint = apop(data->utf32);
		spopn(data->utf8, utf8_bytes_required(codepoint));
	}
}

void text_clear(TextBoxData* data) {
	sclear(data->utf8);
	aclear(data->utf32);
}

void text_free(TextBoxData* data) {
	sfree(data->utf8);
	afree(data->utf32);
	data->utf8 = NULL;
	data->utf32 = NULL;
}

void update_ime_rect(CF_V2 text_pos, TextBoxData* textbox) {
	// Position IME rect below the baseline.
	// TODO: This fails on multiline text.
	CF_V2 text_size = textbox->utf8 ? cf_text_size(textbox->utf8, -1) : cf_text_size("", -1);
	CF_V2 box_pos = cf_world_to_screen((CF_V2){ text_pos.x + text_size.x, text_pos.y - text_size.y });
	// TODO: what should width and height be?
	// * Unikey (Vietnamese IME) always ignores size.
	// * If we need an IME rect, it means the OS does not send us composition
	//   to render so there is nothing to measure.
	cf_input_set_ime_rect((int)box_pos.x, (int)box_pos.y, 1, 1);
}

int main(int argc, char* argv[])
{
	int w = 640;
	int h = 480;
	cf_make_app("IME", 0, 0, 0, w, h, CF_APP_OPTIONS_WINDOW_POS_CENTERED_BIT, argv[0]);

	TextBoxData text = { 0 };
	bool render_composition = false;
	float t = 0;

	float half_width = (float)w * 0.5f;
	float half_height = (float)h * 0.5f;

	while (cf_app_is_running()) {
		cf_app_update(NULL);
		t += CF_DELTA_TIME;

		cf_draw_translate(-half_width, half_height);
		cf_push_font("Calibri");
		cf_push_font_size(20);

		bool text_input_enabled = cf_input_is_ime_enabled();
		const char* instruction = !text_input_enabled
			? "Press Enter to start text input"
			: "Press Enter to stop text input";
		cf_draw_text(instruction, (CF_V2){ 0, 0 }, -1);

		// Draw input box slightly below
		cf_draw_translate(0, -cf_text_size(instruction, -1).y * 1.5f);

		// Append the text from the buffer
		CF_InputTextBuffer buffer;
		bool received_text = cf_input_text_get_buffer(&buffer);
		if (received_text) {
			text_append_input(&text, buffer);
			update_ime_rect((CF_V2){ 0 }, &text);

			cf_input_text_clear();
		}

		// Draw the text input
		if (text.utf8) {  // cf_draw_text on NULL should noop
			cf_draw_text(text.utf8, (CF_V2){ 0.f, 0.f }, -1);
		}

		CF_V2 text_size = text.utf8 ? cf_text_size(text.utf8, -1) : cf_text_size("", -1);

		// Draw composition.
		// TODO: what about multiline?
		// TODO: what about selection?
		CF_ImeComposition composition;
		if (cf_input_get_ime_composition(&composition)) {
			cf_draw_text(
				composition.composition,
				(CF_V2){ text_size.x, 0.f },
				-1
			);
			CF_V2 composition_size = cf_text_size(composition.composition, -1);
			cf_draw_box(
				(CF_Aabb){
					.min.x = text_size.x,
					.min.y = -text_size.y,
					.max.x = text_size.x + composition_size.x,
					.max.y = 0,
				},
				0.f,
				0.f
			);
		}

		// Draw cursor
		if (text_input_enabled) {
			CF_Color color = cf_draw_peek_color();
			color.a = sinf(t * 8.f) * 0.5f + 0.5f;
			cf_draw_push_color(color);
			cf_draw_line(
				(CF_V2){ text_size.x, 0.f },
				(CF_V2){ text_size.x, -text_size.y },
				0.f
			);
			cf_draw_pop_color();

			// Draw origin of IME rect
			if (!render_composition) {
				cf_draw_circle(
					(CF_Circle){ .p.x = text_size.x, .p.y = -text_size.y, .r = 1.f },
					1.f
				);
			}
		}

		// Some IME uses Enter to submit composition so we only toggle if no
		// text was received
		if (!received_text && cf_key_just_pressed(CF_KEY_RETURN)) {
			if (text_input_enabled) {
				cf_input_disable_ime();
			} else {
				cf_input_enable_ime();
				cf_input_text_clear();
				text_clear(&text);
				update_ime_rect((CF_V2){ 0 }, &text);
			}
		}

		// Backspace to delete
		if (
			text_input_enabled
			&& (
				cf_key_repeating(CF_KEY_BACKSPACE)
				|| cf_key_just_pressed(CF_KEY_BACKSPACE)
			)
		) {
			text_pop(&text);
			update_ime_rect((CF_V2){ 0 }, &text);
		}

		// Toggle composition rendering
		if (cf_key_just_pressed(CF_KEY_F1)) {
			render_composition = !render_composition;
			SDL_SetHint(SDL_HINT_IME_IMPLEMENTED_UI, render_composition ? "composition" : "none");
		}

		cf_app_draw_onto_screen(true);
	}

	text_free(&text);
}
