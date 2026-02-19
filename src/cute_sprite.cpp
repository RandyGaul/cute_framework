/*
	Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

#include "cute_sprite.h"
#include "cute_draw.h"
#include "cute_image.h"
#include "cute_file_system.h"

#include <cute/cute_aseprite.h>

#include <internal/cute_app_internal.h>
#include <internal/cute_aseprite_cache_internal.h>
#include <internal/cute_alloc_internal.h>
#include <internal/cute_girl.h>

CF_Sprite cf_sprite_defaults()
{
	CF_Sprite sprite = { 0 };
	sprite.id = CF_SPRITE_ID_INVALID;
	sprite.scale = cf_v2(1, 1);
	sprite.opacity = 1.0f;
	sprite.play_speed_multiplier = 1.0f;
	sprite.transform = cf_make_transform();
	sprite.loop = true;
	return sprite;
}

static CF_Sprite s_insert(CF_Image img)
{
	uint64_t id = app->easy_sprite_id_gen++;
	app->easy_sprites.insert(id, img);

	CF_Sprite sprite = cf_sprite_defaults();
	sprite.name = "easy_sprite";
	sprite.w = img.w;
	sprite.h = img.h;
	sprite.easy_sprite_id = id;

	return sprite;
}

CF_Sprite cf_make_easy_sprite_from_png(const char* png_path, CF_Result* result_out)
{
	png_path = sintern(png_path);
	uint64_t* cached_id = app->easy_sprite_id_cache.try_find(png_path);
	if (cached_id) {
		CF_Image* img = app->easy_sprites.try_find(*cached_id);
		if (img) {
			CF_Sprite sprite = cf_sprite_defaults();
			sprite.name = png_path;
			sprite.w = img->w;
			sprite.h = img->h;
			sprite.easy_sprite_id = *cached_id;
			return sprite;
		}
	}
	CF_Image img;
	CF_Result result = cf_image_load_png(png_path, &img);
	if (cf_is_error(result)) {
		if (result_out) *result_out = result;
		return cf_sprite_defaults();
	}
	cf_image_premultiply(&img);
	CF_Sprite sprite = s_insert(img);
	sprite.name = png_path;
	app->easy_sprite_id_cache.insert(png_path, sprite.easy_sprite_id);
	return sprite;
}

CF_Sprite cf_make_easy_sprite_from_pixels(const CF_Pixel* pixels, int w, int h)
{
	CF_Image img;
	img.w = w;
	img.h = h;
	img.pix = (CF_Pixel*)CF_ALLOC(sizeof(CF_Pixel) * w * h);
	CF_MEMCPY(img.pix, pixels, sizeof(CF_Pixel) * w * h);
	return s_insert(img);
}

void cf_easy_sprite_update_pixels(CF_Sprite* sprite, const CF_Pixel* pixels)
{
	CF_Image* img = app->easy_sprites.try_find(sprite->easy_sprite_id);
	if (img) {
		CF_MEMCPY(img->pix, pixels, sizeof(CF_Pixel) * img->w * img->h);
		spritebatch_t* sb = cf_get_draw_sb();
		spritebatch_invalidate(sb, sprite->easy_sprite_id);
	}
}

CF_Sprite cf_make_sprite(const char* aseprite_path)
{
	CF_Sprite s = cf_sprite_defaults();
	CF_Result err = cf_aseprite_cache_load(aseprite_path, &s);
	if (cf_is_error(err)) {
		char buf[1024];
		snprintf(buf, sizeof buf, "Unable to load sprite at path \"%s\".\n", aseprite_path);
		cf_message_box(CF_MESSAGE_BOX_TYPE_ERROR, "ERROR", buf);
	}
	return s;
}

CF_Sprite cf_make_sprite_from_memory(const char* unique_name, const void* aseprite_data, int size)
{
	CF_Sprite s = cf_sprite_defaults();
	CF_Result err = cf_aseprite_cache_load_from_memory(unique_name, aseprite_data, size, &s);
	if (cf_is_error(err)) {
		char buf[1024];
		snprintf(buf, sizeof buf, "Unable to load sprite from memory with name \"%s\".\n", unique_name);
		cf_message_box(CF_MESSAGE_BOX_TYPE_ERROR, "ERROR", buf);
	}
	return s;
}

CF_Sprite cf_make_demo_sprite()
{
	return cf_make_sprite_from_memory("internal/demo_sprite_girl.ase", girl_data, girl_sz);
}

void cf_sprite_unload(const char* aseprite_path)
{
	cf_aseprite_cache_unload(aseprite_path);
}

void cf_sprite_reload(CF_Sprite* sprite)
{
	if (sprite->id == CF_SPRITE_ID_INVALID) return;
	CF_SpriteAsset* asset = cf_sprite_get_asset(sprite->id);
	if (!asset->ase) return; // External assets can't be reloaded from disk.

	const char* path = asset->path;

	// Reload the ase file from disk.
	size_t sz = 0;
	void* data = cf_fs_read_entire_file_to_memory(path, &sz);
	if (!data) return;

	ase_t* new_ase = cute_aseprite_load_from_memory(data, (int)sz, NULL);
	CF_FREE(data);
	if (!new_ase) return;

	// Premultiply alpha on new frames.
	for (int i = 0; i < new_ase->frame_count; ++i) {
		ase_color_t* pix = new_ase->frames[i].pixels;
		for (int y = 0; y < new_ase->h; ++y) {
			for (int x = 0; x < new_ase->w; ++x) {
				float a = pix[y * new_ase->w + x].a / 255.0f;
				float r = pix[y * new_ase->w + x].r / 255.0f;
				float g = pix[y * new_ase->w + x].g / 255.0f;
				float b = pix[y * new_ase->w + x].b / 255.0f;
				r *= a; g *= a; b *= a;
				pix[y * new_ase->w + x].r = (uint8_t)(r * 255.0f);
				pix[y * new_ase->w + x].g = (uint8_t)(g * 255.0f);
				pix[y * new_ase->w + x].b = (uint8_t)(b * 255.0f);
			}
		}
	}

	// Reuse frame IDs for overlapping frames; allocate new ones for extras.
	cf_aseprite_cache_reload_asset(sprite->id, new_ase);

	// Refresh the sprite from the updated asset.
	asset = cf_sprite_get_asset(sprite->id);
	sprite->w = asset->w;
	sprite->h = asset->h;
	sprite->name = asset->path;

	// Replay the current animation (or fall back to first).
	const char* anim_name = sprite->animation_name;
	if (anim_name && map_get(asset->animations, sintern(anim_name))) {
		cf_sprite_play(sprite, anim_name);
	} else {
		CF_Animation** first_anim = map_items(asset->animations);
		if (map_size(asset->animations) > 0) {
			cf_sprite_play(sprite, (*first_anim)->name);
		}
	}
}

void cf_easy_sprite_unload(CF_Sprite *sprite)
{
	CF_Image* img = app->easy_sprites.try_find(sprite->easy_sprite_id);
	if (img) {
		cf_image_free(img);
		app->easy_sprites.remove(sprite->easy_sprite_id);
	}
	app->easy_sprite_id_cache.remove(sprite->name);
}

//--------------------------------------------------------------------------------------------------
// Sprite animation functions.

// Helper: get the current CF_Animation* for a sprite via asset.
// animation_name is already interned (set by cf_sprite_play from anim->name).
static const CF_Animation* s_get_animation(const CF_Sprite* sprite)
{
	if (sprite->id != CF_SPRITE_ID_INVALID && sprite->animation_name) {
		CF_SpriteAsset* asset = cf_sprite_get_asset(sprite->id);
		return map_get(asset->animations, sprite->animation_name);
	}
	return NULL;
}

// Cache _image_id, _pivot, _center_patch from the current animation state.
static void s_cache_sprite_frame(CF_Sprite* sprite)
{
	if (sprite->id == CF_SPRITE_ID_INVALID) return;
	const CF_Animation* anim = s_get_animation(sprite);
	if (!anim) return;
	int fi = sprite->frame_index;
	if (fi >= asize(anim->frames)) fi = asize(anim->frames) - 1;
	sprite->_image_id = anim->frames[fi].id;

	CF_SpriteAsset* asset = cf_sprite_get_asset(sprite->id);
	int global_frame = fi + anim->frame_offset;
	sprite->_pivot = asset->pivots ? asset->pivots[global_frame] : cf_v2(0, 0);
	CF_Aabb zero_aabb = { 0 };
	sprite->_center_patch = asset->center_patches ? asset->center_patches[global_frame] : zero_aabb;
}

void cf_sprite_play(CF_Sprite* sprite, const char* animation)
{
	CF_ASSERT(sprite);
	if (sprite->id == CF_SPRITE_ID_INVALID) return;
	const char* name = sintern(animation);
	CF_SpriteAsset* asset = cf_sprite_get_asset(sprite->id);
	const CF_Animation* anim = map_get(asset->animations, name);
	CF_ASSERT(anim);
	sprite->animation_name = anim->name;
	cf_sprite_reset(sprite);
	s_cache_sprite_frame(sprite);
}

void cf_sprite_reset(CF_Sprite* sprite)
{
	CF_ASSERT(sprite);
	sprite->paused = false;
	sprite->finished = false;
	sprite->frame_index = 0;
	sprite->loop_count = 0;
	sprite->t = 0;
	const CF_Animation* anim = s_get_animation(sprite);
	if (anim) sprite->play_direction = anim->play_direction;
}

bool cf_sprite_is_playing(CF_Sprite* sprite, const char* animation)
{
	CF_ASSERT(sprite);
	if (!sprite->animation_name) return false;
	return sprite->animation_name == sintern(animation);
}

int cf_sprite_frame_count(const CF_Sprite* sprite)
{
	CF_ASSERT(sprite);
	const CF_Animation* anim = s_get_animation(sprite);
	if (!anim) return 0;
	return asize(anim->frames);
}

int cf_sprite_current_global_frame(const CF_Sprite* sprite)
{
	CF_ASSERT(sprite);
	const CF_Animation* anim = s_get_animation(sprite);
	return sprite->frame_index + (anim ? anim->frame_offset : 0);
}

void cf_sprite_set_frame(CF_Sprite* sprite, int frame)
{
	CF_ASSERT(sprite);
	const CF_Animation* anim = s_get_animation(sprite);
	if (!anim) return;
	int frame_count = asize(anim->frames);
	CF_ASSERT(frame >= 0 && frame < frame_count);
	sprite->frame_index = frame;
	sprite->t = 0;
	s_cache_sprite_frame(sprite);
}

float cf_sprite_frame_delay(CF_Sprite* sprite)
{
	CF_ASSERT(sprite);
	const CF_Animation* anim = s_get_animation(sprite);
	if (!anim) return 0;
	return anim->frames[sprite->frame_index].delay;
}

float cf_sprite_animation_delay(CF_Sprite* sprite)
{
	CF_ASSERT(sprite);
	const CF_Animation* anim = s_get_animation(sprite);
	if (!anim) return 0;
	int count = asize(anim->frames);
	float delay = 0;
	for (int i = 0; i < count; ++i) {
		delay += anim->frames[i].delay;
	}
	return delay;
}

float cf_sprite_animation_interpolant(CF_Sprite* sprite)
{
	CF_ASSERT(sprite);
	const CF_Animation* anim = s_get_animation(sprite);
	if (!anim) return 0;
	float total = cf_sprite_animation_delay(sprite);
	if (total <= 0) return 0;
	int frame_count = asize(anim->frames);

	// Sum delays of completed frames based on play direction.
	float elapsed = sprite->t;
	if (sprite->play_direction == CF_PLAY_DIRECTION_BACKWARDS) {
		for (int i = frame_count - 1; i > sprite->frame_index; --i) {
			elapsed += anim->frames[i].delay;
		}
	} else if (sprite->play_direction == CF_PLAY_DIRECTION_PINGPONG) {
		if (sprite->loop_count % 2) {
			// Backward half: measure progress through backward pass.
			for (int i = frame_count - 1; i > sprite->frame_index; --i) {
				elapsed += anim->frames[i].delay;
			}
		} else {
			// Forward half: measure progress through forward pass.
			for (int i = 0; i < sprite->frame_index; ++i) {
				elapsed += anim->frames[i].delay;
			}
		}
	} else {
		for (int i = 0; i < sprite->frame_index; ++i) {
			elapsed += anim->frames[i].delay;
		}
	}

	return cf_clamp(elapsed / total, 0.0f, 1.0f);
}

bool cf_sprite_will_finish(CF_Sprite* sprite)
{
	CF_ASSERT(sprite);
	const CF_Animation* anim = s_get_animation(sprite);
	if (!anim) return false;
	float dt = CF_DELTA_TIME * sprite->play_speed_multiplier;
	int frame_count = asize(anim->frames);

	if (sprite->play_direction == CF_PLAY_DIRECTION_FORWARDS) {
		if (sprite->frame_index == frame_count - 1) {
			return sprite->t + dt >= anim->frames[sprite->frame_index].delay;
		}
	} else if (sprite->play_direction == CF_PLAY_DIRECTION_BACKWARDS) {
		if (sprite->frame_index == 0) {
			return sprite->t + dt >= anim->frames[0].delay;
		}
	} else { // PINGPONG
		// Finishes when the backward half completes (full cycle).
		if (sprite->loop_count % 2 && sprite->frame_index == 0) {
			return sprite->t + dt >= anim->frames[0].delay;
		}
	}
	return false;
}

CF_Aabb cf_sprite_get_slice(CF_Sprite* sprite, const char* name)
{
	CF_ASSERT(sprite);
	CF_Aabb not_found = { 0 };
	if (sprite->id == CF_SPRITE_ID_INVALID) return not_found;
	name = sintern(name);

	CF_SpriteAsset* asset = cf_sprite_get_asset(sprite->id);
	const CF_Animation* anim = s_get_animation(sprite);
	int frame = sprite->frame_index + (anim ? anim->frame_offset : 0);
	for (int i = 0; i < asize(asset->slices); ++i) {
		if (asset->slices[i].name == name && asset->slices[i].frame_index >= frame) {
			return asset->slices[i].box;
		}
	}
	return not_found;
}

void cf_sprite_update(CF_Sprite* sprite)
{
	CF_ASSERT(sprite);
	if (sprite->paused) return;
	const CF_Animation* anim = s_get_animation(sprite);
	if (!anim) return;

	sprite->t += CF_DELTA_TIME * sprite->play_speed_multiplier;
	int frame_count = asize(anim->frames);
	CF_PlayDirection direction = sprite->play_direction;
	if (direction == CF_PLAY_DIRECTION_FORWARDS) {
		if (sprite->t >= anim->frames[sprite->frame_index].delay) {
			sprite->frame_index++;
			sprite->t = 0;
			if (sprite->frame_index == frame_count) {
				if (sprite->loop) {
					sprite->loop_count++;
					sprite->frame_index = 0;
				} else {
					sprite->frame_index--;
					sprite->finished = true;
				}
			}
		}
	} else if (direction == CF_PLAY_DIRECTION_BACKWARDS) {
		if (sprite->t >= anim->frames[sprite->frame_index].delay) {
			sprite->frame_index--;
			sprite->t = 0;
			if (sprite->frame_index < 0) {
				if (sprite->loop) {
					sprite->loop_count++;
					sprite->frame_index = frame_count - 1;
				} else {
					sprite->frame_index++;
					sprite->finished = true;
				}
			}
		}
	} else if (direction == CF_PLAY_DIRECTION_PINGPONG) {
		if (sprite->t >= anim->frames[sprite->frame_index].delay) {
			sprite->t = 0;
			if (sprite->loop_count % 2) {
				sprite->frame_index--;
				if (sprite->frame_index < 0) {
					if (sprite->loop) {
						sprite->loop_count++;
						sprite->frame_index++;
					} else {
						sprite->frame_index = 0;
						sprite->finished = true;
					}
				}
			} else {
				sprite->frame_index++;
				if (sprite->frame_index == frame_count) {
					sprite->loop_count++;
					sprite->frame_index--;
				}
			}
		}
	}

	s_cache_sprite_frame(sprite);
}

int cf_sprite_animation_count(const CF_Sprite* sprite)
{
	CF_ASSERT(sprite);
	if (sprite->id == CF_SPRITE_ID_INVALID) return 0;
	CF_SpriteAsset* asset = cf_sprite_get_asset(sprite->id);
	return map_size(asset->animations);
}

const char* cf_sprite_animation_name_at(const CF_Sprite* sprite, int index)
{
	CF_ASSERT(sprite);
	if (sprite->id == CF_SPRITE_ID_INVALID) return NULL;
	CF_SpriteAsset* asset = cf_sprite_get_asset(sprite->id);
	if (index < 0 || index >= map_size(asset->animations)) return NULL;
	CF_Animation** anim_vals = map_items(asset->animations);
	return anim_vals[index]->name;
}

CF_V2 cf_sprite_pivot(const CF_Sprite* sprite)
{
	CF_ASSERT(sprite);
	return sprite->_pivot;
}
