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

#include <systems/shadow_system.h>

#include <components/animator.h>
#include <components/board_piece.h>
#include <components/transform.h>
#include <components/shadow.h>

#include <world.h>

sprite_t shadow_sprite;
sprite_t shadow_sprite_small;

void shadow_system_init()
{
	shadow_sprite = load_sprite("data/shadow.aseprite");
	shadow_sprite_small = shadow_sprite;
	shadow_sprite_small.frame_index++;
}

void shadow_system_update(app_t* app, float dt, void* udata, Transform* transforms, Animator* animators, BoardPiece* board_pieces, Shadow* shadows, int entity_count)
{
	for (int i = 0; i < entity_count; ++i) {
		Transform* transform = transforms + i;
		Animator* animator = animators + i;
		BoardPiece* board_piece = board_pieces + i;
		Shadow* shadow = shadows + i;

		if (shadow->visible) {
			transform_t tx = transform->world;
			if (board_piece->is_moving && board_piece->a.y == board_piece->b.y) {
				tx.p.y = board_piece->a.y;
			}
			tx.p.y -= 7.0f - animator->sprite.local_offset.y;
			if (shadow->small) {
				shadow_sprite_small.draw(batch, tx);
			} else {
				shadow_sprite.draw(batch, tx);
			}
		}
	}
}

void shadow_system_post_update(app_t* app, float dt, void* udata)
{
	batch_flush(batch);
}
