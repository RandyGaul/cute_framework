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

#include <cute_defines.h>
#include <cute_c_runtime.h>
#include <cute_gfx.h>

#include <sokol/sokol_gfx.h>

#include <cute/cute_png.h>

#ifdef CUTE_WINDOWS
#	pragma comment (lib, "dxgi.lib")
#	pragma comment (lib, "d3d11.lib")
#	pragma comment (lib, "dxguid.lib")
#endif

cf_texture_t cf_make_texture(cf_pixel_t* pixels, int w, int h)
{
	return cf_make_texture2(pixels, w, h, SG_WRAP_REPEAT, SG_FILTER_NEAREST);
}

cf_texture_t cf_make_texture2(cf_pixel_t* pixels, int w, int h, sg_wrap mode, sg_filter filter)
{
	sg_image_desc params = { 0 };
	params.width = w;
	params.height = h;
	params.wrap_u = mode;
	params.wrap_v = mode;
	params.min_filter = filter;
	params.mag_filter = filter;
	params.data.subimage[0][0].ptr = pixels;
	params.data.subimage[0][0].size = w * h * sizeof(cf_pixel_t);
	params.num_mipmaps = 0;
	sg_image img = sg_make_image(params);
	return (cf_texture_t)img.id;
}

void cf_destroy_texture(cf_texture_t texture)
{
	sg_image img;
	img.id = (uint32_t)texture;
	sg_destroy_image(img);
}

cf_matrix_t cf_matrix_identity()
{
	cf_matrix_t m;
	CUTE_MEMSET(&m, 0, sizeof(m));
	m.data[0] = 1.0f;
	m.data[5] = 1.0f;
	m.data[10] = 1.0f;
	m.data[15] = 1.0f;
	return m;
}

cf_matrix_t cf_matrix_ortho_2d(float w, float h, float x, float y)
{
	float L = -w / 2.0f;
	float R = w / 2.0f;
	float T = h / 2.0f;
	float B = -h / 2.0f;

	cf_matrix_t projection;
	CUTE_MEMSET(&projection, 0, sizeof(projection));

	// ortho
	projection.data[0] = 2.0f / (R - L);
	projection.data[5] = 2.0f / (T - B);
	projection.data[10] = -0.5f;
	projection.data[15] = 1.0f;

	// translate
	projection.data[12] = -x * projection.data[0];
	projection.data[13] = -y * projection.data[5];

	return projection;
}
