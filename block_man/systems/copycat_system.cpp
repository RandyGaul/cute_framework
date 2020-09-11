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

#include <components/animator.h>
#include <components/board_piece.h>
#include <components/copycat.h>
#include <components/transform.h>

#include <systems/copycat_system.h>

void copycat_system_update(app_t* app, float dt, void* udata, Transform* transforms, Animator* animators, BoardPiece* board_pieces, CopyCat* copycats, int entity_count)
{
	for (int i = 0; i < entity_count; ++i) {
		Transform* transform = transforms + i;
		Animator* animator = animators + i;
		BoardPiece* board_piece = board_pieces + i;
		CopyCat* copycat = copycats + i;

		if (copycat->awake) {
			copycat->t += dt;
		}

		if (board_piece->is_moving) continue;

		if (board_piece->was_bonked) {
			board_piece->was_bonked = false;
			animator->sprite.play("awakened");
		}

		if (animator->sprite.is_playing("awakened")) {
			if (animator->sprite.will_finish(dt)) {
				animator->sprite.play("idle");
				copycat->t = 0;
				copycat->awake = true;
			}
		}

		if (copycat->awake) {
			if (copycat->t >= CopyCat::awake_delay) {
				copycat->awake = false;
				animator->sprite.play("sleeping");
			}
		}

		if (copycat->awake) {
			bool try_move = false;
			int x = -1, y = -1;
			const char* next_anim = NULL;
			bool flip = false;
			float hop_h = 0;
			v2 squeeze;

			if (key_was_pressed(app, KEY_A) || key_was_pressed(app, KEY_LEFT)) {
				x = board_piece->x - 1;
				y = board_piece->y;
				try_move = true;
				next_anim = "side";
				flip = true;
				hop_h = 10.0f;
				squeeze = v2(0.75f, 1.5f);
			}

			else if (key_was_pressed(app, KEY_D) || key_was_pressed(app, KEY_RIGHT)) {
				x = board_piece->x + 1;
				y = board_piece->y;
				try_move = true;
				next_anim = "side";
				hop_h = 10.0f;
				squeeze = v2(0.75f, 1.5f);
			}

			else if (key_was_pressed(app, KEY_W) || key_was_pressed(app, KEY_UP)) {
				x = board_piece->x;
				y = board_piece->y - 1;
				try_move = true;
				next_anim = "back";
				hop_h = 20.0f;
				squeeze = v2(0.65f, 1.75f);
			}

			else if (key_was_pressed(app, KEY_S) || key_was_pressed(app, KEY_DOWN)) {
				x = board_piece->x;
				y = board_piece->y + 1;
				try_move = true;
				next_anim = "idle";
				hop_h = 15.0f;
				squeeze = v2(0.75f, 1.5f);
			}

			if (try_move) {
				if (in_board(x, y)) {
					BoardSpace space = world->board.data[y][x];
					if (space.is_empty) {
						board_piece->hop(x, y, CopyCat::hop_delay, 15.0f);
						animator->sprite.play(next_anim);
						if (flip) animator->flip_x();
						else animator->unflip_x();
						animator->squeeze(squeeze, CopyCat::hop_delay);
					}
				}
			}
		}
	}
}
