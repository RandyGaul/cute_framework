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

#ifndef MOCHI_H
#define MOCHI_H

#include <cute.h>
using namespace cute;

#include <components/animator.h>
#include <components/transform.h>

#include <cute/cute_coroutine.h>

struct Mochi
{
	bool busy = false;
	bool awake = false;
	bool going_left = true;
	coroutine_t pacing_co = { 0 };
	float t = 0;
	static constexpr float awake_delay = 3.0f;
	static constexpr float hop_delay = 0.25f;
	entity_t zzz = INVALID_ENTITY;
};

CUTE_INLINE cute::error_t Mochi_serialize(app_t* app, kv_t* kv, bool reading, entity_t entity, void* component, void* udata)
{
	Mochi* mochi = (Mochi*)component;
	if (reading) {
		CUTE_PLACEMENT_NEW(mochi) Mochi;
		Animator* animator = (Animator*)app_get_component(app, entity, "Animator");
		animator->sprite.play("sleeping");

		error_t err = app_make_entity(app, "zzz", &mochi->zzz);
		CUTE_ASSERT(!err.is_error());
		Transform* zzz_transform = (Transform*)app_get_component(app, mochi->zzz, "Transform");
		zzz_transform->relative_to = entity;
		Animator* zzz_animator = (Animator*)app_get_component(app, mochi->zzz, "Animator");
		zzz_animator->sprite.layer = 10000;
		zzz_animator->sprite.opacity = 0;
	}
	return kv_error_state(kv);
}

CUTE_INLINE void Mochi_cleanup(app_t* app, entity_t entity, void* component, void* udata)
{
	Mochi* mochi = (Mochi*)component;
	app_destroy_entity(app, mochi->zzz);
	mochi->zzz = INVALID_ENTITY;
}

#endif // MOCHI_H
