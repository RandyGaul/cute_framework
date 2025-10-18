#include <cute.h>
#include <dcimgui.h>
#include <stdarg.h>
#include "clay.h"
#include "proggy.h"

typedef enum {
	NONE,
	BACK_OUT,
	CUBIC_OUT,
	QUINTIC_OUT,
} EaseKind;

double ease_apply(double p, EaseKind kind) {
	double v;
	switch (kind) {
	case BACK_OUT:    v = cf_back_out(p);
	case CUBIC_OUT:   v = cf_cube_out(p);
	case QUINTIC_OUT: v = cf_quint_out(p);
	default:          v = p;
	}
	return v;
}

// Modified version of https://rxi.github.io/a_simple_ui_animation_system.html
// to support different easing functions.
#define ANIMATION_MAX_ITEMS 32

typedef struct {
	uint64_t id;
	double progress, time, initial, prev;
	EaseKind ease_kind;
} AnimationItem;

AnimationItem animation_items[ANIMATION_MAX_ITEMS];
int animation_item_count;

void animation_update_all(double dt) {
	for (int i = animation_item_count - 1; i >= 0; i--) {
		AnimationItem *it = &animation_items[i];
		// update progress
		it->progress += dt / it->time;
		// remove item if it has completed
		if (it->progress >= 1.0) {
			*it = animation_items[--animation_item_count];
		}
	}
}

void animation_start(uint64_t id, double initial, double time, EaseKind ease_kind) {
	// try to find and replace existing item
	for (int i = 0; i < animation_item_count; i++) {
		AnimationItem *it = &animation_items[i];
		if (it->id == id) {
			it->initial = it->prev;
			it->time = time;
			it->progress = 0;
			it->ease_kind = ease_kind;
			return;
		}
	}
	// push new item if we have room
	if (animation_item_count < ANIMATION_MAX_ITEMS) {
		animation_items[animation_item_count++] = (AnimationItem){
			.id = id,
			.initial = initial,
			.prev = initial,
			.time = time,
			.ease_kind = ease_kind,
		};
	}
}

double animation_get(uint64_t id, double target) {
	for (int i = 0; i < animation_item_count; i++) {
		AnimationItem *it = &animation_items[i];
		if (it->id == id) {
			double p = it->progress;
			p = ease_apply(p, it->ease_kind);
			it->prev = it->initial + p * (target - it->initial);
			return it->prev;
		}
	}
	return target;
}

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

static CF_Arena tmp_arena = { 0 };

typedef enum {
	CUSTOM_ELEMENT_TEXT,
} CustomElementType;

typedef struct {
	const char* font;
	float font_size;
	CF_Color color;
	bool wrap;
} TextStyle;

typedef struct {
	CustomElementType type;

	const char* text;
	int length;

	TextStyle style;
} TextElement;

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

static void cf_text_element(
	Clay_ElementId id,
	TextStyle style,
	const char* fmt,
	...
);

static bool menu_entry(const char* text, TextStyle text_style, CF_Sprite* hover_icon, uint64_t* entry_h_id, float* entry_h, EaseKind entry_h_ease_kind)
{
	// A reusable component is just a function
	bool clicked = false;

	CLAY(
		// Local id so it doesn't clash
		CLAY_SID_LOCAL(((Clay_String){ .chars = text, .length = strlen(text) })),
		{
			.layout = {
				.sizing = {
					CLAY_SIZING_GROW(0),
					CLAY_SIZING_FIXED(animation_get(*entry_h_id, *entry_h))
				},
				.childAlignment = {
					.x = CLAY_ALIGN_X_CENTER,
					.y = CLAY_ALIGN_Y_CENTER,
				},
			},
			.backgroundColor = clay_color_from_cf(cf_color_orange()),
		}
	) {
		bool hovered = Clay_Hovered();
		cf_text_element(
			CLAY_ID_LOCAL("Text"),
			text_style,
			hovered ? "<wave>%s</wave>" : "%s",
			text
		);
		if (hovered) {
			CLAY(CLAY_ID_LOCAL("HoverIndicator"), {
				.layout.sizing = {
					.width = CLAY_SIZING_FIT(hover_icon->w),
					.height = CLAY_SIZING_PERCENT(1.f),
				},
				.aspectRatio = (float)hover_icon->w / (float)hover_icon->h,
				.image = { .imageData = hover_icon },
				// Floating will not affect the size of the parent
				.floating = {
					.attachTo = CLAY_ATTACH_TO_PARENT,
					.attachPoints = {
						.element = CLAY_ATTACH_POINT_RIGHT_CENTER,
						.parent = CLAY_ATTACH_POINT_LEFT_CENTER,
					},
					.offset = { .x = -10.f },
				},
			}) {
			}
			animation_start(*entry_h_id, *entry_h, 0.5, entry_h_ease_kind);
			*entry_h = 140.0f;
		} else {
			animation_start(*entry_h_id, *entry_h, 0.25, entry_h_ease_kind);
			*entry_h = 100.0f;
		}

		clicked = hovered && cf_mouse_just_pressed(CF_MOUSE_BUTTON_LEFT);
	}

	return clicked;
}

int main(int argc, char* argv[])
{
	CF_Result result = cf_make_app(
		"Clay UI Animations",
		0, 0, 0, 1280, 720,
		CF_APP_OPTIONS_WINDOW_POS_CENTERED_BIT,
		argv[0]
	);
	if (cf_is_error(result)) { return -1; }
	cf_app_init_imgui();

	cf_make_font_from_memory(proggy_data, proggy_sz, cf_sintern("ProggyClean"));
	tmp_arena = cf_make_arena(_Alignof(void*), 4096);

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

	CF_Sprite life_icon = { 0 };
	life_icon = cf_make_demo_sprite();
	cf_sprite_play(&life_icon, "spin");

	CF_Sprite hover_icon = life_icon;
	cf_sprite_play(&hover_icon, "hold_down");

	CF_Sprite exit_icon = life_icon;
	cf_sprite_play(&exit_icon, "hold_up");

	float new_game_entry_h = 100.0f;
	uint64_t new_game_entry_h_id = (uint64_t)&new_game_entry_h;
	EaseKind new_game_entry_h_ease_kind = BACK_OUT;

	float continue_entry_h = 100.0f;
	uint64_t continue_entry_h_id = (uint64_t)&continue_entry_h;
	EaseKind continue_entry_h_ease_kind = CUBIC_OUT;

	float exit_entry_h = 100.0f;
	uint64_t exit_entry_h_id = (uint64_t)&exit_entry_h;
	EaseKind exit_entry_h_ease_kind = QUINTIC_OUT;

	while (cf_app_is_running()) {
		cf_app_update(NULL);

		animation_update_all(CF_DELTA_TIME);

		cf_sprite_update(&life_icon);
		cf_sprite_update(&hover_icon);
		cf_sprite_update(&exit_icon);

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

		// UI Layout

		static int font_size = 30;
		static TextStyle text_style;
		text_style.color = cf_color_white();
		text_style.font = ui_fonts[FONT_MENU];
		text_style.font_size = font_size;

		Clay_BeginLayout();
		{
			CLAY(CLAY_ID("Root"), {
				.layout = {
					.sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0) },
					.layoutDirection = CLAY_TOP_TO_BOTTOM,
					.childAlignment = {
						.x = CLAY_ALIGN_X_CENTER,
						.y = CLAY_ALIGN_Y_CENTER,
					},
					.childGap = 10,
				},
			}) {
				CLAY(CLAY_ID_LOCAL("Choice"), {
					.layout = {
						.sizing = { CLAY_SIZING_FIXED(width * 0.2f), CLAY_SIZING_GROW(0) },
						.childAlignment = {
							.x = CLAY_ALIGN_X_CENTER,
							.y = CLAY_ALIGN_Y_CENTER,
						},
						.layoutDirection = CLAY_TOP_TO_BOTTOM,
						.childGap = 20,
					},
				}) {
					if (menu_entry("NEW GAME", text_style, &hover_icon, &new_game_entry_h_id, &new_game_entry_h, new_game_entry_h_ease_kind)) {
						printf("Start a new game\n");
					}

					if (menu_entry("CONTINUE", text_style, &hover_icon, &continue_entry_h_id, &continue_entry_h, continue_entry_h_ease_kind)) {
						printf("Continue\n");
					}

					if (menu_entry("EXIT", text_style, &exit_icon, &exit_entry_h_id, &exit_entry_h, exit_entry_h_ease_kind)) {
						printf("Exit\n");
					}
				}
			}
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
				CustomElementType element_type = *(CustomElementType*)command->renderData.custom.customData;
				switch (element_type) {
					case CUSTOM_ELEMENT_TEXT: {
						const TextElement* element = (const TextElement*)command->renderData.custom.customData;
						cf_push_font(element->style.font);
						cf_push_font_size(element->style.font_size);
						cf_draw_push_color(element->style.color);
						cf_push_text_id(command->id);
						if (element->style.wrap) {
							Clay_ElementData* parent_size = command->userData;
							cf_push_text_wrap_width(parent_size->boundingBox.width);
						}
						cf_draw_text(
							element->text,
							cf_v2(command->boundingBox.x, -command->boundingBox.y),
							element->length
						);
						if (element->style.wrap) {
							cf_pop_text_wrap_width();
						}
						cf_pop_text_id();
						cf_draw_pop_color();
						cf_pop_font();
						cf_pop_font_size();
					} break;
				}
			} else {
				handle_clay_core_commands(command);
			}
		}
		cf_draw_pop();

		cf_app_draw_onto_screen(true);

		cf_arena_reset(&tmp_arena);
	}

	cf_free(clay_mem);
	cf_destroy_arena(&tmp_arena);

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
	// This is only provided so that the inspector can be rendered.
	// CF already has its own text wrapping and text rendering should be done
	// through a custom element.
	begin_text(config);
	CF_V2 size;
	if (text.length == 1 && (text.chars[0] == ' ' || text.chars[1] == '\t')) {
		// Clay will sometimes ask us to measure the size of a standalone space
		// (' ') to get the size of the entire string.
		// CF will (correctly) render a lone or trailing space differently from
		// an intervening space (e.g: 'a b').
		// But that will often result in unexpected sizing.
		// So we will have to estimate.
		// Kerning is complex and Clay's method is not strictly accurate.
		char measure_buf[4] = "Z Z";
		measure_buf[1] = text.chars[0];
		CF_V2 total_size = cf_text_size(measure_buf, 3);
		CF_V2 char_size = cf_text_size("Z", 1);
		size.x = total_size.x - char_size.x * 2.f;
		size.y = total_size.y;
	} else {
		size = cf_text_size(text.chars, text.length);
	}
	end_text();

	return (Clay_Dimensions){ .width = size.x, .height = size.y };
}

static void* make_tmp_copy(void* item, size_t size)
{
	void* copy = cf_arena_alloc(&tmp_arena, size);
	memcpy(copy, item, size);
	return copy;
}

static void cf_text_element(
	Clay_ElementId id,
	TextStyle style,
	const char* fmt,
	...
) {
	va_list args, args_copy;
	va_start(args, fmt);
	va_copy(args_copy, args);

	int len = vsnprintf(NULL, 0, fmt, args_copy);
	char* text_buf = cf_arena_alloc(&tmp_arena, len + 1);
	vsnprintf(text_buf, len + 1, fmt, args);

	va_end(args);
	va_end(args_copy);

	TextElement* element = cf_arena_alloc(&tmp_arena, sizeof(TextElement));
	*element = (TextElement){
		.style = style,
		.text = text_buf,
		.length = len,
	};

	// This custom element has a more accurate size measurement and can support
	// text effect.
	CLAY(id, {
		.layout = {
			.sizing = {
				.width = style.wrap ? CLAY_SIZING_GROW(0) : CLAY_SIZING_FIT(0),
				.height = CLAY_SIZING_GROW(0),
			},
			.childAlignment = {
				.x = CLAY_ALIGN_X_CENTER,
				.y = CLAY_ALIGN_Y_CENTER,
			},
		},
	}) {
		Clay_ElementData parent_size = Clay_GetElementData(id);

		if (parent_size.found) {
			if (style.wrap) {
				cf_push_text_wrap_width(parent_size.boundingBox.width);
			}
			cf_push_font(style.font);
			cf_push_font_size(style.font_size);

			CF_V2 size = cf_text_size(element->text, element->length);

			cf_pop_font();
			cf_pop_font_size();
			if (style.wrap) {
				cf_pop_text_wrap_width();
			}

			CLAY(CLAY_ID_LOCAL("Content"), {
				.layout = {
					.sizing = {
						.width = CLAY_SIZING_FIXED(size.x),
						.height = CLAY_SIZING_FIXED(size.y),
					},
				},
				.custom = element,
				.userData = make_tmp_copy(&parent_size, sizeof(parent_size)),
			}) {}
		}
	}

	// Ensure the type is set
	element->type = CUSTOM_ELEMENT_TEXT;
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
			// we don't handle it.
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
			begin_text(&(Clay_TextElementConfig){
				.fontId = command->renderData.text.fontId,
				.fontSize = command->renderData.text.fontSize,
				.textColor = command->renderData.text.textColor,
			});
			cf_draw_text(
				command->renderData.text.stringContents.chars,
				(CF_V2){ command->boundingBox.x, -command->boundingBox.y },
				command->renderData.text.stringContents.length
			);
			end_text();
		} break;
		case CLAY_RENDER_COMMAND_TYPE_IMAGE: {
			CF_Sprite sprite = *(CF_Sprite*)command->renderData.image.imageData;

			CF_V2 pivot = sprite.pivots[sprite.frame_index];
			sprite.scale.x = command->boundingBox.width / (float)sprite.w;
			sprite.scale.y = command->boundingBox.height / (float)sprite.h;
			sprite.transform.p.x = command->boundingBox.x + pivot.x + (float)sprite.w * sprite.scale.x * 0.5f;
			sprite.transform.p.y = -command->boundingBox.y + pivot.y - (float)sprite.h * sprite.scale.y * 0.5f;

			cf_draw_sprite(&sprite);
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
