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
#include "cute_string.h"
#include "cute_math.h"

//--------------------------------------------------------------------------------------------------
// C API

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

/**
 * Represents one frame of animation within a sprite.
 */
typedef struct CF_Frame
{
	uint64_t id;
	float delay;
} CF_Frame;

#define CF_PLAY_DIRECTION_DEFS \
	CF_ENUM(PLAY_DIRECTION_FORWARDS, 0) \
	CF_ENUM(PLAY_DIRECTION_BACKWARDS, 1) \
	CF_ENUM(PLAY_DIRECTION_PINGPONG, 2) \

/**
 * The direction a sprite plays frames.
 */
typedef enum CF_PlayDirection
{
	#define CF_ENUM(K, V) CF_##K = V,
	CF_PLAY_DIRECTION_DEFS
	#undef CF_ENUM
} CF_PlayDirection;

CUTE_INLINE const char* cf_play_direction_to_string(CF_PlayDirection dir)
{
	switch (dir) {
	#define CF_ENUM(K, V) case CF_##K: return CUTE_STRINGIZE(CF_##K);
	CF_PLAY_DIRECTION_DEFS
	#undef CF_ENUM
	default: return NULL;
	}
}

/**
 * A single `sprite_t` contains a set of `animation_t`. Each animation
 * can define its own frames, and a playing direction for the frames.
 */
typedef struct CF_Animation
{
	const char* name;
	CF_PlayDirection play_direction;
	dyna CF_Frame* frames;
} CF_Animation;

/**
 * The `sprite_t` represents a set of drawable animations. Each animation is a collection
 * of frames, where each frame is one image to display on screen. The frames themselves are stored
 * elsewhere, and the sprite simply refers to them by read-only pointer.
 *
 * Switching between animations can be done by calling the `play` and passing the name of the animation
 * to the `play` method.
 */
typedef struct CF_Sprite
{
	const char* name;
	int w;
	int h;
	CF_V2 scale;
	CF_V2 local_offset;
	float opacity;

	int frame_index;
	int loop_count;
	float play_speed_multiplier;
	const CF_Animation* animation;

	bool paused;
	float t;
	htbl const CF_Animation** animations;

	CF_Transform transform;
} CF_Sprite;

CUTE_INLINE CF_Sprite cf_sprite_defaults()
{
	CF_Sprite sprite = { 0 };
	sprite.scale = cf_v2(1, 1);
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
CUTE_API CF_Sprite CUTE_CALL cf_easy_make_sprite(const char* png_path);

/**
 * Unloads the sprites image resources from the internal cache. Any live `sprite_t` instances for
 * the given `png_path` will now be "dangling" and invalid.
 */
CUTE_API void CUTE_CALL cf_easy_sprite_unload(CF_Sprite sprite);

//--------------------------------------------------------------------------------------------------
// Aseprite sprite API. This is the preferred way to deal with sprites in Cute Framework, by loading
// .ase or .aseprite files directly. However, if you just want to get started with single-frame
// sprites loaded from a single png image, you can try the easy sprite API.

/**
 * Loads a sprite from an aseprite file. This function may be called many times in a row without
 * any significant performance penalties due to internal caching.
 */
CUTE_API CF_Sprite CUTE_CALL cf_make_sprite(const char* aseprite_path);

/**
 * Unloads the sprites image resources from the internal cache. Any live `sprite_t` instances for
 * the given `aseprite_path` will now be "dangling" and invalid.
 */
CUTE_API void CUTE_CALL cf_sprite_unload(const char* aseprite_path);

//--------------------------------------------------------------------------------------------------
// In-line implementation of `sprite_t` functions.

CUTE_API void CUTE_CALL cf_draw_sprite2(const CF_Sprite* sprite, CF_Transform transform);

CUTE_INLINE void cf_sprite_draw(CF_Sprite* sprite)
{
	cf_draw_sprite2(sprite, sprite->transform);
}

/**
 * Updates the sprite's internal timer to flip through different frames.
 */
CUTE_INLINE void cf_sprite_update(CF_Sprite* sprite, float dt)
{
	if (sprite->paused) return;

	sprite->t += dt * sprite->play_speed_multiplier;
	if (sprite->t >= sprite->animation->frames[sprite->frame_index].delay) {
		sprite->frame_index++;
		if (sprite->frame_index == alen(sprite->animation->frames)) {
			sprite->loop_count++;
			sprite->frame_index = 0;
		}

		sprite->t = 0;

		// TODO - Backwards and pingpong.
	}
}

/**
 * Resets the currently playing animation and unpauses the animation.
 */
CUTE_INLINE void cf_sprite_reset(CF_Sprite* sprite)
{
	sprite->paused = false;
	sprite->frame_index = 0;
	sprite->loop_count = 0;
	sprite->t = 0;
}

/**
 * Switches to a new aninmation and starts playing it from the beginning.
 */
CUTE_INLINE void cf_sprite_play(CF_Sprite* sprite, const char* animation)
{
	sprite->animation = hfind(sprite->animations, sintern(animation));
	CUTE_ASSERT(sprite->animation);
	cf_sprite_reset(sprite);
}

/**
 * Returns true if `animation` the is currently playing animation.
 */
CUTE_INLINE bool cf_sprite_is_playing(CF_Sprite* sprite, const char* animation)
{
	return !CUTE_STRCMP(animation, sprite->animation->name);
}

CUTE_INLINE void cf_sprite_pause(CF_Sprite* sprite)
{
	sprite->paused = true;
}

CUTE_INLINE void cf_sprite_unpause(CF_Sprite* sprite)
{
	sprite->paused = false;
}

CUTE_INLINE void cf_sprite_toggle_pause(CF_Sprite* sprite)
{
	sprite->paused = !sprite->paused;
}

CUTE_INLINE void cf_sprite_flip_x(CF_Sprite* sprite)
{
	sprite->scale.x *= -1.0f;
}

CUTE_INLINE void cf_sprite_flip_y(CF_Sprite* sprite)
{
	sprite->scale.y *= -1.0f;
}

CUTE_INLINE int cf_sprite_frame_count(const CF_Sprite* sprite)
{
	return alen(sprite->animation);
}

CUTE_INLINE int cf_sprite_current_frame(const CF_Sprite* sprite)
{
	return sprite->frame_index;
}

/**
 * Returns the `delay` member of the currently playing frame, in milliseconds.
 */
CUTE_INLINE float cf_sprite_frame_delay(CF_Sprite* sprite)
{
	return sprite->animation->frames[sprite->frame_index].delay;
}

/**
 * Sums all the delays of each frame in the animation, and returns the total, in milliseconds.
 */
CUTE_INLINE float cf_sprite_animation_delay(CF_Sprite* sprite)
{
	int count = cf_sprite_frame_count(sprite);
	float delay = 0;
	for (int i = 0; i < count; ++i) {
		delay += sprite->animation->frames[i].delay;
	}
	return delay;
}

/**
 * Returns a value from 0 to 1 representing how far along the animation has played. 0 means
 * just started, while 1 means finished.
 */
CUTE_INLINE float cf_sprite_animation_interpolant(CF_Sprite* sprite)
{
	// TODO -- Backwards and pingpong.
	float delay = cf_sprite_animation_delay(sprite);
	float t = sprite->t + sprite->animation->frames[sprite->frame_index].delay * sprite->frame_index;
	return cf_clamp(t / delay, 0.0f, 1.0f);
}

/**
 * Returns true if the animation will loop around and finish if `update` is called. This is useful
 * to see if you're currently on the last frame of animation, and will finish in the particular
 * `dt` tick.
 */
CUTE_INLINE bool cf_sprite_will_finish(CF_Sprite* sprite, float dt)
{
	// TODO -- Backwards and pingpong.
	if (sprite->frame_index == cf_sprite_frame_count(sprite) - 1) {
		return sprite->t + dt * sprite->play_speed_multiplier >= sprite->animation->frames[sprite->frame_index].delay;
	} else {
		return false;
	}
}

/**
 * Returns true whenever at the very beginning of the animation sequence. This is useful for polling
 * on when the animation restarts itself, for example, polling within an if-statement each game tick.
 */
CUTE_INLINE bool cf_sprite_on_loop(CF_Sprite* sprite)
{
	if (sprite->frame_index == 0 && sprite->t == 0) {
		return true;
	} else {
		return false;
	}
}

CUTE_INLINE void cf_animation_add_frame(CF_Animation* animation, CF_Frame frame) { apush(animation->frames, frame); }

#ifdef __cplusplus
}
#endif // __cplusplus

//--------------------------------------------------------------------------------------------------
// C++ API

#ifdef CUTE_CPP

namespace Cute
{

using frame_t = CF_Frame;
using animation_t = CF_Animation;

using PlayDirection = CF_PlayDirection;
#define CF_ENUM(K, V) CUTE_INLINE constexpr PlayDirection K = CF_##K;
CF_PLAY_DIRECTION_DEFS
#undef CF_ENUM

CUTE_INLINE const char* to_string(PlayDirection dir)
{
	switch (dir) {
	#define CF_ENUM(K, V) case CF_##K: return #K;
	CF_PLAY_DIRECTION_DEFS
	#undef CF_ENUM
	default: return NULL;
	}
}

struct Sprite : public CF_Sprite
{
	Sprite() { *(CF_Sprite*)this = cf_sprite_defaults(); }
	Sprite(CF_Sprite s) { *(CF_Sprite*)this = s; }

	CUTE_INLINE void draw() { cf_sprite_draw(this); }
	CUTE_INLINE void update(float dt) { return cf_sprite_update(this, dt); }
	CUTE_INLINE void play(const char* animation) { return cf_sprite_play(this, animation); }
	CUTE_INLINE bool is_playing(const char* animation) { return cf_sprite_is_playing(this, animation); }
	CUTE_INLINE void reset() { return cf_sprite_reset(this); }
	CUTE_INLINE void pause() { return cf_sprite_pause(this); }
	CUTE_INLINE void unpause() { return cf_sprite_unpause(this); }
	CUTE_INLINE void toggle_pause() { return cf_sprite_toggle_pause(this); }
	CUTE_INLINE void flip_x() { return cf_sprite_flip_x(this); }
	CUTE_INLINE void flip_y() { return cf_sprite_flip_y(this); }
	CUTE_INLINE int frame_count() const { return cf_sprite_frame_count(this); }
	CUTE_INLINE int current_frame() const { return cf_sprite_current_frame(this); }
	CUTE_INLINE float frame_delay() { return cf_sprite_frame_delay(this); }
	CUTE_INLINE float animation_delay() { return cf_sprite_animation_delay(this); }
	CUTE_INLINE float animation_interpolant() { return cf_sprite_animation_interpolant(this); }
	CUTE_INLINE bool will_finish(float dt) { return cf_sprite_will_finish(this, dt); }
	CUTE_INLINE bool on_loop() { return cf_sprite_on_loop(this); }
};

CUTE_INLINE Sprite easy_make_sprite(const char* png_path) { return cf_easy_make_sprite(png_path); }
CUTE_INLINE void easy_sprite_unload(Sprite sprite) { cf_easy_sprite_unload(sprite); }
CUTE_INLINE Sprite make_sprite(const char* aseprite_path) { return cf_make_sprite(aseprite_path); }
CUTE_INLINE void sprite_unload(const char* aseprite_path) { cf_sprite_unload(aseprite_path); }

}

#endif // CUTE_CPP

#endif // CUTE_SPRITE_H
