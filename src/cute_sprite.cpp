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
#include <cute_aseprite_cache.h>
#include <cute_window.h>

#include <internal/cute_app_internal.h>
#include <internal/cute_png_cache_internal.h>

static cf_aseprite_cache_t* cf_s_ase_cache()
{
	if (!cf_app->ase_batch) {
		CUTE_ASSERT(!cf_app->ase_cache);
		cf_app->ase_cache = cf_make_aseprite_cache();
		cf_app->ase_batch = cf_make_batch(cf_aseprite_cache_get_pixels_fn(cf_app->ase_cache), cf_app->ase_cache);
		cf_batch_set_projection(cf_app->ase_batch, cf_matrix_ortho_2d((float)cf_app->offscreen_w, (float)cf_app->offscreen_h, 0, 0));
	}
	return cf_app->ase_cache;
}

static CF_Batch* cf_s_ase_batch()
{
	return cf_app->ase_batch;
}

static cf_png_cache_t* cf_s_png_cache()
{
	if (!cf_app->png_batch) {
		CUTE_ASSERT(!cf_app->png_cache);
		cf_app->png_cache = cf_make_png_cache();
		cf_app->png_batch = cf_make_batch(cf_png_cache_get_pixels_fn(cf_app->png_cache), cf_app->png_cache);
		cf_batch_set_projection(cf_app->png_batch, cf_matrix_ortho_2d((float)cf_app->offscreen_w, (float)cf_app->offscreen_h, 0, 0));
	}
	return cf_app->png_cache;
}

static CF_Batch* cf_s_png_batch()
{
	return cf_app->png_batch;
}

cf_sprite_t cf_easy_make_sprite(const char* png_path)
{
	cf_png_cache_t* cache = cf_s_png_cache();
	const cf_animation_t** table = cf_png_cache_get_animation_table(cache, png_path);
	if (!table) {
		cf_png_t png;
		char buf[1024];
		cf_result_t err = cf_png_cache_load(cache, png_path, &png);
		if (cf_is_error(err)) {
			sprintf(buf, "Unable to load sprite at path \"%s\".\n", png_path);
			cf_window_message_box(CF_WINDOW_MESSAGE_BOX_TYPE_ERROR, "ERROR", buf);
			return cf_sprite_defaults();
		}

		cf_png_t pngs[] = { png };
		float delays[] = { 1.0f };
		const cf_animation_t* anim = cf_make_png_cache_animation(cache, png_path, pngs, CUTE_ARRAY_SIZE(pngs), delays, CUTE_ARRAY_SIZE(delays));
		const cf_animation_t* anims[] = { anim };
		table = cf_make_png_cache_animation_table(cache, png_path, anims, CUTE_ARRAY_SIZE(anims));
	}

	cf_sprite_t s = cf_make_png_cache_sprite(cache, png_path, table);
	return s;
}

void cf_easy_sprite_unload(cf_sprite_t sprite)
{
	cf_png_cache_t* cache = cf_s_png_cache();
	cf_png_t png = hget(cache->pngs, sprite.animations[0]->frames[0].id);
	cf_png_cache_unload(cache, png);
}

CF_Batch* cf_easy_sprite_get_batch()
{
	cf_s_png_cache();
	return cf_app->png_batch;
}

cf_sprite_t cf_make_sprite(const char* aseprite_path)
{
	cf_aseprite_cache_t* cache = cf_s_ase_cache();

	cf_sprite_t s = cf_sprite_defaults();
	cf_result_t err = cf_aseprite_cache_load(cache, aseprite_path, &s);
	char buf[1024];
	if (cf_is_error(err)) {
		sprintf(buf, "Unable to load sprite at path \"%s\".\n", aseprite_path);
		cf_window_message_box(CF_WINDOW_MESSAGE_BOX_TYPE_ERROR, "ERROR", buf);
	}
	return s;
}

void cf_sprite_unload(const char* aseprite_path)
{
	cf_aseprite_cache_t* cache = cf_s_ase_cache();
	cf_aseprite_cache_unload(cache, aseprite_path);
}

CF_Batch* cf_sprite_get_batch()
{
	cf_s_ase_cache();
	return cf_app->ase_batch;
}
