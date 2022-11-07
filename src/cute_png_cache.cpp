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

#include <cute_png_cache.h>
#include <cute_image.h>
#include <cute_sprite.h>
#include <cute_hashtable.h>

#include <internal/cute_png_cache_internal.h>
#include <internal/cute_app_internal.h>

static void cf_s_get_pixels(uint64_t image_id, void* buffer, int bytes_to_fill, void* udata)
{
	cf_png_cache_t* cache = (cf_png_cache_t*)udata;
	void* pixels = hget(cache->id_to_pixels, image_id);
	if (!pixels) {
		CUTE_DEBUG_PRINTF("png cache -- unable to find id %lld.", (long long int)image_id);
		CUTE_MEMSET(buffer, 0, bytes_to_fill);
	} else {
		CUTE_MEMCPY(buffer, pixels, bytes_to_fill);
	}
}

cf_png_cache_t* cf_make_png_cache(void* mem_ctx)
{
	cf_png_cache_t* cache = CUTE_NEW(cf_png_cache_t, cf_app->mem_ctx);
	return cache;
}

void cf_destroy_png_cache(cf_png_cache_t* cache)
{
	cf_animation_t** animations = cache->animations;
	for (int i = 0; i < hsize(cache->animations); ++i) {
		afree(animations[i]->frames);
	}
	hfree(cache->animations);

	cf_animation_t*** tables = cache->animation_tables;
	for (int i = 0; i < hsize(tables); ++i) {
		hfree(tables[i]);
	}
	hfree(tables);

	for (int i = 0; i < alen(cache->pngs); ++i) {
		cf_png_t png = cache->pngs[i];
		cf_png_cache_unload(cache, png);
	}
	afree(cache->pngs);

	CUTE_FREE(cache, NULL);
}

cf_result_t cf_png_cache_load(cf_png_cache_t* cache, const char* png_path, cf_png_t* png)
{
	cf_image_t img;
	cf_result_t err = cf_image_load_png(png_path, &img);
	if (cf_is_error(err)) return err;
	cf_png_t entry;
	entry.path = sintern(png_path);
	entry.id = cache->id_gen++;
	entry.pix = img.pix;
	entry.w = img.w;
	entry.h = img.h;
	hadd(cache->id_to_pixels, entry.id, img.pix);
	hadd(cache->pngs, entry.id, entry);
	if (png) *png = entry;
	return cf_result_success();
}

cf_result_t cf_png_cache_load_mem(cf_png_cache_t* cache, const char* png_path, const void* memory, size_t size, cf_png_t* png)
{
	cf_image_t img;
	cf_result_t err = cf_image_load_png_mem(memory, (int)size, &img);
	if (cf_is_error(err)) return err;
	cf_png_t entry;
	entry.path = sintern(png_path);
	entry.id = cache->id_gen++;
	entry.pix = img.pix;
	entry.w = img.w;
	entry.h = img.h;
	hadd(cache->id_to_pixels, entry.id, img.pix);
	hadd(cache->pngs, entry.id, entry);
	if (png) *png = entry;
	return cf_result_success();
}

void cf_png_cache_unload(cf_png_cache_t* cache, cf_png_t png)
{
	cf_image_t img;
	img.pix = png.pix;
	img.w = png.w;
	img.h = png.h;
	cf_image_free(&img);
	hdel(cache->id_to_pixels, png.id);
	hdel(cache->pngs, png.id);
}

cf_get_pixels_fn* cf_png_cache_get_pixels_fn(cf_png_cache_t* cache)
{
	return cf_s_get_pixels;
}

const cf_animation_t* cf_make_png_cache_animation(cf_png_cache_t* cache, const char* name, const cf_png_t* pngs, int pngs_count, const float* delays, int delays_count)
{
	CUTE_ASSERT(pngs_count == delays_count);
	name = sintern(name);

	// If already made, just return the old animation.
	cf_animation_t* animation = hget(cache->animations, name);
	if (animation) return animation;

	// Otherwise allocate a new animation.
	animation = (cf_animation_t*)CUTE_ALLOC(sizeof(cf_animation_t), cache->mem_ctx);
	CUTE_MEMSET(animation, 0, sizeof(cf_animation_t));
	hadd(cache->animations, name, animation);

	for (int i = 0; i < pngs_count; ++i) {
		cf_frame_t frame;
		frame.id = pngs[i].id;
		frame.delay = delays[i];
		cf_animation_add_frame(animation, frame);
	}

	return animation;
}

const cf_animation_t* cf_png_cache_get_animation(cf_png_cache_t* cache, const char* name)
{
	return hget(cache->animations, sintern(name));
}

const cf_animation_t** cf_make_png_cache_animation_table(cf_png_cache_t* cache, const char* sprite_name, const cf_animation_t* const* animations, int animations_count)
{
	sprite_name = sintern(sprite_name);

	// If already made, just return the old table.
	cf_animation_t** table = hget(cache->animation_tables, sprite_name);
	if (table) return table;

	// Otherwise allocate a new table entry.
	for (int i = 0; i < animations_count; ++i) {
		hadd(table, animations[i]->name, (cf_animation_t*)animations[i]);
	}
	hset(cache->animation_tables, sprite_name, table);

	return table;
}

const cf_animation_t** cf_png_cache_get_animation_table(cf_png_cache_t* cache, const char* sprite_name)
{
	 return hget(cache->animation_tables, sintern(sprite_name));
}

cf_sprite_t cf_make_png_cache_sprite(cf_png_cache_t* cache, const char* sprite_name, const cf_animation_t** table)
{
	sprite_name = sintern(sprite_name);
	cf_animation_t** table = hget(cache->animation_tables, sprite_name);
	CUTE_ASSERT(table);

	cf_png_t png = hget(cache->pngs, table[0]->frames[0].id);
	CUTE_ASSERT(png.path);

	cf_sprite_t sprite = cf_sprite_defaults();
	sprite.name = sprite_name;
	sprite.w = png.w;
	sprite.h = png.h;
	sprite.animations = table;
	cf_sprite_play(&sprite, sprite.animations[0]->name);

	return sprite;
}

namespace cute
{

const animation_t* make_png_cache_animation(png_cache_t* cache, const char* name, const array<png_t>& pngs, const array<float>& delays)
{
	return cf_make_png_cache_animation(cache, name, pngs.data(), pngs.count(), delays.data(), delays.count());
}
const animation_t** make_png_cache_animation_table(png_cache_t* cache, const char* sprite_name, const array<const animation_t*>& animations)
{
	return cf_make_png_cache_animation_table(cache, sprite_name, animations.data(), animations.count());
}

}
