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

void set_player_animation_based_on_facing_direction(Player* player, Animator* animator)
{
	const char* animation = NULL;
	if (player->holding) {
		if (player->xdir) {
			animation = "hold_side";
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
}

void handle_input(app_t* app, Player* player, float dt)
{
#if 0
	int x = hero.x;
	int y = hero.y;

	if (key_was_pressed(app, KEY_R)) {
		LoadLevel(levels[level_index]);
	}

	if (key_was_pressed(app, KEY_SPACE)) {
		if (!hero.holding)
		{
			// search forward from player to look for blocks to pick up
			int sx = x + hero.xdir, sy = y - hero.ydir;
			bool found = false;
			int distance = 0;
			while (in_grid(sy, sx, level.data.count(), level.data[0].count()))
			{
				if (level.data[sy][sx] == 'x')
				{
					found = true;
					break;
				}
				else if (level.data[sy][sx] == '1')
				{
					break;
				}
				sx += hero.xdir;
				sy -= hero.ydir;
				++distance;
			}

			if (found)
			{
                ++hero.moves;
				level.data[sy][sx] = '0';
				//level.data[y - hero.ydir][x + hero.xdir] = 'c';
				hero.sliding_block.is_sliding = true;
				hero.sliding_block.x0 = sx;
				hero.sliding_block.y0 = sy;
				hero.sliding_block.x = x + hero.xdir;
				hero.sliding_block.y = y - hero.ydir;
				hero.sliding_block.t = 0;
				hero.sliding_block.delay = hero.sliding_block.delay_per_tile * distance;
				hero.holding = true;
				SetHeroAnimBasedOnFacingDir();
			}
		}
		else // hero.holding is true
		{
            ++hero.moves;
			// so we need to throw the block we are holding
			int sx = x + hero.xdir * 2, sy = y - hero.ydir * 2;
			int distance = 0;
			while (in_grid(sy, sx, level.data.count(), level.data[0].count()))
			{
				if (level.data[sy][sx] != '0')
					break;
				sx += hero.xdir;
				sy -= hero.ydir;
				++distance;
			}
			level.data[y - hero.ydir][x + hero.xdir] = '0';
			//level.data[sy + hero.ydir][sx - hero.xdir] = 'x';
			hero.sliding_block.is_sliding = true;
			hero.sliding_block.x0 = x + hero.xdir;
			hero.sliding_block.y0 = y - hero.ydir;
			hero.sliding_block.x = sx - hero.xdir;
			hero.sliding_block.y = sy + hero.ydir;
			hero.sliding_block.t = 0;
			hero.sliding_block.delay = hero.sliding_block.delay_per_tile * distance;
			hero.holding = false;
			ice_block_sheen.t = 0;
			ice_block_sheen.frame_index = 0;
			SetHeroAnimBasedOnFacingDir();
		}
	}

	key_button_t keycodes[4] = { KEY_W , KEY_S, KEY_D, KEY_A };
	key_button_t keycodes_arrows[4] = { KEY_UP , KEY_DOWN, KEY_RIGHT, KEY_LEFT };
	int xdirs[4] = {  0,  0,  1, -1 };
	int ydirs[4] = {  1, -1,  0,  0 };
	int xmove[4] = {  0,  0,  1, -1 };
	int ymove[4] = { -1,  1,  0,  0 };

	for (int i = 0; i < 4; ++i)
	{
		if (key_was_pressed(app, keycodes[i]) || key_was_pressed(app, keycodes_arrows[i]))
		{
			bool update_hero_animation = false;

			if (hero.holding)
			{
				// if moving backwards, keep same direction, just back up
				if (hero.xdir == -xdirs[i] && hero.ydir == -ydirs[i])
				{
					// make sure we don't push block through a wall
					if (level.data[y + hero.ydir][x - hero.xdir] == '0')
					{
                        ++hero.moves;

						// move backward
						x += xmove[i];
						y += ymove[i];

						// update hero position
						level.data[hero.y][hero.x] = '0';
						hero.x0 = hero.x;
						hero.y0 = hero.y;
						hero.x = x;
						hero.y = y;
						hero.moving = true;

						// then, move the block
						level.data[y - hero.ydir * 2][x + hero.xdir * 2] = '0';
						//level.data[y - hero.ydir][x + hero.xdir] = 'c';
						hero.held_block.x0 = x + hero.xdir * 2;
						hero.held_block.y0 = y - hero.ydir * 2;
						hero.held_block.x = x + hero.xdir;
						hero.held_block.y = y - hero.ydir;
					}
				}
				// if moving forwards
				else if (hero.xdir == xdirs[i] && hero.ydir == ydirs[i])
				{
					// make sure we don't push block through a wall
					if (level.data[y - hero.ydir * 2][x + hero.xdir * 2] == '0')
					{
                        ++hero.moves;

						// first, move the block
						level.data[y - hero.ydir][x + hero.xdir] = '0';
						//level.data[y - hero.ydir * 2][x + hero.xdir * 2] = 'c';
						hero.held_block.x0 = x + hero.xdir;
						hero.held_block.y0 = y - hero.ydir;
						hero.held_block.x = x + hero.xdir * 2;
						hero.held_block.y = y - hero.ydir * 2;

						// move forward
						x += xmove[i];
						y += ymove[i];

						// update hero position
						level.data[hero.y][hero.x] = '0';
						hero.x0 = hero.x;
						hero.y0 = hero.y;
						hero.x = x;
						hero.y = y;
						hero.moving = true;
					}
				}
				else // if turning 90 degrees
				{
					// turn
					int xdirtemp = xdirs[i];
					int ydirtemp = ydirs[i];

					// check for something
					if (level.data[y - ydirtemp][x + xdirtemp] == '0')
					{
						// also check to make sure we aren't turning through a block
						if (level.data[y - hero.ydir - ydirtemp][x + hero.xdir + xdirtemp] == '0')
						{
                            ++hero.moves;

							// remove block
							level.data[y - hero.ydir][x + hero.xdir] = '0';
							hero.rotating_block.v = v2((float)hero.xdir, (float)hero.ydir);

							// turn hero
							hero.xdir = xdirtemp;
							hero.ydir = ydirtemp;

							// and move block
							//level.data[y - hero.ydir][x + hero.xdir] = 'c';
							hero.rotating_block.x = x + hero.xdir;
							hero.rotating_block.y = y - hero.ydir;
							hero.rotating_block.a = shortest_arc(hero.rotating_block.v, v2((float)hero.xdir, (float)hero.ydir));

							hero.rotating_block.is_rotating = true;
							hero.rotating_block.t = 0;
						}
					}

					update_hero_animation = true;
				}
			}
			else // not holding a block
			{
				if (hero.xdir == xdirs[i] && hero.ydir == ydirs[i])
				{
					// move forward
					x += xmove[i];
					y += ymove[i];

					// check for collisions
					// if we did't collide, assign the new position
					if (level.data[y][x] == '0' || level.data[y][x] == 'e') {
                        ++hero.moves;

						bool won = level.data[y][x] == 'e';

						// update hero position
						level.data[hero.y][hero.x] = '0';
						hero.x0 = hero.x;
						hero.y0 = hero.y;
						hero.x = x;
						hero.y = y;
						hero.moving = true;

						if (won) {
							hero.won = true;
						}
					}
				}
				else
				{
                    ++hero.moves;
					// turn 
					hero.xdir = xdirs[i];
					hero.ydir = ydirs[i];
					update_hero_animation = true;
				}
			}

			if (update_hero_animation) SetHeroAnimBasedOnFacingDir();
		}
	}
#endif
}

void player_system_update(app_t* app, float dt, void* udata, Transform* transforms, Animator* animators, BoardPiece* board_pieces, Player* players, int entity_count)
{
	for (int i = 0; i < entity_count; ++i) {
		Transform* transform = transforms + i;
		Animator* animator = animators + i;
		BoardPiece* board_piece = board_pieces + i;
		Player* player = players + i;

		handle_input(app, player, dt);
		set_player_animation_based_on_facing_direction(player, animator);
	}
}
