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
	bool was_bonked = false;
	bool is_moving = false;
	entity_t notify_player_when_done = INVALID_ENTITY;

	int x = 0;
	int y = 0;
	int x0 = 0;
	int y0 = 0;

	float t = 0;
	float delay = 0;
	v2 a, c0, b;

	CUTE_INLINE void linear(int to_x, int to_y, float delay)
	{
		is_moving = true;
		x0 = x, y0 = y, x = to_x, y = to_y;
		a = tile2world(x0, y0);
		b = tile2world(x, y);
		c0 = (a + b) * 0.5f;
		this->delay = delay;
	}

	CUTE_INLINE void hop(int to_x, int to_y, float delay, float h = 5.0f)
	{
		is_moving = true;
		x0 = x, y0 = y, x = to_x, y = to_y;
		a = tile2world(x0, y0);
		b = tile2world(x, y);
		c0 = (a + b) * 0.5f + v2(0, h);
		this->delay = delay;
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
	}

	CUTE_INLINE v2 interpolate() const
	{
		return bezier(a, c0, b, smoothstep(t / delay));
	}
};

CUTE_INLINE cute::error_t BoardPiece_serialize(app_t* app, kv_t* kv, entity_t entity, void* component, void* udata)
{
	BoardPiece* board_piece = (BoardPiece*)component;
	if (kv_get_state(kv) == KV_STATE_READ) {
		CUTE_PLACEMENT_NEW(board_piece) BoardPiece;
	}
	kv_key(kv, "x"); kv_val(kv, &board_piece->x);
	kv_key(kv, "y"); kv_val(kv, &board_piece->y);
	return kv_error_state(kv);
}

#endif // BOARD_PIECE_H
