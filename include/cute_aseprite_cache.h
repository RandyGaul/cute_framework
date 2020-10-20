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

#ifndef CUTE_ASEPRITE_CACHE_H
#define CUTE_ASEPRITE_CACHE_H

#include <cute_defines.h>
#include <cute_error.h>
#include <cute_batch.h>

#include <cute/cute_aseprite.h>

struct strpool_t;

namespace cute
{

struct sprite_t;
struct aseprite_cache_t;

CUTE_API aseprite_cache_t* CUTE_CALL aseprite_cache_make(void* mem_ctx = NULL);
CUTE_API void CUTE_CALL aseprite_cache_destroy(aseprite_cache_t* cache);

CUTE_API error_t CUTE_CALL aseprite_cache_load(aseprite_cache_t* cache, const char* aseprite_path, sprite_t* sprite);
CUTE_API void CUTE_CALL aseprite_cache_unload(aseprite_cache_t* cache, const char* aseprite_path);
CUTE_API error_t CUTE_CALL aseprite_cache_load_ase(aseprite_cache_t* cache, const char* aseprite_path, ase_t** ase);

CUTE_API get_pixels_fn* CUTE_CALL aseprite_cache_get_pixels_fn(aseprite_cache_t* cache);
CUTE_API strpool_t* CUTE_CALL aseprite_cache_get_strpool_ptr(aseprite_cache_t* cache);

}

#endif // CUTE_ASEPRITE_CACHE_H
