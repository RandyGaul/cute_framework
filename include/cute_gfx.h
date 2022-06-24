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

#ifndef CUTE_GFX_H
#define CUTE_GFX_H

#include "cute_defines.h"
#include "cute_error.h"
#include "cute_app.h"
#include "cute_color.h"
#include "cute_c_runtime.h"

#include "sokol/sokol_gfx.h"

namespace cute
{

using texture_t = uint64_t;

CUTE_API texture_t CUTE_CALL texture_make(pixel_t* pixels, int w, int h, sg_wrap mode = SG_WRAP_REPEAT, sg_filter filter = SG_FILTER_NEAREST);
CUTE_API void CUTE_CALL texture_destroy(texture_t texture);

struct matrix_t
{
	float data[16];
};

CUTE_API matrix_t CUTE_CALL matrix_identity();
CUTE_API matrix_t CUTE_CALL matrix_ortho_2d(float w, float h, float x, float y);

struct buffer_t
{
	struct buf_t
	{
		int stride = 0;
		int offset = 0;
		sg_buffer buffer;
	};

	buf_t vbuf;
	buf_t ibuf;

	CUTE_INLINE sg_bindings bind()
	{
		sg_bindings bind = { 0 };
		bind.vertex_buffers[0] = vbuf.buffer;
		bind.vertex_buffer_offsets[0] = vbuf.offset;
		bind.index_buffer = ibuf.buffer;
		bind.index_buffer_offset = ibuf.offset;
		return bind;
	}

	CUTE_INLINE error_t init(size_t vertex_data_size, size_t vertex_stride, int index_count = 0, int index_stride = 0)
	{
		vbuf.stride = (int)vertex_stride;
		ibuf.stride = (int)index_stride;

		sg_buffer_desc vparams = { 0 };
		vparams.type = SG_BUFFERTYPE_VERTEXBUFFER;
		vparams.usage = SG_USAGE_STREAM;
		vparams.size = (int)vertex_data_size;

		sg_buffer_desc iparams = { 0 };
		iparams.type = SG_BUFFERTYPE_INDEXBUFFER;
		iparams.usage = SG_USAGE_STREAM;
		iparams.size = index_count * ibuf.stride;

		sg_buffer invalid = { SG_INVALID_ID };
		vbuf.buffer = sg_make_buffer(vparams);
		ibuf.buffer = index_count ? sg_make_buffer(iparams) : invalid;
		if (vbuf.buffer.id == SG_INVALID_ID) {
			return error_failure("Failed to create buffer.");
		}

		return error_success();
	}

	CUTE_INLINE void release()
	{
		if (vbuf.buffer.id != SG_INVALID_ID) {
			sg_destroy_buffer(vbuf.buffer);
			vbuf.buffer.id = SG_INVALID_ID;
		}
		if (ibuf.buffer.id != SG_INVALID_ID) {
			sg_destroy_buffer(ibuf.buffer);
			ibuf.buffer.id = SG_INVALID_ID;
		}
	}

	CUTE_INLINE error_t append(int vertex_count, const void* vertices, int index_count = 0, const void* indices = NULL)
	{
		CUTE_ASSERT(vertex_count);
		CUTE_ASSERT(vertices);
		sg_range vertex_range = { vertices, (size_t)vertex_count * (size_t)vbuf.stride };
		sg_range index_range = { indices, (size_t)index_count * (size_t)ibuf.stride };
		if (sg_query_will_buffer_overflow(vbuf.buffer, vertex_range.size)) {
			return error_failure("Overflowed vertex buffer.");
		} else if (sg_query_will_buffer_overflow(ibuf.buffer, index_range.size)) {
			return error_failure("Overflowed index buffer.");
		} else {
			int voffset = sg_append_buffer(vbuf.buffer, vertex_range);
			vbuf.offset = voffset;
			if (index_count) {
				int ioffset = sg_append_buffer(ibuf.buffer, index_range);
				ibuf.offset = ioffset;
			}
			return error_success();
		}
	}
};

}

#endif // CUTE_GFX_H
