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

#include <systems/board_system.h>

#include <components/transform.h>
#include <components/animator.h>
#include <components/board_piece.h>
#include <components/player.h>

#include <world.h>

void board_transform_system_update(app_t* app, float dt, void* udata, Transform* transforms, Animator* animators, BoardPiece* board_pieces, int entity_count)
{
	for (int i = 0; i < entity_count; ++i) {
		Transform* transform = transforms + i;
		Animator* animator = animators + i;
		BoardPiece* board_piece = board_pieces + i;

		v2 p = tile2world(board_piece->x, board_piece->y);

		if (board_piece->is_moving) {
			p = board_piece->interpolate();
		}

		transform->world.p += p;
		animator->sprite.sort_bits = sort_bits(board_piece->x, board_piece->y);
	}
}

void board_system_update(app_t* app, float dt, void* udata, BoardPiece* board_pieces, int entity_count)
{
	for (int i = 0; i < entity_count; ++i) {
		BoardPiece* board_piece = board_pieces + i;

		if (board_piece->is_moving) {
			board_piece->t += dt;
			if (board_piece->t >= board_piece->delay) {
				board_piece->is_moving = false;
				board_piece->t = 0;
				board_piece->delay = 0;
				if (app_is_entity_valid(app, board_piece->notify_player_when_done)) {
					Player* player = (Player*)app_get_component(app, board_piece->notify_player_when_done, "Player");
					player->busy = false;
				}
			}
		}
	}
}
