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

#include <cute_app.h>
#include <cute_audio.h>
#include <cute_array.h>
#include <cute_ecs.h>
#include <cute_math.h>
#include <cute_doubly_list.h>
#include <cute_aseprite_cache.h>
#include <cute_png_cache.h>
#include <cute_batch.h>
#include <cute_gfx.h>
#include <cute_input.h>
#include <cute_string.h>

#include <internal/cute_object_table_internal.h>

#include <sokol/sokol_gfx_imgui.h>

#define CUTE_RETURN_IF_ERROR(x) do { cf_result_t err = (x); if (cf_is_error(err)) return err; } while (0)

struct SDL_Window;
struct cs_context_t;

extern cf_app_t* cf_app;

struct cf_gfx_t;
struct cf_audio_system_t;

struct cf_mouse_state_t
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

struct cf_window_state_t
{
	bool mouse_inside_window = false;
	bool has_keyboard_focus = false;
	bool minimized = false;
	bool maximized = false;
	bool restored = false;
	bool resized = false;
	bool moved = false;
};

struct cf_entity_collection_t
{
	cf_handle_table_t entity_handle_table;
	cute::array<cf_handle_t> entity_handles; // TODO - Replace with a counter? Or delete?
	cute::array<const char*> component_type_tuple;
	cute::array<cf_typeless_array> component_tables;
	int inactive_count = 0;
};

struct cf_system_internal_t
{
	void clear()
	{
		name = { 0 };
		udata = NULL;
		pre_update_fn = NULL;
		update_fn = NULL;
		post_update_fn = NULL;
		component_type_tuple.clear();
	}

	const char* name = { 0 };
	void* udata = NULL;
	void (*pre_update_fn)(float dt, void* udata) = NULL;
	cf_system_update_fn* update_fn = NULL;
	void (*post_update_fn)(float dt, void* udata) = NULL;
	cute::array<const char*> component_type_tuple;
};

struct cf_component_config_t
{
	void clear()
	{
		name = NULL;
		size_of_component = 0;
		serializer_fn = NULL;
		cleanup_fn = NULL;
		serializer_udata = NULL;
		cleanup_udata = NULL;
	}

	const char* name = NULL;
	size_t size_of_component = 0;
	cf_component_serialize_fn* serializer_fn = NULL;
	cf_component_cleanup_fn* cleanup_fn = NULL;
	void* serializer_udata = NULL;
	void* cleanup_udata = NULL;
};

struct cf_entity_config_t
{
	void clear()
	{
		entity_type = NULL;
		component_types.clear();
		schema.clear();
	}

	const char* entity_type = NULL;
	cute::array<const char*> component_types;
	cute::string_t schema;
};

using cf_entity_type_t = uint16_t;
#define CF_INVALID_ENTITY_TYPE ((uint16_t)~0)

struct cf_ecs_arrays_t
{
	int count = 0;
	cf_handle_t* entities = NULL;
	cute::array<const char*> types = NULL;
	cute::array<cf_typeless_array>* ptrs;

	void* find_components(const char* type)
	{
		type = sintern(type);
		for (int i = 0; i < count; ++i) {
			if (types[i] == type) {
				return (*ptrs)[i].data();
			}
		}
		return NULL;
	}
};

struct font_internal_t;

struct cf_app_t
{
	// App stuff.
	float dt = 0;
	bool running = true;
	int options = 0;
	void* platform_handle = NULL;
	SDL_Window* window = NULL;
	cs_context_t* cute_sound = NULL;
	bool spawned_mix_thread = false;
	cf_threadpool_t* threadpool = NULL;
	cf_audio_system_t* audio_system = NULL;
	bool gfx_enabled = false;
	sg_context_desc gfx_ctx_params;
	int w;
	int h;
	int x;
	int y;
	bool offscreen_enabled = false;
	bool fetched_offscreen = false;
	sg_image offscreen_color_buffer;
	sg_image offscreen_depth_buffer;
	sg_pass offscreen_pass;
	sg_buffer quad;
	sg_shader offscreen_shader;
	sg_pipeline offscreen_to_screen_pip;
	cf_v2 upscale;
	int offscreen_w;
	int offscreen_h;
	cf_window_state_t window_state;
	cf_window_state_t window_state_prev;
	bool using_imgui = false;
	sg_imgui_t sg_imgui;

	// Input stuff.
	cute::array<char> ime_composition;
	int ime_composition_cursor = 0;
	int ime_composition_selection_len = 0;
	cute::array<int> input_text;
	int keys[512] = { 0 };
	int keys_prev[512] = { 0 };
	float keys_duration[512] = { 0 };
	cf_mouse_state_t mouse, mouse_prev;
	cf_list_t joypads;
	cute::array<cf_touch_t> touches;

	// Batch stuff.
	cf_batch_t* ase_batch = NULL;
	cf_aseprite_cache_t* ase_cache = NULL;
	cf_batch_t* png_batch = NULL;
	cf_png_cache_t* png_cache = NULL;

	// ECS stuff.
	// TODO: Set allocator context for these data structures.
	cf_system_internal_t system_internal_builder;
	cute::array<cf_system_internal_t> systems;
	cf_entity_config_t entity_config_builder;
	cf_entity_type_t entity_type_gen = 0;
	cute::dictionary<const char*, cf_entity_type_t> entity_type_string_to_id;
	cute::array<const char*> entity_type_id_to_string;
	cute::dictionary<cf_entity_type_t, cf_entity_collection_t> entity_collections;
	cf_entity_type_t current_collection_type_being_iterated = ~0;
	cf_entity_collection_t* current_collection_being_updated = NULL;
	cute::array<cf_entity_t> delayed_destroy_entities;
	cute::array<cf_entity_t> delayed_deactivate_entities;
	cute::array<cf_entity_t> delayed_activate_entities;
	cf_ecs_arrays_t ecs_arrays;

	cf_component_config_t component_config_builder;
	cute::dictionary<const char*, cf_component_config_t> component_configs;
	cute::dictionary<cf_entity_type_t, cf_kv_t*> entity_parsed_schemas;
	cute::dictionary<cf_entity_type_t, uint16_t> entity_schema_inheritence;

	cute::dictionary<cf_entity_t, int>* save_id_table = NULL;
	cute::array<cf_entity_t>* load_id_table = NULL;

	// Font stuff.
	uint64_t font_id_gen = 0;
	cute::dictionary<uint64_t, font_internal_t*> fonts;
};

#endif // CUTE_APP_INTERNAL_H
