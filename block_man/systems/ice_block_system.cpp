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

#include <systems/ice_block_system.h>
#include <systems/reflection_system.h>

#include <components/animator.h>
#include <components/board_piece.h>
#include <components/ice_block.h>
#include <components/transform.h>

sprite_t ice_block_mask;

void ice_block_system_init()
{
	sprite_t ice_block_mask = load_sprite("data/ice_block.aseprite");
	ice_block_mask.play("mask");
}

static float s_float_offset(IceBlock* ice_block, float dt)
{
	float floating_offset = 0;
	coroutine_t* float_co = &ice_block->float_co;
	COROUTINE_START(float_co);
	floating_offset = 1.0f;
	COROUTINE_PAUSE(float_co, IceBlock::float_delay, dt);
	floating_offset = 2.0f;
	COROUTINE_PAUSE(float_co, IceBlock::float_delay, dt);
	floating_offset = 3.0f;
	COROUTINE_PAUSE(float_co, IceBlock::float_delay, dt);
	floating_offset = 2.0f;
	COROUTINE_PAUSE(float_co, IceBlock::float_delay, dt);
	COROUTINE_END(float_co);
	return floating_offset;
}

void ice_block_system_update(app_t* app, float dt, void* udata, Transform* transforms, Animator* animators, BoardPiece* board_pieces, IceBlock* ice_blocks, int entity_count)
{
	for (int i = 0; i < entity_count; ++i) {
		Transform* transform = transforms + i;
		Animator* animator = animators + i;
		BoardPiece* board_piece = board_pieces + i;
		IceBlock* ice_block = ice_blocks + i;

		coroutine_t* co = &ice_block->co;
		COROUTINE_START(co);

		// Do nothing when idling.
		COROUTINE_CASE(co, IDLE);
		{
			if (ice_block->is_held) {
				goto FLOATING;
			} else if (ice_block->is_sliding) {
				goto SLIDING;
			}
			COROUTINE_YIELD(co);
			goto IDLE;
		}

		// Switch to sheen animation when beginning to float.
		COROUTINE_CASE(co, FLOATING);
		{
			coroutine_init(&ice_block->float_co);
			animator->sprite.play("sheen");
		}

		// Pulse up and down while floating.
		COROUTINE_CASE(co, FLOATING_INNER);
		{
			transform->transform.p.y += s_float_offset(ice_block, dt);

			if (!board_piece->is_moving) {
				ice_block->is_held = 0;
				COROUTINE_YIELD(co);
				goto IDLE;
			} else {
				COROUTINE_YIELD(co);
				goto FLOATING_INNER;
			}
		}

		// Small vertical offset while sliding.
		COROUTINE_CASE(co, SLIDING);
		{
			transform->transform.p.y += 2.0f;

			if (!board_piece->is_moving) {
				ice_block->is_sliding = 0;
				COROUTINE_YIELD(co);
				goto IDLE;
			} else {
				COROUTINE_YIELD(co);
				goto SLIDING;
			}
		}

		COROUTINE_END(co);

		// Push masks onto the reflection system.
		ice_block_mask.sort_bits = animator->sprite.sort_bits;
		reflection_system->masks.add(ice_block_mask.quad(transform->transform));
	}
}
