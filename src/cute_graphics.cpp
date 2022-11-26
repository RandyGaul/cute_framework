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
#include <cute_graphics.h>

#include <internal/cute_app_internal.h>

// For now just a sokol_gfx.h backend is implemented. However, there are plans to also
// implement an SDL_Gpu backend here whenever it's released. This should allow for a
// much simpler shader solution.

#include <sokol/sokol_gfx.h>

#include <float.h>

using namespace cute;

static CUTE_INLINE sg_buffer_type s_wrap(CF_BufferType type)
{
	switch (type) {
	case CF_BUFFER_TYPE_VERTEX: return SG_BUFFERTYPE_VERTEXBUFFER;
	case CF_BUFFER_TYPE_INDEX:  return SG_BUFFERTYPE_INDEXBUFFER;
	}
}

static CUTE_INLINE sg_usage s_wrap(CF_UsageType type)
{
	switch (type) {
	case CF_USAGE_TYPE_IMMUTABLE: return SG_USAGE_IMMUTABLE;
	case CF_USAGE_TYPE_DYNAMIC:   return SG_USAGE_DYNAMIC;
	case CF_USAGE_TYPE_STREAM:    return SG_USAGE_STREAM;
	}
}

static CUTE_INLINE sg_image_type s_wrap(CF_TextureType type)
{
	switch (type) {
	case CF_TEXTURE_TYPE_DEFAULT:  return SG_IMAGETYPE_2D;
	case CF_TEXTURE_TYPE_2D:       return SG_IMAGETYPE_2D;
	case CF_TEXTURE_TYPE_2D_ARRAY: return SG_IMAGETYPE_ARRAY;
	case CF_TEXTURE_TYPE_CUBE:     return SG_IMAGETYPE_CUBE;
	}
}

static CUTE_INLINE sg_pixel_format s_wrap(CF_PixelFormat fmt)
{
	CUTE_STATIC_ASSERT(CF_PIXELFORMAT_COUNT == _SG_PIXELFORMAT_NUM - 1, "Must be equal.");
	switch (fmt) {
	case CF_PIXELFORMAT_DEFAULT: return _SG_PIXELFORMAT_DEFAULT;
	default:                     return (sg_pixel_format)(fmt - 1);
	}
}

static CUTE_INLINE CF_PixelFormat s_wrap(sg_pixel_format fmt)
{
	CUTE_STATIC_ASSERT(CF_PIXELFORMAT_COUNT == _SG_PIXELFORMAT_NUM - 1, "Must be equal.");
	switch (fmt) {
	case _SG_PIXELFORMAT_DEFAULT: return CF_PIXELFORMAT_DEFAULT;
	default:                     return (CF_PixelFormat)(fmt + 1);
	}
}

static CUTE_INLINE sg_filter s_wrap(CF_MinMagFilter minmag_filter, CF_MipFilter mip_filter)
{
	switch (minmag_filter) {
	case CF_MINMAG_FILTER_NEAREST:
		switch (mip_filter) {
		case CF_MIP_FILTER_NONE:    return SG_FILTER_NEAREST;
		case CF_MIP_FILTER_NEAREST: return SG_FILTER_NEAREST_MIPMAP_NEAREST;
		case CF_MIP_FILTER_LINEAR:  return SG_FILTER_NEAREST_MIPMAP_LINEAR;
		}
	case CF_MINMAG_FILTER_LINEAR:
		switch (mip_filter) {
		case CF_MIP_FILTER_NONE:    return SG_FILTER_LINEAR;
		case CF_MIP_FILTER_NEAREST: return SG_FILTER_LINEAR_MIPMAP_NEAREST;
		case CF_MIP_FILTER_LINEAR:  return SG_FILTER_LINEAR_MIPMAP_LINEAR;
		}
	}
}

static CUTE_INLINE sg_wrap s_wrap(CF_WrapMode mode)
{
	switch (mode) {
	case WRAP_MODE_DEFAULT:         return _SG_WRAP_DEFAULT;
	case WRAP_MODE_REPEAT:          return SG_WRAP_REPEAT;
	case WRAP_MODE_CLAMP_TO_EDGE:   return SG_WRAP_CLAMP_TO_EDGE;
	case WRAP_MODE_CLAMP_TO_BORDER: return SG_WRAP_CLAMP_TO_BORDER;
	case WRAP_MODE_MIRRORED_REPEAT: return SG_WRAP_MIRRORED_REPEAT;
	}
}

static CUTE_INLINE sg_border_color s_wrap(CF_BorderColor color)
{
	switch (color) {
	case BORDER_COLOR_DEFAULT:           return _SG_BORDERCOLOR_DEFAULT;
	case BORDER_COLOR_TRANSPARENT_BLACK: return SG_BORDERCOLOR_TRANSPARENT_BLACK;
	case BORDER_COLOR_OPAQUE_BLACK:      return SG_BORDERCOLOR_OPAQUE_BLACK;
	case BORDER_COLOR_OPAQUE_WHITE:      return SG_BORDERCOLOR_OPAQUE_WHITE;
	}
}

static CUTE_INLINE sg_image_data s_wrap(CF_TextureData data)
{
	CUTE_STATIC_ASSERT(SG_CUBEFACE_NUM == CF_CUBE_FACE_COUNT, "Must be equal.");
	CUTE_STATIC_ASSERT(SG_MAX_MIPMAPS == CF_MIPMAPS_MAX, "Must be equal.");
	sg_image_data sdata;
	for (int i = 0; i < SG_CUBEFACE_NUM; ++i) {
		for (int j = 0; j < CF_MIPMAPS_MAX; ++j) {
			sdata.subimage[i][j].ptr = data.sub_image[i][j];
			sdata.subimage[i][j].size = data.size[i][j];
		}
	}
	return sdata;
}

static CUTE_INLINE sg_action s_wrap(CF_PassInitOp op)
{
	switch (op) {
	case CF_PASS_INIT_OP_CLEAR:    return SG_ACTION_CLEAR;
	case CF_PASS_INIT_OP_LOAD:     return SG_ACTION_LOAD;
	case CF_PASS_INIT_OP_DONTCARE: return SG_ACTION_DONTCARE;
	}
}


static CUTE_INLINE sg_vertex_format s_wrap(CF_VertexFormat fmt)
{
	switch (fmt) {
	case CF_VERTEX_FORMAT_INVALID:  return SG_VERTEXFORMAT_INVALID;
	case CF_VERTEX_FORMAT_FLOAT:    return SG_VERTEXFORMAT_FLOAT;
	case CF_VERTEX_FORMAT_FLOAT2:   return SG_VERTEXFORMAT_FLOAT2;
	case CF_VERTEX_FORMAT_FLOAT3:   return SG_VERTEXFORMAT_FLOAT3;
	case CF_VERTEX_FORMAT_FLOAT4:   return SG_VERTEXFORMAT_FLOAT4;
	case CF_VERTEX_FORMAT_BYTE4:    return SG_VERTEXFORMAT_BYTE4;
	case CF_VERTEX_FORMAT_BYTE4N:   return SG_VERTEXFORMAT_BYTE4N;
	case CF_VERTEX_FORMAT_UBYTE4:   return SG_VERTEXFORMAT_UBYTE4;
	case CF_VERTEX_FORMAT_UBYTE4N:  return SG_VERTEXFORMAT_UBYTE4N;
	case CF_VERTEX_FORMAT_SHORT2:   return SG_VERTEXFORMAT_SHORT2;
	case CF_VERTEX_FORMAT_SHORT2N:  return SG_VERTEXFORMAT_SHORT2N;
	case CF_VERTEX_FORMAT_USHORT2N: return SG_VERTEXFORMAT_USHORT2N;
	case CF_VERTEX_FORMAT_SHORT4:   return SG_VERTEXFORMAT_SHORT4;
	case CF_VERTEX_FORMAT_SHORT4N:  return SG_VERTEXFORMAT_SHORT4N;
	case CF_VERTEX_FORMAT_USHORT4N: return SG_VERTEXFORMAT_USHORT4N;
	}
}

static CUTE_INLINE sg_compare_func s_wrap(CF_CompareFunction fn)
{
	switch (fn) {
	case COMPARE_FUNCTION_ALWAYS:                return SG_COMPAREFUNC_ALWAYS;
	case COMPARE_FUNCTION_NEVER:                 return SG_COMPAREFUNC_NEVER;
	case COMPARE_FUNCTION_LESS_THAN:             return SG_COMPAREFUNC_LESS;
	case COMPARE_FUNCTION_EQUAL:                 return SG_COMPAREFUNC_EQUAL;
	case COMPARE_FUNCTION_NOT_EQUAL:             return SG_COMPAREFUNC_NOT_EQUAL;
	case COMPARE_FUNCTION_LESS_THAN_OR_EQUAL:    return SG_COMPAREFUNC_LESS_EQUAL;
	case COMPARE_FUNCTION_GREATER_THAN:          return SG_COMPAREFUNC_GREATER;
	case COMPARE_FUNCTION_GREATER_THAN_OR_EQUAL: return SG_COMPAREFUNC_GREATER_EQUAL;
	}
}

static CUTE_INLINE sg_stencil_op s_wrap(CF_StencilOp op)
{
	switch (op) {
	case CF_STENCIL_OP_KEEP:            return SG_STENCILOP_KEEP;
	case CF_STENCIL_OP_ZERO:            return SG_STENCILOP_ZERO;
	case CF_STENCIL_OP_REPLACE:         return SG_STENCILOP_REPLACE;
	case CF_STENCIL_OP_INCREMENT_CLAMP: return SG_STENCILOP_INCR_CLAMP;
	case CF_STENCIL_OP_DECREMENT_CLAMP: return SG_STENCILOP_DECR_CLAMP;
	case CF_STENCIL_OP_INVERT:          return SG_STENCILOP_INVERT;
	case CF_STENCIL_OP_INCREMENT_WRAP:  return SG_STENCILOP_INCR_WRAP;
	case CF_STENCIL_OP_DECREMENT_WRAP:  return SG_STENCILOP_DECR_WRAP;
	}
}

static CUTE_INLINE sg_blend_factor s_wrap(CF_BlendFactor factor)
{
	switch (factor) {
	case CF_BLENDFACTOR_ZERO:                  return SG_BLENDFACTOR_ZERO;
	case CF_BLENDFACTOR_ONE:                   return SG_BLENDFACTOR_ONE;
	case CF_BLENDFACTOR_SRC_COLOR:             return SG_BLENDFACTOR_SRC_COLOR;
	case CF_BLENDFACTOR_ONE_MINUS_SRC_COLOR:   return SG_BLENDFACTOR_ONE_MINUS_SRC_COLOR;
	case CF_BLENDFACTOR_SRC_ALPHA:             return SG_BLENDFACTOR_SRC_ALPHA;
	case CF_BLENDFACTOR_ONE_MINUS_SRC_ALPHA:   return SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
	case CF_BLENDFACTOR_DST_COLOR:             return SG_BLENDFACTOR_DST_COLOR;
	case CF_BLENDFACTOR_ONE_MINUS_DST_COLOR:   return SG_BLENDFACTOR_ONE_MINUS_DST_COLOR;
	case CF_BLENDFACTOR_DST_ALPHA:             return SG_BLENDFACTOR_DST_ALPHA;
	case CF_BLENDFACTOR_ONE_MINUS_DST_ALPHA:   return SG_BLENDFACTOR_ONE_MINUS_DST_ALPHA;
	case CF_BLENDFACTOR_SRC_ALPHA_SATURATED:   return SG_BLENDFACTOR_SRC_ALPHA_SATURATED;
	case CF_BLENDFACTOR_BLEND_COLOR:           return SG_BLENDFACTOR_BLEND_COLOR;
	case CF_BLENDFACTOR_ONE_MINUS_BLEND_COLOR: return SG_BLENDFACTOR_ONE_MINUS_BLEND_COLOR;
	case CF_BLENDFACTOR_BLEND_ALPHA:           return SG_BLENDFACTOR_BLEND_ALPHA;
	case CF_BLENDFACTOR_ONE_MINUS_BLEND_ALPHA: return SG_BLENDFACTOR_ONE_MINUS_BLEND_ALPHA;
	}
}

static CUTE_INLINE sg_blend_op s_wrap(CF_BlendOp op)
{
	switch (op) {
	case CF_BLEND_OP_ADD:              return SG_BLENDOP_ADD;
	case CF_BLEND_OP_SUBTRACT:         return SG_BLENDOP_SUBTRACT;
	case CF_BLEND_OP_REVERSE_SUBTRACT: return SG_BLENDOP_REVERSE_SUBTRACT;
	}
}

static CUTE_INLINE sg_primitive_type s_wrap(CF_PrimitiveType type)
{
	switch (type) {
	case CF_PRIMITIVE_TYPE_POINT:          return SG_PRIMITIVETYPE_POINTS;
	case CF_PRIMITIVE_TYPE_LINE:           return SG_PRIMITIVETYPE_LINES;
	case CF_PRIMITIVE_TYPE_LINE_STRIP:     return SG_PRIMITIVETYPE_LINE_STRIP;
	case CF_PRIMITIVE_TYPE_TRIANGLE:       return SG_PRIMITIVETYPE_TRIANGLES;
	case CF_PRIMITIVE_TYPE_TRIANGLE_STRIP: return SG_PRIMITIVETYPE_TRIANGLE_STRIP;
	}
}

static CUTE_INLINE sg_index_type s_wrap(CF_IndexType type)
{
	switch (type) {
	case CF_INDEX_TYPE_NONE: return SG_INDEXTYPE_NONE;
	case CF_INDEX_TYPE_U16:  return SG_INDEXTYPE_UINT16;
	case CF_INDEX_TYPE_U32:  return SG_INDEXTYPE_UINT32;
	}
}

static CUTE_INLINE sg_cull_mode s_wrap(CF_CullMode mode)
{
	switch (mode) {
	case CF_CULL_MODE_NONE:  SG_CULLMODE_NONE;
	case CF_CULL_MODE_FRONT: SG_CULLMODE_FRONT;
	case CF_CULL_MODE_BACK:  SG_CULLMODE_BACK;
	}
}

static CUTE_INLINE sg_color s_wrap(CF_Color color)
{
	sg_color sgc;
	sgc.r = color.r;
	sgc.g = color.g;
	sgc.b = color.b;
	sgc.a = color.a;
	return sgc;
}

CF_BackendType CF_QueryBackend()
{
	sg_backend backend = sg_query_backend();
	return (CF_BackendType)backend;
}

bool CF_QueryPixelFormat(CF_PixelFormat format, CF_PixelFormatOp op)
{
	sg_pixel_format sgfmt = SG_PIXELFORMAT_NONE;
	switch (format) {
	case PIXELFORMAT_DEFAULT: sgfmt = _SG_PIXELFORMAT_DEFAULT; break;
	default:                  sgfmt = s_wrap(format);          break;
	}

	sg_pixelformat_info info = sg_query_pixelformat(sgfmt);
	bool result = false;

	switch (op) {
	case PIXELFORMAT_OP_NEAREST_FILTER:  result = info.sample; break;
	case PIXELFORMAT_OP_BILINEAR_FILTER: result = info.filter; break;
	case PIXELFORMAT_OP_RENDER_TARGET:   result = info.render; break;
	case PIXELFORMAT_OP_ALPHA_BLENDING:  result = info.blend;  break;
	case PIXELFORMAT_OP_MSAA:            result = info.msaa;   break;
	case PIXELFORMAT_OP_DEPTH:           result = info.depth;  break;
	}

	return result;
}

bool CF_QueryDeviceFeature(CF_DeviceFeature feature)
{
	sg_features sgf = sg_query_features();
	bool result = false;
	switch (feature) {
	case DEVICE_FEATURE_INSTANCING:       result = sgf.instancing;                  break;
	case DEVICE_FEATURE_MRT:              result = sgf.multiple_render_targets;     break;
	case DEVICE_FEATURE_MRT_BLEND_STATES: result = sgf.mrt_independent_blend_state; break;
	case DEVICE_FEATURE_MRT_WRITE_MASKS:  result = sgf.mrt_independent_write_mask;  break;
	case DEVICE_FEATURE_MSAA:             result = sgf.msaa_render_targets;         break;
	case DEVICE_FEATURE_TEXTURE_ARRAY:    result = sgf.imagetype_array;             break;
	case DEVICE_FEATURE_TEXTURE_CLAMP:    result = sgf.image_clamp_to_border;       break;
	}
	return result;
}

int CF_QueryResourceLimit(CF_ResourceLimit resource_limit)
{
	sg_limits sgl = sg_query_limits();
	int result = 0;
	switch (resource_limit) {
	case RESOURCE_LIMIT_TEXTURE_DIMENSION:       result = sgl.max_image_size_2d; break;
	case RESOURCE_LIMIT_TEXTURE_ARRAY_DIMENSION: result = sgl.max_image_size_array; break;
	case RESOURCE_LIMIT_TEXTURE_ARRAY_LAYER:     result = sgl.max_image_array_layers; break;
	case RESOURCE_LIMIT_TEXTURE_CUBE_DIMENSION:  result = sgl.max_image_size_cube; break;
	case RESOURCE_LIMIT_VERTEX_ATTRIBUTE_MAX:    result = sgl.max_vertex_attrs; break;
	}
	return result;
}

CF_Buffer CF_MakeBuffer(CF_BufferType type, CF_UsageType usage, void* data, int size)
{
	sg_buffer_desc desc;
	CUTE_MEMSET(&desc, 0, sizeof(desc));
	desc.type = s_wrap(type);
	desc.usage = s_wrap(usage);
	desc.data.ptr = data;
	desc.data.size = (size_t)size;
	sg_buffer sgb = sg_make_buffer(desc);
	CF_Buffer result = { sgb.id };
	return result;
}

void CF_DestroyBuffer(CF_Buffer buffer)
{
	sg_buffer sgb = { (uint32_t)buffer.id };
	sg_destroy_buffer(sgb);
}

void CF_UpdateBuffer(CF_Buffer buffer, void* data, int size)
{
	sg_range range = { data, size };
	sg_buffer sgb = { (uint32_t)buffer.id };
	sg_update_buffer(sgb, range);
}

int CF_AppendBuffer(CF_Buffer buffer, void* data, int size)
{
	sg_range range = { data, size };
	sg_buffer sgb = { (uint32_t)buffer.id };
	return sg_append_buffer(sgb, range);
}

bool CF_QueryBufferWillOverflow(CF_Buffer buffer, int size)
{
	sg_buffer sgb = { (uint32_t)buffer.id };
	return sg_query_will_buffer_overflow(sgb, size);
}

CF_TextureParams CF_TextureDefaults()
{
	CF_TextureParams params;
	params.type = CF_TEXTURE_TYPE_DEFAULT;
	params.pixel_format = CF_PIXELFORMAT_DEFAULT;
	params.usage = CF_USAGE_TYPE_IMMUTABLE;
	params.min_filter = CF_MINMAG_FILTER_NEAREST;
	params.mag_filter = CF_MINMAG_FILTER_NEAREST;
	params.mip_filter = CF_MIP_FILTER_NEAREST;
	params.wrap_u = CF_WRAP_MODE_DEFAULT;
	params.wrap_v = CF_WRAP_MODE_DEFAULT;
	params.border_color = CF_BORDER_COLOR_DEFAULT;
	params.width = 0;
	params.height = 0;
	params.array_layers = 0;
	params.max_anisotropy = 1;
	params.mip_count = 1;
	params.msaa_sample_count = 1;
	return params;
}

CF_Texture CF_MakeTexture(CF_TextureParams texture_params)
{
	sg_image_desc desc;
	CUTE_MEMSET(&desc, 0, sizeof(desc));
	desc.type = s_wrap(texture_params.type);
	desc.render_target = texture_params.render_target;
	desc.width = texture_params.width;
	desc.height = texture_params.height;
	desc.num_slices = texture_params.array_layers;
	desc.num_mipmaps = texture_params.mip_count;
	desc.usage = s_wrap(texture_params.usage);
	desc.pixel_format = s_wrap(texture_params.pixel_format);
	desc.min_filter = s_wrap(texture_params.min_filter, texture_params.mip_filter);
	desc.mag_filter = s_wrap(texture_params.mag_filter, texture_params.mip_filter);
	desc.wrap_u = s_wrap(texture_params.wrap_u);
	desc.wrap_v = s_wrap(texture_params.wrap_v);
	desc.wrap_w = s_wrap(texture_params.wrap_v);
	desc.border_color = s_wrap(texture_params.border_color);
	desc.max_anisotropy = texture_params.max_anisotropy;
	desc.min_lod = 0;
	desc.max_lod = FLT_MAX;
	desc.data = s_wrap(texture_params.initial_data);
	sg_image sgi = sg_make_image(desc);
	CF_Texture texture = { sgi.id };
	return texture;
}

void CF_DestroyTexture(CF_Texture texture)
{
	sg_image sgi = { (uint32_t)texture.id };
	sg_destroy_image(sgi);
}

void CF_UpdateTexture(CF_Texture texture, CF_TextureData data)
{
	sg_image sgi = { (uint32_t)texture.id };
	sg_update_image(sgi, s_wrap(data));
}

CF_Shader CF_MakeShader(CF_ShaderParams shader_params)
{
}

void CF_DestroyShader(CF_Shader shader)
{
	sg_shader sgshd = { (uint32_t)shader.id };
	sg_destroy_shader(sgshd);
}

CF_PassParams CF_PassDefaults()
{
	CF_PassParams params;
	params.name = NULL;
	for (int i = 0; i < CF_TEXTURE_ATTACHMENTS_MAX; ++i) {
		params.color_ops[i] = CF_PASS_INIT_OP_CLEAR;
		params.color_values[i] = CF_ColorGrey();
	}
	params.depth_op = CF_PASS_INIT_OP_CLEAR;
	params.depth_value = 1.0f;
	params.stencil_op = CF_PASS_INIT_OP_CLEAR;
	params.stencil_value = 0;
	params.texture_count = 0;
	for (int i = 0; i < CF_TEXTURE_ATTACHMENTS_MAX; ++i) {
		params.textures[i].texture = { 0 };
		params.textures[i].mip_level = 0;
		params.textures[i].u.array_layer = 0;
	}
	params.depth_stencil.texture = { 0 };
	params.depth_stencil.mip_level = 0;
	params.depth_stencil.u.array_layer = 0;
	return params;
}

struct CF_PassInternal
{
	sg_pass_action sga;
	sg_pass sgp;
};

CF_Pass CF_MakePass(CF_PassParams pass_params)
{
	CUTE_STATIC_ASSERT(CF_TEXTURE_ATTACHMENTS_MAX == SG_MAX_COLOR_ATTACHMENTS, "Must be equal.");
	CF_PassInternal* pass = (CF_PassInternal*)CUTE_ALLOC(sizeof(CF_PassInternal));
	for (int i = 0; i < CF_TEXTURE_ATTACHMENTS_MAX; ++i) {
		pass->sga.colors[i].action = s_wrap(pass_params.color_ops[i]);
		pass->sga.colors[i].value = s_wrap(pass_params.color_values[i]);
	}
	pass->sga.depth.action = s_wrap(pass_params.depth_op);
	pass->sga.depth.value = pass_params.depth_value;
	pass->sga.stencil.action = s_wrap(pass_params.stencil_op);
	pass->sga.stencil.value = pass_params.stencil_value;
	sg_pass_desc desc;
	CUTE_MEMSET(&desc, 0, sizeof(desc));
	for (int i = 0; i < CF_TEXTURE_ATTACHMENTS_MAX; ++i) {
		desc.color_attachments[i].image = { (uint32_t)pass_params.textures[i].texture.id };
		desc.color_attachments[i].mip_level = pass_params.textures[i].mip_level;
		desc.color_attachments[i].slice = pass_params.textures[i].u.array_layer;
	}
	desc.depth_stencil_attachment.image = { (uint32_t)pass_params.depth_stencil.texture.id };
	desc.depth_stencil_attachment.mip_level = pass_params.depth_stencil.mip_level;
	desc.depth_stencil_attachment.slice = pass_params.depth_stencil.u.array_layer;
	desc.label = pass_params.name;
	pass->sgp = sg_make_pass(desc);
	CF_Pass result;
	result.id = (uint64_t)pass;
	return result;
}

void CF_DestroyPass(CF_Pass pass)
{
	CF_PassInternal* pass_internal = (CF_PassInternal*)pass.id;
	sg_destroy_pass(pass_internal->sgp);
	CUTE_FREE(pass_internal);
}

CF_PipelineParams CF_PipelineDefaults()
{
	CF_PipelineParams params;
	params.name = NULL;
	params.shader = { 0 };
	params.primitive_type = CF_PRIMITIVE_TYPE_TRIANGLE;
	params.index_type = CF_INDEX_TYPE_NONE;
	params.vertex_layout_count = 0;
	for (int i = 0; i < CF_VERTEX_ATTRIBUTES_MAX; ++i) {
		params.vertex_layouts[i].format = CF_VERTEX_FORMAT_INVALID;
		params.vertex_layouts[i].offset = 0;
		params.vertex_layouts[i].stride = 0;
		params.vertex_layouts[i].buffer_index = 0;
		params.vertex_layouts[i].step_rate = 0;
	}
	params.texture_count = 0;
	for (int i = 0; i < CF_TEXTURE_ATTACHMENTS_MAX; ++i) {
		params.texture_slots[i].pixel_format = CF_PIXELFORMAT_DEFAULT;
		params.texture_slots[i].write_R_enabled = true;
		params.texture_slots[i].write_G_enabled = true;
		params.texture_slots[i].write_B_enabled = true;
		params.texture_slots[i].write_A_enabled = true;
		params.texture_slots[i].blending_enabled = false;
		params.texture_slots[i].rgb_op = CF_BLEND_OP_ADD;
		params.texture_slots[i].rgb_src_blend_factor = CF_BLENDFACTOR_ONE;
		params.texture_slots[i].rgb_dst_blend_factor = CF_BLENDFACTOR_ZERO;
		params.texture_slots[i].alpha_op = CF_BLEND_OP_ADD;
		params.texture_slots[i].alpha_src_blend_factor = CF_BLENDFACTOR_ONE;
		params.texture_slots[i].alpha_dst_blend_factor = CF_BLENDFACTOR_ZERO;
	}
	params.cull_mode = CF_CULL_MODE_NONE;
	params.blend_color = CF_ColorBlack();
	params.depth_format = s_wrap(cf_app->gfx_ctx_params.depth_format);
	params.depth_compare = CF_COMPARE_FUNCTION_ALWAYS;
	params.depth_write_enabled = false;
	params.depth_bias = 0;
	params.depth_bias_slope_scale = 0;
	params.depth_bias_clamp = 0;
	params.stencil.enabled = false;
	params.stencil.read_mask = 0;
	params.stencil.write_mask = 0;
	params.stencil.reference = 0;
	params.stencil.front.compare = CF_COMPARE_FUNCTION_ALWAYS;
	params.stencil.front.fail_op = CF_STENCIL_OP_KEEP;
	params.stencil.front.depth_fail_op = CF_STENCIL_OP_KEEP;
	params.stencil.front.pass_op = CF_STENCIL_OP_KEEP;
	params.stencil.back.compare = CF_COMPARE_FUNCTION_ALWAYS;
	params.stencil.back.fail_op = CF_STENCIL_OP_KEEP;
	params.stencil.back.depth_fail_op = CF_STENCIL_OP_KEEP;
	params.stencil.back.pass_op = CF_STENCIL_OP_KEEP;
	return params;
}

CF_Pipeline CF_MakePipeline(CF_PipelineParams pipeline_params)
{
	CUTE_STATIC_ASSERT(CF_VERTEX_ATTRIBUTES_MAX == SG_MAX_VERTEX_ATTRIBUTES, "Must be equal.");
	sg_pipeline_desc desc;
	CUTE_MEMSET(&desc, 0, sizeof(desc));
	desc.shader = { (uint32_t)pipeline_params.shader.id };
	for (int i = 0; i < CF_VERTEX_ATTRIBUTES_MAX; ++i) {
		desc.layout.attrs[i].buffer_index = pipeline_params.vertex_layouts[i].buffer_index;
		desc.layout.attrs[i].offset = pipeline_params.vertex_layouts[i].offset;
		desc.layout.attrs[i].format = s_wrap(pipeline_params.vertex_layouts[i].format);
	}
	for (int i = 0; i < SG_MAX_SHADERSTAGE_BUFFERS; ++i) {
		desc.layout.buffers[i].stride = pipeline_params.vertex_layouts[i].stride;
		desc.layout.buffers[i].step_rate = pipeline_params.vertex_layouts[i].step_rate;
		desc.layout.buffers[i].step_func = pipeline_params.vertex_layouts[i].step_rate > 0 ? SG_VERTEXSTEP_PER_INSTANCE : SG_VERTEXSTEP_PER_VERTEX;
	}
	desc.depth.pixel_format = s_wrap(pipeline_params.depth_format);
	desc.depth.compare = s_wrap(pipeline_params.depth_compare);
	desc.depth.write_enabled = pipeline_params.depth_write_enabled;
	desc.depth.bias = pipeline_params.depth_bias;
	desc.depth.bias_slope_scale = pipeline_params.depth_bias_slope_scale;
	desc.depth.bias_clamp = pipeline_params.depth_bias_clamp;
	desc.stencil.enabled = pipeline_params.stencil.enabled;
	desc.stencil.front.compare = s_wrap(pipeline_params.stencil.front.compare);
	desc.stencil.front.fail_op = s_wrap(pipeline_params.stencil.front.fail_op);
	desc.stencil.front.depth_fail_op = s_wrap(pipeline_params.stencil.front.depth_fail_op);
	desc.stencil.front.pass_op = s_wrap(pipeline_params.stencil.front.pass_op);
	desc.stencil.back.compare = s_wrap(pipeline_params.stencil.back.compare);
	desc.stencil.back.fail_op = s_wrap(pipeline_params.stencil.back.fail_op);
	desc.stencil.back.depth_fail_op = s_wrap(pipeline_params.stencil.back.depth_fail_op);
	desc.stencil.back.pass_op = s_wrap(pipeline_params.stencil.back.pass_op);
	desc.stencil.read_mask = pipeline_params.stencil.read_mask;
	desc.stencil.write_mask = pipeline_params.stencil.write_mask;
	desc.stencil.ref = pipeline_params.stencil.reference;
	desc.color_count = pipeline_params.texture_count;
	for (int i = 0; i < desc.color_count; ++i) {
		desc.colors[i].pixel_format = s_wrap(pipeline_params.texture_slots[i].pixel_format);
		int r = (int)pipeline_params.texture_slots[i].write_R_enabled << 1;
		int g = (int)pipeline_params.texture_slots[i].write_G_enabled << 2;
		int b = (int)pipeline_params.texture_slots[i].write_B_enabled << 4;
		int a = (int)pipeline_params.texture_slots[i].write_A_enabled << 8;
		desc.colors[i].write_mask = (sg_color_mask)(r | g | b | a);
		desc.colors[i].blend.enabled = pipeline_params.texture_slots[i].blending_enabled;
		desc.colors[i].blend.src_factor_rgb = s_wrap(pipeline_params.texture_slots[i].rgb_src_blend_factor);
		desc.colors[i].blend.dst_factor_rgb = s_wrap(pipeline_params.texture_slots[i].rgb_dst_blend_factor);
		desc.colors[i].blend.op_rgb = s_wrap(pipeline_params.texture_slots[i].rgb_op);
		desc.colors[i].blend.src_factor_alpha = s_wrap(pipeline_params.texture_slots[i].alpha_src_blend_factor);
		desc.colors[i].blend.dst_factor_alpha = s_wrap(pipeline_params.texture_slots[i].alpha_dst_blend_factor);
		desc.colors[i].blend.op_alpha = s_wrap(pipeline_params.texture_slots[i].alpha_op);
	}
	desc.primitive_type = s_wrap(pipeline_params.primitive_type);
	desc.index_type = s_wrap(pipeline_params.index_type);
	desc.cull_mode = s_wrap(pipeline_params.cull_mode);
	desc.face_winding = SG_FACEWINDING_CCW;
	desc.sample_count = cf_app->gfx_ctx_params.sample_count;
	desc.blend_color = s_wrap(pipeline_params.blend_color);
	desc.alpha_to_coverage_enabled = false;
	desc.label = pipeline_params.name;
	sg_pipeline sgp = sg_make_pipeline(desc);
	CF_Pipeline pipeline = { sgp.id };
	return pipeline;
}

void CF_DestroyPipeline(CF_Pipeline pipeline)
{
	sg_pipeline sgp = { (uint32_t)pipeline.id };
	sg_destroy_pipeline(sgp);
}

void CF_BeginPass(CF_Pass pass)
{
	CF_PassInternal* pass_internal = (CF_PassInternal*)pass.id;
	sg_begin_pass(pass_internal->sgp, &pass_internal->sga);
}

void CF_ApplyViewport(float x, float y, float width, float height)
{
	sg_apply_viewportf(x, y, width, height, false);
}

void CF_ApplyScissor(float x, float y, float width, float height)
{
	sg_apply_scissor_rectf(x, y, width, height, false);
}

void CF_ApplyPipeline(CF_Pipeline pipeline)
{
	sg_pipeline sgp = { (uint32_t)pipeline.id };
	sg_apply_pipeline(sgp);
}

void CF_ApplyBindings(CF_ResourceBindings bindings)
{
	CUTE_STATIC_ASSERT(CF_SHADER_BINDINGS_BUFFER_MAX == SG_MAX_SHADERSTAGE_BUFFERS, "Must be equal.");
	sg_bindings sgb;
	CUTE_MEMSET(&sgb, 0, sizeof(sgb));
	for (int i = 0; i < CF_SHADER_BINDINGS_BUFFER_MAX; ++i) {
		sgb.vertex_buffers[i].id = (uint32_t)bindings.vertex_buffers[i].id;
		sgb.vertex_buffer_offsets[i] = bindings.vertex_buffer_offsets[i];
	}
	sgb.index_buffer.id = (uint32_t)bindings.index_buffer.id;
	sgb.index_buffer_offset = bindings.index_buffer_offset;
	for (int i = 0; i < CF_SHADER_BINDINGS_BUFFER_MAX; ++i) {
		sgb.vs_images[i].id = (uint32_t)bindings.vs_textures[i].id;
		sgb.fs_images[i].id = (uint32_t)bindings.fs_textures[i].id;
	}
	sg_apply_bindings(sgb);
}

void CF_ApplyVSUniforms(int uniform_buffer_index, void* data, int size)
{
	sg_range range = { data, size };
	sg_apply_uniforms(SG_SHADERSTAGE_VS, uniform_buffer_index, range);
}

void CF_ApplyFSUniforms(int uniform_buffer_index, void* data, int size)
{
	sg_range range = { data, size };
	sg_apply_uniforms(SG_SHADERSTAGE_FS, uniform_buffer_index, range);
}

void CF_DrawElements(int base_element, int element_count, int instance_count)
{
	sg_draw(base_element, element_count, instance_count);
}

void CF_EndPass()
{
	sg_end_pass();
}

void CF_Commit()
{
	sg_commit();
}

