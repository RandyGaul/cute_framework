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

#include <systems/player_system.h>

#include <components/transform.h>
#include <components/animator.h>
#include <components/board_piece.h>
#include <components/player.h>
#include <components/ice_block.h>

void set_player_animation_based_on_facing_direction(Player* player, Animator* animator)
{
	const char* animation = NULL;
	bool flip = false;
	if (player->holding) {
		if (player->xdir) {
			animation = "hold_side";
			if (player->xdir > 0) flip = true;
		} else {
			if (player->ydir > 0) {
				animation = "hold_up";
			} else {
				animation = "hold_down";
			}
		}
	} else {
		if (player->xdir) {
			animation = "side";
			if (player->xdir > 0) flip = true;
		} else if (player->ydir) {
			if (player->ydir > 0) {
				animation = "up";
			} else {
				animation = "idle";
			}
		}
	}
	CUTE_ASSERT(animation);
	animator->sprite.play(animation);
	if (flip) animator->flip_x();
	else animator->unflip_x();
}

bool handle_input(app_t* app, float dt, BoardPiece* board_piece, Player* player)
{
	bool update_hero_animation = false;
	int x = board_piece->x;
	int y = board_piece->y;

	if (key_was_pressed(app, KEY_Z) | key_was_pressed(app, KEY_SPACE)) {
		if (!player->holding) {
			// search forward from player to look for blocks to pick up
			int sx = x + player->xdir, sy = y - player->ydir;
			bool found = false;
			int distance = 0;
			while (in_grid(sy, sx, world->board.data.count(), world->board.data[0].count())) {
				bool has_something = !world->board.data[sy][sx].is_empty;
				bool is_ice_block = has_something && app_entity_is_type(app, world->board.data[sy][sx].entity, "IceBlock");
				if (is_ice_block) {
					found = true;
					break;
				} else if (has_something) {
					break;
				}
				sx += player->xdir;
				sy -= player->ydir;
				++distance;
			}

			if (found) {
				++world->moves;
				entity_t block = world->board.data[sy][sx].entity;
				BoardPiece* block_board_piece = (BoardPiece*)app_get_component(app, block, "BoardPiece");
				block_board_piece->linear(x + player->xdir, y - player->ydir, Player::move_delay * distance);
				block_board_piece->notify_player_when_done = player->entity;
				player->holding = true;
				update_hero_animation = true;
				player->busy = true;
				IceBlock* ice_block = (IceBlock*)app_get_component(app, block, "IceBlock");
				ice_block->is_held = true;
			}
		} else {
			// Pushing blocks forward.
			++world->moves;
			int sx = x + player->xdir * 2, sy = y - player->ydir * 2;
			int distance = 0;
			bool found_fire = false;
			while (in_grid(sy, sx, world->board.data.count(), world->board.data[0].count()))
			{
				if (!world->board.data[sy][sx].is_empty)
				{
					if (app_get_component(app, world->board.data[sy][sx].entity, "Fire")) {
						found_fire = true;
					}
					break;
				}
				sx += player->xdir;
				sy -= player->ydir;
				++distance;
			}
			entity_t block = world->board.data[y - player->ydir][x + player->xdir].entity;
			IceBlock* ice_block = (IceBlock*)app_get_component(app, block, "IceBlock");
			ice_block->is_held = false;
			ice_block->xdir = player->xdir;
			ice_block->ydir = player->ydir;
			ice_block->was_thrown = true;
			if (found_fire) {
				ice_block->fire = world->board.data[sy][sx].entity;
			}
			BoardPiece* block_board_piece = (BoardPiece*)app_get_component(app, block, "BoardPiece");
			if (found_fire) {
				// Move one farther onto the fire space itself.
				block_board_piece->linear(sx, sy, Player::move_delay * (distance + 1));
			} else {
				block_board_piece->linear(sx - player->xdir, sy + player->ydir, Player::move_delay * distance);
			}
			block_board_piece->notify_player_when_done = player->entity;
			player->holding = false;
			player->busy = true;
			update_hero_animation = true;
		}
	} else {
		// Handling movement keys.
		key_button_t keycodes[4] = { KEY_W , KEY_S, KEY_D, KEY_A };
		key_button_t keycodes_arrows[4] = { KEY_UP , KEY_DOWN, KEY_RIGHT, KEY_LEFT };
		int xdirs[4] = {  0,  0,  1, -1 };
		int ydirs[4] = {  1, -1,  0,  0 };
		int xmove[4] = {  0,  0,  1, -1 };
		int ymove[4] = { -1,  1,  0,  0 };

		for (int i = 0; i < 4; ++i)
		{
			if (!key_mod_bit_flags(app) && key_was_pressed(app, keycodes[i]) || key_was_pressed(app, keycodes_arrows[i]))
			{
				if (player->holding)
				{
					// if moving backwards, keep same direction, just back up
					if (player->xdir == -xdirs[i] && player->ydir == -ydirs[i])
					{
						// make sure we don't push block through a wall
						if (world->board.data[y + player->ydir][x - player->xdir].is_empty)
						{
							++world->moves;

							// move backward
							x += xmove[i];
							y += ymove[i];

							// update hero position
							board_piece->hop(x, y, Player::move_delay);
							player->busy = true;

							// then, move the block
							entity_t block = world->board.data[y - player->ydir * 2][x + player->xdir * 2].entity;
							BoardPiece* block_board_piece = (BoardPiece*)app_get_component(app, block, "BoardPiece");
							block_board_piece->linear(x + player->xdir, y - player->ydir, Player::move_delay);
							break;
						}
					}
					// if moving forwards
					else if (player->xdir == xdirs[i] && player->ydir == ydirs[i])
					{
						// make sure we don't push block through a wall
						if (world->board.data[y - player->ydir * 2][x + player->xdir * 2].is_empty)
						{
							++world->moves;

							// first, move the block
							entity_t block = world->board.data[y - player->ydir][x + player->xdir].entity;
							BoardPiece* block_board_piece = (BoardPiece*)app_get_component(app, block, "BoardPiece");
							block_board_piece->linear(x + player->xdir * 2, y - player->ydir * 2, Player::move_delay);
							block_board_piece->notify_player_when_done = player->entity;

							// move forward
							x += xmove[i];
							y += ymove[i];

							// update hero position
							board_piece->hop(x, y, Player::move_delay);
							player->busy = true;
							break;
						}
					}
					else // if turning 90 degrees
					{
						// turn
						int xdirtemp = xdirs[i];
						int ydirtemp = ydirs[i];

						// check for something
						if (world->board.data[y - ydirtemp][x + xdirtemp].is_empty)
						{
							// also check to make sure we aren't turning through a block
							if (world->board.data[y - player->ydir - ydirtemp][x + player->xdir + xdirtemp].is_empty)
							{
								++world->moves;
								entity_t block = world->board.data[y - player->ydir][x + player->xdir].entity;

								// turn hero
								player->xdir = xdirtemp;
								player->ydir = ydirtemp;
								player->busy = true;

								// and move block
								BoardPiece* block_board_piece = (BoardPiece*)app_get_component(app, block, "BoardPiece");
								block_board_piece->rotate(x + player->xdir, y - player->ydir, x, y, Player::move_delay);
								block_board_piece->notify_player_when_done = player->entity;
							}
						}

						update_hero_animation = true;
						break;
					}
				}
				else // not holding a block
				{
					if (player->xdir == xdirs[i] && player->ydir == ydirs[i])
					{
						// move forward
						x += xmove[i];
						y += ymove[i];

						// check for collisions
						// if we did't collide, assign the new position
						if (world->board.data[y][x].is_empty || world->board.data[y][x].is_ladder) {
							++world->moves;

							bool won = world->board.data[y][x].is_ladder;
							if (won) {
								player->won = true;
								player->ladder = world->board.data[y][x].entity;
							}

							// update hero position
							board_piece->hop(x, y, Player::move_delay);
							player->busy = true;
							board_piece->notify_player_when_done = player->entity;
							break;
						}
					}
					else
					{
						++world->moves;
						// turn 
						player->xdir = xdirs[i];
						player->ydir = ydirs[i];
						update_hero_animation = true;
						break;
					}
				}
			}
		}
	}

	return update_hero_animation;
}

void player_system_update(app_t* app, float dt, void* udata, Transform* transforms, Animator* animators, BoardPiece* board_pieces, Player* players, int entity_count)
{
	for (int i = 0; i < entity_count; ++i) {
		Transform* transform = transforms + i;
		Animator* animator = animators + i;
		BoardPiece* board_piece = board_pieces + i;
		Player* player = players + i;

		if (player->busy) continue;

		if (!board_piece->is_moving) {
			if (!player->won) {
				bool update_anim = handle_input(app, dt, board_piece, player);
				if (update_anim) set_player_animation_based_on_facing_direction(player, animator);
			} else {
				coroutine_t* co = &player->co;
				COROUTINE_START(co);
				animator->sprite.play("ladder");
				COROUTINE_CASE(co, GOING_DOWN_LADDER);
				if (!animator->sprite.will_finish(dt)) {
					COROUTINE_YIELD(co);
					transform->local.p -= animator->sprite.local_offset;
					goto GOING_DOWN_LADDER;
				} else {
					// When the player moves onto a ladder it steals the board space of the ladder, which prevents
					// the world from cleaning up. Trigger a delayed cleanup of the ladder entity right here so it
					// doesn't leak.
					app_delayed_destroy_entity(app, player->ladder);
					player->won = false;
					animator->visible = false; // Animation looped once -- don't want to see a frame of girl on top of ladder again.
					world->next_level(world->level_index + 1);
				}
				COROUTINE_END(co);
			}
		}
	}
}
