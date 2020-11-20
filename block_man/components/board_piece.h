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

#ifndef BOARD_PIECE_H
#define BOARD_PIECE_H

#include <cute.h>
using namespace cute;

#include <world.h>

struct BoardPiece
{
	int xdir = 0;
	int ydir = 0;
	bool was_bonked = false;
	int bonk_xdir = 0;
	int bonk_ydir = 0;

	bool is_moving = false;
	entity_t self = INVALID_ENTITY;
	entity_t notify_player_when_done = INVALID_ENTITY;

	int x = 0;
	int y = 0;
	int x0 = 0;
	int y0 = 0;

	float t = 0;
	float delay = 0;
	v2 a, c0, b;

	bool has_replicas = false;
	int x_replicas[3];
	int y_replicas[3];

	CUTE_INLINE void linear(int to_x, int to_y, float delay)
	{
		is_moving = true;
		x0 = x, y0 = y, x = to_x, y = to_y;
		a = tile2world(x0, y0);
		b = tile2world(x, y);
		c0 = (a + b) * 0.5f;
		this->delay = delay;
		move();
	}

	CUTE_INLINE void hop(int to_x, int to_y, float delay, float h = 5.0f)
	{
		is_moving = true;
		x0 = x, y0 = y, x = to_x, y = to_y;
		a = tile2world(x0, y0);
		b = tile2world(x, y);
		c0 = (a + b) * 0.5f + v2(0, h);
		this->delay = delay;
		move();
	}

	CUTE_INLINE void rotate(int to_x, int to_y, int about_x, int about_y, float delay)
	{
		is_moving = true;
		x0 = x, y0 = y, x = to_x, y = to_y;
		a = tile2world(x0, y0);
		b = tile2world(x, y);
		v2 about = tile2world(about_x, about_y);
		v2 mid = (a + b) * 0.5f;
		c0 = mid + norm(mid - about) * sqrt(2.0f) * 16.0f / 2.0f;
		this->delay = delay;
		move();
	}

	CUTE_INLINE v2 interpolate() const
	{
		return bezier(a, c0, b, smoothstep(t / delay));
	}

private:
	CUTE_INLINE void move()
	{
		int dx = x - x0;
		int dy = y - y0;
		xdir = safe_norm(dx);
		ydir = safe_norm(dy);

		// Clear the old board spaces.
		CUTE_ASSERT(world->board.data[y0][x0].entity == self);
		world->board.data[y0][x0].entity = INVALID_ENTITY;
		world->board.data[y0][x0].is_empty = true;
		if (has_replicas) {
			for (int k = 0; k < 3; ++k) {
				int x0 = x_replicas[k];
				int y0 = y_replicas[k];
				CUTE_ASSERT(world->board.data[y0][x0].entity == self);
				world->board.data[y0][x0].entity = INVALID_ENTITY;
				world->board.data[y0][x0].is_empty = true;
			}
		}

		// Set new board spaces.
		world->board.data[y][x].entity = self;
		world->board.data[y][x].is_empty = false;
		if (has_replicas) {
			for (int k = 0; k < 3; ++k) {
				int x = x_replicas[k] += dx;
				int y = y_replicas[k] += dy;
				CUTE_ASSERT(world->board.data[y][x].is_empty || world->board.data[y][x].entity == self);
				world->board.data[y][x].entity = self;
				world->board.data[y][x].is_empty = false;
			}
		}
	}
};

CUTE_INLINE cute::error_t BoardPiece_serialize(app_t* app, kv_t* kv, bool reading, entity_t entity, void* component, void* udata)
{
	BoardPiece* board_piece = (BoardPiece*)component;
	if (reading) {
		CUTE_PLACEMENT_NEW(board_piece) BoardPiece;
		board_piece->self = entity;
	}
	kv_key(kv, "x"); kv_val(kv, &board_piece->x);
	kv_key(kv, "y"); kv_val(kv, &board_piece->y);
	kv_key(kv, "big"); kv_val(kv, &board_piece->has_replicas);
	return kv_error_state(kv);
}

#endif // BOARD_PIECE_H
