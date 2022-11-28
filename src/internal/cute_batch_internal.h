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

#ifndef CUTE_BATCH_INTERNAL_H
#define CUTE_BATCH_INTERNAL_H

extern struct CF_Batch* b;

void cf_make_batch();
void cf_destroy_batch();
void cf_batch_render();

// We slice up a 64-bit int into lo + hi ranges to map where we can fetch pixels
// from. This slices up the 64-bit range into 16 unique ranges, though we're only
// using three of those ranges for now. The ranges are inclusive.
#define CUTE_IMAGE_ID_RANGE_SIZE  ((1ULL << 60) - 1)
#define CUTE_ASEPRITE_ID_RANGE_LO (0ULL)
#define CUTE_ASEPRITE_ID_RANGE_HI (CUTE_ASEPRITE_ID_RANGE_LO + CUTE_IMAGE_ID_RANGE_SIZE)
#define CUTE_PNG_ID_RANGE_LO      (CUTE_ASEPRITE_ID_RANGE_HI + 1)
#define CUTE_PNG_ID_RANGE_HI      (CUTE_PNG_ID_RANGE_LO      + CUTE_IMAGE_ID_RANGE_SIZE)
#define CUTE_FONT_ID_RANGE_LO     (CUTE_PNG_ID_RANGE_HI      + 1)
#define CUTE_FONT_ID_RANGE_HI     (CUTE_FONT_ID_RANGE_LO     + CUTE_IMAGE_ID_RANGE_SIZE)

void cf_get_pixels(uint64_t image_id, void* buffer, int bytes_to_fill, void* udata);

#endif // CUTE_BATCH_INTERNAL_H
