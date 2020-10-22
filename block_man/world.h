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

#ifndef WORLD_H
#define WORLD_H

#include <cute.h>
using namespace cute;

struct BoardSpace
{
	char code = '0';
	bool is_empty = true;
	entity_t entity = INVALID_ENTITY;
};

struct Board
{
	int start_x;
	int start_y;
	array<array<BoardSpace>> data;
	array<batch_sprite_t> background_bricks;
};

struct World
{
	static constexpr int TILE_W = 16;
	static constexpr int TILE_H = 16;
	static constexpr int LEVEL_W = 20;
	static constexpr int LEVEL_H = 15;

	bool load_level_dirty_flag = false;
	int level_index = 0;
	const char* level_name = NULL;
	entity_t player = INVALID_ENTITY;
	Board board;
	bool loaded_level_into_editor = false;
	int moves = 0;

	array<const char*> levels;
	array<const char*> level_names;

	bool lose_screen = false;

	CUTE_INLINE void next_level(int index)
	{
		load_level_dirty_flag = true;
		level_index = index;
		moves = 0;
	}
};

extern World* world;
extern aseprite_cache_t* cache;
extern batch_t* batch;
extern app_t* app;
extern array<const char*> schemas;

struct schema_preview_t
{
	const char* entity_type;
	sprite_t sprite;
	texture_t tex;
	float w, h;
};
extern array<schema_preview_t> schema_previews;

void init_world(int argc, const char** argv);
void destroy_all_entities();
void select_level(int index);
int select_level(const char* name);
void reload_level(const char* name);
sprite_t load_sprite(string_t path);
v2 tile2world(int x, int y);
void world2tile(v2 p, int* x_out, int* y_out);
int sort_bits(v2 p);
int sort_bits(int x, int y);
bool in_grid(int x, int y, int w, int h);
bool in_board(int x, int y);
void draw_background_bricks_system_pre_update(app_t* app, float dt, void* udata);
void make_entity_at(const char* entity_type, int x, int y);
void make_entity_at(int selection, int x, int y);
void destroy_entity_at(int x, int y);
void delayed_destroy_entity_at(int x, int y);
void debug_draw_non_empty_board_spaces();

void play_sound(const char* path, float volume = 1.0f);

#endif // WORLD_H
