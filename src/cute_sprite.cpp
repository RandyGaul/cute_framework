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

#include "cute_sprite.h"
#include "cute_draw.h"
#include "cute_image.h"

#include <internal/cute_app_internal.h>
#include <internal/cute_aseprite_cache_internal.h>
#include <internal/cute_alloc_internal.h>
#include <internal/cute_girl.h>

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
	CF_Image img;
	CF_Result result = cf_image_load_png(png_path, &img);
	if (cf_is_error(result)) {
		if (result_out) *result_out = result;
		return cf_sprite_defaults();
	}
	cf_image_premultiply(&img);
	return s_insert(img);
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
	char buf[1024];
	if (cf_is_error(err)) {
		sprintf(buf, "Unable to load sprite at path \"%s\".\n", aseprite_path);
		cf_message_box(CF_MESSAGE_BOX_TYPE_ERROR, "ERROR", buf);
	}
	return s;
}

CF_Sprite cf_make_sprite_from_memory(const char* unique_name, const void* aseprite_data, int size)
{
	CF_Sprite s = cf_sprite_defaults();
	CF_Result err = cf_aseprite_cache_load_from_memory(unique_name, aseprite_data, size, &s);
	char buf[1024];
	if (cf_is_error(err)) {
		sprintf(buf, "Unable to load sprite from memory with name \"%s\".\n", unique_name);
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

void cf_easy_sprite_unload(CF_Sprite *sprite)
{
	CF_Image* img = app->easy_sprites.try_find(sprite->easy_sprite_id);
	if (img) {
		app->easy_sprites.remove(sprite->easy_sprite_id);
		cf_image_free(img);
	}
}