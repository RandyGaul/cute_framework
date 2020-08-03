/*
	Cute Framework
	Copyright (C) 2019 Randy Gaul https://randygaul.net

	This software is provided 'as-is', without any express or implied
	warranty.  In no event will the authors be held liable for any damages
	arising from the use of this software.

	Permission is granted to anyone to use this software for any purpose,
	including commercial applications, and to alter it and redistribute it
	freely, subject to the following restrictions:

	1. The origin of this software must not be misrepresented; you must not
	   claim that you wrote the original software. If you use this software
	   in a product, an acknowledgment in the product documentation would be
	   appreciated but is not required.
	2. Altered source versions must be plainly marked as such, and must not be
	   misrepresented as being the original software.
	3. This notice may not be removed or altered from any source distribution.
*/

#ifndef CUTE_APP_INTERNAL_H
#define CUTE_APP_INTERNAL_H

#include <cute/cute_font.h>

#include <cute_buffer.h>
#include <cute_audio.h>
#include <cute_array.h>
#include <cute_ecs.h>
#include <cute_dictionary.h>
#include <cute_gfx.h>

#include <internal/cute_object_table_internal.h>

struct SDL_Window;
struct cs_context_t;

namespace cute
{

struct gfx_t;
struct audio_system_t;

struct mouse_state_t
{
	int left_button = 0;
	int right_button = 0;
	int middle_button = 0;
	int wheel_button = 0;
	int wheel_motion = 0;
	int x = 0;
	int y = 0;
	int xrel = 0;
	int yrel = 0;
	int click_type = 0;
};

struct window_state_t
{
	bool mouse_inside_window = false;
	bool has_keyboard_focus = false;
	bool minimized = false;
	bool maximized = false;
	bool restored = false;
	bool resized = false;
	bool moved = false;
};

struct system_t
{
	system_fn* update_func = NULL;
	array<component_type_t> component_types;
};

struct entity_collection_t
{
	handle_table_t entity_handle_table;
	array<handle_t> entity_handles; // TODO - Replace with a counter? Or delete?
	array<component_type_t> component_types;
	array<typeless_array> component_tables;
};

struct app_t
{
	float dt = 0;
	bool running = true;
	int options = 0;
	void* platform_handle = NULL;
	SDL_Window* window = NULL;
	cs_context_t* cute_sound = NULL;
	threadpool_t* threadpool = NULL;
	audio_system_t* audio_system = NULL;
	cute_font_t* courier_new = NULL;
	array<cute_font_vert_t> font_verts;
	gfx_vertex_buffer_t* font_buffer = NULL;
	gfx_shader_t* font_shader = NULL;
	gfx_t* gfx = NULL;
	int w;
	int h;
	int x;
	int y;
	int render_w;
	int render_h;
	window_state_t window_state;
	window_state_t window_state_prev;

	array<int> input_text;
	int keys[512] = { 0 };
	int keys_prev[512] = { 0 };
	float keys_duration[512] = { 0 };
	int key_mod = 0;
	mouse_state_t mouse, mouse_prev;

	// TODO: Set allocator context for these data structures.
	array<system_t> systems;
	dictionary<entity_type_t, entity_collection_t> entity_collections;
	entity_type_t current_collection_type_being_iterated = CUTE_INVALID_ENTITY_TYPE;
	entity_collection_t* current_collection_being_updated = NULL;

	dictionary<const char*, component_type_t> component_name_to_type_table;
	dictionary<component_type_t, component_config_t> component_configs;
	dictionary<entity_type_t, kv_t*> entity_parsed_schemas;
	dictionary<entity_type_t, entity_type_t> entity_schema_inheritence;

	dictionary<entity_t, int>* save_id_table = NULL;
	array<entity_t>* load_id_table = NULL;

	void* mem_ctx = NULL;
};

}

#endif // CUTE_APP_INTERNAL_H
