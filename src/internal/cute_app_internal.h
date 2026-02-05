/*
	Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

#ifndef CF_APP_INTERNAL_H
#define CF_APP_INTERNAL_H

#include <cute_app.h>
#include <cute_audio.h>
#include <cute_array.h>
#include <cute_math.h>
#include <cute_doubly_list.h>
#include <cute_png_cache.h>
#include <cute_graphics.h>
#include <cute_input.h>
#include <cute_string.h>
#include <cute_image.h>
#include <cute_file_system.h>

#include <internal/cute_draw_internal.h>
#include <internal/cute_font_internal.h>
#include <internal/cute_graphics_internal.h>

#include <SDL3/SDL.h>

struct SDL_Window;
struct cs_context_t;

extern struct CF_App* app;

struct CF_MouseState
{
	int left_button = 0;
	int right_button = 0;
	int middle_button = 0;
	int x1_button = 0;
	int x2_button = 0;
	float wheel_motion = 0;
	float x = 0;
	float y = 0;
	float xrel = 0;
	float yrel = 0;
	int click_type = 0;
};

struct CF_WindowState
{
	bool mouse_inside_window = false;
	bool has_keyboard_focus = false;
	bool minimized = false;
	bool maximized = false;
	bool restored = false;
	bool resized = false;
	bool moved = false;
};

struct CF_ShaderFileInfo
{
	CF_Stat stat;
	const char* path;
};

struct CF_App
{
	// App stuff.
	bool running = true;
	int options = 0;
	void* platform_handle = NULL;
	CF_OnUpdateFn* user_on_update = NULL;
	SDL_Window* window = NULL;
	CF_BackendType gfx_backend_type = CF_BACKEND_TYPE_INVALID;
	bool debug_opengl = false;
	cs_context_t* cute_sound = NULL;
	void (*on_shader_changed_fn)(const char* path, void* udata) = NULL;
	void* on_shader_changed_udata = NULL;
	bool shader_directory_set = false;
	Cute::CF_Path shader_directory;
	Cute::Map<CF_ShaderFileInfo> shader_file_infos;
	bool gfx_enabled = false;
	float dpi_scale = 1.0f;
	bool dpi_scale_was_changed = false;
	bool sync_window = false;
	int draw_call_count = 0;
	int w;
	int h;
	int x;
	int y;
	int canvas_w;
	int canvas_h;
	CF_Color clear_color = cf_color_black();
	float clear_depth = 1.0f;
	uint32_t clear_stencil = 0;
	CF_Canvas offscreen_canvas = { };
	CF_WindowState window_state;
	CF_WindowState window_state_prev;
	int sample_count = 0;
	bool use_depth_stencil = false;
	uint64_t default_image_id = CF_PNG_ID_RANGE_LO;
	bool vsync = false;
	bool audio_needs_updates = false;
	void* update_udata = NULL;
	bool on_sound_finish_single_threaded = false;
	Cute::Array<CF_Sound> on_sound_finish_queue;
	void (*on_sound_finish)(CF_Sound, void*) = NULL;
	void (*on_music_finish)(void*) = NULL;
	bool on_music_finish_signal = false;
	void* on_sound_finish_udata = NULL;
	void* on_music_finish_udata = NULL;
	CF_Mutex on_sound_finish_mutex = cf_make_mutex();
	CF_Shader draw_shader = { 0 };
	CF_Shader blit_shader = { 0 };

	// Input stuff.
	Cute::Array<char> ime_composition;
	int ime_composition_cursor = 0;
	int ime_composition_selection_len = 0;
	Cute::Array<int> input_text;
	int keys[512] = { 0 };
	int keys_prev[512] = { 0 };
	double keys_timestamp[512] = { 0 };
	void (*key_callback)(CF_KeyButton key, bool true_down_false_up) = NULL;
	CF_MouseState mouse, mouse_prev;
	Cute::Array<CF_Touch> touches;

	// Dear ImGui stuff.
	bool using_imgui = false;

	// Font stuff.
	uint64_t font_image_id_gen = CF_FONT_ID_RANGE_LO;
	Cute::Map<CF_Font*> fonts;
	Cute::Map<CF_Pixel*> font_pixels;
	Cute::Map<CF_TextEffectState> text_effect_states;
	Cute::Map<CF_ParsedTextState> parsed_text_states;
	Cute::Map<CF_TextEffectFn*> text_effect_fns;

	// Easy sprite stuff.
	uint64_t easy_sprite_id_gen = CF_EASY_ID_RANGE_LO;
	Cute::Map<CF_Image> easy_sprites;
};

#endif // CF_APP_INTERNAL_H
