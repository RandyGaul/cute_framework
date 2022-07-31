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

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

typedef uint64_t cf_texture_t;

CUTE_API cf_texture_t CUTE_CALL cf_texture_make(cf_pixel_t* pixels, int w, int h);
CUTE_API cf_texture_t CUTE_CALL cf_texture_make2(cf_pixel_t* pixels, int w, int h, sg_wrap mode /* = SG_WRAP_REPEAT */, sg_filter filter /* = SG_FILTER_NEAREST */);
CUTE_API void CUTE_CALL cf_texture_destroy(cf_texture_t texture);

typedef struct cf_matrix_t
{
	float data[16];
} cf_matrix_t;

CUTE_API cf_matrix_t CUTE_CALL cf_matrix_identity();
CUTE_API cf_matrix_t CUTE_CALL cf_matrix_ortho_2d(float w, float h, float x, float y);

struct cf_buf_inner_t
{
	int stride = 0;
	int offset = 0;
	sg_buffer buffer;
};

struct cf_buffer_t
{
	cf_buf_inner_t vbuf;
	cf_buf_inner_t ibuf;

	#ifdef  CUTE_CPP
	CUTE_INLINE sg_bindings bind();
	CUTE_INLINE cf_error_t init(size_t vertex_data_size, size_t vertex_stride, int index_count = 0, int index_stride = 0);
	CUTE_INLINE void release();
	CUTE_INLINE cf_error_t append(int vertex_count, const void* vertices, int index_count = 0, const void* indices = NULL);
	#endif //  CUTE_CPP
};

CUTE_INLINE sg_bindings cf_buffer_bind(cf_buffer_t* b)
{
	sg_bindings bind = { 0 };
	bind.vertex_buffers[0] = b->vbuf.buffer;
	bind.vertex_buffer_offsets[0] = b->vbuf.offset;
	bind.index_buffer = b->ibuf.buffer;
	bind.index_buffer_offset = b->ibuf.offset;
	return bind;
}

CUTE_INLINE cf_error_t cf_buffer_init(cf_buffer_t* b, size_t vertex_data_size, size_t vertex_stride, int index_count /*= 0*/, int index_stride /*= 0*/)
{
	b->vbuf.stride = (int)vertex_stride;
	b->ibuf.stride = (int)index_stride;

	sg_buffer_desc vparams = { 0 };
	vparams.type = SG_BUFFERTYPE_VERTEXBUFFER;
	vparams.usage = SG_USAGE_STREAM;
	vparams.size = (int)vertex_data_size;

	sg_buffer_desc iparams = { 0 };
	iparams.type = SG_BUFFERTYPE_INDEXBUFFER;
	iparams.usage = SG_USAGE_STREAM;
	iparams.size = index_count * b->ibuf.stride;

	sg_buffer invalid = { SG_INVALID_ID };
	b->vbuf.buffer = sg_make_buffer(vparams);
	b->ibuf.buffer = index_count ? sg_make_buffer(iparams) : invalid;
	if (b->vbuf.buffer.id == SG_INVALID_ID) {
		return cf_error_failure("Failed to create buffer.");
	}

	return cf_error_success();
}

CUTE_INLINE void cf_buffer_release(cf_buffer_t *b)
{
	if (b->vbuf.buffer.id != SG_INVALID_ID) {
		sg_destroy_buffer(b->vbuf.buffer);
		b->vbuf.buffer.id = SG_INVALID_ID;
	}
	if (b->ibuf.buffer.id != SG_INVALID_ID) {
		sg_destroy_buffer(b->ibuf.buffer);
		b->ibuf.buffer.id = SG_INVALID_ID;
	}
}

CUTE_INLINE cf_error_t cf_buffer_append(cf_buffer_t* b, int vertex_count, const void* vertices, int index_count /*= 0*/, const void* indices /*= NULL*/)
{
	CUTE_ASSERT(vertex_count);
	CUTE_ASSERT(vertices);
	sg_range vertex_range = { vertices, (size_t)vertex_count * (size_t)b->vbuf.stride };
	sg_range index_range = { indices, (size_t)index_count * (size_t)b->ibuf.stride };
	if (sg_query_will_buffer_overflow(b->vbuf.buffer, vertex_range.size)) {
		return cf_error_failure("Overflowed vertex buffer.");
	} else if (sg_query_will_buffer_overflow(b->ibuf.buffer, index_range.size)) {
		return cf_error_failure("Overflowed index buffer.");
	} else {
		int voffset = sg_append_buffer(b->vbuf.buffer, vertex_range);
		b->vbuf.offset = voffset;
		if (index_count) {
			int ioffset = sg_append_buffer(b->ibuf.buffer, index_range);
			b->ibuf.offset = ioffset;
		}
		return cf_error_success();
	}
}

#ifdef __cplusplus
}
#endif // __cplusplus

#ifdef CUTE_CPP

CUTE_INLINE sg_bindings cf_buffer_t::bind() { return cf_buffer_bind(this); }
CUTE_INLINE cf_error_t cf_buffer_t::init(size_t vertex_data_size, size_t vertex_stride, int index_count, int index_stride) { return cf_buffer_init(this, vertex_data_size, vertex_stride, index_count, index_stride); }
CUTE_INLINE void cf_buffer_t::release() { cf_buffer_release(this); }
CUTE_INLINE cf_error_t cf_buffer_t::append(int vertex_count, const void* vertices, int index_count, const void* indices) { return cf_buffer_append(this, vertex_count, vertices, index_count, indices); }


namespace cute
{
using texture_t = uint64_t;


CUTE_INLINE texture_t  texture_make(pixel_t* pixels, int w, int h, sg_wrap mode = SG_WRAP_REPEAT, sg_filter filter = SG_FILTER_NEAREST) { return cf_texture_make2(pixels, w, h, mode, filter); }
CUTE_INLINE void  texture_destroy(texture_t texture) { cf_texture_destroy(texture); }

using matrix_t = cf_matrix_t;

CUTE_INLINE matrix_t  matrix_identity() { return cf_matrix_identity(); }
CUTE_INLINE matrix_t  matrix_ortho_2d(float w, float h, float x, float y) { return cf_matrix_ortho_2d(w, h, x, y); }

using buffer_t = cf_buffer_t;

CUTE_INLINE sg_bindings buffer_bind(buffer_t* buffer) { return cf_buffer_bind(buffer); }
CUTE_INLINE cf_error_t buffer_init(buffer_t* buffer, size_t vertex_data_size, size_t vertex_stride, int index_count = 0, int index_stride = 0) { return cf_buffer_init(buffer, vertex_data_size, vertex_stride, index_count, index_stride); }
CUTE_INLINE void buffer_release(buffer_t* buffer) { cf_buffer_release(buffer); }
CUTE_INLINE cf_error_t buffer_append(buffer_t* buffer, int vertex_count, const void* vertices, int index_count = 0, const void* indices = NULL) { return cf_buffer_append(buffer, vertex_count, vertices, index_count, indices); }

}

#endif // CUTE_CPP


#endif // CUTE_GFX_H
