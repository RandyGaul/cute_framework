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
#include <cute_ecs.h>
#include <cute_math.h>
#include <cute_doubly_list.h>
#include <cute_png_cache.h>
#include <cute_graphics.h>
#include <cute_input.h>
#include <cute_string.h>
#include <cute_image.h>

#include <internal/cute_draw_internal.h>
#include <internal/cute_font_internal.h>
#include <internal/cute_ecs_internal.h>

#include <sokol/sokol_gfx_imgui.h>

struct SDL_Window;
struct cs_context_t;

extern struct CF_App* app;

struct CF_MouseState
{
	int left_button = 0;
	int right_button = 0;
	int middle_button = 0;
	int wheel_motion = 0;
	int x = 0;
	int y = 0;
	int xrel = 0;
	int yrel = 0;
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

struct CF_App
{
	// App stuff.
	bool running = true;
	int options = 0;
	void* platform_handle = NULL;
	CF_OnUpdateFn* user_on_update = NULL;
	SDL_Window* window = NULL;
	cs_context_t* cute_sound = NULL;
	bool spawned_mix_thread = false;
	CF_Threadpool* threadpool = NULL;
	bool gfx_enabled = false;
	sg_context_desc gfx_ctx_params;
	float dpi_scale = 1.0f;
	float dpi_scale_prev = 1.0f;
	bool dpi_scale_was_changed = false;
	int w;
	int h;
	int x;
	int y;
	int draw_call_count = 0;
	int canvas_w;
	int canvas_h;
	CF_Canvas offscreen_canvas = { };
	CF_Canvas backbuffer_canvas = { };
	CF_Mesh backbuffer_quad = { };
	CF_Shader backbuffer_shader = { };
	CF_Material backbuffer_material = { };
	CF_WindowState window_state;
	CF_WindowState window_state_prev;
	bool using_imgui = false;
	sg_imgui_t sg_imgui;
	uint64_t default_image_id = CF_PNG_ID_RANGE_LO;
	bool vsync = false;
	bool audio_needs_updates = false;
	void* update_udata = NULL;
	bool canvas_blit_init = false;
	CF_Mesh blit_mesh;
	CF_Material blit_material;
	CF_Shader blit_shader;
	bool on_sound_finish_single_threaded = false;
	Cute::Array<CF_Sound> on_sound_finish_queue;
	void (*on_sound_finish)(CF_Sound, void*) = NULL;
	void (*on_music_finish)(void*) = NULL;
	bool on_music_finish_signal = false;
	void* on_sound_finish_udata = NULL;
	void* on_music_finish_udata = NULL;
	CF_Mutex on_sound_finish_mutex = cf_make_mutex();

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
	CF_List joypads;
	Cute::Array<CF_Touch> touches;

	// ECS stuff.
	CF_SystemInternal system_internal_builder;
	Cute::Array<CF_SystemInternal> systems;
	CF_EntityConfig entity_config_builder;
	CF_EntityType entity_type_gen = 0;
	Cute::Map<const char*, CF_EntityType> entity_type_string_to_id;
	Cute::Array<const char*> entity_type_id_to_string;
	CF_EntityType current_collection_type_being_iterated = ~0;
	CF_EntityCollection* current_collection_being_updated = NULL;
	CF_ComponentListInternal component_list;

	CF_ComponentConfig component_config_builder;
	Cute::Map<const char*, CF_ComponentConfig> component_configs;

	CF_World world;
	Cute::Array<CF_World> worlds;

	// Font stuff.
	uint64_t font_image_id_gen = CF_FONT_ID_RANGE_LO;
	Cute::Map<const char*, CF_Font*> fonts;
	Cute::Map<uint64_t, CF_Pixel*> font_pixels;
	Cute::Map<const char*, CF_TextEffectState> text_effect_states;
	Cute::Map<const char*, CF_TextEffectFn*> text_effect_fns;

	// Easy sprite stuff.
	uint64_t easy_sprite_id_gen = CF_EASY_ID_RANGE_LO;
	Cute::Map<uint64_t, CF_Image> easy_sprites;
};

#endif // CF_APP_INTERNAL_H
