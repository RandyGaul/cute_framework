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

#include <internal/cute_app_internal.h>

#define INJECT(s) strpool_inject(cache->strpool, s, (int)CUTE_STRLEN(s))

namespace cute
{

struct png_cache_t
{
	dictionary<uint64_t, png_t> pngs;
	dictionary<uint64_t, void*> id_to_pixels;
	dictionary<strpool_id, animation_t*> animations;
	dictionary<strpool_id, animation_table_t*> animation_tables;
	uint64_t id_gen = 0;
	strpool_t* strpool = NULL;
	void* mem_ctx = NULL;
};

static void s_get_pixels(uint64_t image_id, void* buffer, int bytes_to_fill, void* udata)
{
	png_cache_t* cache = (png_cache_t*)udata;
	void* pixels = NULL;
	if (cache->id_to_pixels.find(image_id, &pixels).is_error()) {
		CUTE_DEBUG_PRINTF("png cache -- unable to find id %lld.", (long long int)image_id);
		CUTE_MEMSET(buffer, 0, bytes_to_fill);
	} else {
		CUTE_MEMCPY(buffer, pixels, bytes_to_fill);
	}
}

png_cache_t* png_cache_make(void* mem_ctx)
{
	png_cache_t* cache = CUTE_NEW(png_cache_t, app->mem_ctx);
	cache->strpool = make_strpool(mem_ctx);
	cache->mem_ctx = mem_ctx;
	return cache;
}

void png_cache_destroy(png_cache_t* cache)
{
	int animation_count = cache->animations.count();
	animation_t** animations = cache->animations.items();
	for (int i = 0; i < animation_count; ++i) {
		animations[i]->~animation_t();
		CUTE_FREE(animations[i], cache->mem_ctx);
	}

	int table_count = cache->animation_tables.count();
	animation_table_t** tables = cache->animation_tables.items();
	for (int i = 0; i < table_count; ++i) {
		tables[i]->~animation_table_t();
		CUTE_FREE(tables[i], cache->mem_ctx);
	}

	while (cache->pngs.count()) {
		png_t png = cache->pngs.items()[0];
		png_cache_unload(cache, &png);
	}

	destroy_strpool(cache->strpool);
	void* mem_ctx = cache->mem_ctx;
	cache->~png_cache_t();
	CUTE_FREE(cache, mem_ctx);
}

error_t png_cache_load(png_cache_t* cache, const char* png_path, png_t* png)
{
	image_t img;
	error_t err = image_load_png(png_path, &img, cache->mem_ctx);
	if (err.is_error()) return err;
	png_t entry;
	entry.path = strpool_cstr(cache->strpool, INJECT(png_path));
	entry.id = cache->id_gen++;
	entry.pix = img.pix;
	entry.w = img.w;
	entry.h = img.h;
	cache->id_to_pixels.insert(entry.id, img.pix);
	cache->pngs.insert(entry.id, entry);
	if (png) *png = entry;
	return error_success();
}

error_t png_cache_load_mem(png_cache_t* cache, const char* png_path, const void* memory, size_t size, png_t* png)
{
	image_t img;
	error_t err = image_load_png_mem(memory, (int)size, &img, cache->mem_ctx);
	if (err.is_error()) return err;
	png_t entry;
	entry.path = strpool_cstr(cache->strpool, INJECT(png_path));
	entry.id = cache->id_gen++;
	entry.pix = img.pix;
	entry.w = img.w;
	entry.h = img.h;
	cache->id_to_pixels.insert(entry.id, img.pix);
	cache->pngs.insert(entry.id, entry);
	if (png) *png = entry;
	return error_success();
}

void png_cache_unload(png_cache_t* cache, png_t* png)
{
	image_t img;
	img.pix = png->pix;
	img.w = png->w;
	img.h = png->h;
	image_free(&img);
	cache->id_to_pixels.remove(png->id);
	cache->pngs.remove(png->id);
	CUTE_MEMSET(png, 0, sizeof(*png));
}

get_pixels_fn* png_cache_get_pixels_fn(png_cache_t* cache)
{
	return s_get_pixels;
}

strpool_t* png_cache_get_strpool_ptr(png_cache_t* cache)
{
	return cache->strpool;
}

const animation_t* png_cache_make_animation(png_cache_t* cache, const char* name, const array<png_t>& pngs, const array<float>& delays)
{
	CUTE_ASSERT(pngs.count() == delays.count());
	strpool_id name_id = INJECT(name);

	// If already made, just return the old animation.
	animation_t* animation;
	if (!cache->animations.find(name_id, &animation).is_error()) {
		return animation;
	}

	// Otherwise allocate a new animation.
	animation_t** animation_ptr = cache->animations.insert(name_id);
	animation = (animation_t*)CUTE_ALLOC(sizeof(animation_t), cache->mem_ctx);
	CUTE_PLACEMENT_NEW(animation) animation_t;
	*animation_ptr = animation;

	animation->name = strpool_cstr(cache->strpool, name_id);

	for (int i = 0; i < pngs.count(); ++i) {
		frame_t frame;
		frame.id = pngs[i].id;
		frame.delay = delays[i];
		animation->frames.add(frame);
	}

	return animation;
}

 const animation_t* png_cache_get_animation(png_cache_t* cache, const char* name)
{
	animation_t* animation;
	if (!cache->animations.find(INJECT(name), &animation).is_error()) {
		return animation;
	} else {
		return NULL;
	}
}

const animation_table_t* png_cache_make_animation_table(png_cache_t* cache, const char* sprite_name, const array<const animation_t*>& animations)
{
	strpool_id name_id = INJECT(sprite_name);

	// If already made, just return the old table.
	animation_table_t* table;
	if (!cache->animation_tables.find(name_id, &table).is_error()) {
		return table;
	}

	// Otherwise allocate a new table.
	animation_table_t** table_ptr = cache->animation_tables.insert(name_id);
	table = (animation_table_t*)CUTE_ALLOC(sizeof(animation_table_t), cache->mem_ctx);
	CUTE_PLACEMENT_NEW(table) animation_table_t;
	*table_ptr = table;

	for (int i = 0; i < animations.count(); ++i) {
		table->insert(animations[i]->name, animations[i]);
	}

	return table;
}

const animation_table_t* png_cache_get_animation_table(png_cache_t* cache, const char* sprite_name)
{
	animation_table_t* table;
	if (!cache->animation_tables.find(INJECT(sprite_name), &table).is_error()) {
		return table;
	} else {
		return NULL;
	}
}

sprite_t png_cache_make_sprite(png_cache_t* cache, const char* sprite_name, const animation_table_t* table)
{
	uint64_t name_id = INJECT(sprite_name);
	if (!table) {
		error_t err = cache->animation_tables.find(name_id, (animation_table_t**)&table);
		CUTE_ASSERT(!err.is_error());
	}

	png_t png;
	error_t err = cache->pngs.find(table->items()[0]->frames[0].id, &png);
	CUTE_ASSERT(!err.is_error());

	sprite_t sprite;
	sprite.name = strpool_cstr(cache->strpool, name_id);
	sprite.w = png.w;
	sprite.h = png.h;
	sprite.animations = table;

	return sprite;
}

}
