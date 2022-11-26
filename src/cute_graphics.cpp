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

static CUTE_INLINE sg_usage s_wrap(CF_UsageType type)
{
	switch (type) {
	case CF_USAGE_TYPE_IMMUTABLE: return SG_USAGE_IMMUTABLE;
	case CF_USAGE_TYPE_DYNAMIC:   return SG_USAGE_DYNAMIC;
	case CF_USAGE_TYPE_STREAM:    return SG_USAGE_STREAM;
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

static CUTE_INLINE sg_wrap s_wrap(CF_WrapMode mode)
{
	switch (mode) {
	case CF_WRAP_MODE_DEFAULT:         return _SG_WRAP_DEFAULT;
	case CF_WRAP_MODE_REPEAT:          return SG_WRAP_REPEAT;
	case CF_WRAP_MODE_CLAMP_TO_EDGE:   return SG_WRAP_CLAMP_TO_EDGE;
	case CF_WRAP_MODE_CLAMP_TO_BORDER: return SG_WRAP_CLAMP_TO_BORDER;
	case CF_WRAP_MODE_MIRRORED_REPEAT: return SG_WRAP_MIRRORED_REPEAT;
	}
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
	case CF_COMPARE_FUNCTION_ALWAYS:                return SG_COMPAREFUNC_ALWAYS;
	case CF_COMPARE_FUNCTION_NEVER:                 return SG_COMPAREFUNC_NEVER;
	case CF_COMPARE_FUNCTION_LESS_THAN:             return SG_COMPAREFUNC_LESS;
	case CF_COMPARE_FUNCTION_EQUAL:                 return SG_COMPAREFUNC_EQUAL;
	case CF_COMPARE_FUNCTION_NOT_EQUAL:             return SG_COMPAREFUNC_NOT_EQUAL;
	case CF_COMPARE_FUNCTION_LESS_THAN_OR_EQUAL:    return SG_COMPAREFUNC_LESS_EQUAL;
	case CF_COMPARE_FUNCTION_GREATER_THAN:          return SG_COMPAREFUNC_GREATER;
	case CF_COMPARE_FUNCTION_GREATER_THAN_OR_EQUAL: return SG_COMPAREFUNC_GREATER_EQUAL;
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
	case CF_PIXELFORMAT_DEFAULT: sgfmt = _SG_PIXELFORMAT_DEFAULT; break;
	default:                  sgfmt = s_wrap(format);          break;
	}

	sg_pixelformat_info info = sg_query_pixelformat(sgfmt);
	bool result = false;

	switch (op) {
	case CF_PIXELFORMAT_OP_NEAREST_FILTER:  result = info.sample; break;
	case CF_PIXELFORMAT_OP_BILINEAR_FILTER: result = info.filter; break;
	case CF_PIXELFORMAT_OP_RENDER_TARGET:   result = info.render; break;
	case CF_PIXELFORMAT_OP_ALPHA_BLENDING:  result = info.blend;  break;
	case CF_PIXELFORMAT_OP_MSAA:            result = info.msaa;   break;
	case CF_PIXELFORMAT_OP_DEPTH:           result = info.depth;  break;
	}

	return result;
}

bool CF_QueryDeviceFeature(CF_DeviceFeature feature)
{
	sg_features sgf = sg_query_features();
	bool result = false;
	switch (feature) {
	case CF_DEVICE_FEATURE_INSTANCING:       result = sgf.instancing;                  break;
	case CF_DEVICE_FEATURE_MSAA:             result = sgf.msaa_render_targets;         break;
	case CF_DEVICE_FEATURE_TEXTURE_CLAMP:    result = sgf.image_clamp_to_border;       break;
	}
	return result;
}

int CF_QueryResourceLimit(CF_ResourceLimit resource_limit)
{
	sg_limits sgl = sg_query_limits();
	int result = 0;
	switch (resource_limit) {
	case CF_RESOURCE_LIMIT_TEXTURE_DIMENSION:       result = sgl.max_image_size_2d; break;
	case CF_RESOURCE_LIMIT_VERTEX_ATTRIBUTE_MAX:    result = sgl.max_vertex_attrs; break;
	}
	return result;
}

CF_TextureParams CF_TextureDefaults()
{
	CF_TextureParams params;
	params.pixel_format = CF_PIXELFORMAT_DEFAULT;
	params.usage = CF_USAGE_TYPE_IMMUTABLE;
	params.wrap_u = CF_WRAP_MODE_DEFAULT;
	params.wrap_v = CF_WRAP_MODE_DEFAULT;
	params.width = 0;
	params.height = 0;
	return params;
}

CF_Texture CF_MakeTexture(CF_TextureParams texture_params)
{
	sg_image_desc desc;
	CUTE_MEMSET(&desc, 0, sizeof(desc));
	desc.type = SG_IMAGETYPE_2D;
	desc.render_target = texture_params.render_target;
	desc.width = texture_params.width;
	desc.height = texture_params.height;
	desc.num_slices = 0;
	desc.num_mipmaps = 0;
	desc.usage = s_wrap(texture_params.usage);
	desc.pixel_format = s_wrap(texture_params.pixel_format);
	desc.min_filter = _SG_FILTER_DEFAULT;
	desc.mag_filter = _SG_FILTER_DEFAULT;
	desc.wrap_u = s_wrap(texture_params.wrap_u);
	desc.wrap_v = s_wrap(texture_params.wrap_v);
	desc.wrap_w = s_wrap(texture_params.wrap_v);
	desc.border_color = _SG_BORDERCOLOR_DEFAULT;
	desc.max_anisotropy = 1;
	desc.min_lod = 0;
	desc.max_lod = FLT_MAX;
	desc.data.subimage[0][0].ptr = texture_params.initial_data;
	desc.data.subimage[0][0].size = texture_params.initial_data_size;
	sg_image sgi = sg_make_image(desc);
	CF_Texture texture = { sgi.id };
	return texture;
}

void CF_DestroyTexture(CF_Texture texture)
{
	sg_image sgi = { (uint32_t)texture.id };
	sg_destroy_image(sgi);
}

void CF_UpdateTexture(CF_Texture texture, void* data, int size)
{
	sg_image_data sgid = { };
	sgid.subimage[0][0].ptr = data;
	sgid.subimage[0][0].size = size;
	sg_image sgi = { (uint32_t)texture.id };
	sg_update_image(sgi, sgid);
}

struct CF_ShaderInternal
{
	CF_SokolShader table;
	sg_shader sgshd;
};

CF_Shader CF_MakeShader(CF_SokolShader sokol_shader)
{
	CF_ShaderInternal* shader = (CF_ShaderInternal*)CUTE_ALLOC(sizeof(CF_ShaderInternal));
	shader->table = sokol_shader;
	const sg_shader_desc* desc = shader->table.get_desc_fn((int)sg_query_backend());
	shader->sgshd = sg_make_shader(desc);
	CF_Shader result;
	result.id = { (uint64_t)shader };
	return result;
}

void CF_DestroyShader(CF_Shader shader)
{
	CF_ShaderInternal* shader_internal = (CF_ShaderInternal*)shader.id;
	sg_destroy_shader(shader_internal->sgshd);
	CUTE_FREE(shader_internal);
}

CF_PassParams CF_PassDefaults()
{
	CF_PassParams params;
	params.name = NULL;
	params.color_op = CF_PASS_INIT_OP_CLEAR;
	params.color_value = CF_ColorGrey();
	params.depth_op = CF_PASS_INIT_OP_CLEAR;
	params.depth_value = 1.0f;
	params.stencil_op = CF_PASS_INIT_OP_CLEAR;
	params.stencil_value = 0;
	params.target = { 0 };
	params.depth_stencil = { 0 };
	return params;
}

struct CF_PassInternal
{
	sg_pass_action sga;
	sg_pass sgp;
};

CF_Pass CF_MakePass(CF_PassParams pass_params)
{
	CF_PassInternal* pass = (CF_PassInternal*)CUTE_CALLOC(sizeof(CF_PassInternal));
	pass->sga.colors[0].action = s_wrap(pass_params.color_op);
	pass->sga.colors[0].value = s_wrap(pass_params.color_value);
	pass->sga.depth.action = s_wrap(pass_params.depth_op);
	pass->sga.depth.value = pass_params.depth_value;
	pass->sga.stencil.action = s_wrap(pass_params.stencil_op);
	pass->sga.stencil.value = pass_params.stencil_value;
	sg_pass_desc desc;
	CUTE_MEMSET(&desc, 0, sizeof(desc));
	desc.color_attachments[0].image = { (uint32_t)pass_params.target.id };
	desc.depth_stencil_attachment.image = { (uint32_t)pass_params.depth_stencil.id };
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

CF_Mesh CF_MakeMesh(CF_UsageType usage_type)
{
}

void CF_DestroyMesh(CF_Mesh mesh)
{
}

cf_result_t CF_MeshSetVertexAttributes(CF_Mesh mesh, const char* name, CF_VertexAttribute* attributes, int attribute_count, int stride)
{
}

cf_result_t CF_MeshSetInstanceAttributes(CF_Mesh mesh, const char* name, CF_VertexAttribute* attributes, int attribute_count, int stride)
{
}

void CF_MeshUpdateVertexData(CF_Mesh mesh, void* data, int count)
{
}

int CF_MeshAppendVertexData(CF_Mesh mesh, void* data, int count)
{
}

bool CF_MeshWillOverflowVertexData(CF_Mesh mesh, int count)
{
}

void CF_MeshUpdateInstanceData(CF_Mesh mesh, void* data, int count)
{
}

int CF_MeshAppendInstanceData(CF_Mesh mesh, void* data, int count)
{
}

bool CF_MeshWillOverflowInstanceData(CF_Mesh mesh, int count)
{
}

void CF_MeshUpdateIndexData(CF_Mesh mesh, uint32_t* indices, int count)
{
}

int CF_MeshAppendIndexData(CF_Mesh mesh, uint32_t* indices, int count)
{
}

bool CF_MeshWillOverflowIndexData(CF_Mesh mesh, int count)
{
}

CF_Material CF_MakeMaterial(CF_RenderState render_state)
{
}

void CF_DestroyMaterial(CF_Material material)
{
}

void CF_MaterialSetTextureVS(CF_Material material, const char* name, CF_Texture texture)
{
}

void CF_MaterialSetTextureFS(CF_Material material, const char* name, CF_Texture texture)
{
}

void CF_MaterialSetUniformVS(CF_Material material, const char* name, void* data, int size)
{
}

void CF_MaterialSetUniformFS(CF_Material material, const char* name, void* data, int size)
{
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

void CF_ApplyMesh(CF_Mesh mesh)
{
}

void CF_ApplyMaterial(CF_Material material)
{
}

void CF_ApplyShader(CF_Shader shader)
{
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
