/*
	Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

#ifndef CF_GRAPHICS_INTERNAL_H
#define CF_GRAPHICS_INTERNAL_H

#include <SDL3/SDL.h>
#include <cute_array.h>

CF_INLINE SDL_GPUTextureCreateInfo SDL_GPUTextureCreateInfoDefaults(int w, int h)
{
	SDL_GPUTextureCreateInfo createInfo;
	CF_MEMSET(&createInfo, 0, sizeof(createInfo));
	createInfo.width = (int)w;
	createInfo.height = (int)h;
	createInfo.type = SDL_GPU_TEXTURETYPE_2D;
	createInfo.layer_count_or_depth = 1;
	createInfo.num_levels = 1;
	createInfo.sample_count = SDL_GPU_SAMPLECOUNT_1;
	createInfo.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
	createInfo.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER | SDL_GPU_TEXTUREUSAGE_COLOR_TARGET;
	return createInfo;
}

struct CF_CanvasInternal
{
	CF_Texture cf_texture;
	CF_Texture cf_depth_stencil;
	SDL_GPUTexture* texture;
	SDL_GPUSampler* sampler;
	SDL_GPUTexture* depth_stencil;

	bool clear;

	// These get set by cf_apply_* functions.
	struct CF_MeshInternal* mesh;
	SDL_GPUGraphicsPipeline* pip;
	SDL_GPURenderPass* pass;
};

struct CF_TextureInternal
{
	int w, h;
	SDL_GPUFilter filter;
	SDL_GPUTexture* tex;
	SDL_GPUTransferBuffer* buf;
	SDL_GPUSampler* sampler;
	SDL_GPUTextureFormat format;
};

CF_INLINE SDL_GPUSamplerCreateInfo SDL_GPUSamplerCreateInfoDefaults()
{
	SDL_GPUSamplerCreateInfo samplerInfo;
	CF_MEMSET(&samplerInfo, 0, sizeof(samplerInfo));
	samplerInfo.min_filter = SDL_GPU_FILTER_NEAREST;
	samplerInfo.mag_filter = SDL_GPU_FILTER_NEAREST;
	samplerInfo.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_LINEAR;
	samplerInfo.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_REPEAT;
	samplerInfo.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_REPEAT;
	samplerInfo.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_REPEAT;
	samplerInfo.mip_lod_bias = 0.0f;
	samplerInfo.enable_anisotropy = SDL_FALSE;
	samplerInfo.max_anisotropy = 1.0f;
	samplerInfo.enable_compare = SDL_FALSE;
	samplerInfo.compare_op = SDL_GPU_COMPAREOP_ALWAYS;
	samplerInfo.min_lod = 0.0f;
	samplerInfo.max_lod = FLT_MAX;
	return samplerInfo;
}

CF_INLINE SDL_GPUTextureRegion SDL_GPUTextureRegionDefaults(CF_TextureInternal* tex, int w, int h)
{
	SDL_GPUTextureRegion region;
	CF_MEMSET(&region, 0, sizeof(region));
	region.texture = tex->tex;
	region.w = (Uint32)w;
	region.h = (Uint32)h;
	region.d = 1;
	return region;
}

CF_INLINE SDL_GPUCompareOp s_wrap(CF_CompareFunction compare_function)
{
	switch (compare_function)
	{
	case CF_COMPARE_FUNCTION_ALWAYS:                return SDL_GPU_COMPAREOP_ALWAYS;
	case CF_COMPARE_FUNCTION_NEVER:                 return SDL_GPU_COMPAREOP_NEVER;
	case CF_COMPARE_FUNCTION_LESS_THAN:             return SDL_GPU_COMPAREOP_LESS;
	case CF_COMPARE_FUNCTION_EQUAL:                 return SDL_GPU_COMPAREOP_EQUAL;
	case CF_COMPARE_FUNCTION_NOT_EQUAL:             return SDL_GPU_COMPAREOP_NOT_EQUAL;
	case CF_COMPARE_FUNCTION_LESS_THAN_OR_EQUAL:    return SDL_GPU_COMPAREOP_LESS_OR_EQUAL;
	case CF_COMPARE_FUNCTION_GREATER_THAN:          return SDL_GPU_COMPAREOP_GREATER;
	case CF_COMPARE_FUNCTION_GREATER_THAN_OR_EQUAL: return SDL_GPU_COMPAREOP_GREATER_OR_EQUAL;
	default:                                        return SDL_GPU_COMPAREOP_ALWAYS;
	}
}

CF_INLINE SDL_GPUCullMode s_wrap(CF_CullMode mode)
{
    switch (mode)
    {
    case CF_CULL_MODE_NONE:  return SDL_GPU_CULLMODE_NONE;
    case CF_CULL_MODE_FRONT: return SDL_GPU_CULLMODE_FRONT;
    case CF_CULL_MODE_BACK:  return SDL_GPU_CULLMODE_BACK;
    default:              return SDL_GPU_CULLMODE_NONE;
    }
}

CF_INLINE SDL_GPUStencilOp s_wrap(CF_StencilOp stencil_op)
{
	switch (stencil_op)
	{
	case CF_STENCIL_OP_KEEP:            return SDL_GPU_STENCILOP_KEEP;
	case CF_STENCIL_OP_ZERO:            return SDL_GPU_STENCILOP_ZERO;
	case CF_STENCIL_OP_REPLACE:         return SDL_GPU_STENCILOP_REPLACE;
	case CF_STENCIL_OP_INCREMENT_CLAMP: return SDL_GPU_STENCILOP_INCREMENT_AND_CLAMP;
	case CF_STENCIL_OP_DECREMENT_CLAMP: return SDL_GPU_STENCILOP_DECREMENT_AND_CLAMP;
	case CF_STENCIL_OP_INVERT:          return SDL_GPU_STENCILOP_INVERT;
	case CF_STENCIL_OP_INCREMENT_WRAP:  return SDL_GPU_STENCILOP_INCREMENT_AND_WRAP;
	case CF_STENCIL_OP_DECREMENT_WRAP:  return SDL_GPU_STENCILOP_DECREMENT_AND_WRAP;
	default:                            return SDL_GPU_STENCILOP_KEEP;
	}
}

CF_INLINE SDL_GPUBlendOp s_wrap(CF_BlendOp blend_op)
{
	switch (blend_op)
	{
	case CF_BLEND_OP_ADD:              return SDL_GPU_BLENDOP_ADD;
	case CF_BLEND_OP_SUBTRACT:         return SDL_GPU_BLENDOP_SUBTRACT;
	case CF_BLEND_OP_REVERSE_SUBTRACT: return SDL_GPU_BLENDOP_REVERSE_SUBTRACT;
	case CF_BLEND_OP_MIN:              return SDL_GPU_BLENDOP_MIN;
	case CF_BLEND_OP_MAX:              return SDL_GPU_BLENDOP_MAX;
	default:                           return SDL_GPU_BLENDOP_ADD;
	}
}

CF_INLINE SDL_GPUBlendFactor s_wrap(CF_BlendFactor factor)
{
	switch (factor) {
	case CF_BLENDFACTOR_ZERO:                    return SDL_GPU_BLENDFACTOR_ZERO;
	case CF_BLENDFACTOR_ONE:                     return SDL_GPU_BLENDFACTOR_ONE;
	case CF_BLENDFACTOR_SRC_COLOR:               return SDL_GPU_BLENDFACTOR_SRC_COLOR;
	case CF_BLENDFACTOR_ONE_MINUS_SRC_COLOR:     return SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_COLOR;
	case CF_BLENDFACTOR_DST_COLOR:               return SDL_GPU_BLENDFACTOR_DST_COLOR;
	case CF_BLENDFACTOR_ONE_MINUS_DST_COLOR:     return SDL_GPU_BLENDFACTOR_ONE_MINUS_DST_COLOR;
	case CF_BLENDFACTOR_SRC_ALPHA:               return SDL_GPU_BLENDFACTOR_SRC_ALPHA;
	case CF_BLENDFACTOR_ONE_MINUS_SRC_ALPHA:     return SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
	case CF_BLENDFACTOR_DST_ALPHA:               return SDL_GPU_BLENDFACTOR_DST_ALPHA;
	case CF_BLENDFACTOR_ONE_MINUS_DST_ALPHA:     return SDL_GPU_BLENDFACTOR_ONE_MINUS_DST_ALPHA;
	case CF_BLENDFACTOR_CONSTANT_COLOR:          return SDL_GPU_BLENDFACTOR_CONSTANT_COLOR;
	case CF_BLENDFACTOR_ONE_MINUS_CONSTANT_COLOR:return SDL_GPU_BLENDFACTOR_ONE_MINUS_CONSTANT_COLOR;
	case CF_BLENDFACTOR_SRC_ALPHA_SATURATE:      return SDL_GPU_BLENDFACTOR_SRC_ALPHA_SATURATE;
	default:                                     return SDL_GPU_BLENDFACTOR_ZERO;
	}
}

CF_INLINE SDL_GPUShaderStage s_wrap(CF_ShaderStage stage)
{
	switch (stage) {
	case CF_SHADER_STAGE_VERTEX: return SDL_GPU_SHADERSTAGE_VERTEX;
	case CF_SHADER_STAGE_FRAGMENT: return SDL_GPU_SHADERSTAGE_FRAGMENT;
	default: return SDL_GPU_SHADERSTAGE_VERTEX;
	}
}

CF_INLINE SDL_GPUTextureFormat s_wrap(CF_PixelFormat format)
{
	switch (format)
	{
	case CF_PIXEL_FORMAT_INVALID:                 return SDL_GPU_TEXTUREFORMAT_INVALID;
	case CF_PIXEL_FORMAT_A8_UNORM:                return SDL_GPU_TEXTUREFORMAT_A8_UNORM;
	case CF_PIXEL_FORMAT_R8_UNORM:                return SDL_GPU_TEXTUREFORMAT_R8_UNORM;
	case CF_PIXEL_FORMAT_R8G8_UNORM:              return SDL_GPU_TEXTUREFORMAT_R8G8_UNORM;
	case CF_PIXEL_FORMAT_R8G8B8A8_UNORM:          return SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
	case CF_PIXEL_FORMAT_R16_UNORM:               return SDL_GPU_TEXTUREFORMAT_R16_UNORM;
	case CF_PIXEL_FORMAT_R16G16_UNORM:            return SDL_GPU_TEXTUREFORMAT_R16G16_UNORM;
	case CF_PIXEL_FORMAT_R16G16B16A16_UNORM:      return SDL_GPU_TEXTUREFORMAT_R16G16B16A16_UNORM;
	case CF_PIXEL_FORMAT_R10G10B10A2_UNORM:       return SDL_GPU_TEXTUREFORMAT_R10G10B10A2_UNORM;
	case CF_PIXEL_FORMAT_B5G6R5_UNORM:            return SDL_GPU_TEXTUREFORMAT_B5G6R5_UNORM;
	case CF_PIXEL_FORMAT_B5G5R5A1_UNORM:          return SDL_GPU_TEXTUREFORMAT_B5G5R5A1_UNORM;
	case CF_PIXEL_FORMAT_B4G4R4A4_UNORM:          return SDL_GPU_TEXTUREFORMAT_B4G4R4A4_UNORM;
	case CF_PIXEL_FORMAT_B8G8R8A8_UNORM:          return SDL_GPU_TEXTUREFORMAT_B8G8R8A8_UNORM;
	case CF_PIXEL_FORMAT_BC1_RGBA_UNORM:          return SDL_GPU_TEXTUREFORMAT_BC1_RGBA_UNORM;
	case CF_PIXEL_FORMAT_BC2_RGBA_UNORM:          return SDL_GPU_TEXTUREFORMAT_BC2_RGBA_UNORM;
	case CF_PIXEL_FORMAT_BC3_RGBA_UNORM:          return SDL_GPU_TEXTUREFORMAT_BC3_RGBA_UNORM;
	case CF_PIXEL_FORMAT_BC4_R_UNORM:             return SDL_GPU_TEXTUREFORMAT_BC4_R_UNORM;
	case CF_PIXEL_FORMAT_BC5_RG_UNORM:            return SDL_GPU_TEXTUREFORMAT_BC5_RG_UNORM;
	case CF_PIXEL_FORMAT_BC7_RGBA_UNORM:          return SDL_GPU_TEXTUREFORMAT_BC7_RGBA_UNORM;
	case CF_PIXEL_FORMAT_BC6H_RGB_FLOAT:          return SDL_GPU_TEXTUREFORMAT_BC6H_RGB_FLOAT;
	case CF_PIXEL_FORMAT_BC6H_RGB_UFLOAT:         return SDL_GPU_TEXTUREFORMAT_BC6H_RGB_UFLOAT;
	case CF_PIXEL_FORMAT_R8_SNORM:                return SDL_GPU_TEXTUREFORMAT_R8_SNORM;
	case CF_PIXEL_FORMAT_R8G8_SNORM:              return SDL_GPU_TEXTUREFORMAT_R8G8_SNORM;
	case CF_PIXEL_FORMAT_R8G8B8A8_SNORM:          return SDL_GPU_TEXTUREFORMAT_R8G8B8A8_SNORM;
	case CF_PIXEL_FORMAT_R16_SNORM:               return SDL_GPU_TEXTUREFORMAT_R16_SNORM;
	case CF_PIXEL_FORMAT_R16G16_SNORM:            return SDL_GPU_TEXTUREFORMAT_R16G16_SNORM;
	case CF_PIXEL_FORMAT_R16G16B16A16_SNORM:      return SDL_GPU_TEXTUREFORMAT_R16G16B16A16_SNORM;
	case CF_PIXEL_FORMAT_R16_FLOAT:               return SDL_GPU_TEXTUREFORMAT_R16_FLOAT;
	case CF_PIXEL_FORMAT_R16G16_FLOAT:            return SDL_GPU_TEXTUREFORMAT_R16G16_FLOAT;
	case CF_PIXEL_FORMAT_R16G16B16A16_FLOAT:      return SDL_GPU_TEXTUREFORMAT_R16G16B16A16_FLOAT;
	case CF_PIXEL_FORMAT_R32_FLOAT:               return SDL_GPU_TEXTUREFORMAT_R32_FLOAT;
	case CF_PIXEL_FORMAT_R32G32_FLOAT:            return SDL_GPU_TEXTUREFORMAT_R32G32_FLOAT;
	case CF_PIXEL_FORMAT_R32G32B32A32_FLOAT:      return SDL_GPU_TEXTUREFORMAT_R32G32B32A32_FLOAT;
	case CF_PIXEL_FORMAT_R11G11B10_UFLOAT:        return SDL_GPU_TEXTUREFORMAT_R11G11B10_UFLOAT;
	case CF_PIXEL_FORMAT_R8_UINT:                 return SDL_GPU_TEXTUREFORMAT_R8_UINT;
	case CF_PIXEL_FORMAT_R8G8_UINT:               return SDL_GPU_TEXTUREFORMAT_R8G8_UINT;
	case CF_PIXEL_FORMAT_R8G8B8A8_UINT:           return SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UINT;
	case CF_PIXEL_FORMAT_R16_UINT:                return SDL_GPU_TEXTUREFORMAT_R16_UINT;
	case CF_PIXEL_FORMAT_R16G16_UINT:             return SDL_GPU_TEXTUREFORMAT_R16G16_UINT;
	case CF_PIXEL_FORMAT_R16G16B16A16_UINT:       return SDL_GPU_TEXTUREFORMAT_R16G16B16A16_UINT;
	case CF_PIXEL_FORMAT_R8_INT:                  return SDL_GPU_TEXTUREFORMAT_R8_INT;
	case CF_PIXEL_FORMAT_R8G8_INT:                return SDL_GPU_TEXTUREFORMAT_R8G8_INT;
	case CF_PIXEL_FORMAT_R8G8B8A8_INT:            return SDL_GPU_TEXTUREFORMAT_R8G8B8A8_INT;
	case CF_PIXEL_FORMAT_R16_INT:                 return SDL_GPU_TEXTUREFORMAT_R16_INT;
	case CF_PIXEL_FORMAT_R16G16_INT:              return SDL_GPU_TEXTUREFORMAT_R16G16_INT;
	case CF_PIXEL_FORMAT_R16G16B16A16_INT:        return SDL_GPU_TEXTUREFORMAT_R16G16B16A16_INT;
	case CF_PIXEL_FORMAT_R8G8B8A8_UNORM_SRGB:     return SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM_SRGB;
	case CF_PIXEL_FORMAT_B8G8R8A8_UNORM_SRGB:     return SDL_GPU_TEXTUREFORMAT_B8G8R8A8_UNORM_SRGB;
	case CF_PIXEL_FORMAT_BC1_RGBA_UNORM_SRGB:     return SDL_GPU_TEXTUREFORMAT_BC1_RGBA_UNORM_SRGB;
	case CF_PIXEL_FORMAT_BC2_RGBA_UNORM_SRGB:     return SDL_GPU_TEXTUREFORMAT_BC2_RGBA_UNORM_SRGB;
	case CF_PIXEL_FORMAT_BC3_RGBA_UNORM_SRGB:     return SDL_GPU_TEXTUREFORMAT_BC3_RGBA_UNORM_SRGB;
	case CF_PIXEL_FORMAT_BC7_RGBA_UNORM_SRGB:     return SDL_GPU_TEXTUREFORMAT_BC7_RGBA_UNORM_SRGB;
	case CF_PIXEL_FORMAT_D16_UNORM:               return SDL_GPU_TEXTUREFORMAT_D16_UNORM;
	case CF_PIXEL_FORMAT_D24_UNORM:               return SDL_GPU_TEXTUREFORMAT_D24_UNORM;
	case CF_PIXEL_FORMAT_D32_FLOAT:               return SDL_GPU_TEXTUREFORMAT_D32_FLOAT;
	case CF_PIXEL_FORMAT_D24_UNORM_S8_UINT:       return SDL_GPU_TEXTUREFORMAT_D24_UNORM_S8_UINT;
	case CF_PIXEL_FORMAT_D32_FLOAT_S8_UINT:       return SDL_GPU_TEXTUREFORMAT_D32_FLOAT_S8_UINT;
	default:                                      return SDL_GPU_TEXTUREFORMAT_INVALID;
	}
}

CF_INLINE SDL_GPUFilter s_wrap(CF_Filter filter)
{
	switch (filter) {
	default: return SDL_GPU_FILTER_NEAREST;
	case CF_FILTER_NEAREST: return SDL_GPU_FILTER_NEAREST;
	case CF_FILTER_LINEAR: return SDL_GPU_FILTER_LINEAR;
	}
}

CF_INLINE SDL_GPUSamplerAddressMode s_wrap(CF_WrapMode mode)
{
	switch (mode)
	{
	case CF_WRAP_MODE_REPEAT:           return SDL_GPU_SAMPLERADDRESSMODE_REPEAT;
	case CF_WRAP_MODE_CLAMP_TO_EDGE:    return SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
	case CF_WRAP_MODE_MIRRORED_REPEAT:  return SDL_GPU_SAMPLERADDRESSMODE_MIRRORED_REPEAT;
	default:                            return SDL_GPU_SAMPLERADDRESSMODE_REPEAT;
	}
}

typedef enum CF_ShaderInputFormat
{
	CF_SHADER_INPUT_FORMAT_UNKNOWN,
	CF_SHADER_INPUT_FORMAT_UINT,
	CF_SHADER_INPUT_FORMAT_INT,
	CF_SHADER_INPUT_FORMAT_FLOAT,
	CF_SHADER_INPUT_FORMAT_UVEC2,
	CF_SHADER_INPUT_FORMAT_IVEC2,
	CF_SHADER_INPUT_FORMAT_VEC2,
	CF_SHADER_INPUT_FORMAT_UVEC3,
	CF_SHADER_INPUT_FORMAT_IVEC3,
	CF_SHADER_INPUT_FORMAT_VEC3,
	CF_SHADER_INPUT_FORMAT_UVEC4,
	CF_SHADER_INPUT_FORMAT_IVEC4,
	CF_SHADER_INPUT_FORMAT_VEC4,
} CF_ShaderInputFormat;

CF_INLINE bool s_is_compatible(CF_ShaderInputFormat input_format, CF_VertexFormat vertex_format)
{
	switch (input_format)
	{
	case CF_SHADER_INPUT_FORMAT_UINT:
		return vertex_format == CF_VERTEX_FORMAT_UINT;

	case CF_SHADER_INPUT_FORMAT_FLOAT:
		return vertex_format == CF_VERTEX_FORMAT_FLOAT;

	case CF_SHADER_INPUT_FORMAT_VEC2:
		return vertex_format == CF_VERTEX_FORMAT_FLOAT2;

	case CF_SHADER_INPUT_FORMAT_VEC3:
		return vertex_format == CF_VERTEX_FORMAT_FLOAT3;

	case CF_SHADER_INPUT_FORMAT_VEC4:
		return vertex_format == CF_VERTEX_FORMAT_FLOAT4 || vertex_format == CF_VERTEX_FORMAT_UBYTE4_NORM || vertex_format == CF_VERTEX_FORMAT_UBYTE4;

	case CF_SHADER_INPUT_FORMAT_UVEC4:
		return vertex_format == CF_VERTEX_FORMAT_UBYTE4_NORM || vertex_format == CF_VERTEX_FORMAT_UBYTE4;

	case CF_SHADER_INPUT_FORMAT_IVEC4:
		return vertex_format == CF_VERTEX_FORMAT_SHORT4 || vertex_format == CF_VERTEX_FORMAT_SHORT4_NORM;

	case CF_SHADER_INPUT_FORMAT_IVEC2:
		return vertex_format == CF_VERTEX_FORMAT_SHORT2 || vertex_format == CF_VERTEX_FORMAT_SHORT2_NORM;

	case CF_SHADER_INPUT_FORMAT_UVEC2:
		return vertex_format == CF_VERTEX_FORMAT_HALF2;

	// Not supported.
	case CF_SHADER_INPUT_FORMAT_UVEC3:
	case CF_SHADER_INPUT_FORMAT_IVEC3:
	case CF_SHADER_INPUT_FORMAT_UNKNOWN:
	default:
		return false;
	}
}

CF_INLINE SDL_GPUVertexElementFormat s_wrap(CF_VertexFormat format)
{
	switch (format)
	{
	case CF_VERTEX_FORMAT_INT:           return SDL_GPU_VERTEXELEMENTFORMAT_INT;
	case CF_VERTEX_FORMAT_INT2:          return SDL_GPU_VERTEXELEMENTFORMAT_INT2;
	case CF_VERTEX_FORMAT_INT3:          return SDL_GPU_VERTEXELEMENTFORMAT_INT3;
	case CF_VERTEX_FORMAT_INT4:          return SDL_GPU_VERTEXELEMENTFORMAT_INT4;
	case CF_VERTEX_FORMAT_UINT:          return SDL_GPU_VERTEXELEMENTFORMAT_UINT;
	case CF_VERTEX_FORMAT_UINT2:         return SDL_GPU_VERTEXELEMENTFORMAT_UINT2;
	case CF_VERTEX_FORMAT_UINT3:         return SDL_GPU_VERTEXELEMENTFORMAT_UINT3;
	case CF_VERTEX_FORMAT_UINT4:         return SDL_GPU_VERTEXELEMENTFORMAT_UINT4;
	case CF_VERTEX_FORMAT_FLOAT:         return SDL_GPU_VERTEXELEMENTFORMAT_FLOAT;
	case CF_VERTEX_FORMAT_FLOAT2:        return SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2;
	case CF_VERTEX_FORMAT_FLOAT3:        return SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3;
	case CF_VERTEX_FORMAT_FLOAT4:        return SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4;
	case CF_VERTEX_FORMAT_BYTE2:         return SDL_GPU_VERTEXELEMENTFORMAT_BYTE2;
	case CF_VERTEX_FORMAT_BYTE4:         return SDL_GPU_VERTEXELEMENTFORMAT_BYTE4;
	case CF_VERTEX_FORMAT_UBYTE2:        return SDL_GPU_VERTEXELEMENTFORMAT_UBYTE2;
	case CF_VERTEX_FORMAT_UBYTE4:        return SDL_GPU_VERTEXELEMENTFORMAT_UBYTE4;
	case CF_VERTEX_FORMAT_BYTE2_NORM:    return SDL_GPU_VERTEXELEMENTFORMAT_BYTE2_NORM;
	case CF_VERTEX_FORMAT_BYTE4_NORM:    return SDL_GPU_VERTEXELEMENTFORMAT_BYTE4_NORM;
	case CF_VERTEX_FORMAT_UBYTE2_NORM:   return SDL_GPU_VERTEXELEMENTFORMAT_UBYTE2_NORM;
	case CF_VERTEX_FORMAT_UBYTE4_NORM:   return SDL_GPU_VERTEXELEMENTFORMAT_UBYTE4_NORM;
	case CF_VERTEX_FORMAT_SHORT2:        return SDL_GPU_VERTEXELEMENTFORMAT_SHORT2;
	case CF_VERTEX_FORMAT_SHORT4:        return SDL_GPU_VERTEXELEMENTFORMAT_SHORT4;
	case CF_VERTEX_FORMAT_USHORT2:       return SDL_GPU_VERTEXELEMENTFORMAT_USHORT2;
	case CF_VERTEX_FORMAT_USHORT4:       return SDL_GPU_VERTEXELEMENTFORMAT_USHORT4;
	case CF_VERTEX_FORMAT_SHORT2_NORM:   return SDL_GPU_VERTEXELEMENTFORMAT_SHORT2_NORM;
	case CF_VERTEX_FORMAT_SHORT4_NORM:   return SDL_GPU_VERTEXELEMENTFORMAT_SHORT4_NORM;
	case CF_VERTEX_FORMAT_USHORT2_NORM:  return SDL_GPU_VERTEXELEMENTFORMAT_USHORT2_NORM;
	case CF_VERTEX_FORMAT_USHORT4_NORM:  return SDL_GPU_VERTEXELEMENTFORMAT_USHORT4_NORM;
	case CF_VERTEX_FORMAT_HALF2:         return SDL_GPU_VERTEXELEMENTFORMAT_HALF2;
	case CF_VERTEX_FORMAT_HALF4:         return SDL_GPU_VERTEXELEMENTFORMAT_HALF4;
	default:                             return SDL_GPU_VERTEXELEMENTFORMAT_FLOAT;
	}
}

CF_INLINE int s_uniform_size(CF_UniformType type)
{
	switch (type) {
	case CF_UNIFORM_TYPE_FLOAT:  return 4;
	case CF_UNIFORM_TYPE_FLOAT2: return 8;
	case CF_UNIFORM_TYPE_FLOAT4: return 16;
	case CF_UNIFORM_TYPE_INT:    return 4;
	case CF_UNIFORM_TYPE_INT2:   return 8;
	case CF_UNIFORM_TYPE_INT4:   return 16;
	case CF_UNIFORM_TYPE_MAT4:   return 64;
	default:                     return 0;
	}
}

struct CF_UniformBlockMember
{
	const char* name;
	const char* block_name;
	CF_UniformType type;
	int array_element_count;
	int size; // In bytes. If an array, it's the size in bytes of the whole array.
	int offset;
};

#define CF_MAX_SHADER_INPUTS (32)

struct CF_Uniform
{
	const char* name;
	const char* block_name;
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
	Cute::Array<CF_Uniform> uniforms;
	Cute::Array<CF_MaterialTex> textures;
};

struct CF_MaterialInternal
{
	bool dirty = false;
	CF_RenderState state;
	CF_MaterialState vs;
	CF_MaterialState fs;
	CF_Arena uniform_arena;
	CF_Arena block_arena;
};

struct CF_Pipeline
{
	CF_MaterialInternal* material = NULL;
	SDL_GPUGraphicsPipeline* pip = NULL;
	CF_MeshInternal* mesh = NULL;
};

#define CF_MAX_UNIFORM_BLOCK_COUNT (4)

struct CF_ShaderInternal
{
	SDL_GPUShader* vs = NULL;
	SDL_GPUShader* fs = NULL;
	int input_count = 0;
	const char* input_names[CF_MAX_SHADER_INPUTS];
	int input_locations[CF_MAX_SHADER_INPUTS];
	CF_ShaderInputFormat input_formats[CF_MAX_SHADER_INPUTS];
	int uniform_block_count = 0;
	int vs_block_sizes[CF_MAX_UNIFORM_BLOCK_COUNT];
	int fs_block_sizes[CF_MAX_UNIFORM_BLOCK_COUNT];
	Cute::Array<CF_UniformBlockMember> fs_uniform_block_members[CF_MAX_UNIFORM_BLOCK_COUNT];
	Cute::Array<CF_UniformBlockMember> vs_uniform_block_members[CF_MAX_UNIFORM_BLOCK_COUNT];
	Cute::Array<const char*> image_names;
	Cute::Array<CF_Pipeline> pip_cache;

	CF_INLINE int get_input_index(const char* name)
	{
		for (int i = 0; i < input_count; ++i) {
			if (input_names[i] == name) return i;
		}
		return -1;
	}

	CF_INLINE int fs_index(const char* name, int block_index)
	{
		for (int i = 0; i < fs_uniform_block_members[block_index].size(); ++i) {
			if (fs_uniform_block_members[block_index][i].name == name) return i;
		}
		return -1;
	}

	CF_INLINE int vs_index(const char* name, int block_index)
	{
		for (int i = 0; i < vs_uniform_block_members[block_index].size(); ++i) {
			if (vs_uniform_block_members[block_index][i].name == name) return i;
		}
		return -1;
	}
};

CF_Shader cf_make_draw_shader_internal(const char* path);
CF_Shader cf_make_draw_shader_from_source_internal(const char* src);
void cf_load_internal_shaders();
void cf_unload_internal_shaders();
void cf_shader_watch();
void cf_clear_canvas(CF_Canvas canvas);

#endif // CF_GRAPHICS_INTERNAL_H
