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

#include <cute_camera.h>

#include <internal/cute_app_internal.h>

namespace cute
{

void camera_set_wh(camera_t* cam, int w, int h)
{
	cam->w = (float)w;
	cam->h = (float)h;
}

void camera_set_pos(camera_t* cam, v2 pos)
{
	cam->tx.p = pos;
}

void camera_push_pos(camera_t* cam, v2 pos)
{
	cam->pos_stack.add(cam->tx.p);
	cam->tx.p = pos;
	camera_calc_mvp(cam);
}

void camera_pop_pos(camera_t* cam)
{
	cam->tx.p = cam->pos_stack.pop();
	camera_calc_mvp(cam);
}

void camera_calc_mvp(camera_t* cam)
{
	cam->mvp = matrix_ortho_2d(cam->w, cam->h, cam->tx.p.x, cam->tx.p.y);
}

matrix_t* camera_get_mvp(camera_t* cam)
{
	return &cam->mvp;
}

aabb_t camera_get_cull_aabb(camera_t* cam)
{
	return make_aabb(cam->tx.p, cam->w, cam->h);
}

v2 mouse_pos_in_world_space(app_t* app, camera_t* cam)
{
	float w = (float)app->w;
	float h = (float)app->h;
	float rw = (float)app->offscreen_w;
	float rh = (float)app->offscreen_h;
	float ratio_x = w / rw;
	float ratio_y = h / rh;
	float x = (app->mouse.x - app->w / 2.0f) / ratio_x;
	float y = (-(app->mouse.y - app->h / 2.0f)) / ratio_y;
	x = clamp(x, -rw, rw);
	y = clamp(y, -rh, rh);
	v2 p = v2(x, y) + cam->tx.p;
	return p;
}

}
