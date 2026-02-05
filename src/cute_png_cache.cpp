/*
	Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

#include <cute_png_cache.h>
#include <cute_image.h>
#include <cute_sprite.h>
#include <cute_map.h>

#include <internal/cute_alloc_internal.h>
#include <internal/cute_png_cache_internal.h>
#include <internal/cute_app_internal.h>

struct CF_PngCache
{
	CK_MAP(void*) id_to_pixels = NULL;
	dyna CF_Animation** animations_list = NULL;
	CK_MAP(CF_AnimationTable) animation_tables = NULL;
	CK_MAP(CF_Png) pngs = NULL;
	CK_MAP(CF_Animation*) animations = NULL;
	uint64_t id_gen = CF_PNG_ID_RANGE_LO;
};

CF_GLOBAL static CF_PngCache* cache;

void cf_png_cache_get_pixels(uint64_t image_id, void* buffer, int bytes_to_fill)
{
	void** pixels_ptr = map_get_ptr(cache->id_to_pixels, image_id);
	if (!pixels_ptr) {
		CF_DEBUG_PRINTF("png cache -- unable to find id %lld.", (long long int)image_id);
		CF_MEMSET(buffer, 0, bytes_to_fill);
	} else {
		CF_MEMCPY(buffer, *pixels_ptr, bytes_to_fill);
	}
}

void cf_make_png_cache()
{
	cache = CF_NEW(CF_PngCache);
}

void cf_destroy_png_cache()
{
	for (int i = 0; i < asize(cache->animations_list); ++i) {
		afree(cache->animations_list[i]->frames);
		CF_FREE(cache->animations_list[i]);
	}
	afree(cache->animations_list);

	for (int i = 0; i < map_size(cache->animation_tables); ++i) {
		CF_AnimationTable inner = map_items(cache->animation_tables)[i];
		map_free(inner);
	}
	map_free(cache->animation_tables);

	int i = 0;
	while (map_size(cache->pngs)) {
		CF_Png png = map_items(cache->pngs)[i];
		cf_png_cache_unload(png);
	}
	map_free(cache->pngs);
	map_free(cache->id_to_pixels);
	map_free(cache->animations);

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
	map_set(cache->id_to_pixels, entry.id, img.pix);
	map_set(cache->pngs, entry.id, entry);
	if (png) *png = entry;
	return cf_result_success();
}

CF_Result cf_png_cache_load_from_memory(const char* png_path, const void* memory, size_t size, CF_Png* png)
{
	CF_Image img;
	CF_Result err = cf_image_load_png_from_memory(memory, (int)size, &img);
	if (cf_is_error(err)) return err;
	CF_Png entry;
	entry.path = sintern(png_path);
	entry.id = cache->id_gen++;
	entry.pix = img.pix;
	entry.w = img.w;
	entry.h = img.h;
	map_set(cache->id_to_pixels, entry.id, img.pix);
	map_set(cache->pngs, entry.id, entry);
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
	map_del(cache->id_to_pixels, png.id);
	map_del(cache->pngs, png.id);
}

const CF_Animation* cf_make_png_cache_animation(const char* name, const CF_Png* pngs, int pngs_count, const float* delays, int delays_count)
{
	CF_ASSERT(pngs_count == delays_count);
	name = sintern(name);

	// If already made, just return the old animation.
	if (map_size(cache->animations)) {
		CF_Animation** ptr = map_get_ptr(cache->animations, (uint64_t)name);
		if (ptr) return *ptr;
	}

	// Otherwise allocate a new animation.
	CF_Animation* animation = (CF_Animation*)CF_ALLOC(sizeof(CF_Animation));
	CF_MEMSET(animation, 0, sizeof(CF_Animation));
	animation->name = name;
	map_set(cache->animations, (uint64_t)name, animation);
	apush(cache->animations_list, animation);

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
	CF_Animation** ptr = map_get_ptr(cache->animations, (uint64_t)sintern(name));
	return ptr ? *ptr : NULL;
}

const CF_AnimationTable* cf_make_png_cache_animation_table(const char* sprite_name, const CF_Animation* const* animations, int animations_count)
{
	sprite_name = sintern(sprite_name);

	// If already made, just return the old table.
	if (map_size(cache->animation_tables)) {
		CF_AnimationTable* table = map_get_ptr(cache->animation_tables, (uint64_t)sprite_name);
		if (table && map_size(*table)) return table;
	}

	// Otherwise allocate a new table entry.
	CF_AnimationTable new_table = NULL;
	for (int i = 0; i < animations_count; ++i) {
		map_set(new_table, (uint64_t)animations[i]->name, animations[i]);
	}
	map_set(cache->animation_tables, (uint64_t)sprite_name, new_table);

	return map_get_ptr(cache->animation_tables, (uint64_t)sprite_name);
}

const CF_AnimationTable* cf_png_cache_get_animation_table(const char* sprite_name)
{
	return map_get_ptr(cache->animation_tables, (uint64_t)sintern(sprite_name));
}

CF_Sprite cf_make_png_cache_sprite(const char* sprite_name, const CF_AnimationTable* table)
{
	sprite_name = sintern(sprite_name);
	CF_Sprite sprite = cf_sprite_defaults();
	sprite.name = sprite_name;

	if (table) {
		const CF_Animation** first_anim = (const CF_Animation**)map_items(*table);
		CF_Png* png_ptr = map_get_ptr(cache->pngs, (*first_anim)->frames[0].id);
		CF_ASSERT(png_ptr && png_ptr->path);
		sprite.w = png_ptr->w;
		sprite.h = png_ptr->h;
		sprite.animations = (CF_AnimationTable*)table;
		cf_sprite_play(&sprite, (*first_anim)->name);
	} else {
		CF_Png entry;
		CF_Result result = cf_png_cache_load(sprite_name, &entry);
		if (cf_is_error(result)) {
			return cf_sprite_defaults();
		} else {
			sprite.w = entry.w;
			sprite.h = entry.h;
			sprite.easy_sprite_id = entry.id;
		}
	}

	return sprite;
}

namespace Cute
{

const CF_Animation* make_png_cache_animation(const char* name, const Array<CF_Png>& pngs, const Array<float>& delays)
{
	return cf_make_png_cache_animation(name, pngs.data(), pngs.count(), delays.data(), delays.count());
}
const CF_AnimationTable* make_png_cache_animation_table(const char* sprite_name, const Array<const CF_Animation*>& animations)
{
	return cf_make_png_cache_animation_table(sprite_name, animations.data(), animations.count());
}

}
