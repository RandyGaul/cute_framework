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

#include <cute_c_runtime.h>
#include <cute_gfx.h>

#define SOKOL_IMPL
#define SOKOL_D3D11
#define D3D11_NO_HELPERS
#include <sokol/sokol_gfx.h>

#include <cute/cute_png.h>

namespace cute
{

texture_t texture_make(pixel_t* pixels, int w, int h, sg_wrap mode)
{
	sg_image_desc params = { 0 };
	params.width = w;
	params.height = h;
	params.wrap_u = mode;
	params.wrap_v = mode;
	params.content.subimage[0][0].ptr = pixels;
	params.content.subimage[0][0].size = w * h * sizeof(pixel_t);
	params.num_mipmaps = 0;
	sg_image img = sg_make_image(params);
	return (texture_t)img.id;
}

void texture_destroy(texture_t texture)
{
	sg_image img;
	img.id = (uint32_t)texture;
	sg_destroy_image(img);
}

matrix_t matrix_identity()
{
	matrix_t m;
	CUTE_MEMSET(&m, 0, sizeof(m));
	m.data[0] = 1.0f;
	m.data[5] = 1.0f;
	m.data[10] = 1.0f;
	m.data[15] = 1.0f;
	return m;
}

matrix_t matrix_ortho_2d(float w, float h, float x, float y)
{
	float L = -w / 2.0f;
	float R = w / 2.0f;
	float T = h / 2.0f;
	float B = -h / 2.0f;

	matrix_t projection;
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

triple_buffer_t triple_buffer_make(int vertex_data_size, int vertex_stride, int index_count)
{
	triple_buffer_t buf;
	buf.vbuf.stride = vertex_stride;
	buf.ibuf.stride = (vertex_data_size / vertex_stride) < UINT16_MAX ? sizeof(uint16_t) : sizeof(uint32_t);

	sg_buffer_desc vparams = { 0 };
	vparams.type = SG_BUFFERTYPE_VERTEXBUFFER;
	vparams.usage = SG_USAGE_STREAM;
	vparams.size = vertex_data_size;

	sg_buffer_desc iparams = { 0 };
	iparams.type = SG_BUFFERTYPE_INDEXBUFFER;
	iparams.usage = SG_USAGE_STREAM;
	iparams.size = index_count * buf.ibuf.stride;

	sg_buffer invalid = { SG_INVALID_ID };
	for (int i = 0; i < 1; ++i) {
		buf.vbuf.buffer[i] = sg_make_buffer(vparams);
		buf.ibuf.buffer[i] = index_count ? sg_make_buffer(iparams) : invalid;
	}

	return buf;
}

void triple_buffer_append(triple_buffer_t* buffer, int vertex_count, const void* vertices, int index_count, const void* indices)
{
	int voffset = sg_append_buffer(buffer->get_vbuf(), vertices, vertex_count * buffer->vbuf.stride);
	buffer->vbuf.offset = voffset;
	bool overflowed = sg_query_buffer_overflow(buffer->get_vbuf());
	CUTE_ASSERT(!overflowed);
	//printf("%s\n", overflowed ? "overflow" : "no overflow");
	//CUTE_ASSERT(voffset == buffer->vbuf.element_count * buffer->vbuf.stride);

	if (index_count) {
		(&buffer->ibuf, index_count);
		int ioffset = sg_append_buffer(buffer->get_ibuf(), indices, index_count * buffer->ibuf.stride);
		buffer->ibuf.offset = ioffset;
		//CUTE_ASSERT(ioffset == buffer->ibuf.element_count * buffer->ibuf.stride);
	}
}

}
