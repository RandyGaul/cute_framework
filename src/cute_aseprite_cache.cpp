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

#include <cute_aseprite_cache.h>
#include <cute_sprite.h>
#include <cute_debug_printf.h>
#include <cute_file_system.h>
#include <cute_defer.h>

#include <internal/cute_app_internal.h>

#define CUTE_ASEPRITE_IMPLEMENTATION
#include <cute/cute_aseprite.h>

#include <cute/cute_spritebatch.h>

using namespace cute;

struct aseprite_cache_entry_t
{
	const char* path = NULL;
	ase_t* ase = NULL;
	animation_t** animations = NULL;
	cf_v2 local_offset = V2(0, 0);
};

struct cf_aseprite_cache_t
{
	dictionary<const char*, aseprite_cache_entry_t> aseprites;
	dictionary<uint64_t, void*> id_to_pixels;
	uint64_t id_gen = 0;
};

static void s_get_pixels(uint64_t image_id, void* buffer, int bytes_to_fill, void* udata)
{
	cf_aseprite_cache_t* cache = (cf_aseprite_cache_t*)udata;
	auto pixels_ptr = cache->id_to_pixels.try_find(image_id);
	if (!pixels_ptr) {
		CUTE_DEBUG_PRINTF("Aseprite cache -- unable to find id %lld.", (long long int)image_id);
		CUTE_MEMSET(buffer, 0, bytes_to_fill);
	} else {
		void* pixels = *pixels_ptr;
		CUTE_MEMCPY(buffer, pixels, bytes_to_fill);
	}
}

cf_aseprite_cache_t* cf_make_aseprite_cache()
{
	cf_aseprite_cache_t* cache = CUTE_NEW(cf_aseprite_cache_t);
	return cache;
}

void cf_destroy_aseprite_cache(cf_aseprite_cache_t* cache)
{
	int count = cache->aseprites.count();
	aseprite_cache_entry_t* entries = cache->aseprites.items();
	for (int i = 0; i < count; ++i) 
	{
		aseprite_cache_entry_t* entry = entries + i;
		int animation_count = hcount(entry->animations);
		for (int j = 0; j < animation_count; ++j)
		{
			afree(entry->animations[j]->frames);
			CUTE_FREE(entry->animations[j]);
		}

		hfree(entry->animations);
		cute_aseprite_free(entry->ase);
	}
	cache->~cf_aseprite_cache_t();
	CUTE_FREE(cache);
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

static void s_sprite(cf_aseprite_cache_t* cache, aseprite_cache_entry_t entry, CF_Sprite* sprite)
{
	sprite->name = entry.path;
	sprite->animations = (const animation_t**)entry.animations;
	sprite->w = entry.ase->w;
	sprite->h = entry.ase->h;
	sprite->local_offset = entry.local_offset;
	if (entry.ase->tag_count == 0) {
		cf_sprite_play(sprite, "default");
	} else {
		cf_sprite_play(sprite, sprite->animations[0]->name);
	}
}

CF_Result cf_aseprite_cache_load(cf_aseprite_cache_t* cache, const char* aseprite_path, CF_Sprite* sprite)
{
	// First see if this ase was already cached.
	aseprite_path = sintern(aseprite_path);
	auto entry_ptr = cache->aseprites.try_find(aseprite_path);
	if (entry_ptr) {
		s_sprite(cache, *entry_ptr, sprite);
		return cf_result_success();
	}

	// Load the aseprite file.
	void* data = NULL;
	size_t sz = 0;
	cf_fs_read_entire_file_to_memory(aseprite_path, &data, &sz);
	if (!data) return cf_result_error("Unable to open ase file at `aseprite_path`.");
	CUTE_DEFER(CUTE_FREE(data));
	ase_t* ase = cute_aseprite_load_from_memory(data, (int)sz, NULL);
	if (!ase) return cf_result_error("Unable to open ase file at `aseprite_path`.");

	// Allocate internal cache data structure entries.
	animation_t** animations = NULL;
	array<uint64_t> ids;
	ids.ensure_capacity(ase->frame_count);

	for (int i = 0; i < ase->frame_count; ++i) {
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
	}

	// Fill out the animation table from the aseprite file.
	if (ase->tag_count) {
		// Each tag represents a single animation.
		for (int i = 0; i < ase->tag_count; ++i) {
			ase_tag_t* tag = ase->tags + i;
			int from = tag->from_frame;
			int to = tag->to_frame;
			CF_Animation* animation = (CF_Animation*)CUTE_ALLOC(sizeof(CF_Animation));
			CUTE_MEMSET(animation, 0, sizeof(CF_Animation));

			animation->name = sintern(tag->name);
			animation->play_direction = s_play_direction(tag->loop_animation_direction);
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
		CF_Animation* animation = (CF_Animation*)CUTE_ALLOC(sizeof(CF_Animation));
		CUTE_MEMSET(animation, 0, sizeof(CF_Animation));

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

	// Look for slice information to define the sprite's local offset.
	// The slice named "origin"'s center is used to define the local offset.
	aseprite_cache_entry_t entry;
	for (int i = 0; i < ase->slice_count; ++i) {
		ase_slice_t* slice = ase->slices + i;
		if (!CUTE_STRCMP(slice->name, "origin")) {
			// Invert y-axis since ase saves slice as (0, 0) top-left.
			float y = (float)slice->origin_y + (float)slice->h * 0.25f;
			y = (float)ase->h - y - 1;
			float x = (float)slice->origin_x + (float)slice->w * 0.25f;

			// Transform from top-left coordinates to center of sprite.
			cf_v2 origin = cf_V2(x, y);
			cf_v2 offset = cf_V2((float)ase->w - 1, (float)ase->h - 1) * 0.5f - origin;
			entry.local_offset = offset;
			break;
		}
	}

	// Cache the ase and animation.
	entry.path = aseprite_path;
	entry.ase = ase;
	entry.animations = animations;
	cache->aseprites.insert(aseprite_path, entry);

	s_sprite(cache, entry, sprite);
	return cf_result_success();
}

void cf_aseprite_cache_unload(cf_aseprite_cache_t* cache, const char* aseprite_path)
{
	aseprite_path = sintern(aseprite_path);
	auto entry_ptr = cache->aseprites.try_find(aseprite_path);
	if (!entry_ptr) return;
	
	aseprite_cache_entry_t entry = *entry_ptr;
	for (int i = 0; i < hcount(entry.animations); ++i) {
		CF_Animation* animation = entry.animations[i];
		for (int j = 0; j < alen(animation->frames); ++j) {
			cache->id_to_pixels.remove(animation->frames[j].id);
		}

		afree(animation->frames);
		CUTE_FREE(animation);
	}

	hfree(entry.animations);
	cache->aseprites.remove(aseprite_path);
}

CF_Result cf_aseprite_cache_load_ase(cf_aseprite_cache_t* cache, const char* aseprite_path, ase_t** ase)
{
	aseprite_path = sintern(aseprite_path);
	CF_Sprite s;
	CF_Result err = cf_aseprite_cache_load(cache, aseprite_path, &s);
	if (cf_is_error(err)) return err;

	auto entry_ptr = cache->aseprites.try_find(aseprite_path);
	if (!entry_ptr) return cf_result_error("Unable to load aseprite.");
	else {
		*ase = entry_ptr->ase;
		return cf_result_success();
	}
}

get_pixels_fn* cf_aseprite_cache_get_pixels_fn(cf_aseprite_cache_t* cache)
{
	return s_get_pixels;
}
