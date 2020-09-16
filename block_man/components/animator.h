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

	CUTE_INLINE void update(float dt)
	{
		if (is_squeezing) {
			squeeze_t += dt;
			if (squeeze_t >= squeeze_delay) {
				squeeze_t = 0;
				is_squeezing = false;
				sprite.scale = scale;
			}
		}
		sprite.update(dt);
	}

	CUTE_INLINE void draw(batch_t* batch, transform_t tx)
	{
		if (is_squeezing) {
			float t = smoothstep(squeeze_t / squeeze_delay);
			t = clamp(t, 0.0f, 1.0f);
			if (t < 0.5f) {
				scale_interpolated = lerp(scale, scale_to, t);
			} else {
				scale_interpolated = lerp(scale_to, scale, t);
			}

			sprite.scale = scale_interpolated;
		}
		
		if (visible) {
			sprite.draw(batch, tx);
		}
	}

	bool flipped_x = false;
	CUTE_INLINE void flip_x() { if (!flipped_x) { flipped_x = true; sprite.flip_x(); } }
	CUTE_INLINE void unflip_x() { if (flipped_x) { flipped_x = false; sprite.flip_x(); } }

	v2 scale = v2(1, 1);
	v2 scale_interpolated = v2(1, 1);
	v2 scale_to = v2(1, 1);
	float squeeze_t = 0;
	float squeeze_delay = 0.125f;
	bool is_squeezing = false;
	CUTE_INLINE void squeeze(v2 scale_to, float delay)
	{
		scale = sprite.scale;
		this->scale_to = scale_to;
		if (flipped_x) this->scale_to.x *= -1;
		squeeze_t = 0;
		squeeze_delay = delay;
		is_squeezing = true;
	}

	CUTE_INLINE void no_squeeze()
	{
		squeeze_t = 0;
		is_squeezing = false;
		sprite.scale = scale;
	}
};

CUTE_INLINE cute::error_t Animator_serialize(app_t* app, kv_t* kv, entity_t entity, void* component, void* udata)
{
	Animator* animator = (Animator*)component;
	if (kv_get_state(kv) == KV_STATE_READ) {
		CUTE_PLACEMENT_NEW(animator) Animator;
	}
	kv_key(kv, "name"); kv_val(kv, &animator->sprite.name);
	if (kv_get_state(kv) == KV_STATE_READ) {
		animator->sprite = load_sprite(animator->sprite.name);
	}
	kv_key(kv, "visibile"); kv_val(kv, &animator->visible);
	kv_key(kv, "opacity"); kv_val(kv, &animator->sprite.opacity);
	kv_key(kv, "play_speed_multiplier"); kv_val(kv, &animator->sprite.play_speed_multiplier);
	kv_key(kv, "t"); kv_val(kv, &animator->sprite.t);
	kv_key(kv, "paused"); kv_val(kv, &animator->sprite.paused);
	return kv_error_state(kv);
}

#endif // ANIMATOR_H
