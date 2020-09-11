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
#include <systems/transform_system.h>
#include <systems/shadow_system.h>
#include <systems/copycat_system.h>

#include <components/animator.h>
#include <components/board_piece.h>
#include <components/ice_block.h>
#include <components/player.h>
#include <components/reflection.h>
#include <components/transform.h>
#include <components/shadow.h>
#include <components/copycat.h>

World world_instance;
World* world = &world_instance;
aseprite_cache_t* cache;
batch_t* batch;
app_t* app;

const char* schema_ice_block = CUTE_STRINGIZE(
	entity_type = "IceBlock",
	Transform = { },
	Animator = { name = "data/ice_block.aseprite" },
	BoardPiece = { },
	IceBlock = { },
	Shadow = { },
);

const char* schema_box = CUTE_STRINGIZE(
	entity_type = "Box",
	Transform = { },
	Animator = { name = "data/box.aseprite" },
	Reflection = { },
	BoardPiece = { },
	Shadow = { },
);

const char* schema_ladder = CUTE_STRINGIZE(
	entity_type = "Ladder",
	Transform = { },
	Animator = { name = "data/ladder.aseprite" },
	Reflection = { },
	BoardPiece = { },
);

const char* schema_player = CUTE_STRINGIZE(
	entity_type = "Player",
	Transform = { },
	Animator = { name = "data/girl.aseprite", },
	Reflection = { },
	BoardPiece = { },
	Player = { },
	Shadow = { small = "true" },
);

const char* schema_copycat = CUTE_STRINGIZE(
	entity_type = "CopyCat",
	Transform = { },
	Animator = { name = "data/copycat.aseprite", },
	Reflection = { },
	BoardPiece = { },
	CopyCat = { },
	Shadow = { },
);

#define REGISTER_COMPONENT(name) \
	app_register_component_type(app, { \
		CUTE_STRINGIZE(name), \
		sizeof(name), \
		NULL, name##_serialize, \
	})

void ecs_registration(app_t* app)
{
	// Order of component registration does not matter.
	REGISTER_COMPONENT(Animator);
	REGISTER_COMPONENT(BoardPiece);
	REGISTER_COMPONENT(IceBlock);
	REGISTER_COMPONENT(Player);
	REGISTER_COMPONENT(Reflection);
	REGISTER_COMPONENT(Transform);
	REGISTER_COMPONENT(Shadow);
	REGISTER_COMPONENT(CopyCat);

	// Order of entity registration matters if using `inherits_from`.
	// Any time `inherits_from` is used, that type must have been already registered.
	app_register_entity_type(app, schema_ice_block);
	app_register_entity_type(app, schema_box);
	app_register_entity_type(app, schema_ladder);
	app_register_entity_type(app, schema_player);
	app_register_entity_type(app, schema_copycat);

	/*
	   Order of system registration is the order updates are called.
	   The flow is like so:

	   for each system
	       call system pre update
	       for each entity type with matching component set
	           call system update
	       call system post update

	   Please note that each of these three callbacks are optional and can be NULL.
	*/
	system_t s;
	s.udata = NULL;
	s.pre_update_fn = NULL;
	s.update_fn = (void*)transform_system_update;
	s.post_update_fn = NULL;
	s.component_types = {
		"Transform",
	};
	app_register_system(app, s);

	s.udata = NULL;
	s.pre_update_fn = NULL;
	s.update_fn = (void*)animator_transform_system_update;
	s.post_update_fn = NULL;
	s.component_types = {
			"Transform",
			"Animator",
	};
	app_register_system(app, s);

	s.udata = NULL;
	s.pre_update_fn = NULL;
	s.update_fn = (void*)player_system_update;
	s.post_update_fn = NULL;
	s.component_types = {
			"Transform",
			"Animator",
			"BoardPiece",
			"Player",
	};
	app_register_system(app, s);

	s.udata = NULL;
	s.pre_update_fn = NULL;
	s.update_fn = (void*)board_system_update;
	s.post_update_fn = NULL;
	s.component_types = {
			"Transform",
			"Animator",
			"BoardPiece",
	};
	app_register_system(app, s);

	s.udata = NULL;
	s.pre_update_fn = ice_block_system_pre_update;
	s.update_fn = (void*)ice_block_system_update;
	s.post_update_fn = NULL;
	s.component_types = {
			"Transform",
			"Animator",
			"BoardPiece",
			"IceBlock",
	};
	app_register_system(app, s);

	s.udata = NULL;
	s.pre_update_fn = NULL;
	s.update_fn = (void*)copycat_system_update;
	s.post_update_fn = NULL;
	s.component_types = {
			"Transform",
			"Animator",
			"BoardPiece",
			"CopyCat",
	};
	app_register_system(app, s);

	s.udata = NULL;
	s.pre_update_fn = draw_background_bricks_system_pre_update;
	s.update_fn = NULL;
	s.post_update_fn = NULL;
	s.component_types = { };
	app_register_system(app, s);

	s.udata = NULL;
	s.pre_update_fn = NULL;
	s.update_fn = (void*)shadow_system_update;
	s.post_update_fn = shadow_system_post_update;
	s.component_types = {
			"Transform",
			"Animator",
			"BoardPiece",
			"Shadow",
	};
	app_register_system(app, s);

	s.udata = NULL;
	s.pre_update_fn = NULL;
	s.update_fn = (void*)animator_system_update;
	s.post_update_fn = animator_system_post_update;
	s.component_types = {
			"Transform",
			"Animator",
	};
	app_register_system(app, s);

	s.udata = NULL;
	s.pre_update_fn = reflection_system_pre_update;
	s.update_fn = (void*)reflection_system_update;
	s.post_update_fn = reflection_system_post_update;
	s.component_types = {
			"Transform",
			"Animator",
			"Reflection",
	};
	app_register_system(app, s);
}

void init_world()
{
	int options = CUTE_APP_OPTIONS_WINDOW_POS_CENTERED;
#ifdef CUTE_WINDOWS
	options |= CUTE_APP_OPTIONS_D3D11_CONTEXT;
#else
	options |= CUTE_APP_OPTIONS_OPENGL_CONTEXT;
#endif
	app = app_make("Block Man", 0, 0, 960, 720, options);
	app_init_upscaling(app, UPSCALE_PIXEL_PERFECT_AT_LEAST_2X, 320, 240);
	ImGui::SetCurrentContext(app_init_imgui(app));

	cache = aseprite_cache_make(app);
	batch = batch_make(aseprite_cache_get_pixels_fn(cache), cache);
	batch_set_mvp(batch, matrix_ortho_2d(320, 240, 0, 0));

	ecs_registration(app);
	ice_block_system_init();
	shadow_system_init();
}

sprite_t load_sprite(string_t path)
{
	sprite_t s;
	cute::error_t err = aseprite_cache_load(cache, path.c_str(), &s);
	if (err.is_error()) {
		char buf[1024];
		sprintf(buf, "Unable to load sprite at path \"%s\".\n", path.c_str());
		app_window_message_box(app, APP_MESSAGE_BOX_TYPE_ERROR, "ERROR", buf);
	}
	return s;
}

v2 tile2world(int x, int y)
{
	float w = 16; // width of tiles in pixels
	float h = 16;
	float y_offset = world->board.data.count() * h;
	return v2((float)(x-6) * w, -(float)(y+6) * h + y_offset);
}

void world2tile(v2 p, int* x_out, int* y_out)
{
	float w = 16; // width of tiles in pixels
	float h = 16;
	float y_offset = world->board.data.count() * h;
	float x = p.x / w + 6;
	float y = -((p.y - y_offset) / h) - 6;
	*x_out = (int)round(x);
	*y_out = (int)round(y);
}

array<array<string_t>> background_maps = {
	{
		"XXXX0000XXXX00000XXX",
		"XXXX00000XXXXXX000XX",
		"00XX0000000XXX000000",
		"00000000000XXX0000XX",
		"00XX000000000X00XXXX",
		"00XXX000XX00XXX00XX0",
		"00XXXX00XX0000000000",
		"0000XX0000XX00XXX000",
		"XXX0000XX0XX00XXX000",
		"XXX0XX0XX000XXXX0000",
		"XXXXXX0XX00XXXXX00XX",
		"XXXX0X0XX00XXXXX00X0",
		"XXX0000XX000XXX0000X",
		"XXX00XXXX000XXX00XXX",
		"XXX00XXXX000XXX00XX0",
	},
	{
		"00000000000XXX0000XX",
		"XXXX00000XXXXXX000XX",
		"00XX0000000XXX000000",
		"XXXX0X0XX00XXXXX00X0",
		"00XX000000000X00XXXX",
		"00XXX000XX00XXX00XX0",
		"XXXX0000XXXX00000XXX",
		"00XXXX00XX0000000000",
		"0000XX0000XX00XXX000",
		"XXX0000XX000XXX0000X",
		"XXX0000XX0XX00XXX000",
		"XXX0XX0XX000XXXX0000",
		"XXXXXX0XX00XXXXX00XX",
		"XXX00XXXX000XXX00XXX",
		"XXX00XXXX000XXX00XX0",
	},
};

array<array<string_t>> levels = {
	{
		"01111110",
		"10000001",
		"10000001",
		"10000001",
		"10000001",
		"10000001",
		"10000001",
		"10000001",
		"01111110",
	},
	{
		"01111110",
		"1p0x0001",
		"01101110",
		"01101110",
		"100x0001",
		"1000xxx1",
		"1000xex1",
		"1000xxx1",
		"01111110",
	},
	{
		"00000010",
		"000001p1",
		"00000101",
		"011111x1",
		"1exxx001",
		"1xxx0001",
		"1xx00001",
		"1x000001",
		"01111110",
	},
	{
		"011110",
		"1e0x01",
		"10x0p1",
		"1x0001",
		"011110",
	},
	{
		"011100",
		"1px010",
		"1xxx01",
		"10xe10",
		"011100",
	},
	{ // 12
		"01110",
		"1px01",
		"10xe1",
		"01110",
	},
	{ // 13
		"01110",
		"1p001",
		"1x011",
		"1xxe1",
		"01110",
	},
	{ // 26
		"01110",
		"1p001",
		"1x001",
		"1x111",
		"1x0e1",
		"01110",
	},
	{ // 28
		"011110",
		"1p00x1",
		"1x00x1",
		"1111x1",
		"1e00x1",
		"011110",
	},
	{ // 23
		"011110",
		"1p00x1",
		"100xx1",
		"10xxx1",
		"1xxxe1",
		"011110",
	},
	{ // 44
		"01111110",
		"1100xx11",
		"1p001xe1",
		"1100xx11",
		"01111110",
	},
	{ // 39
		"011110",
		"1p0001",
		"1xxxx1",
		"1xx101",
		"1000e1",
		"011110",
	},
	{ // 57
		"111111111",
		"1p0011111",
		"10xxxxxx1",
		"1000111e1",
		"111111111",
	},
};

int sort_bits(v2 p)
{
	int x, y;
	world2tile(p, &x, &y);
	return world->board.w * y + x;
}

int sort_bits(int x, int y)
{
	return world->board.w * y + x;
}

bool in_grid(int x, int y, int w, int h)
{
	return x >= 0 && y >= 0 && x < w && y < h;
}

bool in_board(int x, int y)
{
	return in_grid(x, y, world->board.w, world->board.h);
}

void init_bg_bricks(int seed)
{
	rnd_t rnd = rnd_seed(seed);
	int background_index = rnd_next_range(rnd, 0, background_maps.count() - 1);

	world->board.background_bricks.clear();
	transform_t transform = make_transform();
	for (int i = 0; i < 15; ++i) {
		for (int j = 0; j < 20; ++j) {
			sprite_t sprite;
			if (background_maps[background_index][i][j] == 'X') {
				if ((i & 1) ^ (j & 1)) {
					sprite = load_sprite("data/bricks_even.aseprite");
				} else {
					sprite = load_sprite("data/bricks_odd.aseprite");
				}
			} else {
				sprite = load_sprite("data/bricks_empty.aseprite");
			}
			sprite.frame_index = rnd_next_range(rnd, 0, sprite.frame_count() - 1);
			transform.p = v2((float)(j * 16 + 8 - 320/2), (float)((15 - 1 - i) * 16 + 8 - 240/2));
			world->board.background_bricks.add(sprite.quad(transform));
		}
	}
}

void draw_background_bricks_system_pre_update(app_t* app, float dt, void* udata)
{
	for (int i = 0; i < world->board.background_bricks.count(); ++i) {
		batch_push(batch, world->board.background_bricks[i]);
	}
	batch_flush(batch);
}

void load_level()
{
	world->load_level_dirty_flag = false;

	if (world->level_index >= levels.size()) {
		char buf[1024];
		sprintf(buf, "Tried to load level %d, but the highest is %d. Loading level 0 instead.", world->level_index, levels.size() - 1);
		app_window_message_box(app, APP_MESSAGE_BOX_TYPE_ERROR, "BAD LEVEL INDEX", buf);
		world->level_index = 0;
	}

	const array<string_t>& l = levels[world->level_index];

	// Delete old entities.
	for (int i = 0; i < world->board.data.count(); ++i) {
		int len = world->board.data[i].count();

		for (int j = 0; j < len; ++j) {
			BoardSpace space = world->board.data[i][j];
			if (!space.is_empty) {
				app_destroy_entity(app, space.entity);
			}
		}
	}

	world->board.data.clear();
	world->board.data.ensure_count(l.count());
	world->board.w = l[0].len();
	world->board.h = l.count();

	// Load up new entities.
	for (int i = 0; i < l.count(); ++i) {
		const char* s = l[i].c_str();
		int len = l[i].len();

		for (int j = 0; j < len; ++j) {
			char c = s[j];
			entity_t e = INVALID_ENTITY;
			cute::error_t err;
			switch (c) {
			case '0': break;
			case '1': err = app_make_entity(app, "Box", &e); break;
			case 'x': err = app_make_entity(app, "IceBlock", &e); break;
			case 'p': err = app_make_entity(app, "Player", &e); break;
			case 'e': err = app_make_entity(app, "Ladder", &e); break;
			case 'c': err = app_make_entity(app, "CopyCat", &e); break;
			}
			CUTE_ASSERT(!err.is_error());
			BoardSpace space;
			space.code = c;
			space.entity = e;
			space.is_empty = c == '0' ? true : false;
			space.is_ladder = c == 'e' ? true : false;
			if (!space.is_empty) {
				BoardPiece* board_piece = (BoardPiece*)app_get_component(app, e, "BoardPiece");
				board_piece->x = j;
				board_piece->y = i;
			}
			world->board.data[i].add(space);
		}
	}

	init_bg_bricks(world->level_index);
}
