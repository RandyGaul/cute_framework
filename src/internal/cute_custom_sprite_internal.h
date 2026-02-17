/*
	Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

#ifndef CF_CUSTOM_SPRITE_INTERNAL_H
#define CF_CUSTOM_SPRITE_INTERNAL_H

#include <cute_defines.h>

CF_API void cf_make_custom_sprite_cache();
CF_API void cf_destroy_custom_sprite_cache();
void cf_custom_sprite_get_pixels(uint64_t image_id, void* buffer, int bytes_to_fill);

#endif // CF_CUSTOM_SPRITE_INTERNAL_H
