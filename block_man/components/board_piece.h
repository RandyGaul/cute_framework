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

struct BoardPiece
{
	uint8_t code = '1';
	int x = 0;
	int y = 0;
	int x0 = 0;
	int y0 = 0;

	float t = 0;
	float delay = 0.125f;
	v2 a, c0, b;

	enum state_t
	{
		STATE_IDLE,
		STATE_START_MOVING,
		STATE_IS_MOVING,
	};

	state_t state;
};

CUTE_INLINE error_t BoardPiece_serialize(app_t* app, kv_t* kv, entity_t entity, void* component, void* udata)
{
	BoardPiece* board_piece = (BoardPiece*)component;
	if (kv_get_state(kv) == KV_STATE_READ) {
		CUTE_PLACEMENT_NEW(board_piece) BoardPiece;
	}
	kv_key(kv, "code"); kv_val(kv, &board_piece->code);
	kv_key(kv, "x"); kv_val(kv, &board_piece->x);
	kv_key(kv, "y"); kv_val(kv, &board_piece->y);
	return kv_error_state(kv);
}

#endif // BOARD_PIECE_H
