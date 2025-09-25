#include <cute.h>
#include "clay.h"
#include "proggy.h"

// Clay uses a 16-bit id for fonts while CF uses interned strings which can be
// 64 bits.
// A mapping to a lower numerical range is needed.
enum {
	FONT_DEFAULT,
	FONT_MENU,

	FONT_COUNT,
};

static const char* ui_fonts[FONT_COUNT] = {
	[FONT_DEFAULT] = "Calibri",
	[FONT_MENU] = "ProggyClean",
};

static void handle_clay_error(Clay_ErrorData errorText);

static void handle_clay_core_commands(const Clay_RenderCommand* command);

static Clay_Dimensions measure_text(Clay_StringSlice text, Clay_TextElementConfig* config, void* userdata);

static inline CF_Color cf_color_from_clay(Clay_Color color)
{
	return (CF_Color){
		.a = color.a / 255.f,
		.r = color.r / 255.f,
		.g = color.g / 255.f,
		.b = color.b / 255.f,
	};
}

static inline Clay_Color clay_color_from_cf(CF_Color color)
{
	// The inspector uses a [0, 255] color range instead of [0, 1]
	return (Clay_Color){
		.a = color.a * 255.f,
		.r = color.r * 255.f,
		.g = color.g * 255.f,
		.b = color.b * 255.f,
	};
}

static inline CF_Aabb cf_aabb_from_clay(Clay_BoundingBox aabb)
{
	return (CF_Aabb){
		.min = {
			.x = aabb.x,
			.y = -(aabb.y + aabb.height),
		},
		.max = {
			.x = aabb.x + aabb.width,
			.y = -aabb.y,
		},
	};
}

int main(int argc, char* argv[])
{
	CF_Result result = cf_make_app(
		"Clay",
		0, 0, 0, 1280, 720,
		CF_APP_OPTIONS_WINDOW_POS_CENTERED_BIT,
		argv[0]
	);
	if (cf_is_error(result)) { return -1; }

	cf_make_font_from_memory(proggy_data, proggy_sz, cf_sintern("ProggyClean"));

	// Clay only uses a static block of memory
	size_t clay_mem_size = Clay_MinMemorySize();
	void* clay_mem = cf_alloc(clay_mem_size);
	Clay_Initialize(
		Clay_CreateArenaWithCapacityAndMemory(clay_mem_size, clay_mem),
		(Clay_Dimensions){
			.width = cf_app_get_width(),
			.height = cf_app_get_height(),
		},
		(Clay_ErrorHandler){ .errorHandlerFunction = handle_clay_error }
	);
	Clay_SetMeasureTextFunction(measure_text, NULL);

	cf_clear_color(0.5, 0.5, 0.5, 0);

	while (cf_app_is_running()) {
		cf_app_update(NULL);

		// Update dimension in case of window resize
		int width, height;
		cf_app_get_size(&width, &height);
		Clay_SetLayoutDimensions((Clay_Dimensions){ (float)width, (float)height });

		// Pass mouse input to Clay
		Clay_SetPointerState(
			(Clay_Vector2){ cf_mouse_x(), cf_mouse_y() },
			cf_mouse_down(CF_MOUSE_BUTTON_LEFT)
		);
		Clay_UpdateScrollContainers(
			true,
			(Clay_Vector2){ .y = cf_mouse_wheel_motion() },
			CF_DELTA_TIME
		);

		// Debug toggle
		if (cf_key_just_pressed(CF_KEY_F12)) {
			Clay_SetDebugModeEnabled(!Clay_IsDebugModeEnabled());
		}

		// UI Layout

		Clay_BeginLayout();
		{
		}
		Clay_RenderCommandArray render_cmds = Clay_EndLayout();

		// UI rendering

		// Clay uses a 0,0 at the top-left and y-down coordinate system
		cf_draw_push();
		float half_width = width * 0.5f;
		float half_height = height * 0.5f;
		cf_draw_translate(-half_width, half_height);
		for (int i = 0; i < render_cmds.length; i++) {
			const Clay_RenderCommand* command = &render_cmds.internalArray[i];
			if (command->commandType == CLAY_RENDER_COMMAND_TYPE_CUSTOM) {
			} else {
				handle_clay_core_commands(command);
			}
		}
		cf_draw_pop();

		cf_app_draw_onto_screen(true);
	}

	cf_free(clay_mem);

	return 0;
}

static void handle_clay_error(Clay_ErrorData errorText)
{
	fprintf(stderr, "Clay error: %.*s\n", errorText.errorText.length, errorText.errorText.chars);
}

static void begin_text(const Clay_TextElementConfig* config)
{
	cf_push_font(ui_fonts[config->fontId < FONT_COUNT ? config->fontId : FONT_DEFAULT]);
	cf_push_font_size(config->fontSize);
	cf_draw_push_color(cf_color_from_clay(config->textColor));
	// Clay's text measuring method does not account for effect mark up
	cf_push_text_effect_active(false);
}

static void end_text(void)
{
	cf_pop_text_effect_active();
	cf_draw_pop_color();
	cf_pop_font_size();
	cf_pop_font();
}

static Clay_Dimensions measure_text(Clay_StringSlice text, Clay_TextElementConfig* config, void* userdata)
{
	begin_text(config);
	CF_V2 size = cf_text_size(text.chars, text.length);
	end_text();

	return (Clay_Dimensions){ .width = size.x, .height = size.y };
}

static void handle_clay_core_commands(const Clay_RenderCommand* command)
{
	switch (command->commandType) {
		case CLAY_RENDER_COMMAND_TYPE_NONE:
		case CLAY_RENDER_COMMAND_TYPE_CUSTOM:
			// Custom elements are handled inline in the main loop
			break;
		case CLAY_RENDER_COMMAND_TYPE_RECTANGLE: {
			cf_draw_push_color(
				cf_color_from_clay(
					command->renderData.rectangle.backgroundColor
				)
			);
			cf_draw_box_rounded_fill(
				cf_aabb_from_clay(command->boundingBox),
				command->renderData.rectangle.cornerRadius.topLeft
			);
			cf_draw_pop_color();
		} break;
		case CLAY_RENDER_COMMAND_TYPE_BORDER: {
			Clay_BorderRenderData border = command->renderData.border;
			CF_Aabb aabb = cf_aabb_from_clay(command->boundingBox);
			cf_draw_push_color(cf_color_from_clay(border.color));
			// The most common case is when all borders are set so we can just draw
			// a box.
			// But the inspector uses partial border a lot and it looks ugly if
			// we don't handle it
			if (
				border.width.top == border.width.left
				&&
				border.width.top == border.width.right
				&&
				border.width.top == border.width.bottom
				&&
				border.cornerRadius.topLeft == border.cornerRadius.topRight
				&&
				border.cornerRadius.topLeft == border.cornerRadius.bottomRight
				&&
				border.cornerRadius.topLeft == border.cornerRadius.bottomLeft
			) {
				cf_draw_box_rounded(
					aabb,
					(float)border.width.top * 0.5f,
					border.cornerRadius.topLeft
				);
			} else {
				if (border.width.top > 0) {
					cf_draw_line(
						cf_v2(
							aabb.min.x + border.cornerRadius.topLeft,
							aabb.max.y
						),
						cf_v2(
							aabb.max.x - border.cornerRadius.topRight,
							aabb.max.y
						),
						(float)border.width.top * 0.5f
					);
				}

				if (border.width.bottom > 0) {
					cf_draw_line(
						cf_v2(
							aabb.min.x + border.cornerRadius.bottomLeft,
							aabb.min.y
						),
						cf_v2(
							aabb.max.x - border.cornerRadius.bottomRight,
							aabb.min.y
						),
						(float)border.width.bottom * 0.5f
					);
				}

				if (border.width.left > 0) {
					cf_draw_line(
						cf_v2(
							aabb.min.x,
							aabb.max.y - border.cornerRadius.topLeft
						),
						cf_v2(
							aabb.min.x,
							aabb.min.y + border.cornerRadius.bottomLeft
						),
						(float)border.width.left * 0.5f
					);
				}

				if (border.width.right > 0) {
					cf_draw_line(
						cf_v2(
							aabb.max.x,
							aabb.max.y - border.cornerRadius.topRight
						),
						cf_v2(
							aabb.max.x,
							aabb.min.y + border.cornerRadius.bottomRight
						),
						(float)border.width.right * 0.5f
					);
				}
			}
			cf_draw_pop_color();
		} break;
		case CLAY_RENDER_COMMAND_TYPE_TEXT: {
			// While Clay's text wrapping is redundant for CF and it does not
			// play well with effect markup, we still implement it for the
			// inspector.
			begin_text(&(Clay_TextElementConfig){
				.fontId = command->renderData.text.fontId,
				.fontSize = command->renderData.text.fontSize,
				.textColor = command->renderData.text.textColor,
				.userData = command->userData,
			});
			cf_draw_text(
				command->renderData.text.stringContents.chars,
				(CF_V2){ command->boundingBox.x, -command->boundingBox.y },
				command->renderData.text.stringContents.length
			);
			end_text();
		} break;
		case CLAY_RENDER_COMMAND_TYPE_IMAGE: {
		} break;
		case CLAY_RENDER_COMMAND_TYPE_SCISSOR_START: {
			cf_draw_push_scissor((CF_Rect){
				.x = command->boundingBox.x,
				.y = command->boundingBox.y,
				.w = command->boundingBox.width,
				.h = command->boundingBox.height,
			});
		} break;
		case CLAY_RENDER_COMMAND_TYPE_SCISSOR_END: {
			cf_draw_pop_scissor();
		} break;
	}
}

#define CLAY_IMPLEMENTATION
#include "clay.h"
