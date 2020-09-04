/*
	Cute Framework
	Copyright (C) 2019 Randy Gaul https://randygaul.net

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

#ifndef CUTE_SPRITE_H
#define CUTE_SPRITE_H

#include <cute_defines.h>
#include <cute_array.h>
#include <cute_dictionary.h>
#include <cute_batch.h>

#include <cute/cute_aseprite.h>
#include <mattiasgustavsson/strpool.h>

namespace cute
{

struct sprite_t;
struct aseprite_cache_t;

extern CUTE_API aseprite_cache_t* CUTE_CALL aseprite_cache_make(app_t* app);
extern CUTE_API void CUTE_CALL aseprite_cache_destroy(aseprite_cache_t* cache);

extern CUTE_API error_t CUTE_CALL aseprite_cache_load(aseprite_cache_t* cache, const char* aseprite_path, sprite_t* sprite);
extern CUTE_API void CUTE_CALL aseprite_cache_unload(aseprite_cache_t* cache, const char* aseprite_path);

extern CUTE_API batch_t* CUTE_CALL aseprite_cache_get_batch_ptr(aseprite_cache_t* cache);
extern CUTE_API strpool_t* CUTE_CALL aseprite_cache_get_strpool_ptr(aseprite_cache_t* cache);

//--------------------------------------------------------------------------------------------------

struct frame_t
{
	uint64_t id;
	float delay;
};

enum play_direction_t
{
	PLAY_DIRECTION_FORWARDS,
	PLAY_DIRECTION_BACKWARDS,
	PLAY_DIRECTION_PINGPONG,
};

struct animation_t
{
	const char* name;
	play_direction_t play_direction = PLAY_DIRECTION_FORWARDS;
	array<frame_t> frames;
};

using animation_table_t = dictionary<const char*, const animation_t*>;

struct sprite_t
{
	CUTE_INLINE void update(float dt);
	CUTE_INLINE void play(const char* animation);
	CUTE_INLINE void reset();
	CUTE_INLINE void draw(batch_t* batch, transform_t transform);
	CUTE_INLINE batch_quad_t quad(transform_t transform);

	CUTE_INLINE void pause();
	CUTE_INLINE void unpause();
	CUTE_INLINE void toggle_pause();
	CUTE_INLINE void flip_x();
	CUTE_INLINE void flip_y();
	CUTE_INLINE int frame_count();
	CUTE_INLINE float frame_delay();
	CUTE_INLINE float animation_delay();

	const char* name;
	int w = 0;
	int h = 0;
	v2 scale = v2(1, 1);
	v2 local_offset = v2(0, 0);
	float opacity = 1.0f;
	int sort_bits = 0;

	int frame_index = 0;
	int loop_count = 0;
	float play_speed_multiplier = 1.0f;
	const animation_t* animation = NULL;

	bool paused = false;
	float t = 0;
	const animation_table_t* animations = NULL;
};

//--------------------------------------------------------------------------------------------------

#include <cute_debug_printf.h>

void sprite_t::update(float dt)
{
	if (paused) return;

	t += dt * play_speed_multiplier;
	if (t >= animation->frames[frame_index].delay) {
		frame_index++;
		if (frame_index == animation->frames.count()) {
			loop_count++;
			frame_index = 0;
		}

		t = 0;

		// TODO - Backwards and pingpong.
	}
}

void sprite_t::play(const char* animation)
{
	this->animation = NULL;
	if (animations->find(animation, &this->animation).is_error()) {
		CUTE_DEBUG_PRINTF("Unable to find animation %s within sprite %s.\n", animation, name);
		CUTE_ASSERT(false);
	}
	reset();
}

void sprite_t::reset()
{
	paused = false;
	frame_index = 0;
	loop_count = 0;
	t = 0;
}

void sprite_t::draw(batch_t* batch, transform_t transform)
{
	batch_push(batch, quad(transform));
}

batch_quad_t sprite_t::quad(transform_t transform)
{
	batch_quad_t q;
	q.id = animation->frames[frame_index].id;
	q.transform = transform;
	q.transform.p += local_offset;
	q.w = w;
	q.h = h;
	q.scale_x = scale.x * w;
	q.scale_y = scale.y * h;
	q.sort_bits = sort_bits;
	q.alpha = opacity;
	return q;
}

void sprite_t::pause()
{
	paused = true;
}

void sprite_t::unpause()
{
	paused = false;
}

void sprite_t::toggle_pause()
{
	paused = !paused;
}


void sprite_t::flip_x()
{
	scale.x *= -1.0f;
}

void sprite_t::flip_y()
{
	scale.y *= -1.0f;
}

int sprite_t::frame_count()
{
	return animation->frames.count();
}

float sprite_t::frame_delay()
{
	return animation->frames[frame_index].delay;
}

float sprite_t::animation_delay()
{
	int count = frame_count();
	float delay = 0;
	for (int i = 0; i < count; ++i) {
		delay += animation->frames[i].delay;
	}
	return delay;
}

}

#endif // CUTE_SPRITE_H
