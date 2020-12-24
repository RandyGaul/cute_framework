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
#include <systems/light_system.h>
#include <systems/ice_block_system.h>

#include <components/transform.h>
#include <components/animator.h>
#include <components/board_piece.h>
#include <components/player.h>
#include <components/ice_block.h>
#include <components/lamp.h>

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

static bool s_player_can_move_here(int x, int y)
{
	if (!in_board(x, y)) return false;
	entity_t e = world->board.data[y][x].entity;
	if (world->board.data[y][x].is_empty) return true;
	else if (app_entity_is_type(app, e, "Ladder")) return true;
	else if (app_entity_is_type(app, e, "Oil")) return true;
	return false;
}

static bool s_is_type(int x, int y, const char* type)
{
	return app_entity_is_type(app, world->board.data[y][x].entity, type);
}

bool fit_check(int px, int py, BoardPiece* board_piece, int x, int y, bool skip_fires = true)
{
	if (x == px && y == py) return true; // Skip checking the player.
	if (!in_board(x, y)) return false;
	BoardSpace space = world->board.data[y][x];
	if (!space.is_empty && space.entity != board_piece->self && (skip_fires ? true : !s_is_type(x, y, "Fire"))) {
		return false;
	}

	return true;
}

bool can_big_ice_block_fit(int px, int py, BoardPiece* board_piece, int dx, int dy, bool skip_fires = true)
{
	if (!fit_check(px, py, board_piece, board_piece->x + dx, board_piece->y + dy, skip_fires)) {
		return false;
	}

	if (board_piece->has_replicas) {
		for (int k = 0; k < 3; ++k) {
			int x = board_piece->x_replicas[k] + dx;
			int y = board_piece->y_replicas[k] + dy;
			if (!fit_check(px, py, board_piece, x, y, skip_fires)) {
				return false;
			}
		}
	}

	return true;
}

bool can_big_ice_block_slide(int px, int py, int sx, int sy, int xdir, int ydir, int distance)
{
	BoardPiece* board_piece = (BoardPiece*)app_get_component(app, world->board.data[sy][sx].entity, "BoardPiece");
	if (!board_piece->has_replicas) return true;
	for (int i = 1; i < distance; ++i) {
		if (!can_big_ice_block_fit(px, py, board_piece, -xdir * i, ydir * i)) {
			return false;
		}
	}

	return true;
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
			bool is_ice_block = false;
			bool is_lamp = false;
			while (in_grid(sy, sx, world->board.data.count(), world->board.data[0].count())) {
				bool has_something = !world->board.data[sy][sx].is_empty;
				is_ice_block = has_something && app_has_component(app, world->board.data[sy][sx].entity, "IceBlock");
				is_lamp = has_something && app_entity_is_type(app, world->board.data[sy][sx].entity, "Lamp");
				if (is_ice_block) {
					if (!can_big_ice_block_slide(x, y, sx, sy, player->xdir, player->ydir, distance + 1)) {
						break;
					}
					found = true;
					break;
				} else if (is_lamp && distance == 0) {
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
				if (distance) {
					++world->moves;
				}
				entity_t block = world->board.data[sy][sx].entity;
				BoardPiece* block_board_piece = (BoardPiece*)app_get_component(app, block, "BoardPiece");
				block_board_piece->linear(block_board_piece->x + x + player->xdir - sx, block_board_piece->y + y - player->ydir - sy, Player::move_delay * distance);
				block_board_piece->notify_player_when_done = player->entity;
				player->holding = true;
				update_hero_animation = true;
				player->busy = true;
				if (is_ice_block) {
					IceBlock* ice_block = (IceBlock*)app_get_component(app, block, "IceBlock");
					ice_block->is_held = true;
					play_sound("block_grab.wav", 2.0f);
				} else if (is_lamp) {
					Lamp* lamp = (Lamp*)app_get_component(app, block, "Lamp");
					lamp->is_held = true;
					Animator* lamp_anim = (Animator*)app_get_component(app, block, "Animator");
					lamp_anim->sprite.local_offset.y += 1;
					play_sound("lamp_pick_up.wav", 2.0f);
				}
			}
		} else {
			// Pushing blocks forward.
			int dx = player->xdir;
			int dy = -player->ydir;
			int sx = x + player->xdir * 2, sy = y - player->ydir * 2;
			int distance = 0;
			bool found_fire = false;
			entity_t e = world->board.data[y - player->ydir][x + player->xdir].entity;
			IceBlock* ice_block = (IceBlock*)app_get_component(app, e, "IceBlock");
			BoardPiece* ice_board_piece = (BoardPiece*)app_get_component(app, e, "BoardPiece");
			array<entity_t> fires;
			array<int> fires_distance;
			BoardSpace empty_space;
			empty_space.entity = INVALID_ENTITY;
			empty_space.code = '0';
			empty_space.is_empty = true;
			while (in_board(sx, sy))
			{
				// Need the "has_replicas" check because the `can_big_ice_block_fit` function does not
				// work with fires at all the same way as little ice.
				if (ice_board_piece->has_replicas && !can_big_ice_block_fit(x, y, ice_board_piece, dx, dy)) {
					// Look for fires.
					if (!world->board.data[sy][sx].is_empty) {
						if (s_is_type(sx, sy, "Fire")) {
							fires.add(world->board.data[sy][sx].entity);
							world->board.data[sy][sx] = empty_space;
							fires_distance.add(distance);
						}
					}
					for (int k = 0; k < 3; ++k) {
						int rx = ice_board_piece->x_replicas[k] + dx;
						int ry = ice_board_piece->y_replicas[k] + dy;
						if (in_board(rx, ry) && !world->board.data[ry][rx].is_empty) {
							if (s_is_type(rx, ry, "Fire")) {
								fires.add(world->board.data[ry][rx].entity);
								world->board.data[ry][rx] = empty_space;
								fires_distance.add(distance);
							}
						}
					}
					// See if still can not fit while ignoring fires.
					if (!can_big_ice_block_fit(x, y, ice_board_piece, dx, dy, false)) {
						break;
					}
				} else if (!ice_board_piece->has_replicas && !world->board.data[sy][sx].is_empty) {
					if (app_entity_is_type(app, world->board.data[sy][sx].entity, "Fire")) {
						found_fire = true;
					}
					break;
				}
				dx += player->xdir;
				dy -= player->ydir;
				sx += player->xdir;
				sy -= player->ydir;
				++distance;
			}
			if (distance) {
				++world->moves;
			}
			for (int i = 0; i < fires.count(); ++i) {
				ice_block_system_add_fire_to_smash_by_big_block(fires[i], Player::move_delay * fires_distance[i]);
			}
			if (ice_block) {
				ice_block->is_held = false;
				ice_block->xdir = player->xdir;
				ice_block->ydir = player->ydir;
				ice_block->was_thrown = true;
				if (found_fire) {
					ice_block->fire = world->board.data[sy][sx].entity;
				}
				BoardPiece* block_board_piece = (BoardPiece*)app_get_component(app, e, "BoardPiece");
				if (found_fire) {
					// Move one farther onto the fire space itself.
					block_board_piece->linear(sx, sy, Player::move_delay * (distance + 1));
				} else {
					block_board_piece->linear(block_board_piece->x + dx - player->xdir, block_board_piece->y + dy + player->ydir, Player::move_delay * distance);
				}
				block_board_piece->notify_player_when_done = player->entity;
				player->busy = true;
				play_sound("block_throw.wav", 2.0f);
			} else {
				Lamp* lamp = (Lamp*)app_get_component(app, e, "Lamp");
				lamp->is_held = false;
				Animator* lamp_anim = (Animator*)app_get_component(app, e, "Animator");
				lamp_anim->sprite.local_offset.y -= 1;
				play_sound("lamp_put_down.wav", 2.0f);
			}
			player->holding = false;
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

		for (int i = 0; i < 4; ++i) {
			if (!key_mod_bit_flags(app) && key_was_pressed(app, keycodes[i]) || key_was_pressed(app, keycodes_arrows[i])) {
				if (player->holding) {
					// if moving backwards, keep same direction, just back up
					if (player->xdir == -xdirs[i] && player->ydir == -ydirs[i]) {
						if (!in_board(x - player->xdir, y + player->ydir)) break;
						// make sure we don't push block through a wall
						if (world->board.data[y + player->ydir][x - player->xdir].is_empty) {
							entity_t block = world->board.data[y - player->ydir][x + player->xdir].entity;
							BoardPiece* block_board_piece = (BoardPiece*)app_get_component(app, block, "BoardPiece");
							if (!can_big_ice_block_fit(x, y, block_board_piece, -player->xdir, player->ydir)) {
								break;
							}
							++world->moves;

							// move backward
							x += xmove[i];
							y += ymove[i];

							// update hero position
							board_piece->hop(x, y, Player::move_delay);
							player->busy = true;

							// then, move the block
							block_board_piece->linear(block_board_piece->x - player->xdir, block_board_piece->y + player->ydir, Player::move_delay);
							break;
						}
					} else if (player->xdir == xdirs[i] && player->ydir == ydirs[i]) { // if moving forwards
						// make sure we don't push block through a wall
						int sx = x + player->xdir * 2;
						int sy = y - player->ydir * 2;
						if (!in_board(sx, sy)) break;
						int dx = player->xdir;
						int dy = -player->ydir;
						entity_t block = world->board.data[y - player->ydir][x + player->xdir].entity;
						BoardPiece* block_board_piece = (BoardPiece*)app_get_component(app, block, "BoardPiece");
						if (world->board.data[sy][sx].is_empty || world->board.data[sy][sx].entity == block) {
							if (!can_big_ice_block_fit(x, y, block_board_piece, dx, dy)) {
								break;
							}
							++world->moves;

							// first, move the block
							block_board_piece->linear(block_board_piece->x + dx, block_board_piece->y + dy, Player::move_delay);
							block_board_piece->notify_player_when_done = player->entity;

							// move forward
							x += xmove[i];
							y += ymove[i];

							// update hero position
							board_piece->hop(x, y, Player::move_delay);
							player->busy = true;
							play_sound("step.wav", 2.0f);
							break;
						}
					} else { // if turning 90 degrees
						// turn
						int xdirtemp = xdirs[i];
						int ydirtemp = ydirs[i];

						// check for something
						if (world->board.data[y - ydirtemp][x + xdirtemp].is_empty) {
							// also check to make sure we aren't turning through a block
							if (world->board.data[y - player->ydir - ydirtemp][x + player->xdir + xdirtemp].is_empty) {
								entity_t block = world->board.data[y - player->ydir][x + player->xdir].entity;
								BoardPiece* block_board_piece = (BoardPiece*)app_get_component(app, block, "BoardPiece");
								if (block_board_piece->has_replicas) break;
								++world->moves;

								// turn hero
								player->xdir = xdirtemp;
								player->ydir = ydirtemp;
								player->busy = true;

								// and move block
								block_board_piece->rotate(x + player->xdir, y - player->ydir, x, y, Player::move_delay);
								block_board_piece->notify_player_when_done = player->entity;
							}
						}

						update_hero_animation = true;
						break;
					}
				} else { // not holding a block
					if (player->xdir == xdirs[i] && player->ydir == ydirs[i]) {
						// move forward
						x += xmove[i];
						y += ymove[i];

						// check for collisions
						// if we did't collide, assign the new position
						if (s_player_can_move_here(x, y)) {
							++world->moves;

							entity_t e = world->board.data[y][x].entity;
							bool won = app_entity_is_type(app, e, "Ladder");
							if (won) {
								player->won = true;
								player->ladder = world->board.data[y][x].entity;
							} else if (app_entity_is_type(app, e, "Oil")) {
								player->oil = e;
							}

							// update hero position
							board_piece->hop(x, y, Player::move_delay);
							player->busy = true;
							board_piece->notify_player_when_done = player->entity;
							play_sound("step.wav", 2.0f);
							break;
						}
					} else {
						//++world->moves;
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

static float s_dist(int ax, int ay, int bx, int by)
{
	float d0 = (float)(ax - bx);
	float d1 = (float)(ay - by);
	return sqrtf(d0 * d0 + d1 * d1);
}

static transform_t s_last_tx;

transform_t player_last_tx()
{
	return s_last_tx;
}

void player_system_update(app_t* app, float dt, void* udata, Transform* transforms, Animator* animators, BoardPiece* board_pieces, Player* players, int entity_count)
{
	for (int i = 0; i < entity_count; ++i) {
		Transform* transform = transforms + i;
		Animator* animator = animators + i;
		BoardPiece* board_piece = board_pieces + i;
		Player* player = players + i;

		s_last_tx = transform->get();

		if (player->busy) continue;

		if (!board_piece->is_moving) {
			if (!player->won) {
				if (LAMP && Darkness::is_dark) {
					int lamp_x, lamp_y;
					LAMP->position(&lamp_x, &lamp_y);
					float d = s_dist(lamp_x, lamp_y, board_piece->x, board_piece->y);
					float r = Darkness::radius / 16.0f;
					if (r - d < -0.5f) {
						LAMP->add_oil(-100);
						world->lose_screen = true;
					}
				}

				int old_moves = world->moves;
				bool update_anim = handle_input(app, dt, board_piece, player);
				if (old_moves != world->moves) {
					if (LAMP) LAMP->add_oil(-1);
				}
				if (update_anim) set_player_animation_based_on_facing_direction(player, animator);
				if (player->oil != INVALID_ENTITY) {
					app_delayed_destroy_entity(app, player->oil);
					player->oil = INVALID_ENTITY;
					if (LAMP) LAMP->add_oil(30);
					play_sound("oil_get.wav", 2.0f);
				}
			} else {
				coroutine_t* co = &player->co;
				COROUTINE_START(co);
				animator->sprite.play("ladder");
				COROUTINE_CASE(co, GOING_DOWN_LADDER);
				if (!animator->sprite.will_finish(dt)) {
					COROUTINE_YIELD(co);
					transform->local.p -= animator->sprite.local_offset;
					static int last_index = 0;
					if (last_index - animator->sprite.frame_index < -1) {
						last_index = animator->sprite.frame_index;
						play_sound("ladder_down.wav");
					}
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
