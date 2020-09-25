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

#ifndef ICE_BLOCK_H
#define ICE_BLOCK_H

#include <cute.h>
using namespace cute;

#include <components/animator.h>
#include <cute/cute_coroutine.h>

struct IceBlock
{
	static constexpr float float_delay = 0.35f;

	bool big = false;
	bool is_held = false;
	coroutine_t co = { 0 };
	coroutine_t float_co = { 0 };

	bool was_thrown = false;
	int xdir = 0;
	int ydir = 0;
	entity_t fire = INVALID_ENTITY;
};

CUTE_INLINE cute::error_t IceBlock_serialize(app_t* app, kv_t* kv, entity_t entity, void* component, void* udata)
{
	IceBlock* ice_block = (IceBlock*)component;
	if (kv_get_state(kv) == KV_STATE_READ) {
		CUTE_PLACEMENT_NEW(ice_block) IceBlock;
		Animator* animator = (Animator*)app_get_component(app, entity, "Animator");
		animator->sprite.play("idle");
	}
	kv_key(kv, "big"); kv_val(kv, &ice_block->big);
	return kv_error_state(kv);
}

#endif // ICE_BLOCK_H
