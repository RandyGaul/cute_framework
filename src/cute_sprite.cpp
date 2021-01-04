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

namespace cute
{

static aseprite_cache_t* s_cache(app_t* app)
{
	if (!app->batch) {
		CUTE_ASSERT(!app->ase_cache);
		app->ase_cache = aseprite_cache_make(app->mem_ctx);
		app->batch = batch_make(aseprite_cache_get_pixels_fn(app->ase_cache), app, app->mem_ctx);
	}
	return app->ase_cache;
}

static batch_t* s_batch(app_t* app)
{
	return app->batch;
}

sprite_t sprite_make(app_t* app, const char* aseprite_path)
{
	aseprite_cache_t* cache = s_cache(app);

	sprite_t s;
	cute::error_t err = aseprite_cache_load(cache, aseprite_path, &s);
	char buf[1024];
	if (err.is_error()) {
		sprintf(buf, "Unable to load sprite at path \"%s\".\n", aseprite_path);
		window_message_box(app, WINDOW_MESSAGE_BOX_TYPE_ERROR, "ERROR", buf);
	}
	s.batch = app->batch;
	return s;
}

void sprite_unload(app_t* app, const char* aseprite_path)
{
	aseprite_cache_t* cache = s_cache(app);
	aseprite_cache_unload(cache, aseprite_path);
}

void flush_sprites(app_t* app)
{
	batch_flush(s_batch(app));
}

}
