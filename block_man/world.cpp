/*
	Cute Framework
	Copyright (C) 2020 Randy Gaul https://randygaul.net

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

#include <world.h>
#include <imgui/imgui.h>

#include <systems/animator_system.h>
#include <systems/board_system.h>
#include <systems/ice_block_system.h>
#include <systems/player_system.h>
#include <systems/reflection_system.h>

#include <components/animator.h>
#include <components/block.h>
#include <components/board_piece.h>
#include <components/ladder.h>
#include <components/player.h>
#include <components/reflection.h>
#include <components/transform.h>

World world_instance;
World* world = &world_instance;

#define REGISTER_COMPONENT(name) \
	app_register_component_type(app, { \
		CUTE_STRINGIZE(name), \
		sizeof(name), \
		NULL, name##_serialize, \
	})

void ecs_registration(app_t* app)
{
	REGISTER_COMPONENT(Transform);
	REGISTER_COMPONENT(Animator);
	REGISTER_COMPONENT(Player);

	app_register_entity_type(app, CUTE_STRINGIZE(
		entity_type = "Player",
		Transform = { },
		Animator = { name = "data/girl.aseprite", },
		Player = { },
	));

	app_register_system(app, {
		NULL, animator_system_update, {
			"Transform",
			"Animator",
		}
	});
}

void init_world()
{
	int options = CUTE_APP_OPTIONS_WINDOW_POS_CENTERED | CUTE_APP_OPTIONS_D3D11_CONTEXT;
	app = app_make("Block Man", 0, 0, 960, 720, options);
	file_system_mount(file_system_get_base_dir(), "");
	app_init_upscaling(app, UPSCALE_PIXEL_PERFECT_AT_LEAST_2X, 320, 240);
	ImGui::SetCurrentContext(app_init_imgui(app));

	cache = aseprite_cache_make(app);
	batch = aseprite_cache_get_batch_ptr(cache);

	ecs_registration(app);
}

sprite_t load_sprite(string_t path)
{
	sprite_t s;
	error_t err = aseprite_cache_load(cache, path.c_str(), &s);
	if (err.is_error()) {
		char buf[1024];
		sprintf(buf, "Unable to load sprite at path \"%s\".\n", path.c_str());
		app_window_message_box(app, APP_MESSAGE_BOX_TYPE_ERROR, "ERROR", buf);
	}
	return s;
}

v2 tile2world(int sprite_h, int x, int y)
{
	float w = 16; // width of tiles in pixels
	float h = 16;
	float y_offset = world->board.data.count() * h;
	float y_diff = sprite_h > 16 ? (sprite_h - h) / 2 : 0;
	return v2((float)(x-6) * w, -(float)(y+6) * h + y_offset + y_diff);
}

void world2tile(int sprite_h, v2 p, int* x_out, int* y_out)
{
	float w = 16; // width of tiles in pixels
	float h = 16;
	float y_offset = world->board.data.count() * h;
	float y_diff = sprite_h > 16 ? (sprite_h - h) / 2 : 0;
	float x = p.x / w + 6;
	float y = -((p.y - y_offset - y_diff) / h) - 6;
	*x_out = (int)round(x);
	*y_out = (int)round(y);
}
