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
#include <cute_string.h>

#include <cute/cute_aseprite.h>

namespace cute
{

struct sprite_t;
struct aseprite_cache_t;

aseprite_cache_t* aseprite_cache_make(app_t* app);
void aseprite_cache_destroy(aseprite_cache_t* cache);

error_t aseprite_cache_load(aseprite_cache_t* cache, const char* aseprite_path, sprite_t* sprite);
void aseprite_cache_unload(aseprite_cache_t* cache, const char* aseprite_path);

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
	string_t name;
	play_direction_t play_direction = PLAY_DIRECTION_FORWARDS;
	array<frame_t> frames;
};

using animation_table_t = dictionary<string_t, const animation_t*>;

struct sprite_t
{
	CUTE_INLINE void update(float dt);
	CUTE_INLINE void play(string_t animation);
	CUTE_INLINE void reset();
	CUTE_INLINE void draw();
	CUTE_INLINE batch_quad_t quad();

	CUTE_INLINE void pause();
	CUTE_INLINE void unpause();
	CUTE_INLINE void toggle_pause();

	string_t name;
	int w = 0;
	int h = 0;
	bool visible = true;
	transform_t tx = make_transform();
	v2 scale = v2(1, 1);
	v2 local_offset = v2(0, 0);
	float opacity = 1.0f;

	int frame_index = 0;
	int loop_count = 0;
	float play_speed_multiplier = 1.0f;
	const animation_t* animation = NULL;

	bool paused = false;
	float t = 0;
	const animation_table_t* animations;
	batch_t* batch;
};

//--------------------------------------------------------------------------------------------------

void sprite_t::update(float dt)
{
	if (paused) return;

	t += dt;
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

void sprite_t::play(string_t animation)
{
	this->animation = NULL;
	animations->find(animation, &this->animation);
	reset();
}

void sprite_t::reset()
{
	paused = false;
	frame_index = 0;
	loop_count = 0;
	t = 0;
}

void sprite_t::draw()
{
	batch_push(batch, quad());
}

batch_quad_t sprite_t::quad()
{
	batch_quad_t q;
	q.id = animation->frames[frame_index].id;
	q.transform = tx;
	q.transform.p += local_offset;
	q.w = w;
	q.h = h;
	q.scale_x = scale.x * w;
	q.scale_y = scale.y * h;
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

}

#endif // CUTE_SPRITE_H
