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
			if (animator->sprite.is_playing("sleeping")) {
				animator->sprite.play("awakened");
				animator->no_squeeze();
				Animator* zzz_animator = (Animator*)app_get_component(app, copycat->zzz, "Animator");
				zzz_animator->visible = false;
				zzz_animator->sprite.opacity = 1.0f;
 				if (board_piece->bonk_xdir < 0) copycat->going_left = true;
				else if (board_piece->bonk_xdir > 0) copycat->going_left = false;
			}
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
				Animator* zzz_animator = (Animator*)app_get_component(app, copycat->zzz, "Animator");
				zzz_animator->visible = true;
				zzz_animator->sprite.opacity = 0;
			}
		}

		if (animator->sprite.is_playing("sleeping")) {
			if (animator->sprite.on_loop()) {
				animator->squeeze(v2(1.2f, 1.0f), animator->sprite.animation_delay());
			}
			Animator* zzz_animator = (Animator*)app_get_component(app, copycat->zzz, "Animator");
			float t = smoothstep(animator->sprite.animation_interpolant());
			if (t > 0.3f && t < 0.6f) {
				zzz_animator->sprite.opacity = remap(t, 0.3f, 0.6f);
			} else if (t > 0.6f) {
				zzz_animator->sprite.opacity = 1.0f - remap(t, 0.6f, 1.0f);
			}
		}

		if (copycat->awake) {
			bool move = false;
			coroutine_t* co = &copycat->pacing_co;
			COROUTINE_START(co);
			COROUTINE_PAUSE(co, 1.0f, dt);
			move = true;
			COROUTINE_END(co);

			if (move) {
				int x = board_piece->x, y = board_piece->y;
				const char* next_anim = "side";
				bool flip = false;
				float hop_h = 15.0f;
				v2 squeeze = v2(0.75f, 1.5f);

				if (copycat->going_left) {
					flip = true;
					x = x - 1;
				} else {
					x = x + 1;
				}

				if (in_board(x, y)) {
					BoardSpace space = world->board.data[y][x];
					if (space.is_empty) {
						board_piece->hop(x, y, CopyCat::hop_delay, hop_h);
						animator->sprite.play(next_anim);
						if (flip) animator->flip_x();
						else animator->unflip_x();
						animator->squeeze(squeeze, CopyCat::hop_delay);
					} else {
						if (copycat->going_left) {
							flip = false;
							x = x + 1;
							copycat->going_left = false;
						} else {
							flip = true;
							x = x - 1;
							copycat->going_left = true;
						}

						space = world->board.data[y][x];
						if (space.is_empty) {
							board_piece->hop(x, y, CopyCat::hop_delay, hop_h);
							animator->sprite.play(next_anim);
							if (flip) animator->flip_x();
							else animator->unflip_x();
							animator->squeeze(squeeze, CopyCat::hop_delay);
						} else {
							animator->sprite.play("idle");
						}
					}
				}
			}
		}
	}
}
