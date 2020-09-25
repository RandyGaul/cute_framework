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

sprite_t ice_block_idle;
sprite_t ice_block_mask;
sprite_t big_ice_block_idle;
sprite_t big_ice_block_mask;

void ice_block_system_init()
{
	ice_block_mask = ice_block_idle = load_sprite("ice_block.aseprite");
	ice_block_idle.play("idle");
	ice_block_mask.play("mask");
	big_ice_block_mask = big_ice_block_idle = load_sprite("big_ice_block.aseprite");
	big_ice_block_idle.play("idle");
	big_ice_block_mask.play("mask");
}

static float s_float_offset(IceBlock* ice_block, float dt)
{
	static float floating_offset = 0;
	coroutine_t* co = &ice_block->float_co;
	COROUTINE_START(co);
	floating_offset = 1.0f;
	COROUTINE_PAUSE(co, IceBlock::float_delay, dt);
	floating_offset = 2.0f;
	COROUTINE_PAUSE(co, IceBlock::float_delay, dt);
	floating_offset = 3.0f;
	COROUTINE_PAUSE(co, IceBlock::float_delay, dt);
	floating_offset = 2.0f;
	COROUTINE_PAUSE(co, IceBlock::float_delay, dt);
	COROUTINE_END(co);
	return floating_offset;
}

void ice_block_system_pre_update(app_t* app, float dt, void* udata)
{
	ice_block_idle.update(dt);
	big_ice_block_idle.update(dt);
}

static BoardPiece* s_get_piece(int x, int y, int xdir, int ydir)
{
	if (!in_board(x + xdir, y - ydir)) return NULL;
	BoardSpace space = world->board.data[y - ydir][x + xdir];
	if (space.is_empty) return NULL;
	return (BoardPiece*)app_get_component(app, space.entity, "BoardPiece");
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
			animator->sprite.play("idle");

			if (ice_block->was_thrown) {
				ice_block->was_thrown = false;
				CUTE_ASSERT(ice_block->xdir || ice_block->ydir);
				BoardPiece* other = s_get_piece(board_piece->x, board_piece->y, ice_block->xdir, ice_block->ydir);
				if (other) {
					other->was_bonked = true;
					other->bonk_xdir = board_piece->xdir;
					other->bonk_ydir = board_piece->ydir;
				}
				if (ice_block->fire != INVALID_ENTITY) {
					delayed_destroy_entity_at(board_piece->x, board_piece->y);
					app_delayed_destroy_entity(app, ice_block->fire);
				}
			}

			COROUTINE_CASE(co, IDLE_INNER);
			if (ice_block->is_held) {
				if (!board_piece->is_moving) {
					goto FLOATING;
				} else {
					goto SLIDING;
				}
			}
			COROUTINE_YIELD(co);
			// Overwrite the Animator's sprite with this static one to keep all of them in-sync
			// together no matter what. If different ice block idle animations start playing out
			// of sync it looks visiually jarring.
			int sort_bits = animator->sprite.sort_bits;
			animator->sprite = ice_block->big ? big_ice_block_idle : ice_block_idle;
			animator->sprite.sort_bits = sort_bits;
			goto IDLE_INNER;
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
			transform->local.p.y += s_float_offset(ice_block, dt);

			if (!ice_block->is_held) {
				if (board_piece->is_moving) {
					goto SLIDING;
				}
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
			transform->local.p.y += 2.0f;

			if (!board_piece->is_moving) {
				COROUTINE_YIELD(co);
				goto IDLE;
			} else {
				COROUTINE_YIELD(co);
				goto SLIDING;
			}
		}

		COROUTINE_END(co);

		// Push masks onto the reflection system.
		transform_t tx = transform->get();
		if (ice_block->big) {
			big_ice_block_mask.sort_bits = sort_bits(board_piece->x, board_piece->y);
			reflection_system->masks.add(big_ice_block_mask.batch_sprite(tx));
		} else {
			ice_block_mask.sort_bits = sort_bits(board_piece->x, board_piece->y);
			reflection_system->masks.add(ice_block_mask.batch_sprite(tx));
		}
	}
}
