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

#include <sokol/sokol_gfx.h>

#include <cute/cute_png.h>

#ifdef CUTE_WINDOWS
#	pragma comment (lib, "dxgi.lib")
#	pragma comment (lib, "d3d11.lib")
#	pragma comment (lib, "dxguid.lib")
#endif

namespace cute
{

texture_t texture_make(pixel_t* pixels, int w, int h, sg_wrap mode, sg_filter filter)
{
	sg_image_desc params = { 0 };
	params.width = w;
	params.height = h;
	params.wrap_u = mode;
	params.wrap_v = mode;
	params.min_filter = filter;
	params.mag_filter = filter;
	params.data.subimage[0][0].ptr = pixels;
	params.data.subimage[0][0].size = w * h * sizeof(pixel_t);
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

triple_buffer_t triple_buffer_make(int vertex_data_size, int vertex_stride, int index_count, int index_stride)
{
	triple_buffer_t buf;
	buf.vbuf.stride = vertex_stride;
	buf.ibuf.stride = index_stride;

	sg_buffer_desc vparams = { 0 };
	vparams.type = SG_BUFFERTYPE_VERTEXBUFFER;
	vparams.usage = SG_USAGE_STREAM;
	vparams.size = vertex_data_size;

	sg_buffer_desc iparams = { 0 };
	iparams.type = SG_BUFFERTYPE_INDEXBUFFER;
	iparams.usage = SG_USAGE_STREAM;
	iparams.size = index_count * buf.ibuf.stride;

	sg_buffer invalid = { SG_INVALID_ID };
	for (int i = 0; i < 3; ++i) {
		buf.vbuf.buffer[i] = sg_make_buffer(vparams);
		buf.ibuf.buffer[i] = index_count ? sg_make_buffer(iparams) : invalid;
	}

	return buf;
}

error_t triple_buffer_append(triple_buffer_t* buffer, int vertex_count, const void* vertices, int index_count, const void* indices)
{
	bool overflowed = false;

	sg_range range;
	range.ptr = vertices;
	range.size = vertex_count * buffer->vbuf.stride;
	int voffset = sg_append_buffer(buffer->vbuf.buffer[buffer->vbuf.buffer_number], range);
	buffer->vbuf.offset = voffset;
	overflowed |= sg_query_buffer_overflow(buffer->vbuf.buffer[buffer->vbuf.buffer_number]);

	if (index_count) {
		range.ptr = indices;
		range.size = index_count * buffer->ibuf.stride;
		int ioffset = sg_append_buffer(buffer->ibuf.buffer[buffer->ibuf.buffer_number], range);
		buffer->ibuf.offset = ioffset;
		overflowed |= sg_query_buffer_overflow(buffer->ibuf.buffer[buffer->ibuf.buffer_number]);
	}

	if (overflowed) return error_failure("Overflowed one of the internal buffers -- sokol_gfx will silently drop he associated draw calls.");
	return error_success();
}

}
