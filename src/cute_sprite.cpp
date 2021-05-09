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

namespace cute
{

static aseprite_cache_t* s_ase_cache(app_t* app)
{
	if (!app->ase_batch) {
		CUTE_ASSERT(!app->ase_cache);
		app->ase_cache = aseprite_cache_make(app->mem_ctx);
		app->ase_batch = batch_make(aseprite_cache_get_pixels_fn(app->ase_cache), app->ase_cache, app->mem_ctx);
		int w, h;
		app_offscreen_size(app, &w, &h);
		batch_set_projection(app->ase_batch, matrix_ortho_2d((float)w, (float)h, 0, 0));
	}
	return app->ase_cache;
}

static batch_t* s_ase_batch(app_t* app)
{
	return app->ase_batch;
}

static png_cache_t* s_png_cache(app_t* app)
{
	if (!app->png_batch) {
		CUTE_ASSERT(!app->ase_cache);
		app->png_cache = png_cache_make(app->mem_ctx);
		app->png_batch = batch_make(aseprite_cache_get_pixels_fn(app->ase_cache), app->ase_cache, app->mem_ctx);
		int w, h;
		app_offscreen_size(app, &w, &h);
		batch_set_projection(app->png_batch, matrix_ortho_2d((float)w, (float)h, 0, 0));
	}
	return app->png_cache;
}

static batch_t* s_png_batch(app_t* app)
{
	return app->png_batch;
}

sprite_t easy_sprite_make(app_t* app, const char* png_path)
{
	png_cache_t* cache = s_png_cache(app);
	const animation_table_t* table = png_cache_get_animation_table(cache, png_path);
	if (!table) {
		png_t png;
		char buf[1024];
		error_t err = png_cache_load(cache, png_path, &png);
		if (err.is_error()) {
			sprintf(buf, "Unable to load sprite at path \"%s\".\n", png_path);
			window_message_box(app, WINDOW_MESSAGE_BOX_TYPE_ERROR, "ERROR", buf);
			return sprite_t();
		}
		const animation_t* anim = png_cache_make_animation(cache, png_path, { png }, { 1.0f });
		table = png_cache_make_animation_table(cache, png_path, { anim });
	}

	sprite_t s = png_cache_make_sprite(cache, png_path, table);
	return s;
}

void easy_sprite_unload(app_t* app, sprite_t sprite)
{
	png_cache_t* cache = s_png_cache(app);
	png_t png;
	error_t err = cache->pngs.find(sprite.animations->items()[0]->frames[0].id, &png);
	png_cache_unload(cache, &png);
}

batch_t* easy_sprite_get_batch(app_t* app)
{
	s_png_cache(app);
	return app->png_batch;
}

sprite_t sprite_make(app_t* app, const char* aseprite_path)
{
	aseprite_cache_t* cache = s_ase_cache(app);

	sprite_t s;
	cute::error_t err = aseprite_cache_load(cache, aseprite_path, &s);
	char buf[1024];
	if (err.is_error()) {
		sprintf(buf, "Unable to load sprite at path \"%s\".\n", aseprite_path);
		window_message_box(app, WINDOW_MESSAGE_BOX_TYPE_ERROR, "ERROR", buf);
	}
	return s;
}

void sprite_unload(app_t* app, const char* aseprite_path)
{
	aseprite_cache_t* cache = s_ase_cache(app);
	aseprite_cache_unload(cache, aseprite_path);
}

batch_t* sprite_get_batch(app_t* app)
{
	s_ase_cache(app);
	return app->ase_batch;
}

}
