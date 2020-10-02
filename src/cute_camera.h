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

#ifndef CUTE_CAMERA_H
#define CUTE_CAMERA_H

#include <cute_gfx.h>
#include <cute_math.h>
#include <cute_array.h>

namespace cute
{

struct camera_t
{
	transform_t tx = make_transform();
	float w = 0;
	float h = 0;
	matrix_t mvp;
	array<v2> pos_stack;
};

CUTE_API void CUTE_CALL camera_set_wh(camera_t* cam, int w, int h);
CUTE_API void CUTE_CALL camera_set_pos(camera_t* cam, v2 pos);
CUTE_API void CUTE_CALL camera_push_pos(camera_t* cam, v2 pos);
CUTE_API void CUTE_CALL camera_pop_pos(camera_t* cam);
CUTE_API void CUTE_CALL camera_calc_mvp(camera_t* cam);
CUTE_API matrix_t* CUTE_CALL camera_get_mvp(camera_t* cam);
CUTE_API aabb_t CUTE_CALL camera_get_cull_aabb(camera_t* cam);

}

#endif // CUTE_CAMERA_H
