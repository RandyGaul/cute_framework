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

#ifndef CUTE_SPRITE_H
#define CUTE_SPRITE_H

#include <cute_defines.h>
#include <cute_math.h>
#include <cute_error.h>

#include <cute/cute_png.h>

namespace cute
{

struct sprite_t
{
	uint64_t id;

	transform_t transform; // Position and location rotation of the sprite.
	float scale_x; // Scaling along the sprite's local x-axis.
	float scale_y; // Scaling along the sprite's local y-axis.
};

struct sprite_batch_t;

error_t sprite_batcher_push(sprite_batch_t* sb, sprite_t sprite_t);

}

#endif // CUTE_SPRITE_H
