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
#include <cute_batch.h>

#include <internal/cute_app_internal.h>
#include <internal/cute_png_cache_internal.h>
#include <internal/cute_batch_internal.h>

static cf_aseprite_cache_t* s_ase_cache()
{
	if (!app->ase_cache) {
		CUTE_ASSERT(!app->ase_cache);
		app->ase_cache = cf_make_aseprite_cache();
	}
	return app->ase_cache;
}

static cf_png_cache_t* s_png_cache()
{
	if (!app->png_cache) {
		CUTE_ASSERT(!app->png_cache);
		app->png_cache = cf_make_png_cache();
	}
	return app->png_cache;
}

CF_Sprite cf_easy_make_sprite(const char* png_path)
{
	cf_png_cache_t* cache = s_png_cache();
	const CF_Animation** table = cf_png_cache_get_animation_table(cache, png_path);
	if (!table) {
		cf_png_t png;
		char buf[1024];
		CF_Result err = cf_png_cache_load(cache, png_path, &png);
		if (cf_is_error(err)) {
			sprintf(buf, "Unable to load sprite at path \"%s\".\n", png_path);
			cf_window_message_box(CF_WINDOW_MESSAGE_BOX_TYPE_ERROR, "ERROR", buf);
			return cf_sprite_defaults();
		}

		cf_png_t pngs[] = { png };
		float delays[] = { 1.0f };
		const CF_Animation* anim = cf_make_png_cache_animation(cache, png_path, pngs, CUTE_ARRAY_SIZE(pngs), delays, CUTE_ARRAY_SIZE(delays));
		const CF_Animation* anims[] = { anim };
		table = cf_make_png_cache_animation_table(cache, png_path, anims, CUTE_ARRAY_SIZE(anims));
	}

	CF_Sprite s = cf_make_png_cache_sprite(cache, png_path, table);
	return s;
}

void cf_easy_sprite_unload(CF_Sprite sprite)
{
	cf_png_cache_t* cache = s_png_cache();
	cf_png_t png = hget(cache->pngs, sprite.animations[0]->frames[0].id);
	cf_png_cache_unload(cache, png);
}

CF_Sprite cf_make_sprite(const char* aseprite_path)
{
	cf_aseprite_cache_t* cache = s_ase_cache();

	CF_Sprite s = cf_sprite_defaults();
	CF_Result err = cf_aseprite_cache_load(cache, aseprite_path, &s);
	char buf[1024];
	if (cf_is_error(err)) {
		sprintf(buf, "Unable to load sprite at path \"%s\".\n", aseprite_path);
		cf_window_message_box(CF_WINDOW_MESSAGE_BOX_TYPE_ERROR, "ERROR", buf);
	}
	return s;
}

void cf_sprite_unload(const char* aseprite_path)
{
	cf_aseprite_cache_t* cache = s_ase_cache();
	cf_aseprite_cache_unload(cache, aseprite_path);
}
