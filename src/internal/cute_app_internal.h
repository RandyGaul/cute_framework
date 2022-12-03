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
#include <cute_graphics.h>
#include <cute_input.h>
#include <cute_string.h>

#include <internal/cute_draw_internal.h>

#include <sokol/sokol_gfx_imgui.h>

#define CUTE_RETURN_IF_ERROR(x) do { CF_Result err = (x); if (cf_is_error(err)) return err; } while (0)

struct SDL_Window;
struct cs_context_t;

extern struct CF_App* app;

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
	Cute::HandleTable entity_handle_table;
	Cute::Array<CF_Handle> entity_handles; // TODO - Replace with a counter? Or delete?
	Cute::Array<const char*> component_type_tuple;
	Cute::Array<CF_TypelessArray> component_tables;
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
	CF_SystemUpdateFn* update_fn = NULL;
	void (*post_update_fn)(float dt, void* udata) = NULL;
	Cute::Array<const char*> component_type_tuple;
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
	CF_ComponentSerializeFn* serializer_fn = NULL;
	CF_ComponentCleanupFn* cleanup_fn = NULL;
	void* serializer_udata = NULL;
	void* cleanup_udata = NULL;
};

struct cf_entity_config_t
{
	void clear()
	{
		Entityype = NULL;
		component_types.clear();
		schema.clear();
	}

	const char* Entityype = NULL;
	Cute::Array<const char*> component_types;
	Cute::String schema;
};

using CF_Entityype_t = uint16_t;
#define CF_INVALID_ENTITY_TYPE ((uint16_t)~0)

struct CF_EcsArrays
{
	int count = 0;
	CF_Handle* entities = NULL;
	Cute::Array<const char*> types = NULL;
	Cute::Array<CF_TypelessArray>* ptrs;

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

struct FontInternal;

struct CF_App
{
	// App stuff.
	float dt = 0;
	bool running = true;
	int options = 0;
	void* platform_handle = NULL;
	SDL_Window* window = NULL;
	cs_context_t* cute_sound = NULL;
	bool spawned_mix_thread = false;
	CF_Threadpool* threadpool = NULL;
	cf_audio_system_t* audio_system = NULL;
	bool gfx_enabled = false;
	sg_context_desc gfx_ctx_params;
	int w;
	int h;
	int x;
	int y;
	int draw_call_count = 0;
	CF_Texture backbuffer;
	CF_Texture backbuffer_depth_stencil;
	CF_Canvas offscreen_canvas;
	CF_Canvas backbuffer_canvas;
	CF_Mesh backbuffer_quad;
	CF_Shader backbuffer_shader;
	CF_Material backbuffer_material;
	cf_window_state_t window_state;
	cf_window_state_t window_state_prev;
	bool using_imgui = false;
	sg_imgui_t sg_imgui;
	uint64_t default_image_id = CUTE_PNG_ID_RANGE_LO;

	// Input stuff.
	Cute::Array<char> ime_composition;
	int ime_composition_cursor = 0;
	int ime_composition_selection_len = 0;
	Cute::Array<int> input_text;
	int keys[512] = { 0 };
	int keys_prev[512] = { 0 };
	float keys_duration[512] = { 0 };
	cf_mouse_state_t mouse, mouse_prev;
	CF_List joypads;
	Cute::Array<CF_Touch> touches;

	// ECS stuff.
	// TODO: Set allocator context for these data structures.
	cf_system_internal_t system_internal_builder;
	Cute::Array<cf_system_internal_t> systems;
	cf_entity_config_t entity_config_builder;
	CF_Entityype_t Entityype_gen = 0;
	Cute::Dictionary<const char*, CF_Entityype_t> Entityype_string_to_id;
	Cute::Array<const char*> Entityype_id_to_string;
	Cute::Dictionary<CF_Entityype_t, cf_entity_collection_t> entity_collections;
	CF_Entityype_t current_collection_type_being_iterated = ~0;
	cf_entity_collection_t* current_collection_being_updated = NULL;
	Cute::Array<CF_Entity> delayed_destroy_entities;
	Cute::Array<CF_Entity> delayed_deactivate_entities;
	Cute::Array<CF_Entity> delayed_activate_entities;
	CF_EcsArrays ecs_arrays;

	cf_component_config_t component_config_builder;
	Cute::Dictionary<const char*, cf_component_config_t> component_configs;
	Cute::Dictionary<CF_Entityype_t, CF_KeyValue*> entity_parsed_schemas;
	Cute::Dictionary<CF_Entityype_t, uint16_t> entity_schema_inheritence;

	Cute::Dictionary<CF_Entity, int>* save_id_table = NULL;
	Cute::Array<CF_Entity>* load_id_table = NULL;

	// Font stuff.
	uint64_t font_id_gen = 0;
	Cute::Dictionary<uint64_t, FontInternal*> fonts;
};

#endif // CUTE_APP_INTERNAL_H
