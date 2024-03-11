/*
	Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

#ifndef CF_PNG_CACHE_INTERNAL_H
#define CF_PNG_CACHE_INTERNAL_H

#include <cute_defines.h>

void cf_make_png_cache();
void cf_destroy_png_cache();
void cf_png_cache_get_pixels(uint64_t image_id, void* buffer, int bytes_to_fill);

#endif // CF_PNG_CACHE_INTERNAL_H
