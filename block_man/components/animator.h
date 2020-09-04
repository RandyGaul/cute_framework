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

#ifndef ANIMATOR_H
#define ANIMATOR_H

#include <cute.h>
using namespace cute;

#include <serialize.h>
#include <world.h>

struct Animator
{
	bool visible = true;
	sprite_t sprite = sprite_t();
	transform_t transform_local = make_transform();
	transform_t transform_world = make_transform();
};

CUTE_INLINE error_t Animator_serialize(app_t* app, kv_t* kv, entity_t entity, void* component, void* udata)
{
	Animator* animator = (Animator*)component;
	if (kv_get_state(kv) == KV_STATE_READ) {
		CUTE_PLACEMENT_NEW(animator) Animator;
	}
	kv_key(kv, "name"); kv_val(kv, &animator->sprite.name);
	if (kv_get_state(kv) == KV_STATE_READ) {
		animator->sprite = load_sprite(animator->sprite.name);
	}
	serialize_transform(kv, "transform_local", &animator->transform_local);
	serialize_transform(kv, "transform_world", &animator->transform_world);
	kv_key(kv, "visibile"); kv_val(kv, &animator->visible);
	kv_key(kv, "opacity"); kv_val(kv, &animator->sprite.opacity);
	kv_key(kv, "play_speed_multiplier"); kv_val(kv, &animator->sprite.play_speed_multiplier);
	kv_key(kv, "t"); kv_val(kv, &animator->sprite.t);
	kv_key(kv, "paused"); kv_val(kv, &animator->sprite.paused);
	return kv_error_state(kv);
}

#endif // ANIMATOR_H
