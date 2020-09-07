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

#ifndef PLAYER_H
#define PLAYER_H

#include <cute.h>
using namespace cute;

#include <components/animator.h>

#include <cute/cute_coroutine.h>

struct Player
{
	static constexpr float move_delay = 0.125f;

	entity_t entity = INVALID_ENTITY;
	int xdir = 0;
	int ydir = -1;
	bool holding = false;
	bool won = false;
	bool busy = false;
	int moves = 0;
	coroutine_t co = { 0 };
	entity_t ladder = INVALID_ENTITY;

	// ----------------------------
	// For spinning upon level load.
	v2 spin_p0, spin_p;
	float spin_t = 0;
	float spin_delay;
	const float spin_delay_per_tile = 0.35f;
	// For spinning upon level load.
	// ----------------------------
};

CUTE_INLINE error_t Player_serialize(app_t* app, kv_t* kv, entity_t entity, void* component, void* udata)
{
	Player* player = (Player*)component;
	if (kv_get_state(kv) == KV_STATE_READ) {
		CUTE_PLACEMENT_NEW(player) Player;
		player->entity = entity;
		Animator* animator = (Animator*)app_get_component(app, entity, "Animator");
		animator->sprite.play("idle");
	}
	return kv_error_state(kv);
}

#endif // PLAYER_H
