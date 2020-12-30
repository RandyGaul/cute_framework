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

#ifndef LAMP_H
#define LAMP_H

#include <cute.h>
using namespace cute;

#include <components/board_piece.h>

#include <systems/light_system.h>

struct Lamp
{
	entity_t self;
	int oil_count = 50;
	int oil_capacity = 50;
	bool is_held = false;

	CUTE_INLINE void add_oil(int oil)
	{
		oil_count = clamp(oil_count + oil, 0, oil_capacity);
		float t = (float)oil_count / oil_capacity;
		Darkness::lerp_to(t);
	}

	CUTE_INLINE void position(int* x, int* y)
	{
		BoardPiece* board_piece = (BoardPiece*)entity_get_component(app, self, "BoardPiece");
		*x = board_piece->x;
		*y = board_piece->y;
	}
};

extern Lamp* LAMP;

CUTE_INLINE cute::error_t Lamp_serialize(app_t* app, kv_t* kv, bool reading, entity_t entity, void* component, void* udata)
{
	Lamp* lamp = (Lamp*)component;
	if (reading) {
		CUTE_PLACEMENT_NEW(lamp) Lamp;
		lamp->self = entity;
		LAMP = lamp;
		Darkness::lerp_to(1.0f);
	}
	return kv_error_state(kv);
}

CUTE_INLINE void Lamp_cleanup(app_t* app, entity_t entity, void* component, void* udata)
{
	LAMP = NULL;
}

#endif // LAMP_H
