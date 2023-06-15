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

#ifndef CF_ASEPRITE_CACHE_INTERNAL_H
#define CF_ASEPRITE_CACHE_INTERNAL_H

#include <cute_defines.h>

#include <cute/cute_aseprite.h>

CF_Result cf_aseprite_cache_load(const char* aseprite_path, CF_Sprite* sprite_out);
CF_Result cf_aseprite_cache_load_from_memory(const char* unique_name, const void* data, int sz, CF_Sprite* sprite_out);
void cf_aseprite_cache_unload(const char* aseprite_path);
CF_Result cf_aseprite_cache_load_ase(const char* aseprite_path, ase_t** ase);

void cf_make_aseprite_cache();
void cf_destroy_aseprite_cache();
void cf_aseprite_cache_get_pixels(uint64_t image_id, void* buffer, int bytes_to_fill);


#endif // CF_ASEPRITE_CACHE_INTERNAL_H
