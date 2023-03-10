/*
	Cute Framework
	Copyright (C) 2023 Randy Gaul https://randygaul.github.io/

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

#include <internal/cute_alloc_internal.h>
#include <internal/cute_png_cache_internal.h>
#include <internal/cute_app_internal.h>

struct CF_PngCache
{
	htbl void** id_to_pixels = NULL;
	dyna CF_Animation** animations = NULL;
	htbl CF_Animation*** animation_tables = NULL;
	htbl CF_Png* pngs = NULL;
	uint64_t id_gen = CF_PNG_ID_RANGE_LO;
};

static CF_PngCache* cache;

void cf_png_cache_get_pixels(uint64_t image_id, void* buffer, int bytes_to_fill)
{
	void* pixels = hget(cache->id_to_pixels, image_id);
	if (!pixels) {
		CF_DEBUG_PRINTF("png cache -- unable to find id %lld.", (long long int)image_id);
		CF_MEMSET(buffer, 0, bytes_to_fill);
	} else {
		CF_MEMCPY(buffer, pixels, bytes_to_fill);
	}
}

CF_Png cf_png_cache_get_png(uint64_t image_id)
{
	CF_Png png = hget(cache->pngs, image_id);
	return png;
}

void cf_make_png_cache()
{
	cache = CF_NEW(CF_PngCache);
}

void cf_destroy_png_cache()
{
	for (int i = 0; i < hsize(cache->animations); ++i) {
		afree(cache->animations[i]->frames);
		CF_FREE(cache->animations[i]);
	}
	hfree(cache->animations);

	for (int i = 0; i < hsize(cache->animation_tables); ++i) {
		hfree(cache->animation_tables[i]);
	}
	hfree(cache->animation_tables);

	int i = 0;
	while (hsize(cache->pngs)) {
		CF_Png png = cache->pngs[i++];
		cf_png_cache_unload(png);
	}
	hfree(cache->pngs);
	hfree(cache->id_to_pixels);

	CF_FREE(cache);
	cache = NULL;
}

CF_Result cf_png_cache_load(const char* png_path, CF_Png* png)
{
	CF_Image img;
	CF_Result err = cf_image_load_png(png_path, &img);
	if (cf_is_error(err)) return err;
	cf_image_premultiply(&img);
	CF_Png entry;
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

CF_Result cf_png_cache_load_mem(const char* png_path, const void* memory, size_t size, CF_Png* png)
{
	CF_Image img;
	CF_Result err = cf_image_load_png_mem(memory, (int)size, &img);
	if (cf_is_error(err)) return err;
	CF_Png entry;
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

void cf_png_cache_unload(CF_Png png)
{
	CF_Image img;
	img.pix = png.pix;
	img.w = png.w;
	img.h = png.h;
	cf_image_free(&img);
	hdel(cache->id_to_pixels, png.id);
	hdel(cache->pngs, png.id);
}

const CF_Animation* cf_make_png_cache_animation(const char* name, const CF_Png* pngs, int pngs_count, const float* delays, int delays_count)
{
	CF_ASSERT(pngs_count == delays_count);
	name = sintern(name);

	// If already made, just return the old animation.
	CF_Animation* animation = NULL;
	if (cache->animations) {
		animation = hget(cache->animations, name);
		if (animation) return animation;
	}

	// Otherwise allocate a new animation.
	animation = (CF_Animation*)CF_ALLOC(sizeof(CF_Animation));
	CF_MEMSET(animation, 0, sizeof(CF_Animation));
	animation->name = name;
	hadd(cache->animations, name, animation);

	for (int i = 0; i < pngs_count; ++i) {
		CF_Frame frame;
		frame.id = pngs[i].id;
		frame.delay = delays[i];
		cf_animation_add_frame(animation, frame);
	}

	return animation;
}

const CF_Animation* cf_png_cache_get_animation(const char* name)
{
	return hget(cache->animations, sintern(name));
}

const CF_Animation** cf_make_png_cache_animation_table(const char* sprite_name, const CF_Animation* const* animations, int animations_count)
{
	sprite_name = sintern(sprite_name);

	// If already made, just return the old table.
	CF_Animation** table = NULL;
	if (cache->animation_tables) {
		table = hget(cache->animation_tables, sprite_name);
		if (table) return (const CF_Animation**)table;
	}

	// Otherwise allocate a new table entry.
	for (int i = 0; i < animations_count; ++i) {
		hadd(table, animations[i]->name, (CF_Animation*)animations[i]);
	}
	hset(cache->animation_tables, sprite_name, table);

	return (const CF_Animation**)table;
}

const CF_Animation** cf_png_cache_get_animation_table(const char* sprite_name)
{
	return (const CF_Animation**)hget(cache->animation_tables, sintern(sprite_name));
}

CF_Sprite cf_make_png_cache_sprite(const char* sprite_name, const CF_Animation** table)
{
	sprite_name = sintern(sprite_name);
	CF_ASSERT(table);

	CF_Png png = hget(cache->pngs, table[0]->frames[0].id);
	CF_ASSERT(png.path);

	CF_Sprite sprite = cf_sprite_defaults();
	sprite.name = sprite_name;
	sprite.w = png.w;
	sprite.h = png.h;
	sprite.animations = table;
	cf_sprite_play(&sprite, sprite.animations[0]->name);

	return sprite;
}

namespace Cute
{

const animation_t* make_png_cache_animation(const char* name, const Array<Png>& pngs, const Array<float>& delays)
{
	return cf_make_png_cache_animation(name, pngs.data(), pngs.count(), delays.data(), delays.count());
}
const animation_t** make_png_cache_animation_table(const char* sprite_name, const Array<const animation_t*>& animations)
{
	return cf_make_png_cache_animation_table(sprite_name, animations.data(), animations.count());
}

}
