/*
	Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

#include <cute_sprite.h>
#include <cute_debug_printf.h>
#include <cute_file_system.h>
#include <cute_defer.h>

#include <internal/cute_alloc_internal.h>
#include <internal/cute_app_internal.h>

#define CUTE_ASEPRITE_IMPLEMENTATION
#include <cute/cute_aseprite.h>

using namespace Cute;

struct CF_AsepriteCacheEntry
{
	const char* path = NULL;
	ase_t* ase = NULL;
	htbl CF_Animation** animations = NULL;
	dyna CF_SpriteSlice* slices = NULL;
	dyna v2* pivots = NULL;
	dyna CF_Aabb* center_patches = NULL;
};

struct CF_AsepriteCache
{
	Map<const char*, CF_AsepriteCacheEntry> aseprites;
	Map<uint64_t, void*> id_to_pixels;
	uint64_t id_gen = CF_ASEPRITE_ID_RANGE_LO;
};

CF_GLOBAL static CF_AsepriteCache* cache;

void cf_aseprite_cache_get_pixels(uint64_t image_id, void* buffer, int bytes_to_fill)
{
	auto pixels_ptr = cache->id_to_pixels.try_find(image_id);
	if (!pixels_ptr) {
		CF_DEBUG_PRINTF("Aseprite cache -- unable to find id %lld.\n", (long long int)image_id);
		CF_MEMSET(buffer, 0, bytes_to_fill);
	} else {
		void* pixels = *pixels_ptr;
		CF_MEMCPY(buffer, pixels, bytes_to_fill);
	}
}

void cf_make_aseprite_cache()
{
	cache = CF_NEW(CF_AsepriteCache);
}

void cf_destroy_aseprite_cache()
{
	int count = cache->aseprites.count();
	CF_AsepriteCacheEntry* entries = cache->aseprites.items();
	for (int i = 0; i < count; ++i) {
		CF_AsepriteCacheEntry* entry = entries + i;
		int animation_count = hcount(entry->animations);
		for (int j = 0; j < animation_count; ++j) {
			afree(entry->animations[j]->frames);
			CF_FREE(entry->animations[j]);
		}

		hfree(entry->animations);
		afree(entry->slices);
		afree(entry->pivots);
		cute_aseprite_free(entry->ase);
	}
	cache->~CF_AsepriteCache();
	CF_FREE(cache);
}

static CF_PlayDirection s_play_direction(ase_animation_direction_t direction)
{
	switch (direction) {
	case ASE_ANIMATION_DIRECTION_FORWARDS: return CF_PLAY_DIRECTION_FORWARDS;
	case ASE_ANIMATION_DIRECTION_BACKWORDS: return CF_PLAY_DIRECTION_BACKWARDS;
	case ASE_ANIMATION_DIRECTION_PINGPONG: return CF_PLAY_DIRECTION_PINGPONG;
	}
	return CF_PLAY_DIRECTION_FORWARDS;
}

static void s_sprite(CF_AsepriteCacheEntry entry, CF_Sprite* sprite)
{
	sprite->name = entry.path;
	sprite->animations = (const CF_Animation**)entry.animations;
	sprite->w = entry.ase->w;
	sprite->h = entry.ase->h;
	sprite->pivots = entry.pivots;
	sprite->center_patches = entry.center_patches;
	sprite->slices = entry.slices;
	if (entry.ase->tag_count == 0) {
		cf_sprite_play(sprite, "default");
	} else {
		cf_sprite_play(sprite, sprite->animations[0]->name);
	}
}

static CF_Result s_aseprite_cache_load_from_memory(const char* unique_name, const void* data, int sz, CF_Sprite* sprite_out)
{
 	ase_t* ase = cute_aseprite_load_from_memory(data, (int)sz, NULL);
	if (!ase) return cf_result_error("Unable to open ase file at `aseprite_path`.");

	// Allocate internal cache data structure entries.
	CF_Animation** animations = NULL;
	Array<uint64_t> ids;
	v2* pivots = NULL;
	CF_Aabb* center_patches = NULL;
	afit(pivots, ase->frame_count);
	afit(center_patches, ase->frame_count);
	ids.ensure_capacity(ase->frame_count);

	for (int i = 0; i < ase->frame_count; ++i) {
		// Unique sprite id.
		uint64_t id = cache->id_gen++;
		ids.add(id);

		// Premultiply alpha.
		ase_color_t* pix = ase->frames[i].pixels;
		for (int i = 0; i < ase->h; ++i) {
			for (int j = 0; j < ase->w; ++j) {
				float a = pix[i * ase->w + j].a / 255.0f;
				float r = pix[i * ase->w + j].r / 255.0f;
				float g = pix[i * ase->w + j].g / 255.0f;
				float b = pix[i * ase->w + j].b / 255.0f;
				r *= a;
				g *= a;
				b *= a;
				pix[i * ase->w + j].r = (uint8_t)(r * 255.0f);
				pix[i * ase->w + j].g = (uint8_t)(g * 255.0f);
				pix[i * ase->w + j].b = (uint8_t)(b * 255.0f);
			}
		}
		cache->id_to_pixels.insert(id, ase->frames[i].pixels);

		// Fill in zero'd out pivots initially. These can get overwritten from slice data
		// if a slice has a pivot.
		apush(pivots, V2(0,0));
	}

	// Fill out the animation table from the aseprite file.
	if (ase->tag_count) {
		// Each tag represents a single animation.
		for (int i = 0; i < ase->tag_count; ++i) {
			ase_tag_t* tag = ase->tags + i;
			int from = tag->from_frame;
			int to = tag->to_frame;
			CF_Animation* animation = (CF_Animation*)CF_ALLOC(sizeof(CF_Animation));
			CF_MEMSET(animation, 0, sizeof(CF_Animation));

			animation->name = sintern(tag->name);
			animation->play_direction = s_play_direction(tag->loop_animation_direction);
			animation->frame_offset = from;
			for (int i = from; i <= to; ++i) {
				uint64_t id = ids[i];
				CF_Frame frame;
				frame.delay = ase->frames[i].duration_milliseconds / 1000.0f;
				frame.id = id;
				cf_animation_add_frame(animation, frame);
			}
			hadd(animations, animation->name, animation);
		}
	} else {
		// Treat the entire frame set as a single animation if there are no tags.
		CF_Animation* animation = (CF_Animation*)CF_ALLOC(sizeof(CF_Animation));
		CF_MEMSET(animation, 0, sizeof(CF_Animation));

		animation->name = sintern("default");
		animation->play_direction = CF_PLAY_DIRECTION_FORWARDS;
		for (int i = 0; i < ase->frame_count; ++i) {
			uint64_t id = ids[i];
			CF_Frame frame;
			frame.delay = ase->frames[i].duration_milliseconds / 1000.0f;
			frame.id = id;
			cf_animation_add_frame(animation, frame);
		}
		hadd(animations, animation->name, animation);
	}

	// Fill out slices.
	// Look for slice information to define the sprite's local offset.
	// The slice named "origin"'s center is used to define the local offset.
	CF_AsepriteCacheEntry entry;
	entry.pivots = pivots;
	entry.center_patches = center_patches;
	afit(entry.slices, ase->slice_count);
	float sw = (float)ase->w;
	float sh = (float)ase->h;
	const char* origin_slice_name = sintern("origin");
	for (int i = 0; i < ase->slice_count; ++i) {
		ase_slice_t* slice = ase->slices + i;
		float x = (float)slice->origin_x - sw*0.5f;
		float y = (float)slice->origin_y;
		float w = (float)slice->w;
		float h = (float)slice->h;

		// Invert y-axis since ase saves slice as (0, 0) top-left.
		y = sh - y;
		y = y - sh*0.5f;

		// Record the slice.
		CF_Aabb bb = make_aabb(V2(x,y-h), V2(x+w,y));
		const char* slice_name = sintern(slice->name);
		apush(entry.slices, CF_SpriteSlice {
			.frame_index = slice->frame_number,
			.name = slice_name,
			.box = bb
		});

		if (slice->has_center_as_9_slice) {
			CF_V2 min = cf_v2(slice->center_x, slice->center_y);
			CF_V2 max = cf_v2(slice->center_x + slice->center_w, slice->center_y + slice->center_h);
			CF_Aabb center_patch = cf_make_aabb(min, max);
			for (int frame_number = slice->frame_number; frame_number < ase->frame_count; ++frame_number) {
				entry.center_patches[frame_number] = center_patch;
			}
		}
		if (slice->has_pivot && slice_name == origin_slice_name) {
			v2 pivot = V2((float)slice->pivot_x, (float)slice->pivot_y);
			// Transform from CF's (0, 0) at center to ase's (0, 0) at top-left.
			pivot.x = pivot.x - sw * 0.5f + 0.5f;
			pivot.y = pivot.y - sh * 0.5f + 0.5f;

			for (int frame_number = slice->frame_number; frame_number < ase->frame_count; ++frame_number) {
				entry.pivots[frame_number] = pivot;
			}
		}
	}

	// Cache the ase and animation.
	entry.path = unique_name;
	entry.ase = ase;
	entry.animations = animations;
	cache->aseprites.insert(unique_name, entry);

	s_sprite(entry, sprite_out);
	return cf_result_success();
}

CF_Result cf_aseprite_cache_load_from_memory(const char* unique_name, const void* data, int sz, CF_Sprite* sprite_out)
{
	// First see if this ase was already cached.
	unique_name = sintern(unique_name);
	auto entry_ptr = cache->aseprites.try_find(unique_name);
	if (entry_ptr) {
		s_sprite(*entry_ptr, sprite_out);
		return cf_result_success();
	}

	return s_aseprite_cache_load_from_memory(unique_name, data, sz, sprite_out);
}

CF_Result cf_aseprite_cache_load(const char* aseprite_path, CF_Sprite* sprite)
{
	// First see if this ase was already cached.
	aseprite_path = sintern(aseprite_path);
	auto entry_ptr = cache->aseprites.try_find(aseprite_path);
	if (entry_ptr) {
		s_sprite(*entry_ptr, sprite);
		return cf_result_success();
	}

	// Load the aseprite file.
	size_t sz = 0;
	void* data = cf_fs_read_entire_file_to_memory(aseprite_path, &sz);
	if (!data) return cf_result_error("Unable to open ase file at `aseprite_path`.");
	CF_DEFER(CF_FREE(data));

	return s_aseprite_cache_load_from_memory(aseprite_path, data, (int)sz, sprite);
}

void cf_aseprite_cache_unload(const char* aseprite_path)
{
	aseprite_path = sintern(aseprite_path);
	auto entry_ptr = cache->aseprites.try_find(aseprite_path);
	if (!entry_ptr) return;

	CF_AsepriteCacheEntry entry = *entry_ptr;
	for (int i = 0; i < hcount(entry.animations); ++i) {
		CF_Animation* animation = entry.animations[i];
		for (int j = 0; j < alen(animation->frames); ++j) {
			uint64_t id = animation->frames[j].id;
			cache->id_to_pixels.remove(id);
			spritebatch_invalidate(&draw->sb, id);
		}

		afree(animation->frames);
		CF_FREE(animation);
	}

	hfree(entry.animations);
	cute_aseprite_free(entry.ase);
	cache->aseprites.remove(aseprite_path);
}

CF_Result cf_aseprite_cache_load_ase(const char* aseprite_path, ase_t** ase)
{
	aseprite_path = sintern(aseprite_path);
	CF_Sprite s;
	CF_Result err = cf_aseprite_cache_load(aseprite_path, &s);
	if (cf_is_error(err)) return err;

	auto entry_ptr = cache->aseprites.try_find(aseprite_path);
	if (!entry_ptr) return cf_result_error("Unable to load aseprite.");
	else {
		*ase = entry_ptr->ase;
		return cf_result_success();
	}
}
