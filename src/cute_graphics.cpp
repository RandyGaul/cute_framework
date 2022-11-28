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

#define CF_VERTEX_BUFFER_SLOT (0)
#define CF_INSTANCE_BUFFER_SLOT (1)

using namespace cute;

CF_Matrix4x4 cf_matrix_identity()
{
	CF_Matrix4x4 m;
	CUTE_MEMSET(&m, 0, sizeof(m));
	m.elements[0] = 1.0f;
	m.elements[5] = 1.0f;
	m.elements[10] = 1.0f;
	m.elements[15] = 1.0f;
	return m;
}

CF_Matrix4x4 cf_matrix_ortho_2d(float w, float h, float x, float y)
{
	float L = -w / 2.0f;
	float R = w / 2.0f;
	float T = h / 2.0f;
	float B = -h / 2.0f;

	CF_Matrix4x4 projection;
	CUTE_MEMSET(&projection, 0, sizeof(projection));

	// ortho
	projection.elements[0] = 2.0f / (R - L);
	projection.elements[5] = 2.0f / (T - B);
	projection.elements[10] = -0.5f;
	projection.elements[15] = 1.0f;

	// translate
	projection.elements[12] = -x * projection.elements[0];
	projection.elements[13] = -y * projection.elements[5];

	return projection;
}

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

static CUTE_INLINE sg_filter s_wrap(CF_Filter filter)
{
	switch (filter) {
	case CF_FILTER_NEAREST: return SG_FILTER_NEAREST;
	case CF_FILTER_LINEAR:  return SG_FILTER_LINEAR;
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

CF_BackendType cf_query_backend()
{
	sg_backend backend = sg_query_backend();
	return (CF_BackendType)backend;
}

bool cf_query_pixel_format(CF_PixelFormat format, CF_PixelFormatOp op)
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

bool cf_query_device_feature(CF_DeviceFeature feature)
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

int cf_query_resource_limit(CF_ResourceLimit resource_limit)
{
	sg_limits sgl = sg_query_limits();
	int result = 0;
	switch (resource_limit) {
	case CF_RESOURCE_LIMIT_TEXTURE_DIMENSION:       result = sgl.max_image_size_2d; break;
	case CF_RESOURCE_LIMIT_VERTEX_ATTRIBUTE_MAX:    result = sgl.max_vertex_attrs; break;
	}
	return result;
}

CF_TextureParams cf_texture_defaults()
{
	CF_TextureParams params;
	params.pixel_format = CF_PIXELFORMAT_DEFAULT;
	params.filter = CF_FILTER_NEAREST;
	params.usage = CF_USAGE_TYPE_IMMUTABLE;
	params.wrap_u = CF_WRAP_MODE_DEFAULT;
	params.wrap_v = CF_WRAP_MODE_DEFAULT;
	params.width = 0;
	params.height = 0;
	return params;
}

CF_Texture cf_make_texture(CF_TextureParams texture_params)
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
	desc.min_filter = s_wrap(texture_params.filter);
	desc.mag_filter = s_wrap(texture_params.filter);
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

void cf_destroy_texture(CF_Texture texture)
{
	sg_image sgi = { (uint32_t)texture.id };
	sg_destroy_image(sgi);
}

void cf_update_texture(CF_Texture texture, void* data, int size)
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
	const sg_shader_desc* desc;
	sg_shader shd;
	int uniform_block_size;
	void* uniform_block;
};

CF_Shader cf_make_shader(CF_SokolShader sokol_shader)
{
	CF_ShaderInternal* shader = (CF_ShaderInternal*)CUTE_ALLOC(sizeof(CF_ShaderInternal));
	shader->table = sokol_shader;
	const sg_shader_desc* desc = shader->table.get_desc_fn(sg_query_backend());
	shader->desc = desc;
	shader->shd = sg_make_shader(desc);
	CF_Shader result;
	result.id = { (uint64_t)shader };
	return result;
}

void cf_destroy_shader(CF_Shader shader)
{
	CF_ShaderInternal* shader_internal = (CF_ShaderInternal*)shader.id;
	sg_destroy_shader(shader_internal->shd);
	CUTE_FREE(shader_internal);
}

CF_PassParams cf_pass_defaults()
{
	CF_PassParams params;
	params.name = NULL;
	params.color_op = CF_PASS_INIT_OP_CLEAR;
	params.color_value = cf_color_grey();
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
	sg_pass_action action;
	sg_pass pass;
	sg_pipeline pip;
	CF_MeshInternal* mesh;
};

CF_Pass cf_make_pass(CF_PassParams pass_params)
{
	CF_PassInternal* pass = (CF_PassInternal*)CUTE_CALLOC(sizeof(CF_PassInternal));
	pass->action.colors[0].action = s_wrap(pass_params.color_op);
	pass->action.colors[0].value = s_wrap(pass_params.color_value);
	pass->action.depth.action = s_wrap(pass_params.depth_op);
	pass->action.depth.value = pass_params.depth_value;
	pass->action.stencil.action = s_wrap(pass_params.stencil_op);
	pass->action.stencil.value = pass_params.stencil_value;
	sg_pass_desc desc;
	CUTE_MEMSET(&desc, 0, sizeof(desc));
	desc.color_attachments[0].image = { (uint32_t)pass_params.target.id };
	desc.depth_stencil_attachment.image = { (uint32_t)pass_params.depth_stencil.id };
	desc.label = pass_params.name;
	pass->pass = sg_make_pass(desc);
	CF_Pass result;
	result.id = (uint64_t)pass;
	return result;
}

void cf_destroy_pass(CF_Pass pass)
{
	CF_PassInternal* pass_internal = (CF_PassInternal*)pass.id;
	sg_destroy_pass(pass_internal->pass);
	CUTE_FREE(pass_internal);
}

struct CF_Buffer
{
	bool was_appended;
	int element_count;
	int size;
	int offset;
	int stride;
	sg_buffer handle;
};

struct CF_MeshInternal
{
	sg_usage usage;
	bool need_vertex_sync;
	bool need_index_sync;
	bool need_instance_sync;
	CF_Buffer vertices;
	CF_Buffer indices;
	CF_Buffer instances;
	int attribute_count;
	CF_VertexAttribute attributes[SG_MAX_VERTEX_ATTRIBUTES];
};

CF_Mesh cf_make_mesh(CF_UsageType usage_type, int vertex_buffer_size, int index_buffer_size, int instance_buffer_size)
{
	CF_MeshInternal* mesh = (CF_MeshInternal*)CUTE_CALLOC(sizeof(CF_MeshInternal));
	mesh->need_index_sync = true;
	mesh->vertices.size = vertex_buffer_size;
	mesh->indices.size = index_buffer_size;
	mesh->instances.size = instance_buffer_size;
	mesh->indices.stride = sizeof(uint32_t);
	CF_Mesh result = { (uint64_t)mesh };
	return result;
}

void cf_destroy_mesh(CF_Mesh mesh_handle)
{
	CF_MeshInternal* mesh = (CF_MeshInternal*)mesh_handle.id;
	if (mesh->vertices.handle.id) sg_destroy_buffer(mesh->vertices.handle);
	if (mesh->indices.handle.id) sg_destroy_buffer(mesh->indices.handle);
	if (mesh->instances.handle.id) sg_destroy_buffer(mesh->instances.handle);
	CUTE_FREE(mesh);
}

void cf_mesh_set_attributes(CF_Mesh mesh_handle, const CF_VertexAttribute* attributes, int attribute_count, int vertex_stride, int instance_stride)
{
	CF_MeshInternal* mesh = (CF_MeshInternal*)mesh_handle.id;
	attribute_count = min(attribute_count, SG_MAX_VERTEX_ATTRIBUTES);
	mesh->attribute_count = attribute_count;
	mesh->vertices.stride = vertex_stride;
	mesh->instances.stride = instance_stride;
	for (int i = 0; i < attribute_count; ++i) {
		mesh->attributes[i] = attributes[i];
		mesh->attributes[i].name = sintern(attributes[i].name);
	}
}

static void s_sync_vertex_buffer(CF_MeshInternal* mesh, void* data, int size)
{
	mesh->need_vertex_sync = false;
	if (mesh->vertices.handle.id) sg_destroy_buffer(mesh->vertices.handle);
	sg_buffer_desc desc = { };
	desc.size = mesh->vertices.size;
	desc.type = SG_BUFFERTYPE_VERTEXBUFFER;
	desc.usage = mesh->usage;
	desc.data.ptr = data;
	desc.data.size = size;
	mesh->vertices.handle = sg_make_buffer(desc);
	mesh->vertices.offset = 0;
}

void cf_mesh_update_vertex_data(CF_Mesh mesh_handle, void* data, int count)
{
	CF_MeshInternal* mesh = (CF_MeshInternal*)mesh_handle.id;
	int size = count * mesh->vertices.stride;
	if (size > mesh->vertices.size) {
		mesh->vertices.size = size;
		mesh->need_vertex_sync = true;
	}
	CUTE_ASSERT(mesh->attribute_count);
	if (mesh->need_vertex_sync) {
		s_sync_vertex_buffer(mesh, data, size);
	} else {
		sg_range range = { data, size };
		sg_update_buffer(mesh->vertices.handle, range);
	}
	mesh->vertices.element_count = count;
}

int cf_mesh_append_vertex_data(CF_Mesh mesh_handle, void* data, int append_count)
{
	CF_MeshInternal* mesh = (CF_MeshInternal*)mesh_handle.id;
	int size = append_count * mesh->vertices.stride;
	CUTE_ASSERT(mesh->attribute_count);
	CUTE_ASSERT(mesh->vertices.size >= size);
	if (mesh->need_vertex_sync) {
		s_sync_vertex_buffer(mesh, NULL, mesh->vertices.size);
	}
	sg_range range = { data, size };
	if (!sg_query_will_buffer_overflow(mesh->vertices.handle, size)) {
		int offset = sg_append_buffer(mesh->vertices.handle, range);
		mesh->vertices.offset = offset;
		mesh->vertices.element_count += append_count;
		mesh->vertices.was_appended = true;
		return offset;
	} else {
		return 0;
	}
}

bool cf_mesh_will_overflow_vertex_data(CF_Mesh mesh_handle, int append_count)
{
	CF_MeshInternal* mesh = (CF_MeshInternal*)mesh_handle.id;
	return sg_query_will_buffer_overflow(mesh->vertices.handle, append_count * mesh->vertices.stride);
}

static void s_sync_isntance_buffer(CF_MeshInternal* mesh, void* data, int size)
{
	mesh->need_instance_sync = false;
	if (mesh->vertices.handle.id) sg_destroy_buffer(mesh->vertices.handle);
	sg_buffer_desc desc = { };
	desc.size = mesh->instances.size;
	desc.type = SG_BUFFERTYPE_VERTEXBUFFER;
	desc.usage = mesh->usage;
	desc.data.ptr = data;
	desc.data.size = size;
	mesh->instances.handle = sg_make_buffer(desc);
	mesh->instances.offset = 0;
}

void cf_mesh_update_instance_data(CF_Mesh mesh_handle, void* data, int count)
{
	CF_MeshInternal* mesh = (CF_MeshInternal*)mesh_handle.id;
	int size = count * mesh->instances.stride;
	if (size > mesh->instances.size) {
		mesh->instances.size = size;
		mesh->need_instance_sync = true;
	}
	CUTE_ASSERT(mesh->attribute_count);
	if (mesh->need_instance_sync) {
		s_sync_isntance_buffer(mesh, data, size);
	} else {
		sg_range range = { data, size };
		sg_update_buffer(mesh->instances.handle, range);
	}
	mesh->instances.element_count = count;
}

int cf_mesh_append_instance_data(CF_Mesh mesh_handle, void* data, int append_count)
{
	CF_MeshInternal* mesh = (CF_MeshInternal*)mesh_handle.id;
	int size = append_count * mesh->instances.stride;
	CUTE_ASSERT(mesh->attribute_count);
	CUTE_ASSERT(mesh->instances.size >= size);
	if (mesh->need_instance_sync) {
		s_sync_isntance_buffer(mesh, NULL, mesh->instances.size);
	}
	sg_range range = { data, size };
	if (!sg_query_will_buffer_overflow(mesh->instances.handle, size)) {
		int offset = sg_append_buffer(mesh->instances.handle, range);
		mesh->instances.offset = offset;
		mesh->instances.element_count += append_count;
		mesh->instances.was_appended = true;
		return offset;
	} else {
		return 0;
	}
}

bool cf_mesh_will_overflow_instance_data(CF_Mesh mesh_handle, int append_count)
{
	CF_MeshInternal* mesh = (CF_MeshInternal*)mesh_handle.id;
	return sg_query_will_buffer_overflow(mesh->instances.handle, append_count * mesh->instances.stride);
}

static void s_sync_index_buffer(CF_MeshInternal* mesh, uint32_t* indices, int size)
{
	mesh->need_index_sync = false;
	if (mesh->indices.handle.id) sg_destroy_buffer(mesh->indices.handle);
	sg_buffer_desc desc = { };
	desc.size = mesh->indices.size;
	desc.type = SG_BUFFERTYPE_INDEXBUFFER;
	desc.usage = mesh->usage;
	desc.data.ptr = indices;
	desc.data.size = size;
	mesh->indices.handle = sg_make_buffer(desc);
	mesh->indices.offset = 0;
}

void cf_mesh_update_index_data(CF_Mesh mesh_handle, uint32_t* indices, int count)
{
	CF_MeshInternal* mesh = (CF_MeshInternal*)mesh_handle.id;
	int size = count * sizeof(uint32_t);
	if (size > mesh->indices.size) {
		mesh->indices.size = size;
		mesh->need_index_sync = true;
	}
	CUTE_ASSERT(mesh->attribute_count);
	if (mesh->need_index_sync) {
		s_sync_index_buffer(mesh, indices, size);
	} else {
		sg_range range = { indices, size };
		sg_update_buffer(mesh->indices.handle, range);
	}
	mesh->indices.element_count = count;
}

int cf_mesh_append_index_data(CF_Mesh mesh_handle, uint32_t* indices, int append_count)
{
	CF_MeshInternal* mesh = (CF_MeshInternal*)mesh_handle.id;
	int size = append_count * sizeof(uint32_t);
	CUTE_ASSERT(mesh->indices.size >= size);
	if (mesh->need_index_sync) {
		s_sync_index_buffer(mesh, NULL, mesh->indices.size);
	}
	sg_range range = { indices, size };
	if (!sg_query_will_buffer_overflow(mesh->indices.handle, size)) {
		int offset = sg_append_buffer(mesh->indices.handle, range);
		mesh->indices.offset = offset;
		mesh->indices.element_count += append_count;
		mesh->indices.was_appended = true;
		return offset;
	} else {
		return 0;
	}
}

bool cf_mesh_will_overflow_index_data(CF_Mesh mesh_handle, int append_count)
{
	CF_MeshInternal* mesh = (CF_MeshInternal*)mesh_handle.id;
	return sg_query_will_buffer_overflow(mesh->indices.handle, append_count * sizeof(uint32_t));
}

CF_RenderState cf_render_state_defaults()
{
	CF_RenderState state;
	state.blend.enabled = false;
	state.cull_mode = CF_CULL_MODE_FRONT;
	state.blend.pixel_format = CF_PIXELFORMAT_DEFAULT;
	state.blend.write_R_enabled = true;
	state.blend.write_G_enabled = true;
	state.blend.write_B_enabled = true;
	state.blend.write_A_enabled = true;
	state.blend.rgb_op = CF_BLEND_OP_ADD;
	state.blend.rgb_src_blend_factor = CF_BLENDFACTOR_ONE;
	state.blend.rgb_dst_blend_factor = CF_BLENDFACTOR_ZERO;
	state.blend.alpha_op = CF_BLEND_OP_ADD;
	state.blend.alpha_src_blend_factor = CF_BLENDFACTOR_ONE;
	state.blend.alpha_dst_blend_factor = CF_BLENDFACTOR_ZERO;
	state.depth_compare = CF_COMPARE_FUNCTION_ALWAYS;
	state.depth_write_enabled = false;
	state.stencil.enabled = false;
	state.stencil.read_mask = 0;
	state.stencil.write_mask = 0;
	state.stencil.reference = 0;
	state.stencil.front.compare = CF_COMPARE_FUNCTION_ALWAYS;
	state.stencil.front.fail_op = CF_STENCIL_OP_KEEP;
	state.stencil.front.depth_fail_op = CF_STENCIL_OP_KEEP;
	state.stencil.front.pass_op = CF_STENCIL_OP_KEEP;
	state.stencil.back.compare = CF_COMPARE_FUNCTION_ALWAYS;
	state.stencil.back.fail_op = CF_STENCIL_OP_KEEP;
	state.stencil.back.depth_fail_op = CF_STENCIL_OP_KEEP;
	state.stencil.back.pass_op = CF_STENCIL_OP_KEEP;
	return state;
}

struct CF_UniformInfo
{
	const char* block_name;
	const char* name;
	CF_UniformType type;
	int array_length;
	void* data;
	int size;
};

struct CF_MaterialTex
{
	const char* name;
	CF_Texture handle;
};

struct CF_MaterialState
{
	array<CF_UniformInfo> uniforms;
	array<CF_MaterialTex> textures;
};

struct CF_MaterialInternal
{
	CF_RenderState state;
	CF_MaterialState vs;
	CF_MaterialState fs;
	CF_Arena uniform_arena;
	CF_Arena block_arena;
};

CF_Material cf_make_material()
{
	CF_MaterialInternal* material = CUTE_NEW(CF_MaterialInternal);
	cf_arena_init(&material->uniform_arena, 4, 1024);
	cf_arena_init(&material->block_arena, 4, 1024);
	material->state = cf_render_state_defaults();
	CF_Material result = { (uint64_t)material };
	return result;
}

void cf_destroy_material(CF_Material material_handle)
{
	CF_MaterialInternal* material = (CF_MaterialInternal*)material_handle.id;
	material->~CF_MaterialInternal();
	CUTE_FREE(material);
}

void cf_material_set_render_state(CF_Material material_handle, CF_RenderState render_state)
{
	CF_MaterialInternal* material = (CF_MaterialInternal*)material_handle.id;
	material->state = render_state;
}

static void s_material_set_texture(CF_MaterialState* state, const char* name, CF_Texture texture)
{
	bool found = false;
	for (int i = 0; i < state->textures.count(); ++i) {
		if (state->textures[i].name == name) {
			state->textures[i].handle = texture;
			found = false;
			break;
		}
	}
	if (!found) {
		CF_MaterialTex tex;
		tex.name = name;
		tex.handle = texture;
		state->textures.add(tex);
	}
}

void cf_material_set_texture_vs(CF_Material material_handle, const char* name, CF_Texture texture)
{
	CF_MaterialInternal* material = (CF_MaterialInternal*)material_handle.id;
	name = sintern(name);
	s_material_set_texture(&material->vs, name, texture);
}

void cf_material_set_texture_fs(CF_Material material_handle, const char* name, CF_Texture texture)
{
	CF_MaterialInternal* material = (CF_MaterialInternal*)material_handle.id;
	name = sintern(name);
	s_material_set_texture(&material->fs, name, texture);
}

static int s_uniform_size(CF_UniformType type)
{
	switch (type) {
	case CF_UNIFORM_TYPE_FLOAT:  return 4;
	case CF_UNIFORM_TYPE_FLOAT2: return 8;
	case CF_UNIFORM_TYPE_FLOAT4: return 16;
	case CF_UNIFORM_TYPE_INT:    return 4;
	case CF_UNIFORM_TYPE_INT2:   return 8;
	case CF_UNIFORM_TYPE_INT4:   return 16;
	case CF_UNIFORM_TYPE_MAT4:   return 64;
	}
}

static void s_material_set_uniform(CF_Arena* arena, CF_MaterialState* state, const char* block_name, const char* name, void* data, CF_UniformType type, int array_length)
{
	if (array_length <= 0) array_length = 1;
	CF_UniformInfo* uniform = NULL;
	for (int i = 0; i < state->uniforms.count(); ++i) {
		if (state->uniforms[i].block_name == block_name && state->uniforms[i].name == name) {
			uniform = state->uniforms + i;
			break;
		}
	}
	int size = s_uniform_size(type) * array_length;
	if (!uniform) {
		uniform = &state->uniforms.add();
		uniform->block_name = block_name;
		uniform->name = name;
		uniform->data = cf_arena_alloc(arena, size);
		uniform->size = size;
		uniform->type = type;
		uniform->array_length = array_length;
	}
	CUTE_ASSERT(uniform->type == type);
	CUTE_ASSERT(uniform->array_length == array_length);
	CUTE_MEMCPY(uniform->data, data, size);
}

void cf_material_set_uniform_vs(CF_Material material_handle, const char* block_name, const char* name, void* data, CF_UniformType type, int array_length)
{
	CF_MaterialInternal* material = (CF_MaterialInternal*)material_handle.id;
	block_name = sintern(block_name);
	name = sintern(name);
	s_material_set_uniform(&material->uniform_arena, &material->vs, block_name, name, data, type, array_length);
}

void cf_material_set_uniform_fs(CF_Material material_handle, const char* block_name, const char* name, void* data, CF_UniformType type, int array_length)
{
	CF_MaterialInternal* material = (CF_MaterialInternal*)material_handle.id;
	block_name = sintern(block_name);
	name = sintern(name);
	s_material_set_uniform(&material->uniform_arena, &material->fs, block_name, name, data, type, array_length);
}

static CF_PassInternal* s_pass = NULL;

void cf_begin_pass(CF_Pass pass_handle)
{
	CF_PassInternal* pass = (CF_PassInternal*)pass_handle.id;
	CUTE_ASSERT(!s_pass);
	s_pass = pass;
	sg_begin_pass(pass->pass, &pass->action);
}

void cf_apply_viewport(float x, float y, float width, float height)
{
	sg_apply_viewportf(x, y, width, height, false);
}

void cf_apply_scissor(float x, float y, float width, float height)
{
	sg_apply_scissor_rectf(x, y, width, height, false);
}

void cf_apply_mesh(CF_Mesh mesh_handle)
{
	CUTE_ASSERT(s_pass);
	CF_MeshInternal* mesh = (CF_MeshInternal*)mesh_handle.id;
	s_pass->mesh = mesh;
}

static void s_copy_uniforms(CF_Arena* arena, CF_SokolShader table, CF_MaterialState* mstate, sg_shader_stage stage)
{
	// Create any required uniform blocks for all uniforms matching between which uniforms
	// the material has and the shader needs.
	void* ub_ptrs[SG_MAX_SHADERSTAGE_UBS] = { };
	int ub_sizes[SG_MAX_SHADERSTAGE_UBS] = { };
	for (int i = 0; i < mstate->uniforms.count(); ++i) {
		CF_UniformInfo uniform = mstate->uniforms[i];
		int slot = table.get_uniformblock_slot(stage, uniform.block_name);
		if (slot >= 0) {
			if (!ub_ptrs[slot]) {
				// Create temporary space for a uniform block.
				int size = table.get_uniformblock_size(stage, uniform.block_name);
				void* block = cf_arena_alloc(arena, size);
				CUTE_MEMSET(block, 0, size);
				ub_ptrs[slot] = block;
				ub_sizes[slot] = size;
			}
			// Copy a single matched uniform into the block.
			void* block = ub_ptrs[slot];
			int offset = table.get_uniform_offset(stage, uniform.block_name, uniform.name);
			void* dst = (void*)(((uintptr_t)block) + offset);
			CUTE_MEMCPY(dst, uniform.data, uniform.size);
		}
	}
	// Send each fully constructed uniform block to the GPU.
	// Any missing uniforms have been MEMSET to 0.
	for (int i = 0; i < SG_MAX_SHADERSTAGE_UBS; ++i) {
		if (ub_ptrs[i]) {
			sg_range range = { ub_ptrs[i], ub_sizes[i] };
			sg_apply_uniforms(stage, i, range);
		}
	}
	cf_arena_reset(arena);
}

void cf_apply_shader(CF_Shader shader_handle, CF_Material material_handle)
{
	// TODO - Somehow cache results from all the get_*** callbacks.

	CUTE_ASSERT(s_pass);
	CF_MeshInternal* mesh = s_pass->mesh;
	CF_MaterialInternal* material = (CF_MaterialInternal*)material_handle.id;
	CF_ShaderInternal* shader = (CF_ShaderInternal*)shader_handle.id;
	CF_SokolShader table = shader->table;

	// Align all buffers, and setup any matched texture names from the material to
	// the shader's expected textures.
	sg_bindings bind = { };
	bind.vertex_buffers[CF_VERTEX_BUFFER_SLOT] = mesh->vertices.handle;
	bind.vertex_buffer_offsets[CF_VERTEX_BUFFER_SLOT] = mesh->vertices.offset;
	bind.vertex_buffers[CF_INSTANCE_BUFFER_SLOT] = mesh->instances.handle;
	bind.vertex_buffer_offsets[CF_INSTANCE_BUFFER_SLOT] = mesh->instances.offset;
	bind.index_buffer = mesh->indices.handle;
	bind.index_buffer_offset = mesh->indices.offset;
	for (int i = 0; i < material->vs.textures.count(); ++i) {
		int slot = table.get_image_slot(SG_SHADERSTAGE_VS, material->vs.textures[i].name);
		if (slot >= 0) {
			bind.vs_images[slot].id = (uint32_t)material->vs.textures[i].handle.id;
		}
	}
	for (int i = 0; i < material->fs.textures.count(); ++i) {
		int slot = table.get_image_slot(SG_SHADERSTAGE_FS, material->fs.textures[i].name);
		if (slot >= 0) {
			bind.fs_images[slot].id = (uint32_t)material->fs.textures[i].handle.id;
		}
	}
	sg_apply_bindings(bind);

	// Apply the render state and vertex attributes.
	// Match any attributes the shader needs to the attributes in the material.
	CUTE_ASSERT(mesh->attribute_count < SG_MAX_VERTEX_ATTRIBUTES);
	sg_layout_desc layout = { };
	for (int i = 0; i < mesh->attribute_count; ++i) {
		CF_VertexAttribute attr = mesh->attributes[i];
		int slot = table.get_attr_slot(attr.name);
		if (slot >= 0) {
			layout.attrs[slot].buffer_index = attr.step_type == SG_VERTEXSTEP_PER_VERTEX ? CF_VERTEX_BUFFER_SLOT : CF_INSTANCE_BUFFER_SLOT;
			layout.attrs[slot].format = s_wrap(attr.format);
			layout.attrs[slot].offset = attr.offset;
		}
	}
	layout.buffers[CF_VERTEX_BUFFER_SLOT].stride = mesh->vertices.stride;
	bool has_instance_data = mesh->instances.size > 0;
	if (has_instance_data) {
		layout.buffers[CF_INSTANCE_BUFFER_SLOT].step_func = SG_VERTEXSTEP_PER_INSTANCE;
		layout.buffers[CF_INSTANCE_BUFFER_SLOT].stride = mesh->instances.stride;
	}

	// Copy over render state from the material into the pipeline.
	sg_pipeline_desc desc = { };
	CF_RenderState* state = &material->state;
	desc.shader = shader->shd;
	desc.layout = layout;
	desc.depth.compare = s_wrap(state->depth_compare);
	desc.depth.write_enabled = state->depth_write_enabled;
	desc.stencil.enabled = state->stencil.enabled;
	desc.stencil.read_mask = state->stencil.read_mask;
	desc.stencil.write_mask = state->stencil.write_mask;
	desc.stencil.ref = state->stencil.reference;
	desc.stencil.front.compare = s_wrap(state->stencil.front.compare);
	desc.stencil.front.fail_op = s_wrap(state->stencil.front.fail_op);
	desc.stencil.front.depth_fail_op = s_wrap(state->stencil.front.depth_fail_op);
	desc.stencil.front.pass_op = s_wrap(state->stencil.front.pass_op);
	desc.stencil.back.compare = s_wrap(state->stencil.back.compare);
	desc.stencil.back.fail_op = s_wrap(state->stencil.back.fail_op);
	desc.stencil.back.depth_fail_op = s_wrap(state->stencil.back.depth_fail_op);
	desc.stencil.back.pass_op = s_wrap(state->stencil.back.pass_op);
	desc.color_count = 1;
	int mask_r = (int)state->blend.write_R_enabled << 0;
	int mask_g = (int)state->blend.write_R_enabled << 1;
	int mask_b = (int)state->blend.write_R_enabled << 2;
	int mask_a = (int)state->blend.write_R_enabled << 3;
	desc.colors[0].write_mask = (sg_color_mask)(mask_r | mask_g | mask_b | mask_a);
	desc.colors[0].blend.enabled = state->blend.enabled;
	desc.colors[0].blend.src_factor_rgb = s_wrap(state->blend.rgb_src_blend_factor);
	desc.colors[0].blend.dst_factor_rgb = s_wrap(state->blend.rgb_dst_blend_factor);
	desc.colors[0].blend.op_rgb = s_wrap(state->blend.rgb_op);
	desc.colors[0].blend.src_factor_alpha = s_wrap(state->blend.alpha_src_blend_factor);
	desc.colors[0].blend.dst_factor_alpha = s_wrap(state->blend.alpha_dst_blend_factor);
	desc.colors[0].blend.op_alpha = s_wrap(state->blend.alpha_op);
	if (mesh->indices.size > 0) desc.index_type = SG_INDEXTYPE_UINT32;
	desc.cull_mode = s_wrap(state->cull_mode);

	// Apply the pipeline.
	sg_pipeline pip = sg_make_pipeline(desc);
	sg_apply_pipeline(pip);
	s_pass->pip = pip;

	// Copy over uniform data.
	s_copy_uniforms(&material->block_arena, table, &material->vs, SG_SHADERSTAGE_VS);
	s_copy_uniforms(&material->block_arena, table, &material->fs, SG_SHADERSTAGE_FS);
	cf_arena_reset(&material->block_arena);
}

void cf_draw_elements()
{
	CF_MeshInternal* mesh = s_pass->mesh;
	sg_draw(0, mesh->vertices.element_count, mesh->instances.element_count);
}

void cf_end_pass()
{
	CUTE_ASSERT(s_pass);
	sg_end_pass();
	sg_destroy_pipeline(s_pass->pip);
	CF_MeshInternal* mesh = s_pass->mesh;
	if (mesh->vertices.was_appended) {
		mesh->vertices.was_appended = false;
		mesh->vertices.element_count = 0;
	}
	if (mesh->indices.was_appended) {
		mesh->indices.was_appended = false;
		mesh->indices.element_count = 0;
	}
	if (mesh->instances.was_appended) {
		mesh->instances.was_appended = false;
		mesh->instances.element_count = 0;
	}
	s_pass = NULL;
}

void cf_commit()
{
	sg_commit();
}
