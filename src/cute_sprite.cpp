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

#include <cute_sprite.h>
#include <cute_debug_printf.h>

#include <internal/cute_app_internal.h>

#define CUTE_ASEPRITE_IMPLEMENTATION
#include <cute/cute_aseprite.h>
#include <mattiasgustavsson/strpool.h>

#define INJECT(s) strpool_inject(cache->strpool, s, (int)CUTE_STRLEN(s))

namespace cute
{

struct aseprite_cache_entry_t
{
	STRPOOL_U64 path;
	ase_t* ase;
	animation_table_t* animations;
	v2 local_offset;
};

struct aseprite_cache_t
{
	dictionary<STRPOOL_U64, aseprite_cache_entry_t> aseprites;
	dictionary<uint64_t, void*> id_to_pixels;
	batch_t* batch = NULL;
	uint64_t id_gen = 0;
	strpool_t strpool_instance;
	strpool_t* strpool = NULL;
	void* mem_ctx = NULL;
};

static void s_get_pixels(uint64_t image_id, void* buffer, int bytes_to_fill, void* udata)
{
	aseprite_cache_t* cache = (aseprite_cache_t*)udata;
	void* pixels = NULL;
	if (cache->id_to_pixels.find(image_id, &pixels).is_error()) {
		CUTE_DEBUG_PRINTF("Aseprite cache -- unable to find id %lld.", (long long int)image_id);
		CUTE_MEMSET(buffer, 0, bytes_to_fill);
	} else {
		CUTE_MEMCPY(buffer, pixels, bytes_to_fill);
	}
}

aseprite_cache_t* aseprite_cache_make(app_t* app)
{
	aseprite_cache_t* cache = CUTE_NEW(aseprite_cache_t, app->mem_ctx);
	cache->batch = batch_make(app, s_get_pixels, cache);
	strpool_config_t config = strpool_default_config;
	config.memctx = app->mem_ctx;
	strpool_init(&cache->strpool_instance, &config);
	cache->strpool = &cache->strpool_instance;
	cache->mem_ctx = app->mem_ctx;
	return cache;
}

void aseprite_cache_destroy(aseprite_cache_t* cache)
{
	cache->~aseprite_cache_t();
}

static play_direction_t s_play_direction(ase_animation_direction_t direction)
{
	switch (direction) {
	case ASE_ANIMATION_DIRECTION_FORWARDS: return PLAY_DIRECTION_FORWARDS;
	case ASE_ANIMATION_DIRECTION_BACKWORDS: return PLAY_DIRECTION_BACKWARDS;
	case ASE_ANIMATION_DIRECTION_PINGPONG: return PLAY_DIRECTION_PINGPONG;
	}
	return PLAY_DIRECTION_FORWARDS;
}

static void s_sprite(aseprite_cache_t* cache, aseprite_cache_entry_t entry, sprite_t* sprite)
{
	sprite->name = strpool_cstr(cache->strpool, entry.path);
	sprite->animations = entry.animations;
	sprite->w = entry.ase->w;
	sprite->h = entry.ase->h;
	sprite->local_offset = entry.local_offset;
	if (entry.ase->tag_count == 0) {
		sprite->play("default");
	}
}

error_t aseprite_cache_load(aseprite_cache_t* cache, const char* aseprite_path, sprite_t* sprite)
{
	// First see if this ase was already cached.
	STRPOOL_U64 path = INJECT(aseprite_path);
	aseprite_cache_entry_t entry;
	if (!cache->aseprites.find(path, &entry).is_error()) {
		s_sprite(cache, entry, sprite);
		return error_success();
	}

	// Load the aseprite file.
	ase_t* ase = cute_aseprite_load_from_file(aseprite_path, cache->mem_ctx);
	if (!ase) return error_failure("Unable to open ase file at `aseprite_path`.");

	// Allocate internal cache data structure entries.
	animation_table_t* animations = CUTE_NEW(animation_table_t, cache->mem_ctx);
	array<uint64_t> ids;
	ids.ensure_capacity(ase->frame_count);

	for (int i = 0; i < ase->frame_count; ++i) {
		uint64_t id = cache->id_gen++;
		ids.add(id);
		cache->id_to_pixels.insert(id, ase->frames[i].pixels);
	}

	// Fill out the animation table from the aseprite file.
	if (ase->tag_count) {
		// Each tag represents a single animation.
		for (int i = 0; i < ase->tag_count; ++i) {
			ase_tag_t* tag = ase->tags + i;
			int from = tag->from_frame;
			int to = tag->to_frame;
			animation_t* animation = CUTE_NEW(animation_t, cache->mem_ctx);
			animation->name = tag->name;
			animation->play_direction = s_play_direction(tag->loop_animation_direction);
			for (int i = from; i <= to; ++i) {
				uint64_t id = ids[i];
				frame_t frame;
				frame.delay = ase->frames[i].duration_milliseconds / 1000.0f;
				frame.id = id;
				animation->frames.add(frame);
			}
			animations->insert(animation->name, animation);
		}
	} else {
		// Treat the entire frame set as a single animation if there are no tags.
		animation_t* animation = CUTE_NEW(animation_t, cache->mem_ctx);
		animation->name = "default";
		animation->play_direction = PLAY_DIRECTION_FORWARDS;
		for (int i = 0; i < ase->frame_count; ++i) {
			uint64_t id = ids[i];
			frame_t frame;
			frame.delay = ase->frames[i].duration_milliseconds / 1000.0f;
			frame.id = id;
			animation->frames.add(frame);
		}
		animations->insert(animation->name, animation);
	}

	// Look for slice information to define the sprite's local offset.
	// The slice named "origin"'s center is used to define the local offset.
	entry.local_offset = v2(0, 0);
	for (int i = 0; i < ase->slice_count; ++i) {
		ase_slice_t* slice = ase->slices + i;
		if (!CUTE_STRCMP(slice->name, "origin")) {
			// Invert y-axis since ase saves slice as (0, 0) top-left.
			float y = (float)slice->origin_y + (float)slice->h * 0.25f;
			y = (float)ase->h - y - 1;
			float x = (float)slice->origin_x + (float)slice->w * 0.25f;

			// Transform from top-left coordinates to center of sprite.
			v2 origin = v2(x, y);
			v2 offset = v2((float)ase->w - 1, (float)ase->h - 1) * 0.5f - origin;
			entry.local_offset = offset;
			break;
		}
	}

	// Cache the ase and animation.
	entry.path = path;
	entry.ase = ase;
	entry.animations = animations;
	cache->aseprites.insert(path, entry);

	s_sprite(cache, entry, sprite);
	return error_success();
}

void aseprite_cache_unload(aseprite_cache_t* cache, const char* aseprite_path)
{
	STRPOOL_U64 path = INJECT(aseprite_path);
	aseprite_cache_entry_t entry;
	if (cache->aseprites.find(path, &entry).is_error()) return;
	
	int animation_count = entry.animations->count();
	const animation_t** animations = entry.animations->items();

	for (int i = 0; i < animation_count; ++i) {
		animation_t* animation = (animation_t*)animations[i];
		for (int j = 0; j < animation->frames.count(); ++j) {
			cache->id_to_pixels.remove(animation->frames[j].id);
		}
		animation->~animation_t();
		CUTE_FREE(animation, cache->mem_ctx);
	}

	entry.animations->~animation_table_t();
	CUTE_FREE(entry.animations, cache->mem_ctx);
	cache->aseprites.remove(path);
}

batch_t* aseprite_cache_get_batch_ptr(aseprite_cache_t* cache)
{
	return cache->batch;
}

strpool_t* aseprite_cache_get_strpool_ptr(aseprite_cache_t* cache)
{
	return cache->strpool;
}

}
