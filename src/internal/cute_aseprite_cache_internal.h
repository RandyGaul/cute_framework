/*
	Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
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
