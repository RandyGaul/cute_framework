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

#include "cute_defines.h"
#include "cute_array.h"
#include "cute_hashtable.h"
#include "cute_dictionary.h"
#include "cute_batch.h"

//--------------------------------------------------------------------------------------------------
// C API

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

/**
 * Represents one frame of animation within a sprite.
 */
typedef struct cf_frame_t
{
	uint64_t id;
	float delay;
} cf_frame_t;

/**
 * The direction a sprite plays frames.
 */
typedef enum cf_play_direction_t
{
	CF_PLAY_DIRECTION_FORWARDS,
	CF_PLAY_DIRECTION_BACKWARDS,
	CF_PLAY_DIRECTION_PINGPONG,
} cf_play_direction_t;

/**
 * A single `sprite_t` contains a set of `animation_t`. Each animation
 * can define its own frames, and a playing direction for the frames.
 */
typedef struct cf_animation_t
{
	const char* name; /*= NULL*/
	cf_play_direction_t play_direction; /*= CF_PLAY_DIRECTION_FORWARDS*/
	int frames_count;
	int frames_capacity;
	cf_frame_t* frames;
} cf_animation_t;

CUTE_INLINE void cf_animation_add_frame(cf_animation_t* animation, cf_frame_t frame)
{
	CUTE_ASSERT(animation->frames_count >= 0); // Didn't zero initialize cf_animation_t
	CUTE_ASSERT(animation->frames_capacity >= 0); // Didn't zero initialize cf_animation_t

	cf_array_ensure_capacity((void**)&animation->frames, animation->frames_count, &animation->frames_capacity, sizeof(cf_frame_t), animation->frames_count + 1, NULL);

	animation->frames[animation->frames_count++] = frame;
}

CUTE_INLINE void cf_animation_cleanup(cf_animation_t* animation, void* user_allocator_context)
{
	CUTE_FREE(animation->frames, user_allocator_context);
}

/**
 * An animation table is a set of animations a particular sprite references.
 * Each sprite instance contains a pointer to an animation table in a "read-only" fashion.
 */
typedef struct cf_hashtable_t cf_animation_table_t;

CUTE_INLINE int cf_animation_table_init(cf_animation_table_t* table, void* user_allocator_context)
{
	return cf_hashtable_init(table, sizeof(cf_dictionary_string_block_t), sizeof(const cf_animation_t*), 256, user_allocator_context);
}

CUTE_INLINE void cf_animation_table_cleanup(cf_animation_table_t* table, void* user_allocator_context){cf_hashtable_cleanup(table);}
CUTE_INLINE int cf_animation_table_count(const cf_animation_table_t* table) { return cf_hashtable_count(table); }
CUTE_INLINE const cf_dictionary_string_block_t* cf_animation_table_keys(const cf_animation_table_t* table) { return (const cf_dictionary_string_block_t*)cf_hashtable_keys(table); }
CUTE_INLINE const cf_animation_t** cf_animation_table_items(const cf_animation_table_t* table) { return (const cf_animation_t**)cf_hashtable_items(table); }
CUTE_INLINE const cf_animation_t** cf_animation_table_find(const cf_animation_table_t* table, const char* key)
{
	cf_dictionary_string_block_t block = cf_s_dictionary_make_block(key);
	return (const cf_animation_t**)cf_hashtable_find(table, &block);
}

CUTE_INLINE cf_animation_t** cf_animation_table_insert(cf_animation_table_t* table, const char* key, const cf_animation_t* val)
{
	cf_dictionary_string_block_t block = cf_s_dictionary_make_block(key);
	return (cf_animation_t**)cf_hashtable_insert(table, &block, &val);
}

/**
 * The `sprite_t` represents a set of drawable animations. Each animation is a collection
 * of frames, where each frame is one image to display on screen. The frames themselves are stored
 * elsewhere, and the sprite simply refers to them by read-only pointer.
 *
 * Switching between animations can be done by calling the `play` and passing the name of the animation
 * to the `play` method.
 */
typedef struct cf_sprite_t
{
	const char* name; /*= NULL*/
	int w; /*= 0*/
	int h; /*= 0*/
	cf_v2 scale; /*= cf_V2(1, 1)*/
	cf_v2 local_offset; /*= cf_V2(0, 0)*/
	float opacity; /*= 1.0f*/
	int layer; /*= 0*/

	int frame_index; /*= 0*/
	int loop_count; /*= 0*/
	float play_speed_multiplier; /*= 1.0f*/
	const cf_animation_t* animation; /*= NULL*/

	bool paused; /*= false*/
	float t; /*= 0*/
	const cf_animation_table_t* animations; /*= NULL*/

	cf_transform_t transform; /*= cf_make_transform()*/
} cf_sprite_t;

CUTE_INLINE cf_sprite_t cf_sprite_defaults()
{
	cf_sprite_t sprite = { 0 };
	sprite.scale = cf_V2(1, 1);
	sprite.opacity = 1.0f;
	sprite.play_speed_multiplier = 1.0f;
	sprite.transform = cf_make_transform();
	return sprite;
}

//--------------------------------------------------------------------------------------------------
// Easy sprite API. These functions are for loading single-frame sprites with no animations in a
// very simple way to get started. The preferred way to deal with sprites is the Aseprite sprite API,
// but the easy API is good for testing or just getting started with Cute Framework, or very simple
// games that don't require animations.

/**
 * Loads a single-frame sprite from a single png file. This function may be called many times in a row without
 * any significant performance penalties due to internal caching.
 */
CUTE_API cf_sprite_t CUTE_CALL cf_easy_sprite_make(const char* png_path);

/**
 * Unloads the sprites image resources from the internal cache. Any live `sprite_t` instances for
 * the given `png_path` will now be "dangling" and invalid.
 */
CUTE_API void CUTE_CALL cf_easy_sprite_unload(cf_sprite_t sprite);

/**
 * Gets the internal batch used for `easy_sprite_make` and `easy_sprite_unload`. The batch is used to get
 * sprites onto the screen by calling `batch_flush`.
 */
CUTE_API cf_batch_t* CUTE_CALL cf_easy_sprite_get_batch();

//--------------------------------------------------------------------------------------------------
// Aseprite sprite API. This is the preferred way to deal with sprites in Cute Framework, by loading
// .ase or .aseprite files directly. However, if you just want to get started with single-frame
// sprites loaded from a single png image, you can try the easy sprite API.

/**
 * Loads a sprite from an aseprite file. This function may be called many times in a row without
 * any significant performance penalties due to internal caching.
 */
CUTE_API cf_sprite_t CUTE_CALL cf_sprite_make(const char* aseprite_path);

/**
 * Unloads the sprites image resources from the internal cache. Any live `sprite_t` instances for
 * the given `aseprite_path` will now be "dangling" and invalid.
 */
CUTE_API void CUTE_CALL cf_sprite_unload(const char* aseprite_path);

/**
 * Gets the internal batch used for `sprite_make` and `sprite_unload`. The batch is used to get
 * sprites onto the screen by calling `batch_flush`.
 */
CUTE_API cf_batch_t* CUTE_CALL cf_sprite_get_batch();

//--------------------------------------------------------------------------------------------------
// In-line implementation of `sprite_t` functions.

#include "cute_debug_printf.h"

CUTE_INLINE void cf_sprite_update(cf_sprite_t* sprite, float dt)
{
	if (sprite->paused) return;

	sprite->t += dt * sprite->play_speed_multiplier;
	if (sprite->t >= sprite->animation->frames[sprite->frame_index].delay) {
		sprite->frame_index++;
		if (sprite->frame_index == sprite->animation->frames_count) {
			sprite->loop_count++;
			sprite->frame_index = 0;
		}

		sprite->t = 0;

		// TODO - Backwards and pingpong.
	}
}

CUTE_INLINE void cf_sprite_reset(cf_sprite_t* sprite)
{
	sprite->paused = false;
	sprite->frame_index = 0;
	sprite->loop_count = 0;
	sprite->t = 0;
}

CUTE_INLINE void cf_sprite_play(cf_sprite_t* sprite, const char* animation)
{
	sprite->animation = NULL;

	sprite->animation = *cf_animation_table_find(sprite->animations, animation);

	if (sprite->animation == NULL) {

		CUTE_DEBUG_PRINTF("Unable to find animation %s within sprite %s.\n", animation, sprite->name);
		CUTE_ASSERT(false);
	}

	cf_sprite_reset(sprite);
}

CUTE_INLINE bool cf_sprite_is_playing(cf_sprite_t* sprite, const char* animation)
{
	return !CUTE_STRCMP(animation, sprite->animation->name);
}

CUTE_INLINE cf_batch_sprite_t cf_sprite_batch_sprite_tf(cf_sprite_t* sprite, cf_transform_t transform)
{
	cf_batch_sprite_t q;
	q.id = sprite->animation->frames[sprite->frame_index].id;
	q.transform = transform;
	q.transform.p = cf_add_v2(q.transform.p, sprite->local_offset);
	q.w = sprite->w;
	q.h = sprite->h;
	q.scale_x = sprite->scale.x * sprite->w;
	q.scale_y = sprite->scale.y * sprite->h;
	q.sort_bits = sprite->layer;
	q.alpha = sprite->opacity;
	return q;
}

CUTE_INLINE cf_batch_sprite_t cf_sprite_batch_sprite(cf_sprite_t* sprite)
{
	cf_batch_sprite_t q;
	q.id = sprite->animation->frames[sprite->frame_index].id;
	q.transform = sprite->transform;
	q.transform.p = cf_add_v2(q.transform.p, sprite->local_offset);
	q.w = sprite->w;
	q.h = sprite->h;
	q.scale_x = sprite->scale.x * sprite->w;
	q.scale_y = sprite->scale.y * sprite->h;
	q.sort_bits = sprite->layer;
	q.alpha = sprite->opacity;
	return q;
}

CUTE_INLINE void cf_sprite_draw(cf_sprite_t* sprite, cf_batch_t* batch)
{
	CUTE_ASSERT(batch);
	cf_batch_push(batch, cf_sprite_batch_sprite_tf(sprite, sprite->transform));
}

CUTE_INLINE void cf_sprite_pause(cf_sprite_t* sprite)
{
	sprite->paused = true;
}

CUTE_INLINE void cf_sprite_unpause(cf_sprite_t* sprite)
{
	sprite->paused = false;
}

CUTE_INLINE void cf_sprite_toggle_pause(cf_sprite_t* sprite)
{
	sprite->paused = !sprite->paused;
}


CUTE_INLINE void cf_sprite_flip_x(cf_sprite_t* sprite)
{
	sprite->scale.x *= -1.0f;
}

CUTE_INLINE void cf_sprite_flip_y(cf_sprite_t* sprite)
{
	sprite->scale.y *= -1.0f;
}

CUTE_INLINE int cf_sprite_frame_count(const cf_sprite_t* sprite)
{
	return sprite->animation->frames_count;
}

CUTE_INLINE int cf_sprite_current_frame(const cf_sprite_t* sprite)
{
	return sprite->frame_index;
}

CUTE_INLINE float cf_sprite_frame_delay(cf_sprite_t* sprite)
{
	return sprite->animation->frames[sprite->frame_index].delay;
}

CUTE_INLINE float cf_sprite_animation_delay(cf_sprite_t* sprite)
{
	int count = cf_sprite_frame_count(sprite);
	float delay = 0;
	for (int i = 0; i < count; ++i) {
		delay += sprite->animation->frames[i].delay;
	}
	return delay;
}

CUTE_INLINE float cf_sprite_animation_interpolant(cf_sprite_t* sprite)
{
	// TODO -- Backwards and pingpong.
	float delay = cf_sprite_animation_delay(sprite);
	float t = sprite->t + sprite->animation->frames[sprite->frame_index].delay * sprite->frame_index;
	return cf_clamp(t / delay, 0.0f, 1.0f);
}

CUTE_INLINE bool cf_sprite_will_finish(cf_sprite_t* sprite, float dt)
{
	// TODO -- Backwards and pingpong.
	if (sprite->frame_index == cf_sprite_frame_count(sprite) - 1) {
		return sprite->t + dt * sprite->play_speed_multiplier >= sprite->animation->frames[sprite->frame_index].delay;
	} else {
		return false;
	}
}

CUTE_INLINE bool cf_sprite_on_loop(cf_sprite_t* sprite)
{
	if (sprite->frame_index == 0 && sprite->t == 0) {
		return true;
	} else {
		return false;
	}
}

#ifdef __cplusplus
}
#endif // __cplusplus

//--------------------------------------------------------------------------------------------------
// C++ API

#ifdef CUTE_CPP

namespace cute
{

using frame_t = cf_frame_t;
using play_direction_t = cf_play_direction_t;
using animation_table_t = cf_animation_table_t;
using animation_t = cf_animation_t;

struct sprite_t : public cf_sprite_t
{
	sprite_t() { *(cf_sprite_t*)this = cf_sprite_defaults(); }
	sprite_t(cf_sprite_t s) { *(cf_sprite_t*)this = s; }

	// TODO - Move these docs to the C APIs, rather than here.

	/**
	 * Updates the sprite's internal timer to flip through different frames.
	 */
	CUTE_INLINE void update(float dt);

	/**
	 * Switches to a new aninmation and starts playing it from the beginning.
	 */
	CUTE_INLINE void play(const char* animation);

	/**
	 * Returns true if `animation` the is currently playing animation.
	 */
	CUTE_INLINE bool is_playing(const char* animation);

	/**
	 * Resets the currently playing animation and unpauses the animation.
	 */
	CUTE_INLINE void reset();

	/**
	 * Pushes an instance of this sprite onto the `batch` member, which will be drawn the next time
	 * `batch_flush` is called on `batch`.
	 *
	 * `batch` must not be NULL.
	 */
	CUTE_INLINE void draw(cf_batch_t* batch);

	/**
	 * A lower level utility function used within the `draw` method. This is useful to prepare
	 * the sprite's drawable quad in a specific way before handing it off to a `batch`, to implement
	 * custom graphics effects.
	 */
	CUTE_INLINE batch_sprite_t batch_sprite(transform_t transform);
	CUTE_INLINE batch_sprite_t batch_sprite();

	CUTE_INLINE void pause();
	CUTE_INLINE void unpause();
	CUTE_INLINE void toggle_pause();
	CUTE_INLINE void flip_x();
	CUTE_INLINE void flip_y();
	CUTE_INLINE int frame_count() const;
	CUTE_INLINE int current_frame() const;

	/**
	 * Returns the `delay` member of the currently playing frame, in milliseconds.
	 */
	CUTE_INLINE float frame_delay();

	/**
	 * Sums all the delays of each frame in the animation, and returns the total, in milliseconds.
	 */
	CUTE_INLINE float animation_delay();

	/**
	 * Returns a value from 0 to 1 representing how far along the animation has played. 0 means
	 * just started, while 1 means finished.
	 */
	CUTE_INLINE float animation_interpolant();

	/**
	 * Returns true if the animation will loop around and finish if `update` is called. This is useful
	 * to see if you're currently on the last frame of animation, and will finish in the particular
	 * `dt` tick.
	 */
	CUTE_INLINE bool will_finish(float dt);

	/**
	 * Returns true whenever at the very beginning of the animation sequence. This is useful for polling
	 * on when the animation restarts itself, for example, polling within an if-statement each game tick.
	 */
	CUTE_INLINE bool on_loop();
};

void sprite_t::update(float dt) { return cf_sprite_update(this, dt); }
void sprite_t::play(const char* animation) { return cf_sprite_play(this, animation); }
bool sprite_t::is_playing(const char* animation) { return cf_sprite_is_playing(this, animation); }
void sprite_t::reset() { return cf_sprite_reset(this); }
void sprite_t::draw(cf_batch_t* batch) { return cf_sprite_draw(this, batch); }
batch_sprite_t sprite_t::batch_sprite(transform_t transform) { return cf_sprite_batch_sprite_tf(this, transform); }
batch_sprite_t sprite_t::batch_sprite() { return cf_sprite_batch_sprite(this); }
void sprite_t::pause() { return cf_sprite_pause(this); }
void sprite_t::unpause() { return cf_sprite_unpause(this); }
void sprite_t::toggle_pause() { return cf_sprite_toggle_pause(this); }
void sprite_t::flip_x() { return cf_sprite_flip_x(this); }
void sprite_t::flip_y() { return cf_sprite_flip_y(this); }
int sprite_t::frame_count() const { return cf_sprite_frame_count(this); }
int sprite_t::current_frame() const { return cf_sprite_current_frame(this); }
float sprite_t::frame_delay() { return cf_sprite_frame_delay(this); }
float sprite_t::animation_delay() { return cf_sprite_animation_delay(this); }
float sprite_t::animation_interpolant() { return cf_sprite_animation_interpolant(this); }
bool sprite_t::will_finish(float dt) { return cf_sprite_will_finish(this, dt); }
bool sprite_t::on_loop() { return cf_sprite_on_loop(this); }

CUTE_INLINE sprite_t easy_sprite_make(const char* png_path) { return cf_easy_sprite_make(png_path); }
CUTE_INLINE void easy_sprite_unload(cf_sprite_t sprite) { cf_easy_sprite_unload(sprite); }
CUTE_INLINE batch_t* easy_sprite_get_batch() { return cf_easy_sprite_get_batch(); }
CUTE_INLINE sprite_t sprite_make(const char* aseprite_path) { return cf_sprite_make(aseprite_path); }
CUTE_INLINE void sprite_unload(const char* aseprite_path) { cf_sprite_unload(aseprite_path); }
CUTE_INLINE batch_t* sprite_get_batch() { return cf_sprite_get_batch(); }

}

#endif // CUTE_CPP

#endif // CUTE_SPRITE_H
