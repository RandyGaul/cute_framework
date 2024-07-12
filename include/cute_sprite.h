/*
	Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

#ifndef CF_SPRITE_H
#define CF_SPRITE_H

#include "cute_defines.h"
#include "cute_array.h"
#include "cute_hashtable.h"
#include "cute_string.h"
#include "cute_math.h"
#include "cute_time.h"
#include "cute_color.h"
#include "cute_result.h"
#include "cute_math.h"

//--------------------------------------------------------------------------------------------------
// C API

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

/**
 * @struct   CF_Frame
 * @category sprite
 * @brief    Represents one frame of animation within a sprite.
 * @related  CF_Frame CF_Animation CF_Sprite
 */
typedef struct CF_Frame
{
	/* @member Unique identifier for this frame's image. */
	uint64_t id;

	/* @member Number of seconds to display this frame. */
	float delay;
} CF_Frame;
// @end

/**
 * @enum     CF_PlayDirection
 * @category sprite
 * @brief    The direction a sprite plays frames.
 * @related  CF_PlayDirection cf_play_direction_to_string CF_Animation
 */
#define CF_PLAY_DIRECTION_DEFS \
	/* @entry Flips through the frames of an animation forwards. */ \
	CF_ENUM(PLAY_DIRECTION_FORWARDS, 0) \
	/* @entry Flips through the frames of an animation backwards. */ \
	CF_ENUM(PLAY_DIRECTION_BACKWARDS, 1) \
	/* @entry Flips through the frames of an animation forwards, then backwards, repeating. */ \
	CF_ENUM(PLAY_DIRECTION_PINGPONG, 2) \
	/* @end */

typedef enum CF_PlayDirection
{
	#define CF_ENUM(K, V) CF_##K = V,
	CF_PLAY_DIRECTION_DEFS
	#undef CF_ENUM
} CF_PlayDirection;

/**
 * @function cf_play_direction_to_string
 * @category sprite
 * @brief    Returns a `CF_PlayDirection` converted to a C string.
 * @related  CF_PlayDirection cf_play_direction_to_string CF_Animation
 */
CF_INLINE const char* cf_play_direction_to_string(CF_PlayDirection dir)
{
	switch (dir) {
	#define CF_ENUM(K, V) case CF_##K: return CF_STRINGIZE(CF_##K);
	CF_PLAY_DIRECTION_DEFS
	#undef CF_ENUM
	default: return NULL;
	}
}

/**
 * @struct   CF_Animation
 * @category sprite
 * @brief    A named set of frames in sequence.
 * @related  CF_Frame CF_Animation CF_Sprite
 */
typedef struct CF_Animation
{
	/* The name of the animation. */
	const char* name;

	/* The direction to play the animation. See `CF_PlayDirection`. */
	CF_PlayDirection play_direction;

	/* The frames of the animation. See `dyna`. */
	dyna CF_Frame* frames;
} CF_Animation;
// @end

/**
 * @struct   CF_Sprite
 * @category sprite
 * @brief    A sprite represents a drawable entity, made of 2D animations/images.
 * @remarks  Sprites can be drawn by `cf_draw_sprite`. Since sprites are [plain old data POD](https://stackoverflow.com/questions/146452/what-are-pod-types-in-c) you may create one on the stack anywhere
 *           and freely copy it around. In C++ you may simply draw via `sprite.draw()`.
 * @related  CF_Frame CF_Animation CF_Sprite cf_make_easy_sprite cf_make_sprite
 */
typedef struct CF_Sprite
{
	/* @member The name of the sprite. */
	const char* name;

	/* @member Width of the sprite in pixels. */
	int w;

	/* @member Height of the sprite in pixels. */
	int h;

	/* @member Scale factor for the sprite when drawing. Default of `(1, 1)`. See `cf_draw_sprite`. */
	CF_V2 scale;

	/* @member A local offset/origin for the sprite when drawing. See `cf_draw_sprite`. This value is automatically set for .ase files if a slice called "origin" is present. */
	CF_V2 local_offset;

	/* @member An opacity value for the entire sprite. Default of 1.0f. See `cf_draw_sprite`. */
	float opacity;

	/* @member The current frame within `animation` to display. */
	int frame_index;

	/* @member The number of times this sprite has completed an animation. */
	int loop_count;

	/* @member A speed multiplier for updating frames. Default of 1.0f. */
	float play_speed_multiplier;

	/* @member Whether or not to pause updates to the animation. */
	bool paused;

	/* @member The current elapsed time within a frame of animation. */
	float t;

	/* @member For internal use only. */
	uint64_t easy_sprite_id;

	/* @member Controls the animation play direction. This gets set each time `cf_sprite_play` is called to the animation's play direction. You may override this member yourself after calling `cf_sprite_play`. */
	CF_PlayDirection play_direction;

	/* @member A pointer to the current animation to display, from within the set `animations`. See `CF_Animation`. */
	const CF_Animation* animation;

	/* @member The set of named animations for this sprite. See `CF_Animation` and `htbl`. */
	htbl const CF_Animation** animations;

	/* @member An optional transform for rendering within a particular space. See `CF_Transform`. */
	CF_Transform transform;
} CF_Sprite;
// @end

/**
 * @function cf_sprite_defaults
 * @category sprite
 * @brief    Returns an empty sprite.
 * @remarks  Empty sprites can not be drawn. You probably don't want this function unless you know what you're doing, instead,
 *           you may be looking for `cf_make_sprite` or `cf_make_easy_sprite`.
 * @related  CF_Sprite cf_sprite_defaults cf_make_easy_sprite cf_make_sprite
 */
CF_INLINE CF_Sprite cf_sprite_defaults()
{
	CF_Sprite sprite = { 0 };
	sprite.scale = cf_v2(1, 1);
	sprite.opacity = 1.0f;
	sprite.play_speed_multiplier = 1.0f;
	sprite.transform = cf_make_transform();
	return sprite;
}

/**
 * @function cf_make_easy_sprite_from_png
 * @category sprite
 * @brief    Loads a single-frame sprite from a single png file.
 * @param    png_path     Virtual path to the .png file.
 * @return   Returns a `CF_Sprite` that can be drawn with `cf_sprite_draw`. The sprite is not animated,
 *           as it's only a single-frame made from a png file.
 * @remarks  The preferred way to make a sprite is `cf_make_sprite`, but this function provides a very simple way to get started
 *           by merely loading a single .png image, or for games that don't require animations. See [Virtual File System](https://randygaul.github.io/cute_framework/#/topics/virtual_file_system).
 * @related  CF_Sprite cf_make_easy_sprite_from_png cf_make_easy_sprite_from_pixels cf_easy_sprite_update_pixels cf_easy_sprite_unload
 */
CF_API CF_Sprite CF_CALL cf_make_easy_sprite_from_png(const char* png_path, CF_Result* result_out);

/**
 * @function cf_make_easy_sprite_from_pixels
 * @category sprite
 * @brief    Constructs a sprite from an array of pixels.
 * @related  CF_Sprite cf_make_easy_sprite_from_png cf_make_easy_sprite_from_pixels cf_easy_sprite_update_pixels cf_easy_sprite_unload
 */
CF_API CF_Sprite CF_CALL cf_make_easy_sprite_from_pixels(const CF_Pixel* pixels, int w, int h);

/**
 * @function cf_easy_sprite_update_pixels
 * @category sprite
 * @brief    Updates the pixels of a sprite created from `cf_make_easy_sprite_from_pixels`.
 * @remarks  This is not a particularly fast function - you've been warned.
 * @related  CF_Sprite cf_make_easy_sprite_from_png cf_make_easy_sprite_from_pixels cf_easy_sprite_update_pixels cf_easy_sprite_unload
 */
CF_API void CF_CALL cf_easy_sprite_update_pixels(CF_Sprite* sprite, const CF_Pixel* pixels);

/**
 * @function cf_easy_sprite_unload
 * @category sprite
 * @brief    Unloads an easy sprite's image resources.
 * @param    sprite The `CF_Sprite` to unload. This `CF_Sprite` should have been created by a `cf_make_easy_sprite_*` function.
 * @related  CF_Sprite cf_make_easy_sprite_from_png cf_make_easy_sprite_from_pixels cf_easy_sprite_update_pixels cf_easy_sprite_unload
 */
CF_API void CF_CALL cf_easy_sprite_unload(CF_Sprite *sprite);

/**
 * @function cf_make_sprite
 * @category sprite
 * @brief    Loads a sprite from an aseprite file.
 * @param    aseprite_path  Virtual path to a .ase file.
 * @return   Returns a `CF_Sprite` that can be drawn with `cf_sprite_draw`.
 * @remarks  This function caches the sprite internally. Subsequent calls to load the same sprite will be very fast; you can use
 *           this function directly to fetch sprites that were already loaded. If you want to load sprites with your own custom
 *           animation data, instead of using the .ase/.aseprite format, you can try out `cf_png_cache_load` for a more low-level option.
 *           See [Virtual File System](https://randygaul.github.io/cute_framework/#/topics/virtual_file_system).
 * @related  CF_Sprite cf_make_easy_sprite_from_png cf_make_easy_sprite_from_pixels cf_easy_sprite_update_pixels cf_make_sprite_from_memory
 */
CF_API CF_Sprite CF_CALL cf_make_sprite(const char* aseprite_path);

/**
 * @function cf_make_sprite_from_memory
 * @category sprite
 * @brief    Loads a sprite from an aseprite file already in memory.
 * @param    unique_name   A completely unique string from all other sprites.
 * @return   Returns a `CF_Sprite` that can be drawn with `cf_sprite_draw`.
 * @remarks  This function caches the sprite internally. Subsequent calls to load the same sprite will be very fast; you can use
 *           this function directly to fetch sprites that were already loaded. If you want to load sprites with your own custom
 *           animation data, instead of using the .ase/.aseprite format, you can try out `cf_png_cache_load` for a more low-level option.
 *           See [Virtual File System](https://randygaul.github.io/cute_framework/#/topics/virtual_file_system).
 * @related  CF_Sprite cf_make_easy_sprite_from_png cf_make_easy_sprite_from_pixels cf_easy_sprite_update_pixels
 */
CF_API CF_Sprite CF_CALL cf_make_sprite_from_memory(const char* unique_name, const void* aseprite_data, int size);

/**
 * @function cf_make_demo_sprite
 * @category sprite
 * @brief    Loads a pixel sprite of a girl, used for testing/learning purposes.
 * @return   The sprite contains a few animations called "up", "side", "hold_down", "hold_side", "hold_up", "idle" you may try out.
 * @related  CF_Sprite cf_make_easy_sprite_from_png cf_make_easy_sprite_from_pixels cf_easy_sprite_update_pixels
 */
CF_API CF_Sprite CF_CALL cf_make_demo_sprite();

/**
 * @function cf_sprite_unload
 * @category sprite
 * @brief    Unloads the sprite's image resources from the internal cache.
 * @param    aseprite_path  Virtual path to a .ase file.
 * @remarks  Any live `CF_Sprite` instances for `aseprite_path` will now by "dangling". See [Virtual File System](https://randygaul.github.io/cute_framework/#/topics/virtual_file_system).
 * @related  CF_Sprite cf_make_sprite cf_sprite_unload cf_sprite_reload
 */
CF_API void CF_CALL cf_sprite_unload(const char* aseprite_path);

/**
 * @function cf_sprite_reload
 * @category sprite
 * @brief    Reloads the sprite's pixels from disk.
 * @param    sprite        The sprite to reload.
 * @return   The reloaded sprite.
 * @remarks  This function is designed to help support asset or image hotloading/reloading during development.
 *           This function is *not* designed to be called once you ship your game.
 *           All old instances of the sprite are now invalid and should be reset to this return value.
 * @related  CF_Sprite cf_make_sprite cf_sprite_unload cf_sprite_reload
 */
CF_API CF_Sprite CF_CALL cf_sprite_reload(const CF_Sprite* sprite);

//--------------------------------------------------------------------------------------------------
// In-line implementation of `CF_Sprite` functions.

CF_API void CF_CALL cf_draw_sprite(const CF_Sprite* sprite);

CF_INLINE void cf_sprite_draw(CF_Sprite* sprite)
{
	cf_draw_sprite(sprite);
}

/**
 * @function cf_sprite_width
 * @category sprite
 * @brief    Returns the sprite's width in pixels.
 * @related  CF_Sprite cf_sprite_width cf_sprite_height
 */
CF_INLINE int cf_sprite_width(CF_Sprite* sprite) { return sprite->w; }

/**
 * @function cf_sprite_height
 * @category sprite
 * @brief    Returns the sprite's height in pixels.
 * @related  CF_Sprite cf_sprite_width cf_sprite_height
 */
CF_INLINE int cf_sprite_height(CF_Sprite* sprite) { return sprite->h; }

/**
 * @function cf_sprite_get_scale_x
 * @category sprite
 * @brief    Returns the sprite's scale on the x-axis.
 * @related  CF_Sprite cf_sprite_get_scale_x cf_sprite_get_scale_y cf_sprite_set_scale_x cf_sprite_set_scale_y cf_sprite_set_scale
 */
CF_INLINE float cf_sprite_get_scale_x(CF_Sprite* sprite) { return sprite->scale.x; }

/**
 * @function cf_sprite_get_scale_y
 * @category sprite
 * @brief    Returns the sprite's scale on the y-axis.
 * @related  CF_Sprite cf_sprite_get_scale_x cf_sprite_get_scale_y cf_sprite_set_scale_x cf_sprite_set_scale_y cf_sprite_set_scale
 */
CF_INLINE float cf_sprite_get_scale_y(CF_Sprite* sprite) { return sprite->scale.y; }

/**
 * @function cf_sprite_set_scale
 * @category sprite
 * @brief    Returns the sprite's scale on the x-axis.
 * @related  CF_Sprite cf_sprite_get_scale_x cf_sprite_get_scale_y cf_sprite_set_scale_x cf_sprite_set_scale_y cf_sprite_set_scale
 */
CF_INLINE void cf_sprite_set_scale(CF_Sprite* sprite, CF_V2 scale) { sprite->scale = scale; }

/**
 * @function cf_sprite_set_scale_x
 * @category sprite
 * @brief    Returns the sprite's scale on the x-axis.
 * @related  CF_Sprite cf_sprite_get_scale_x cf_sprite_get_scale_y cf_sprite_set_scale_x cf_sprite_set_scale_y cf_sprite_set_scale
 */
CF_INLINE void cf_sprite_set_scale_x(CF_Sprite* sprite, float x) { sprite->scale.x = x; }

/**
 * @function cf_sprite_set_scale_y
 * @category sprite
 * @brief    Returns the sprite's scale on the y-axis.
 * @related  CF_Sprite cf_sprite_get_scale_x cf_sprite_get_scale_y cf_sprite_set_scale_x cf_sprite_set_scale_y cf_sprite_set_scale
 */
CF_INLINE void cf_sprite_set_scale_y(CF_Sprite* sprite, float y) { sprite->scale.y = y; }

/**
 * @function cf_sprite_get_offset_x
 * @category sprite
 * @brief    Returns the sprite's local offset on the x-axis.
 * @related  CF_Sprite cf_sprite_get_offset_x cf_sprite_get_offset_y cf_sprite_set_offset_x cf_sprite_set_offset_y
 */
CF_INLINE float cf_sprite_get_offset_x(CF_Sprite* sprite) { return sprite->local_offset.x; }

/**
 * @function cf_sprite_get_offset_y
 * @category sprite
 * @brief    Returns the sprite's local offset on the y-axis.
 * @related  CF_Sprite cf_sprite_get_offset_x cf_sprite_get_offset_y cf_sprite_set_offset_x cf_sprite_set_offset_y
 */
CF_INLINE float cf_sprite_get_offset_y(CF_Sprite* sprite) { return sprite->local_offset.y; }

/**
 * @function cf_sprite_set_offset_x
 * @category sprite
 * @brief    Returns the sprite's local offset on the x-axis.
 * @related  CF_Sprite cf_sprite_get_offset_x cf_sprite_get_offset_y cf_sprite_set_offset_x cf_sprite_set_offset_y
 */
CF_INLINE void cf_sprite_set_offset_x(CF_Sprite* sprite, float offset) { sprite->local_offset.x = offset; }

/**
 * @function cf_sprite_set_offset_y
 * @category sprite
 * @brief    Returns the sprite's local offset on the y-axis.
 * @related  CF_Sprite cf_sprite_get_offset_x cf_sprite_get_offset_y cf_sprite_set_offset_x cf_sprite_set_offset_y
 */
CF_INLINE void cf_sprite_set_offset_y(CF_Sprite* sprite, float offset) { sprite->local_offset.y = offset; }

/**
 * @function cf_sprite_get_opacity
 * @category sprite
 * @brief    Returns the sprite's opacity, a value from 0-1.
 */
CF_INLINE float cf_sprite_get_opacity(CF_Sprite* sprite) { return sprite->opacity; }

/**
 * @function cf_sprite_set_opacity
 * @category sprite
 * @brief    Sets the sprite's opacity, a value from 0-1.
 */
CF_INLINE void cf_sprite_set_opacity(CF_Sprite* sprite, float opacity) { sprite->opacity = opacity; }

/**
 * @function cf_sprite_get_play_speed_multiplier
 * @category sprite
 * @brief    Returns the sprite's playing speed multiplier.
 */
CF_INLINE float cf_sprite_get_play_speed_multiplier(CF_Sprite* sprite) { return sprite->play_speed_multiplier; }

/**
 * @function cf_sprite_set_play_speed_multiplier
 * @category sprite
 * @brief    Sets the sprite's playing speed multiplier.
 */
CF_INLINE void cf_sprite_set_play_speed_multiplier(CF_Sprite* sprite, float multiplier) { sprite->play_speed_multiplier = multiplier; }

/**
 * @function cf_sprite_get_loop_count
 * @category sprite
 * @brief    Returns the sprite's loop count.
 */
CF_INLINE int cf_sprite_get_loop_count(CF_Sprite* sprite) { return sprite->loop_count; }

/**
 * @function cf_sprite_update
 * @category sprite
 * @brief    Updates a sprite's animation.
 * @param    sprite     The sprite.
 * @remarks  Call this once per frame.
 * @related  CF_Sprite cf_make_sprite cf_sprite_update cf_sprite_play cf_sprite_pause
 */
CF_INLINE void cf_sprite_update(CF_Sprite* sprite)
{
	if (sprite->paused) return;
	if (!sprite->animation) return;

	sprite->t += CF_DELTA_TIME * sprite->play_speed_multiplier;
	int frame_count = alen(sprite->animation->frames);
	CF_PlayDirection direction = sprite->play_direction;
	if (direction == CF_PLAY_DIRECTION_FORWARDS) {
		if (sprite->t >= sprite->animation->frames[sprite->frame_index].delay) {
			sprite->frame_index++;
			sprite->t = 0;
			if (sprite->frame_index == frame_count) {
				sprite->loop_count++;
				sprite->frame_index = 0;
			}
		}
	} else if (direction == CF_PLAY_DIRECTION_BACKWARDS) {
		if (sprite->t >= sprite->animation->frames[sprite->frame_index].delay) {
			sprite->frame_index--;
			sprite->t = 0;
			if (sprite->frame_index < 0) {
				sprite->loop_count++;
				sprite->frame_index = cf_min(1, frame_count);
			}
		}
	} else if (direction == CF_PLAY_DIRECTION_PINGPONG) {
		if (sprite->t >= sprite->animation->frames[sprite->frame_index].delay) {
			sprite->t = 0;
			if (sprite->loop_count % 2) {
				sprite->frame_index--;
				if (sprite->frame_index < 0) {
					sprite->loop_count++;
					sprite->frame_index = cf_min(1, frame_count);
				}
			} else {
				sprite->frame_index++;
				if (sprite->frame_index == frame_count) {
					sprite->loop_count++;
					sprite->frame_index = cf_max(0, frame_count - 2);
				}
			}
		}
	}
}

/**
 * @function cf_sprite_reset
 * @category sprite
 * @brief    Resets the currently playing animation and unpauses the animation.
 * @param    sprite     The sprite.
 * @related  CF_Sprite cf_sprite_update cf_sprite_play
 */
CF_INLINE void cf_sprite_reset(CF_Sprite* sprite)
{
	sprite->paused = false;
	sprite->frame_index = 0;
	sprite->loop_count = 0;
	sprite->t = 0;
	if (sprite->animation) sprite->play_direction = sprite->animation->play_direction;
}

/**
 * @function cf_sprite_play
 * @category sprite
 * @brief    Switches to a new aninmation and starts playing it from the beginning.
 * @param    sprite     The sprite.
 * @param    animation  Name of the animation to switch to and start playing.
 * @related  CF_Sprite cf_sprite_update cf_sprite_play cf_sprite_is_playing
 */
CF_INLINE void cf_sprite_play(CF_Sprite* sprite, const char* animation)
{
	if (!sprite->animations) return;
	sprite->animation = hfind(sprite->animations, sintern(animation));
	CF_ASSERT(sprite->animation);
	cf_sprite_reset(sprite);
}

/**
 * @function cf_sprite_is_playing
 * @category sprite
 * @brief    Returns true if `animation` the is currently playing animation.
 * @param    sprite     The sprite.
 * @param    animation  Name of the animation.
 * @related  CF_Sprite cf_sprite_update cf_sprite_play cf_sprite_is_playing
 */
CF_INLINE bool cf_sprite_is_playing(CF_Sprite* sprite, const char* animation)
{
	if (!sprite->animation) return false;
	return !CF_STRCMP(animation, sprite->animation->name);
}

/**
 * @function cf_sprite_pause
 * @category sprite
 * @brief    Pauses the sprite's animation.
 * @param    sprite     The sprite.
 * @related  CF_Sprite cf_sprite_update cf_sprite_play cf_sprite_pause cf_sprite_unpause cf_sprite_toggle_pause
 */
CF_INLINE void cf_sprite_pause(CF_Sprite* sprite)
{
	sprite->paused = true;
}

/**
 * @function cf_sprite_unpause
 * @category sprite
 * @brief    Unpauses the sprite's animation.
 * @param    sprite     The sprite.
 * @related  CF_Sprite cf_sprite_update cf_sprite_play cf_sprite_pause cf_sprite_unpause cf_sprite_toggle_pause
 */
CF_INLINE void cf_sprite_unpause(CF_Sprite* sprite)
{
	sprite->paused = false;
}

/**
 * @function cf_sprite_toggle_pause
 * @category sprite
 * @brief    Toggles the sprite's pause state.
 * @param    sprite     The sprite.
 * @related  CF_Sprite cf_sprite_update cf_sprite_play cf_sprite_pause cf_sprite_unpause cf_sprite_toggle_pause
 */
CF_INLINE void cf_sprite_toggle_pause(CF_Sprite* sprite)
{
	sprite->paused = !sprite->paused;
}

/**
 * @function cf_sprite_flip_x
 * @category sprite
 * @brief    Flip's the sprite on the x-axis.
 * @param    sprite     The sprite.
 * @remarks  Works by flipping the sign of the sprite's scale on the x-axis.
 * @related  CF_Sprite cf_sprite_flip_x cf_sprite_flip_y
 */
CF_INLINE void cf_sprite_flip_x(CF_Sprite* sprite)
{
	sprite->scale.x *= -1.0f;
}

/**
 * @function cf_sprite_flip_y
 * @category sprite
 * @brief    Flip's the sprite on the y-axis.
 * @param    sprite     The sprite.
 * @remarks  Works by flipping the sign of the sprite's scale on the y-axis.
 * @related  CF_Sprite cf_sprite_flip_x cf_sprite_flip_y
 */
CF_INLINE void cf_sprite_flip_y(CF_Sprite* sprite)
{
	sprite->scale.y *= -1.0f;
}

/**
 * @function cf_sprite_frame_count
 * @category sprite
 * @brief    Returns the number of frames in the sprite's currently playing animation.
 * @param    sprite     The sprite.
 * @related  CF_Sprite cf_sprite_frame_count cf_sprite_current_frame cf_sprite_frame_delay cf_sprite_animation_delay
 */
CF_INLINE int cf_sprite_frame_count(const CF_Sprite* sprite)
{
	if (!sprite->animation) return 0;
	return alen(sprite->animation->frames);
}

/**
 * @function cf_sprite_current_frame
 * @category sprite
 * @brief    Returns the index of the currently playing frame.
 * @param    sprite     The sprite.
 * @related  CF_Sprite cf_sprite_frame_count cf_sprite_current_frame cf_sprite_frame_delay cf_sprite_animation_delay
 */
CF_INLINE int cf_sprite_current_frame(const CF_Sprite* sprite)
{
	return sprite->frame_index;
}

/**
 * @function cf_sprite_frame_delay
 * @category sprite
 * @brief    Returns the `delay` member of the currently playing frame, in milliseconds.
 * @param    sprite     The sprite.
 * @related  CF_Sprite CF_Frame cf_sprite_frame_count cf_sprite_current_frame cf_sprite_frame_delay cf_sprite_animation_delay cf_sprite_animation_interpolant
 */
CF_INLINE float cf_sprite_frame_delay(CF_Sprite* sprite)
{
	if (!sprite->animation) return 0;
	return sprite->animation->frames[sprite->frame_index].delay;
}

/**
 * @function cf_sprite_animation_delay
 * @category sprite
 * @brief    Sums all the delays of each frame in the animation, and returns the total, in milliseconds.
 * @param    sprite     The sprite.
 * @related  CF_Sprite CF_Frame CF_Animation cf_sprite_frame_count cf_sprite_current_frame cf_sprite_frame_delay cf_sprite_animation_delay cf_sprite_animation_interpolant
 */
CF_INLINE float cf_sprite_animation_delay(CF_Sprite* sprite)
{
	if (!sprite->animation) return 0;
	int count = cf_sprite_frame_count(sprite);
	float delay = 0;
	for (int i = 0; i < count; ++i) {
		delay += sprite->animation->frames[i].delay;
	}
	return delay;
}

/**
 * @function cf_sprite_animation_interpolant
 * @category sprite
 * @brief    Returns a value from 0 to 1 representing how far along the animation has played.
 * @param    sprite     The sprite.
 * @remarks  0 means just started, while 1 means finished.
 * @related  CF_Sprite CF_Frame CF_Animation cf_sprite_frame_count cf_sprite_current_frame cf_sprite_frame_delay cf_sprite_animation_delay
 */
CF_INLINE float cf_sprite_animation_interpolant(CF_Sprite* sprite)
{
	// TODO -- Backwards and pingpong.
	if (!sprite->animation) return 0;
	float delay = cf_sprite_animation_delay(sprite);
	float t = sprite->t + sprite->animation->frames[sprite->frame_index].delay * sprite->frame_index;
	return cf_clamp(t / delay, 0.0f, 1.0f);
}

/**
 * @function cf_sprite_will_finish
 * @category sprite
 * @brief    Returns true if the animation will loop around and finish if `cf_sprite_update` is called.
 * @param    sprite     The sprite.
 * @remarks  This is useful to see if you're currently on the last frame of animation, and will finish in this particular update.
 * @related  CF_Sprite cf_sprite_frame_count cf_sprite_current_frame cf_sprite_frame_delay cf_sprite_animation_delay cf_sprite_will_finish
 */
CF_INLINE bool cf_sprite_will_finish(CF_Sprite* sprite)
{
	// TODO -- Backwards and pingpong.
	if (!sprite->animation) return false;
	if (sprite->frame_index == cf_sprite_frame_count(sprite) - 1) {
		return sprite->t + CF_DELTA_TIME * sprite->play_speed_multiplier >= sprite->animation->frames[sprite->frame_index].delay;
	} else {
		return false;
	}
}

/**
 * @function cf_sprite_on_loop
 * @category sprite
 * @brief    Returns true whenever at the very beginning of the animation sequence.
 * @param    sprite     The sprite.
 * @remarks  This is useful for polling on when the animation restarts itself, for example, polling within an if-statement each game tick.
 * @related  CF_Sprite cf_sprite_frame_count cf_sprite_current_frame cf_sprite_frame_delay cf_sprite_animation_delay cf_sprite_will_finish
 */
CF_INLINE bool cf_sprite_on_loop(CF_Sprite* sprite)
{
	if (sprite->frame_index == 0 && sprite->t == 0) {
		return true;
	} else {
		return false;
	}
}

/**
 * @function cf_animation_add_frame
 * @category sprite
 * @brief    Adds a frame to an animation.
 * @param    animation   The sprite.
 * @param    frame       The frame.
 * @remarks  You can use this function to build your own animations in a custom manner. It's recommend to just use `cf_make_sprite`, which
 *           loads a full sprite out of a .ase file. But, this function provides another low-level option if desired.
 * @related  CF_Sprite CF_Animation CF_Frame dyna htbl
 */
CF_INLINE void cf_animation_add_frame(CF_Animation* animation, CF_Frame frame) { apush(animation->frames, frame); }

#ifdef __cplusplus
}
#endif // __cplusplus

//--------------------------------------------------------------------------------------------------
// C++ API

#ifdef CF_CPP

namespace Cute
{

using Frame = CF_Frame;
using Animation = CF_Animation;

using PlayDirection = CF_PlayDirection;
#define CF_ENUM(K, V) CF_INLINE constexpr PlayDirection K = CF_##K;
CF_PLAY_DIRECTION_DEFS
#undef CF_ENUM

CF_INLINE const char* to_string(PlayDirection dir)
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

	CF_INLINE void draw() { cf_sprite_draw(this); }
	CF_INLINE void update() { return cf_sprite_update(this); }
	CF_INLINE void play(const char* animation) { return cf_sprite_play(this, animation); }
	CF_INLINE bool is_playing(const char* animation) { return cf_sprite_is_playing(this, animation); }
	CF_INLINE void reset() { return cf_sprite_reset(this); }
	CF_INLINE void pause() { return cf_sprite_pause(this); }
	CF_INLINE void unpause() { return cf_sprite_unpause(this); }
	CF_INLINE void toggle_pause() { return cf_sprite_toggle_pause(this); }
	CF_INLINE void flip_x() { return cf_sprite_flip_x(this); }
	CF_INLINE void flip_y() { return cf_sprite_flip_y(this); }
	CF_INLINE int frame_count() const { return cf_sprite_frame_count(this); }
	CF_INLINE int current_frame() const { return cf_sprite_current_frame(this); }
	CF_INLINE float frame_delay() { return cf_sprite_frame_delay(this); }
	CF_INLINE float animation_delay() { return cf_sprite_animation_delay(this); }
	CF_INLINE float animation_interpolant() { return cf_sprite_animation_interpolant(this); }
	CF_INLINE bool will_finish() { return cf_sprite_will_finish(this); }
	CF_INLINE bool on_loop() { return cf_sprite_on_loop(this); }
};

CF_INLINE Sprite easy_make_sprite(const char* png_path, Result* result) { return cf_make_easy_sprite_from_png(png_path, result); }
CF_INLINE Sprite easy_make_sprite(const Pixel* pixels, int w, int h) { return cf_make_easy_sprite_from_pixels(pixels, w, h); }
CF_INLINE Sprite make_sprite(const char* aseprite_path) { return cf_make_sprite(aseprite_path); }
CF_INLINE void sprite_unload(const char* aseprite_path) { cf_sprite_unload(aseprite_path); }
CF_INLINE Sprite sprite_reload(const Sprite* sprite) { return cf_sprite_reload(sprite); }
CF_INLINE Sprite sprite_reload(Sprite& sprite) { return (sprite = cf_sprite_reload(&sprite)); }

}

#endif // CF_CPP

#endif // CF_SPRITE_H
