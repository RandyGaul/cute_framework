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
	char code;
	bool is_empty = true;
	bool is_ladder = false;
	entity_t entity = INVALID_ENTITY;
};

struct Board
{
	int start_x;
	int start_y;
	int w, h;
	array<array<BoardSpace>> data;
	array<batch_quad_t> background_bricks;
};

struct World
{
	bool load_level_dirty_flag = false;
	int level_index = 0;
	entity_t player;
	Board board;
	bool loaded_level_into_editor = false;

	CUTE_INLINE void next_level(int index)
	{
		load_level_dirty_flag = true;
		level_index = index + 1;
	}
};

extern array<array<string_t>> levels;
extern World* world;
extern aseprite_cache_t* cache;
extern batch_t* batch;
extern app_t* app;

void init_world();
void load_level();
sprite_t load_sprite(string_t path);
v2 tile2world(int x, int y);
void world2tile(v2 p, int* x_out, int* y_out);
int sort_bits(v2 p);
int sort_bits(int x, int y);
bool in_grid(int x, int y, int w, int h);
bool in_board(int x, int y);
void draw_background_bricks_system_pre_update(app_t* app, float dt, void* udata);

#endif // WORLD_H
