#include <cute_defines.h>

#ifndef CF_EMSCRIPTEN

#include "internal/cute_graphics_internal.h"
#include "internal/cute_app_internal.h"
#include "internal/cute_alloc_internal.h"

#include <cute_math.h>

#include <SDL3/SDL_gpu.h>
#include <SDL3_shadercross/SDL_shadercross.h>
#include <spirv_cross_c.h>

#include <float.h>

using namespace Cute;

struct CF_CanvasInternal
{
	int w, h;
	CF_Texture cf_texture;
	CF_Texture cf_resolve_texture;
	CF_Texture cf_depth_stencil;
	CF_SampleCount sample_count;
	SDL_GPUTexture* texture;
	SDL_GPUTexture* resolve_texture;
	SDL_GPUSampler* sampler;
	SDL_GPUTexture* depth_stencil;

	bool clear;

	// These get set by cf_apply_* functions.
	struct CF_MeshInternal* mesh;
	SDL_GPURenderPass* pass;
};

struct CF_TextureInternal
{
	int w, h;
	SDL_GPUFilter filter;
	SDL_GPUTexture* tex;
	SDL_GPUTransferBuffer* buf;
	SDL_GPUSampler* sampler;
	CF_PixelFormat pixel_format;
	SDL_GPUTextureFormat format;
	SDL_GPUTextureSamplerBinding binding;
};

// Content-based pipeline cache key. All fields that affect pipeline creation.
// Zeroed before filling so memcmp is safe even with padding.
struct CF_PipelineKey
{
	// Canvas state.
	SDL_GPUTextureFormat color_format;
	SDL_GPUTextureFormat depth_format;
	bool has_depth_stencil;
	int sample_count;

	// Material render state.
	CF_RenderState render_state;

	// Vertex layout (matched attributes in shader input order).
	int vertex_stride;
	int instance_stride;
	int attribute_count;
	struct
	{
		int location;
		CF_VertexFormat format;
		int offset;
		bool per_instance;
	} attrs[CF_MAX_SHADER_INPUTS];
};

struct CF_Pipeline
{
	CF_PipelineKey key;
	SDL_GPUGraphicsPipeline* pip;
};

struct CF_ShaderInternal
{
	SDL_GPUShader* vs = NULL;
	SDL_GPUShader* fs = NULL;
	int input_count = 0;
	const char* input_names[CF_MAX_SHADER_INPUTS];
	int input_locations[CF_MAX_SHADER_INPUTS];
	CF_ShaderInputFormat input_formats[CF_MAX_SHADER_INPUTS];
	int vs_uniform_block_count = 0;
	int fs_uniform_block_count = 0;
	int vs_block_sizes[CF_MAX_UNIFORM_BLOCK_COUNT];
	int fs_block_sizes[CF_MAX_UNIFORM_BLOCK_COUNT];
	Cute::Array<CF_UniformBlockMember> fs_uniform_block_members[CF_MAX_UNIFORM_BLOCK_COUNT];
	Cute::Array<CF_UniformBlockMember> vs_uniform_block_members[CF_MAX_UNIFORM_BLOCK_COUNT];
	Cute::Array<const char*> vs_image_names;
	Cute::Array<int> vs_image_binding_slots;
	Cute::Array<const char*> fs_image_names;
	Cute::Array<int> fs_image_binding_slots;
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

struct CF_Buffer
{
	int element_count;
	int size;
	int stride;
	SDL_GPUBuffer* buffer;
	SDL_GPUTransferBuffer* transfer_buffer;
};

struct CF_MeshInternal
{
	CF_Buffer vertices;
	CF_Buffer indices;
	CF_Buffer instances;
	int attribute_count;
	CF_VertexAttribute attributes[CF_MESH_MAX_VERTEX_ATTRIBUTES];
};

static struct
{
	SDL_GPUDevice* device;
	SDL_GPUCommandBuffer* cmd;
	SDL_Window* window;
	SDL_GPUTexture* swapchain_tex;
	uint32_t swapchain_tex_w, swapchain_tex_h;
	bool skip_drawing;
	int msaa_sample_count;
	CF_CanvasInternal* canvas;
	SDL_GPUSampler* sampler_override;
	SDL_GPURenderPass* active_pass;
} g_ctx = { };

static inline void s_end_active_pass() {
	if (g_ctx.active_pass) {
		SDL_EndGPURenderPass(g_ctx.active_pass);
		g_ctx.active_pass = NULL;
		if (g_ctx.canvas) g_ctx.canvas->pass = NULL;
	}
}

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
       samplerInfo.enable_anisotropy = false;
       samplerInfo.max_anisotropy = 1.0f;
       samplerInfo.enable_compare = false;
       samplerInfo.compare_op = SDL_GPU_COMPAREOP_NEVER;
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

static SDL_GPUTextureLocation SDL_GPUTextureLocationDefaults(CF_TextureInternal* tex, float x, float y)
{
	SDL_GPUTextureLocation location;
	CF_MEMSET(&location, 0, sizeof(location));
	location.texture = tex->tex;
	location.x = (Uint32)(x * tex->w);
	location.y = (Uint32)(y * tex->h);
	return location;
}

static inline SDL_GPUCompareOp s_wrap(CF_CompareFunction compare_function)
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

static inline SDL_GPUCullMode s_wrap(CF_CullMode mode)
{
    switch (mode)
    {
    case CF_CULL_MODE_NONE:  return SDL_GPU_CULLMODE_NONE;
    case CF_CULL_MODE_FRONT: return SDL_GPU_CULLMODE_FRONT;
    case CF_CULL_MODE_BACK:  return SDL_GPU_CULLMODE_BACK;
    default:              return SDL_GPU_CULLMODE_NONE;
    }
}

static inline SDL_GPUStencilOp s_wrap(CF_StencilOp stencil_op)
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

static inline SDL_GPUBlendOp s_wrap(CF_BlendOp blend_op)
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

static inline SDL_GPUBlendFactor s_wrap(CF_BlendFactor factor)
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

static inline SDL_GPUPrimitiveType s_wrap(CF_PrimitiveType type)
{
	switch (type)
	{
	case CF_PRIMITIVE_TYPE_TRIANGLELIST:   return SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
	case CF_PRIMITIVE_TYPE_TRIANGLESTRIP:  return SDL_GPU_PRIMITIVETYPE_TRIANGLESTRIP;
	case CF_PRIMITIVE_TYPE_LINELIST:       return SDL_GPU_PRIMITIVETYPE_LINELIST;
	case CF_PRIMITIVE_TYPE_LINESTRIP:      return SDL_GPU_PRIMITIVETYPE_LINESTRIP;
	default:                               return SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
	}
}

static inline SDL_GPUShaderStage s_wrap(CF_ShaderStage stage)
{
	switch (stage) {
	case CF_SHADER_STAGE_VERTEX: return SDL_GPU_SHADERSTAGE_VERTEX;
	case CF_SHADER_STAGE_FRAGMENT: return SDL_GPU_SHADERSTAGE_FRAGMENT;
	default: return SDL_GPU_SHADERSTAGE_VERTEX;
	}
}

static inline SDL_GPUTextureFormat s_wrap(CF_PixelFormat format)
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

static inline SDL_GPUFilter s_wrap(CF_Filter filter)
{
	switch (filter) {
	default: return SDL_GPU_FILTER_LINEAR;
	case CF_FILTER_NEAREST: return SDL_GPU_FILTER_NEAREST;
	case CF_FILTER_LINEAR: return SDL_GPU_FILTER_LINEAR;
	}
}

static inline SDL_GPUSamplerMipmapMode s_wrap(CF_MipFilter filter)
{
	switch (filter) {
	default: return SDL_GPU_SAMPLERMIPMAPMODE_LINEAR;
	case CF_MIP_FILTER_NEAREST: return SDL_GPU_SAMPLERMIPMAPMODE_NEAREST;
	case CF_MIP_FILTER_LINEAR: return SDL_GPU_SAMPLERMIPMAPMODE_LINEAR;
	}
}

static inline SDL_GPUSamplerAddressMode s_wrap(CF_WrapMode mode)
{
	switch (mode)
	{
	case CF_WRAP_MODE_REPEAT:           return SDL_GPU_SAMPLERADDRESSMODE_REPEAT;
	case CF_WRAP_MODE_CLAMP_TO_EDGE:    return SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
	case CF_WRAP_MODE_MIRRORED_REPEAT:  return SDL_GPU_SAMPLERADDRESSMODE_MIRRORED_REPEAT;
	default:                            return SDL_GPU_SAMPLERADDRESSMODE_REPEAT;
	}
}

static inline SDL_GPUVertexElementFormat s_wrap(CF_VertexFormat format)
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

static inline CF_BackendType s_query_backend()
{
	const char* driver = SDL_GetGPUDeviceDriver(g_ctx.device);
	if (sequ(driver, "vulkan")) {
		return CF_BACKEND_TYPE_VULKAN;
	} else if (sequ(driver, "metal")) {
		return CF_BACKEND_TYPE_METAL;
	} else if (sequ(driver, "direct3d11")) {
		return CF_BACKEND_TYPE_D3D11;
	} else if (sequ(driver, "direct3d12")) {
		return CF_BACKEND_TYPE_D3D12;
	} else if (sequ(driver, "private")) { // Is this the right string??
		return CF_BACKEND_TYPE_PRIVATE;
	} else {
		return CF_BACKEND_TYPE_INVALID;
	}
}

static inline CF_ShaderInputFormat s_wrap(CF_ShaderInfoDataType type)
{
	switch (type) {
	case CF_SHADER_INFO_TYPE_UNKNOWN: return CF_SHADER_INPUT_FORMAT_UNKNOWN;
	case CF_SHADER_INFO_TYPE_UINT:	return CF_SHADER_INPUT_FORMAT_UINT;
	case CF_SHADER_INFO_TYPE_SINT:	return CF_SHADER_INPUT_FORMAT_INT;
	case CF_SHADER_INFO_TYPE_FLOAT:   return CF_SHADER_INPUT_FORMAT_FLOAT;
	case CF_SHADER_INFO_TYPE_UINT2:   return CF_SHADER_INPUT_FORMAT_UVEC2;
	case CF_SHADER_INFO_TYPE_SINT2:   return CF_SHADER_INPUT_FORMAT_IVEC2;
	case CF_SHADER_INFO_TYPE_FLOAT2:  return CF_SHADER_INPUT_FORMAT_VEC2;
	case CF_SHADER_INFO_TYPE_UINT3:   return CF_SHADER_INPUT_FORMAT_UVEC3;
	case CF_SHADER_INFO_TYPE_SINT3:   return CF_SHADER_INPUT_FORMAT_IVEC3;
	case CF_SHADER_INFO_TYPE_FLOAT3:  return CF_SHADER_INPUT_FORMAT_VEC3;
	case CF_SHADER_INFO_TYPE_UINT4:   return CF_SHADER_INPUT_FORMAT_UVEC4;
	case CF_SHADER_INFO_TYPE_SINT4:   return CF_SHADER_INPUT_FORMAT_IVEC4;
	case CF_SHADER_INFO_TYPE_FLOAT4: return CF_SHADER_INPUT_FORMAT_VEC4;
	default: return CF_SHADER_INPUT_FORMAT_UNKNOWN;
	}
}

static inline bool s_texture_supports_format(CF_PixelFormat format, CF_TextureUsageBits usage)
{
	return SDL_GPUTextureSupportsFormat(
		g_ctx.device,
		s_wrap(format),
		SDL_GPU_TEXTURETYPE_2D,
		usage
	);
}

static inline bool s_texture_supports_stencil(const CF_TextureInternal* texture)
{
	if (!texture) return false;

	if (cf_pixel_format_has_stencil(texture->pixel_format)) return true;

	switch (texture->format)
	{
	case SDL_GPU_TEXTUREFORMAT_D24_UNORM_S8_UINT:
	case SDL_GPU_TEXTUREFORMAT_D32_FLOAT_S8_UINT:
		return true;
	default:
		return false;
	}
}

static inline CF_Texture s_make_texture(CF_TextureParams params, CF_SampleCount sample_count)
{
	SDL_GPUTextureCreateInfo tex_info = SDL_GPUTextureCreateInfoDefaults(params.width, params.height);
	tex_info.width = (Uint32)params.width;
	tex_info.height = (Uint32)params.height;
	tex_info.format = s_wrap(params.pixel_format);

	// Not allowed to sample from MSAA textures.
	tex_info.usage = sample_count == CF_SAMPLE_COUNT_1 ? params.usage : (params.usage & ~(SDL_GPU_TEXTUREUSAGE_SAMPLER));

	tex_info.sample_count = (SDL_GPUSampleCount)sample_count;
	if (params.allocate_mipmaps) {
		tex_info.num_levels = params.mip_count > 0
			? (Uint32)params.mip_count
			: (Uint32)(1 + (int)CF_FLOORF(CF_LOG2F((float)cf_max(params.width, params.height))));
	}

	SDL_GPUTexture* tex = SDL_CreateGPUTexture(g_ctx.device, &tex_info);
	CF_ASSERT(tex);
	if (!tex) return { 0 };

	SDL_GPUSampler* sampler = NULL;
	// Depth/stencil textures don't need their own sampler, as the associated color
	// texture in the owning canvas already has a sampler attached.
	if (!s_is_depth(params.pixel_format)) {
		SDL_GPUSamplerCreateInfo sampler_info = SDL_GPUSamplerCreateInfoDefaults();
		sampler_info.mip_lod_bias = params.mip_lod_bias;
		sampler_info.max_anisotropy = params.max_anisotropy;
		sampler_info.min_filter = s_wrap(params.filter);
		sampler_info.mag_filter = s_wrap(params.filter);
		sampler_info.mipmap_mode = s_wrap(params.mip_filter);
		sampler_info.address_mode_u = s_wrap(params.wrap_u);
		sampler_info.address_mode_v = s_wrap(params.wrap_v);
		sampler = SDL_CreateGPUSampler(g_ctx.device, &sampler_info);
		CF_ASSERT(sampler);
		if (!sampler) {
			SDL_ReleaseGPUTexture(g_ctx.device, tex);
			return { 0 };
		}
	}

	SDL_GPUTransferBuffer* buf = NULL;
	if (params.stream) {
		int texel_size = (int)SDL_GPUTextureFormatTexelBlockSize(tex_info.format);
		SDL_GPUTransferBufferCreateInfo tbuf_info = {
			.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
			.size = (Uint32)(texel_size * params.width * params.height),
			.props = 0,
		};
		buf = SDL_CreateGPUTransferBuffer(g_ctx.device, &tbuf_info);
	}

	CF_TextureInternal* tex_internal = CF_NEW(CF_TextureInternal);
	tex_internal->w = params.width;
	tex_internal->h = params.height;
	tex_internal->filter = sampler ? s_wrap(params.filter) : SDL_GPU_FILTER_NEAREST;
	tex_internal->tex = tex;
	tex_internal->buf = buf;
	tex_internal->sampler = sampler;
	tex_internal->pixel_format = params.pixel_format;
	tex_internal->format = tex_info.format;
	tex_internal->binding.texture = tex;
	tex_internal->binding.sampler = sampler;
	CF_Texture result;
	result.id = (uint64_t)(uintptr_t)tex_internal;
	return result;
}

static inline SDL_GPUShader* s_load_shader_bytecode(CF_ShaderInternal* shader_internal, CF_ShaderBytecode bytecode, CF_ShaderStage stage)
{
	bool vs = stage == CF_SHADER_STAGE_VERTEX ? true : false;

	// Load reflection info
	const CF_ShaderInfo* shader_info = &bytecode.shader_info;

	if (vs) {
		for (int i = 0; i < shader_info->num_images; ++i) {
			shader_internal->vs_image_names.add(sintern(shader_info->image_names[i]));
			shader_internal->vs_image_binding_slots.add(shader_info->image_binding_slots ? shader_info->image_binding_slots[i] : i);
		}
	} else {
		for (int i = 0; i < shader_info->num_images; ++i) {
			shader_internal->fs_image_names.add(sintern(shader_info->image_names[i]));
			shader_internal->fs_image_binding_slots.add(shader_info->image_binding_slots ? shader_info->image_binding_slots[i] : i);
		}
	}

	if (stage == CF_SHADER_STAGE_VERTEX) {
		shader_internal->vs_uniform_block_count = shader_info->num_uniforms;
	} else {
		shader_internal->fs_uniform_block_count = shader_info->num_uniforms;
	}
	const CF_ShaderUniformMemberInfo* member_infos = shader_info->uniform_members;
	for (int i = 0; i < shader_info->num_uniforms; ++i) {
		const CF_ShaderUniformInfo* block_info = &shader_info->uniforms[i];
		int block_index = block_info->block_index;

		if (vs) {
			shader_internal->vs_block_sizes[block_index] = block_info->block_size;
		} else {
			shader_internal->fs_block_sizes[block_index] = block_info->block_size;
		}

		const char* block_name = sintern(block_info->block_name);
		for (int j = 0; j < block_info->num_members; ++j) {
			const CF_ShaderUniformMemberInfo* member_info = &member_infos[j];

			CF_UniformBlockMember block_member;
			block_member.name = sintern(member_info->name);
			block_member.block_name = block_name;
			block_member.type = s_uniform_type(member_info->type);
			CF_ASSERT(block_member.type != CF_UNIFORM_TYPE_UNKNOWN);
			block_member.array_element_count = member_info->array_length;
			block_member.size = s_uniform_size(block_member.type) * member_info->array_length;
			block_member.offset = member_info->offset;

			if (vs) {
				shader_internal->vs_uniform_block_members[block_index].add(block_member);
			} else {
				shader_internal->fs_uniform_block_members[block_index].add(block_member);
			}
		}

		member_infos += block_info->num_members;
	}

	if (vs) {
		CF_ASSERT(shader_info->num_inputs <= CF_MAX_SHADER_INPUTS); // Increase `CF_MAX_SHADER_INPUTS`, or refactor the shader with less vertex attributes.
		shader_internal->input_count = shader_info->num_inputs;
		for (int i = 0; i < shader_info->num_inputs; ++i) {
			CF_ShaderInputInfo* input = &shader_info->inputs[i];
			shader_internal->input_names[i] = sintern(input->name);
			shader_internal->input_locations[i] = input->location;
			shader_internal->input_formats[i] = s_wrap(input->format);
		}
	}

	// Create the actual shader.
	SDL_GPUShaderCreateInfo shaderCreateInfo = {};
	shaderCreateInfo.code = bytecode.content;
	shaderCreateInfo.code_size = bytecode.size;
	shaderCreateInfo.entrypoint = "main";
	shaderCreateInfo.format = SDL_GPU_SHADERFORMAT_SPIRV;
	shaderCreateInfo.stage = s_wrap(stage);
	shaderCreateInfo.num_samplers = shader_info->num_samplers;
	shaderCreateInfo.num_storage_textures = shader_info->num_storage_textures;
	shaderCreateInfo.num_storage_buffers = shader_info->num_storage_buffers;
	shaderCreateInfo.num_uniform_buffers = shader_info->num_uniforms;
	SDL_GPUShader* sdl_shader = NULL;
	if (SDL_GetGPUShaderFormats(g_ctx.device) == SDL_GPU_SHADERFORMAT_SPIRV) {
		sdl_shader = (SDL_GPUShader*)SDL_CreateGPUShader(g_ctx.device, &shaderCreateInfo);
	} else {
		SDL_ShaderCross_GraphicsShaderMetadata metadata = {};
		metadata.num_samplers = shader_info->num_samplers;
		metadata.num_storage_textures = shader_info->num_storage_textures;
		metadata.num_storage_buffers = shader_info->num_storage_buffers;
		metadata.num_uniform_buffers = shader_info->num_uniforms;

		SDL_ShaderCross_SPIRV_Info spirvInfo;
		spirvInfo.bytecode = bytecode.content;
		spirvInfo.bytecode_size = bytecode.size;
		spirvInfo.entrypoint = "main";
		spirvInfo.shader_stage = stage == CF_SHADER_STAGE_VERTEX ? SDL_SHADERCROSS_SHADERSTAGE_VERTEX : SDL_SHADERCROSS_SHADERSTAGE_FRAGMENT;
		spirvInfo.enable_debug = false;
		spirvInfo.name = "shader.shd";
		spirvInfo.props = SDL_CreateProperties();
		sdl_shader = (SDL_GPUShader*)SDL_ShaderCross_CompileGraphicsShaderFromSPIRV(g_ctx.device, &spirvInfo, &metadata);
		SDL_DestroyProperties(spirvInfo.props);
	}
	CF_ASSERT(sdl_shader);
	return sdl_shader;
}

#ifdef _WIN32
#include <d3d12.h>
#include "d3d12_layout.gen.h"

// Suppress D3D12 debug warning #820 (CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE).
// We want to suppress this because CF is designed where the clear value can be set by the
// user at pretty much anytime, so our design conflicts with D3D12's expectation, but, you know,
// fuck you Microsoft, we do what we want.
// 
// SDL3 GPU doesn't expose the native ID3D12Device* publicly, so we reach into
// private struct layouts using offsets computed at build time by gen_d3d12_layout. This is
// another case of someone tryna cramp our style. So yeah, fuck you SDL, we do what we want.

static void s_silence_d3d12_clear_value_warning(SDL_GPUDevice* gpu_device)
{
	if (SDL_strcmp(SDL_GetGPUDeviceDriver(gpu_device), "direct3d12") != 0) return;

	// SDL_GPUDevice (SDL_sysgpu.h): N function pointer slots, then driverData, then backend.
	void** device_slots = (void**)gpu_device;
	void* driver_data = device_slots[SDL_GPU_DEVICE_FN_SLOT_COUNT];

	// Sanity: next slot should be the backend string.
	const char* backend = (const char*)device_slots[SDL_GPU_DEVICE_FN_SLOT_COUNT + 1];
	if (SDL_strcmp(backend, "direct3d12") != 0) return;

	// Sanity: D3D12Renderer's first field is a back-pointer to SDL_GPUDevice.
	void** renderer_slots = (void**)driver_data;
	if (renderer_slots[0] != gpu_device) return;

	// Access ID3D12Device* at build-time-computed byte offset.
	ID3D12Device* d3d_device = *(ID3D12Device**)((char*)driver_data + D3D12_RENDERER_DEVICE_BYTE_OFFSET);

	ID3D12InfoQueue* info_queue = NULL;
	if (SUCCEEDED(d3d_device->QueryInterface(IID_PPV_ARGS(&info_queue)))) {
		D3D12_MESSAGE_ID hide[] = {
			D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE,
		};
		D3D12_INFO_QUEUE_FILTER filter = {};
		filter.DenyList.NumIDs = _countof(hide);
		filter.DenyList.pIDList = hide;
		info_queue->AddStorageFilterEntries(&filter);
		info_queue->Release();
	}
}
#endif

CF_Result cf_sdlgpu_init(const char* device_name, bool debug, CF_BackendType* backend_type)
{
	if(!SDL_ShaderCross_Init()) {
		return cf_result_error("Failed to initialize SDL_ShaderCross.");
	}
	g_ctx.device = SDL_CreateGPUDevice(SDL_ShaderCross_GetSPIRVShaderFormats(), debug, device_name);
	if (!g_ctx.device) {
		return cf_result_error("Failed to create GPU Device.");
	}

#ifdef _WIN32
	if (debug) {
		s_silence_d3d12_clear_value_warning(g_ctx.device);
	}
#endif

	*backend_type = s_query_backend();

	return cf_result_success();
}

void cf_sdlgpu_cleanup()
{
	if (g_ctx.cmd) {
		SDL_SubmitGPUCommandBuffer(g_ctx.cmd);
		g_ctx.cmd = NULL;
	}
	SDL_WaitForGPUIdle(g_ctx.device);
	SDL_ReleaseWindowFromGPUDevice(g_ctx.device, g_ctx.window);
	SDL_DestroyGPUDevice(g_ctx.device);
	SDL_ShaderCross_Quit();
}

SDL_GPUDevice* cf_sdlgpu_get_device()
{
	return g_ctx.device;
}

SDL_GPUTexture* cf_sdlgpu_get_swapchain_texture()
{
	return g_ctx.swapchain_tex;
}

SDL_GPUCommandBuffer* cf_sdlgpu_get_command_buffer()
{
	return g_ctx.cmd;
}

void cf_sdlgpu_attach(SDL_Window* window)
{
	SDL_ClaimWindowForGPUDevice(g_ctx.device, window);
	g_ctx.window = window;
	cf_sdlgpu_set_vsync_mailbox(false);
	g_ctx.cmd = SDL_AcquireGPUCommandBuffer(g_ctx.device);
}

bool cf_sdlgpu_supports_msaa(int sample_count)
{
	SDL_GPUTextureFormat fmt = SDL_GetGPUSwapchainTextureFormat(g_ctx.device, g_ctx.window);
	return SDL_GPUTextureSupportsSampleCount(g_ctx.device, fmt, (SDL_GPUSampleCount)sample_count);
}

void cf_sdlgpu_flush()
{
	if (g_ctx.cmd)
	{
		SDL_SubmitGPUCommandBuffer(g_ctx.cmd);
		g_ctx.cmd = NULL;
	}
}

void cf_sdlgpu_set_vsync(bool vsync)
{
	SDL_SetGPUSwapchainParameters(g_ctx.device, g_ctx.window, SDL_GPU_SWAPCHAINCOMPOSITION_SDR, vsync ? SDL_GPU_PRESENTMODE_VSYNC : SDL_GPU_PRESENTMODE_IMMEDIATE);
}

void cf_sdlgpu_set_vsync_mailbox(bool vsync)
{
	SDL_SetGPUSwapchainParameters(g_ctx.device, g_ctx.window, SDL_GPU_SWAPCHAINCOMPOSITION_SDR, vsync ? SDL_GPU_PRESENTMODE_MAILBOX : SDL_GPU_PRESENTMODE_IMMEDIATE);
}

void cf_sdlgpu_begin_frame()
{
	g_ctx.cmd = SDL_AcquireGPUCommandBuffer(g_ctx.device);
	g_ctx.skip_drawing = false;
}

void cf_sdlgpu_blit_canvas(CF_Canvas canvas)
{
	s_end_active_pass();
	// Try to acquire a swapchain texture
	if (g_ctx.swapchain_tex == NULL && !g_ctx.skip_drawing) {
		if (
			!SDL_WaitAndAcquireGPUSwapchainTexture(g_ctx.cmd, g_ctx.window, &g_ctx.swapchain_tex, &g_ctx.swapchain_tex_w, &g_ctx.swapchain_tex_h)
			|| g_ctx.swapchain_tex == NULL
		) {
			g_ctx.skip_drawing = true;
		}
	}

	// Stretch the app canvas onto the backbuffer canvas.
	if (g_ctx.swapchain_tex != NULL) {
		// Blit onto the screen.
		CF_CanvasInternal* canvas_internal = (CF_CanvasInternal*)canvas.id;
		SDL_GPUBlitRegion src = {
			.texture = canvas_internal->resolve_texture ? canvas_internal->resolve_texture : canvas_internal->texture,
			.w = (Uint32)canvas_internal->w,
			.h = (Uint32)canvas_internal->h,
		};
		SDL_GPUBlitRegion dst = {
			.texture = g_ctx.swapchain_tex,
			.w = g_ctx.swapchain_tex_w,
			.h = g_ctx.swapchain_tex_h,
		};
		SDL_GPUBlitInfo blit_info = {
			.source = src,
			.destination = dst,
			.load_op = SDL_GPU_LOADOP_CLEAR,
			.flip_mode = SDL_FLIP_NONE,
			.filter = SDL_GPU_FILTER_NEAREST,
			.cycle = true,
		};
		SDL_BlitGPUTexture(g_ctx.cmd, &blit_info);
	}
}

void cf_sdlgpu_end_frame()
{
	s_end_active_pass();
	SDL_SubmitGPUCommandBuffer(g_ctx.cmd);
	g_ctx.cmd = NULL;
	g_ctx.canvas = NULL;
	g_ctx.swapchain_tex = NULL;
}

bool cf_sdlgpu_texture_supports_format(CF_PixelFormat format, CF_TextureUsageBits usage)
{
	return SDL_GPUTextureSupportsFormat(
		g_ctx.device,
		s_wrap(format),
		SDL_GPU_TEXTURETYPE_2D,
		usage
	);
}

bool cf_sdlgpu_query_pixel_format(CF_PixelFormat format, CF_PixelFormatOp op)
{
	switch (op) {
	case CF_PIXELFORMAT_OP_NEAREST_FILTER:
	case CF_PIXELFORMAT_OP_BILINEAR_FILTER:
		return cf_sdlgpu_texture_supports_format(format, CF_TEXTURE_USAGE_SAMPLER_BIT);
	case CF_PIXELFORMAT_OP_RENDER_TARGET:
		return cf_sdlgpu_texture_supports_format(format, CF_TEXTURE_USAGE_COLOR_TARGET_BIT);
	case CF_PIXELFORMAT_OP_ALPHA_BLENDING:
		return cf_sdlgpu_texture_supports_format(format, CF_TEXTURE_USAGE_COLOR_TARGET_BIT) && cf_pixel_format_has_alpha(format);
	case CF_PIXELFORMAT_OP_MSAA:
		if (cf_pixel_format_is_depth(format)) return cf_sdlgpu_texture_supports_format(format, CF_TEXTURE_USAGE_DEPTH_STENCIL_TARGET_BIT);
		return cf_sdlgpu_texture_supports_format(format, CF_TEXTURE_USAGE_COLOR_TARGET_BIT);
	case CF_PIXELFORMAT_OP_DEPTH:
		return cf_sdlgpu_texture_supports_format(format, CF_TEXTURE_USAGE_DEPTH_STENCIL_TARGET_BIT);
	default:
		return false;
	}
}

CF_Texture cf_sdlgpu_make_texture(CF_TextureParams params)
{
	return s_make_texture(params, CF_SAMPLE_COUNT_1);
}

void cf_sdlgpu_destroy_texture(CF_Texture texture_handle)
{
	CF_TextureInternal* tex = (CF_TextureInternal*)texture_handle.id;
	SDL_ReleaseGPUTexture(g_ctx.device, tex->tex);
	if (tex->sampler) SDL_ReleaseGPUSampler(g_ctx.device, tex->sampler);
	if (tex->buf) SDL_ReleaseGPUTransferBuffer(g_ctx.device, tex->buf);
	CF_FREE(tex);
}

void cf_sdlgpu_texture_update(CF_Texture texture_handle, void* data, int size)
{
	s_end_active_pass();
	CF_TextureInternal* tex = (CF_TextureInternal*)texture_handle.id;

	// Copy bytes over to the driver.
	SDL_GPUTransferBuffer* buf = tex->buf;
	if (!buf) {
		SDL_GPUTransferBufferCreateInfo tbuf_info = {
			.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
			.size = (Uint32)size,
			.props = 0,
		};
		buf = SDL_CreateGPUTransferBuffer(g_ctx.device, &tbuf_info);
	}
	void* p = SDL_MapGPUTransferBuffer(g_ctx.device, buf, true);
	CF_MEMCPY(p, data, size);
	SDL_UnmapGPUTransferBuffer(g_ctx.device, buf);

	// Tell the driver to upload the bytes to the GPU.
	SDL_GPUCommandBuffer* cmd = g_ctx.cmd ? g_ctx.cmd : SDL_AcquireGPUCommandBuffer(g_ctx.device);
	SDL_GPUCopyPass* pass = SDL_BeginGPUCopyPass(cmd);
	SDL_GPUTextureTransferInfo src;
	src.transfer_buffer = buf;
	src.offset = 0;
	// SDL_GPU: 0 means tightly packed, inferred from the texture region dimensions.
	src.pixels_per_row = 0;
	src.rows_per_layer = 0;
	SDL_GPUTextureRegion dst = SDL_GPUTextureRegionDefaults(tex, tex->w, tex->h);
	SDL_UploadToGPUTexture(pass, &src, &dst, true);
	SDL_EndGPUCopyPass(pass);
	if (!tex->buf) SDL_ReleaseGPUTransferBuffer(g_ctx.device, buf);
	if (!g_ctx.cmd) SDL_SubmitGPUCommandBuffer(cmd);
}

void cf_sdlgpu_texture_update_mip(CF_Texture texture_handle, void* data, int size, int mip_level)
{
	s_end_active_pass();
	CF_TextureInternal* tex = (CF_TextureInternal*)texture_handle.id;

	// Create a temporary transfer buffer if needed.
	SDL_GPUTransferBuffer* buf = tex->buf;
	if (!buf) {
		SDL_GPUTransferBufferCreateInfo tbuf_info = {
			.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
			.size = (Uint32)size,
			.props = 0,
		};
		buf = SDL_CreateGPUTransferBuffer(g_ctx.device, &tbuf_info);
	}

	// Copy data into the transfer buffer
	void* p = SDL_MapGPUTransferBuffer(g_ctx.device, buf, true);
	CF_MEMCPY(p, data, size);
	SDL_UnmapGPUTransferBuffer(g_ctx.device, buf);

	// Compute dimensions for the mip level.
	int w = cf_max(tex->w >> mip_level, 1);
	int h = cf_max(tex->h >> mip_level, 1);

	// Tell the driver to upload the bytes to the GPU.
	SDL_GPUCommandBuffer* cmd = g_ctx.cmd ? g_ctx.cmd : SDL_AcquireGPUCommandBuffer(g_ctx.device);
	SDL_GPUCopyPass* pass = SDL_BeginGPUCopyPass(cmd);
	SDL_GPUTextureTransferInfo src;
	src.transfer_buffer = buf;
	src.offset = 0;
	// SDL_GPU: 0 means tightly packed, inferred from the texture region dimensions.
	src.pixels_per_row = 0;
	src.rows_per_layer = 0;
	SDL_GPUTextureRegion dst = SDL_GPUTextureRegionDefaults(tex, w, h);
	dst.mip_level = (Uint32)mip_level;
	SDL_UploadToGPUTexture(pass, &src, &dst, true);
	SDL_EndGPUCopyPass(pass);
	if (!tex->buf) SDL_ReleaseGPUTransferBuffer(g_ctx.device, buf);
	if (!g_ctx.cmd) SDL_SubmitGPUCommandBuffer(cmd);
}

void cf_sdlgpu_generate_mipmaps(CF_Texture texture_handle)
{
	s_end_active_pass();
	CF_TextureInternal* tex = (CF_TextureInternal*)texture_handle.id;
	SDL_GPUCommandBuffer* cmd = g_ctx.cmd ? g_ctx.cmd : SDL_AcquireGPUCommandBuffer(g_ctx.device);
	SDL_GenerateMipmapsForGPUTexture(cmd, tex->tex);
	if (!g_ctx.cmd) SDL_SubmitGPUCommandBuffer(cmd);
}

uint64_t cf_sdlgpu_texture_handle(CF_Texture texture)
{
	return (uint64_t)((CF_TextureInternal*)texture.id)->tex;
}

uint64_t cf_sdlgpu_texture_binding_handle(CF_Texture texture)
{
	return (uint64_t)&((CF_TextureInternal*)texture.id)->binding;
}

CF_Shader cf_sdlgpu_make_shader_from_bytecode(CF_ShaderBytecode vertex_bytecode, CF_ShaderBytecode fragment_bytecode)
{
	CF_ShaderInternal* shader_internal = CF_NEW(CF_ShaderInternal);
	CF_MEMSET(shader_internal, 0, sizeof(*shader_internal));

	shader_internal->vs = s_load_shader_bytecode(shader_internal, vertex_bytecode, CF_SHADER_STAGE_VERTEX);
	shader_internal->fs = s_load_shader_bytecode(shader_internal, fragment_bytecode, CF_SHADER_STAGE_FRAGMENT);
	CF_ASSERT(shader_internal->vs);
	CF_ASSERT(shader_internal->fs);

	CF_Shader result;
	result.id = { (uint64_t)shader_internal };
	return result;
}

void cf_sdlgpu_destroy_shader_internal(CF_Shader shader_handle)
{
	CF_ShaderInternal* shd = (CF_ShaderInternal*)shader_handle.id;
	SDL_ReleaseGPUShader(g_ctx.device, shd->vs);
	SDL_ReleaseGPUShader(g_ctx.device, shd->fs);
	for (int i = 0; i < shd->pip_cache.count(); ++i) {
		SDL_ReleaseGPUGraphicsPipeline(g_ctx.device, shd->pip_cache[i].pip);
	}
	shd->~CF_ShaderInternal();
	CF_FREE(shd);
}

CF_Canvas cf_sdlgpu_make_canvas(CF_CanvasParams params)
{
	CF_CanvasInternal* canvas = (CF_CanvasInternal*)CF_CALLOC(sizeof(CF_CanvasInternal));
	if (params.target.width > 0 && params.target.height > 0) {
		canvas->w = params.target.width;
		canvas->h = params.target.height;
		canvas->cf_texture = s_make_texture(params.target, params.sample_count);
		canvas->sample_count = params.sample_count;
		if (canvas->cf_texture.id) {
			canvas->texture = ((CF_TextureInternal*)canvas->cf_texture.id)->tex;
			canvas->sampler = ((CF_TextureInternal*)canvas->cf_texture.id)->sampler;
		}
		if (params.depth_stencil_enable) {
			canvas->cf_depth_stencil = s_make_texture(params.depth_stencil_target, params.sample_count);
			if (canvas->cf_depth_stencil.id) {
				canvas->depth_stencil = ((CF_TextureInternal*)canvas->cf_depth_stencil.id)->tex;
			}
		} else {
			canvas->cf_depth_stencil = { 0 };
		}
		if (canvas->sample_count != CF_SAMPLE_COUNT_1) {
			params.target.usage = CF_TEXTURE_USAGE_COLOR_TARGET_BIT | CF_TEXTURE_USAGE_SAMPLER_BIT;
						canvas->cf_resolve_texture = cf_sdlgpu_make_texture(params.target);
			if (canvas->cf_resolve_texture.id) {
				canvas->resolve_texture = ((CF_TextureInternal*)canvas->cf_resolve_texture.id)->tex;
			}
		}
	} else {
		return { 0 };
	}
	CF_Canvas result;
	result.id = (uint64_t)canvas;
	return result;
}

void cf_sdlgpu_canvas_get_size(CF_Canvas canvas_handle, int* w, int* h)
{
	CF_CanvasInternal* canvas = (CF_CanvasInternal*)(uintptr_t)canvas_handle.id;
	if (canvas) {
		if (w) { *w = canvas->w; }
		if (h) { *h = canvas->h; }
	}
}

void cf_sdlgpu_clear_canvas(CF_Canvas canvas_handle)
{
	s_end_active_pass();
	CF_CanvasInternal* canvas = (CF_CanvasInternal*)canvas_handle.id;
	SDL_GPUCommandBuffer* cmd = g_ctx.cmd ? g_ctx.cmd : SDL_AcquireGPUCommandBuffer(g_ctx.device);

	SDL_GPUColorTargetInfo color_info = {
		.texture = canvas->texture,
		.clear_color = { app->clear_color.r, app->clear_color.g, app->clear_color.b, app->clear_color.a },
		.load_op = SDL_GPU_LOADOP_CLEAR,
		.store_op = SDL_GPU_STOREOP_STORE,
		.cycle = true,
	};
	SDL_GPUDepthStencilTargetInfo depth_stencil_info = {
		.texture = canvas->depth_stencil,
		.clear_depth = 1.0f,
		.load_op = SDL_GPU_LOADOP_CLEAR,
		.store_op = SDL_GPU_STOREOP_STORE,
		.stencil_load_op = SDL_GPU_LOADOP_CLEAR,
		.stencil_store_op = SDL_GPU_STOREOP_STORE,
		.cycle = true,
		.clear_stencil = 0,
	};
	SDL_GPURenderPass* renderPass = SDL_BeginGPURenderPass(cmd, &color_info, 1, canvas->depth_stencil ? &depth_stencil_info : NULL);
	SDL_EndGPURenderPass(renderPass);
	canvas->clear = false;

	if (!g_ctx.cmd) SDL_SubmitGPUCommandBuffer(cmd);
}

void cf_sdlgpu_destroy_canvas(CF_Canvas canvas_handle)
{
	CF_CanvasInternal* canvas = (CF_CanvasInternal*)canvas_handle.id;
		cf_sdlgpu_destroy_texture(canvas->cf_texture);
		if (canvas->resolve_texture) cf_sdlgpu_destroy_texture(canvas->cf_resolve_texture);
		if (canvas->depth_stencil) cf_sdlgpu_destroy_texture(canvas->cf_depth_stencil);
	CF_FREE(canvas);
}

CF_Texture cf_sdlgpu_canvas_get_target(CF_Canvas canvas_handle)
{
	CF_CanvasInternal* canvas = (CF_CanvasInternal*)canvas_handle.id;
	return canvas->resolve_texture ? canvas->cf_resolve_texture : canvas->cf_texture;
}

CF_Texture cf_sdlgpu_canvas_get_depth_stencil_target(CF_Canvas canvas_handle)
{
	CF_CanvasInternal* canvas = (CF_CanvasInternal*)canvas_handle.id;
	return canvas->cf_depth_stencil;
}

CF_Mesh cf_sdlgpu_make_mesh(int vertex_buffer_size, const CF_VertexAttribute* attributes, int attribute_count, int vertex_stride)
{
	CF_MeshInternal* mesh = (CF_MeshInternal*)CF_CALLOC(sizeof(CF_MeshInternal));
	mesh->vertices.size = vertex_buffer_size;
	if (vertex_buffer_size) {
		SDL_GPUBufferCreateInfo buf_info = {
			.usage = SDL_GPU_BUFFERUSAGE_VERTEX,
			.size = (Uint32)vertex_buffer_size,
			.props = 0,
		};
		mesh->vertices.buffer = SDL_CreateGPUBuffer(g_ctx.device, &buf_info);
		SDL_GPUTransferBufferCreateInfo tbuf_info = {
			.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
			.size = (Uint32)vertex_buffer_size,
			.props = 0,
		};
		mesh->vertices.transfer_buffer = SDL_CreateGPUTransferBuffer(g_ctx.device, &tbuf_info);
	}
	attribute_count = cf_min(attribute_count, CF_MESH_MAX_VERTEX_ATTRIBUTES);
	mesh->attribute_count = attribute_count;
	mesh->vertices.stride = vertex_stride;
	for (int i = 0; i < attribute_count; ++i) {
		mesh->attributes[i] = attributes[i];
		mesh->attributes[i].name = sintern(attributes[i].name);
	}
	CF_Mesh result = { (uint64_t)mesh };
	return result;
}

void cf_sdlgpu_mesh_set_index_buffer(CF_Mesh mesh_handle, int index_buffer_size_in_bytes, int index_bit_count)
{
	CF_ASSERT(index_bit_count == 16 || index_bit_count == 32);
	CF_MeshInternal* mesh = (CF_MeshInternal*)mesh_handle.id;
	mesh->indices.size = index_buffer_size_in_bytes;
	mesh->indices.stride = index_bit_count / 8;
	SDL_GPUBufferCreateInfo buf_info = {
		.usage = SDL_GPU_BUFFERUSAGE_INDEX,
		.size = (Uint32)index_buffer_size_in_bytes,
		.props = 0,
	};
	mesh->indices.buffer = SDL_CreateGPUBuffer(g_ctx.device, &buf_info);
	SDL_GPUTransferBufferCreateInfo tbuf_info = {
		.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
		.size = (Uint32)index_buffer_size_in_bytes,
		.props = 0,
	};
	mesh->indices.transfer_buffer = SDL_CreateGPUTransferBuffer(g_ctx.device, &tbuf_info);
}

void cf_sdlgpu_mesh_set_instance_buffer(CF_Mesh mesh_handle, int instance_buffer_size_in_bytes, int instance_stride)
{
	CF_MeshInternal* mesh = (CF_MeshInternal*)mesh_handle.id;
	mesh->instances.size = instance_buffer_size_in_bytes;
	mesh->instances.stride = instance_stride;
	SDL_GPUBufferCreateInfo buf_info = {
		.usage = SDL_GPU_BUFFERUSAGE_VERTEX,
		.size = (Uint32)instance_buffer_size_in_bytes,
		.props = 0,
	};
	mesh->instances.buffer = SDL_CreateGPUBuffer(g_ctx.device, &buf_info);
	SDL_GPUTransferBufferCreateInfo tbuf_info = {
		.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
		.size = (Uint32)instance_buffer_size_in_bytes,
		.props = 0,
	};
	mesh->instances.transfer_buffer = SDL_CreateGPUTransferBuffer(g_ctx.device, &tbuf_info);
}

void cf_sdlgpu_destroy_mesh(CF_Mesh mesh_handle)
{
	CF_MeshInternal* mesh = (CF_MeshInternal*)mesh_handle.id;
	if (mesh->vertices.buffer) {
		SDL_ReleaseGPUBuffer(g_ctx.device, mesh->vertices.buffer);
		SDL_ReleaseGPUTransferBuffer(g_ctx.device, mesh->vertices.transfer_buffer);
	}
	if (mesh->indices.buffer) {
		SDL_ReleaseGPUBuffer(g_ctx.device, mesh->indices.buffer);
		SDL_ReleaseGPUTransferBuffer(g_ctx.device, mesh->indices.transfer_buffer);
	}
	if (mesh->instances.buffer) {
		SDL_ReleaseGPUBuffer(g_ctx.device, mesh->instances.buffer);
		SDL_ReleaseGPUTransferBuffer(g_ctx.device, mesh->instances.transfer_buffer);
	}
	CF_FREE(mesh);
}

static inline void s_update_buffer(CF_Buffer* buffer, int element_count, void* data, int size, SDL_GPUBufferUsageFlags flags)
{
	s_end_active_pass();

	// Resize buffer if necessary.
	if (size > buffer->size) {
		SDL_ReleaseGPUBuffer(g_ctx.device, buffer->buffer);
		SDL_ReleaseGPUTransferBuffer(g_ctx.device, buffer->transfer_buffer);

		int new_size = size * 2;
		buffer->size = new_size;
		SDL_GPUBufferCreateInfo buf_info = {
			.usage = flags,
			.size = (Uint32)new_size,
			.props = 0,
		};
		buffer->buffer = SDL_CreateGPUBuffer(g_ctx.device, &buf_info);
		SDL_GPUTransferBufferCreateInfo tbuf_info = {
			.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
			.size = (Uint32)new_size,
			.props = 0,
		};
		buffer->transfer_buffer = SDL_CreateGPUTransferBuffer(g_ctx.device, &tbuf_info);
	}

	// Copy vertices over to the driver.
	CF_ASSERT(size <= buffer->size);
	void* p = SDL_MapGPUTransferBuffer(g_ctx.device, buffer->transfer_buffer, true);
	CF_MEMCPY(p, data, size);
	SDL_UnmapGPUTransferBuffer(g_ctx.device, buffer->transfer_buffer);
	buffer->element_count = element_count;

	// Submit the upload command to the GPU.
	SDL_GPUCommandBuffer* cmd = g_ctx.cmd ? g_ctx.cmd : SDL_AcquireGPUCommandBuffer(g_ctx.device);
	SDL_GPUCopyPass *pass = SDL_BeginGPUCopyPass(cmd);
	SDL_GPUTransferBufferLocation location;
	location.offset = 0;
	location.transfer_buffer = buffer->transfer_buffer;
	SDL_GPUBufferRegion region;
	region.buffer = buffer->buffer;
	region.offset = 0;
	region.size = size;
	SDL_UploadToGPUBuffer(pass, &location, &region, true);
	SDL_EndGPUCopyPass(pass);
	if (!g_ctx.cmd) SDL_SubmitGPUCommandBuffer(cmd);
}

void cf_sdlgpu_mesh_update_vertex_data(CF_Mesh mesh_handle, void* data, int count)
{
	CF_MeshInternal* mesh = (CF_MeshInternal*)mesh_handle.id;
	CF_ASSERT(mesh->attribute_count);
	s_update_buffer(&mesh->vertices, count, data, count * mesh->vertices.stride, SDL_GPU_BUFFERUSAGE_VERTEX);
}

void cf_sdlgpu_mesh_update_index_data(CF_Mesh mesh_handle, void* data, int count)
{
	CF_MeshInternal* mesh = (CF_MeshInternal*)mesh_handle.id;
	s_update_buffer(&mesh->indices, count, data, count * mesh->indices.stride, SDL_GPU_BUFFERUSAGE_INDEX);
}

void cf_sdlgpu_mesh_update_instance_data(CF_Mesh mesh_handle, void* data, int count)
{
	CF_MeshInternal* mesh = (CF_MeshInternal*)mesh_handle.id;
	s_update_buffer(&mesh->instances, count, data, count * mesh->instances.stride, SDL_GPU_BUFFERUSAGE_VERTEX);
}

void cf_sdlgpu_apply_canvas(CF_Canvas canvas_handle, bool clear)
{
	CF_CanvasInternal* canvas = (CF_CanvasInternal*)canvas_handle.id;
	CF_ASSERT(canvas);
	// End any active pass if switching canvases or requesting a clear.
	if (g_ctx.active_pass && (canvas != g_ctx.canvas || clear)) {
		s_end_active_pass();
	}
	g_ctx.canvas = canvas;
	g_ctx.canvas->clear = clear;
}

void cf_sdlgpu_apply_viewport(int x, int y, int w, int h)
{
	CF_ASSERT(g_ctx.canvas);
	CF_ASSERT(g_ctx.canvas->pass);
	SDL_GPUViewport viewport;
	viewport.x = (float)x;
	viewport.y = (float)y;
	viewport.w = (float)w;
	viewport.h = (float)h;
	viewport.min_depth = 0;
	viewport.max_depth = 1;
	SDL_SetGPUViewport(g_ctx.canvas->pass, &viewport);
}

void cf_sdlgpu_apply_scissor(int x, int y, int w, int h)
{
	CF_ASSERT(g_ctx.canvas);
	CF_ASSERT(g_ctx.canvas->pass);
	SDL_Rect scissor;
	scissor.x = x;
	scissor.y = y;
	scissor.w = w;
	scissor.h = h;
	SDL_SetGPUScissor(g_ctx.canvas->pass, &scissor);
}

void cf_sdlgpu_apply_stencil_reference(int reference)
{
	CF_ASSERT(g_ctx.canvas);
	CF_ASSERT(g_ctx.canvas->pass);
	SDL_SetGPUStencilReference(g_ctx.canvas->pass, reference);
}

void cf_sdlgpu_apply_blend_constants(float r, float g, float b, float a)
{
	CF_ASSERT(g_ctx.canvas);
	CF_ASSERT(g_ctx.canvas->pass);
	SDL_FColor color;
	color.r = r;
	color.g = g;
	color.b = b;
	color.a = a;
	SDL_SetGPUBlendConstants(g_ctx.canvas->pass, color);
}

void cf_sdlgpu_apply_mesh(CF_Mesh mesh_handle)
{
	CF_ASSERT(g_ctx.canvas);
	CF_MeshInternal* mesh = (CF_MeshInternal*)mesh_handle.id;
	g_ctx.canvas->mesh = mesh;
}

static inline void s_copy_uniforms(SDL_GPUCommandBuffer* cmd, CF_Arena* arena, CF_ShaderInternal* shd, CF_MaterialState* mstate, bool vs)
{
	// Create any required uniform blocks for all uniforms matching between which uniforms
	// the material has and the shader needs.
	void* ub_ptrs[CF_MAX_UNIFORM_BLOCK_COUNT] = { };
	int ub_sizes[CF_MAX_UNIFORM_BLOCK_COUNT] = { };
	int block_count = vs ? shd->vs_uniform_block_count : shd->fs_uniform_block_count;
	for (int block_index = 0; block_index < block_count; ++block_index) {
		for (int i = 0; i < mstate->uniforms.count(); ++i) {
			CF_Uniform uniform = mstate->uniforms[i];
			int idx = vs ? shd->vs_index(uniform.name, block_index) : shd->fs_index(uniform.name, block_index);
			if (idx >= 0) {
				if (!ub_ptrs[block_index]) {
					// Create temporary space for a uniform block.
					int size = vs ? shd->vs_block_sizes[block_index] : shd->fs_block_sizes[block_index];
					void* block = cf_arena_alloc(arena, size);
					CF_MEMSET(block, 0, size);
					ub_ptrs[block_index] = block;
					ub_sizes[block_index] = size;
				}

				// Copy in the uniform's value into the block.
				int offset = vs ? shd->vs_uniform_block_members[block_index][idx].offset : shd->fs_uniform_block_members[block_index][idx].offset;
				void* block = ub_ptrs[block_index];
				void* dst = (void*)(((uintptr_t)block) + offset);
				CF_MEMCPY(dst, uniform.data, uniform.size);
			}
		}
	}

	// Send uniform data to the GPU.
	for (int i = 0; i < CF_MAX_UNIFORM_BLOCK_COUNT; ++i) {
		if (ub_ptrs[i]) {
			void* block = ub_ptrs[i];
			int size = ub_sizes[i];
			if (vs) {
				SDL_PushGPUVertexUniformData(cmd, i, block, (uint32_t)size);
			} else {
				SDL_PushGPUFragmentUniformData(cmd, i, block, (uint32_t)size);
			}
		}
	}

	cf_arena_reset(arena);
}

static inline SDL_GPUGraphicsPipeline* s_build_pipeline(CF_ShaderInternal* shader, CF_RenderState* state, CF_MeshInternal* mesh)
{
	CF_TextureInternal* tex = (CF_TextureInternal*)g_ctx.canvas->cf_texture.id;
	SDL_GPUColorTargetDescription color_info;
	CF_MEMSET(&color_info, 0, sizeof(color_info));
	CF_ASSERT(g_ctx.canvas->texture);
	color_info.format = tex->format;
	color_info.blend_state.enable_blend = state->blend.enabled;
	color_info.blend_state.alpha_blend_op = s_wrap(state->blend.alpha_op);
	color_info.blend_state.color_blend_op = s_wrap(state->blend.rgb_op);
	color_info.blend_state.src_color_blendfactor = s_wrap(state->blend.rgb_src_blend_factor);
	color_info.blend_state.src_alpha_blendfactor = s_wrap(state->blend.alpha_src_blend_factor);
	color_info.blend_state.dst_color_blendfactor = s_wrap(state->blend.rgb_dst_blend_factor);
	color_info.blend_state.dst_alpha_blendfactor = s_wrap(state->blend.alpha_dst_blend_factor);
	int mask_r = (int)state->blend.write_R_enabled << 0;
	int mask_g = (int)state->blend.write_G_enabled << 1;
	int mask_b = (int)state->blend.write_B_enabled << 2;
	int mask_a = (int)state->blend.write_A_enabled << 3;
	color_info.blend_state.color_write_mask = (uint32_t)(mask_r | mask_g | mask_b | mask_a);

	SDL_GPUGraphicsPipelineCreateInfo pip_info;
	CF_MEMSET(&pip_info, 0, sizeof(pip_info));
	pip_info.primitive_type = s_wrap(state->primitive_type);
	pip_info.target_info.num_color_targets = 1;
	pip_info.target_info.color_target_descriptions = &color_info;
	pip_info.vertex_shader = shader->vs;
	pip_info.fragment_shader = shader->fs;
	// Always declare depth-stencil target if the canvas has one, regardless of whether
	// the material enables depth/stencil testing. This ensures all pipelines are compatible
	// with the render pass (which always includes the depth-stencil attachment).
	CF_TextureInternal* depth_texture = g_ctx.canvas->depth_stencil ? (CF_TextureInternal*)g_ctx.canvas->cf_depth_stencil.id : NULL;
	if (depth_texture) {
		pip_info.target_info.depth_stencil_format = depth_texture->format;
		pip_info.target_info.has_depth_stencil_target = true;
	}

	// Make sure the mesh vertex format is fully compatible with the vertex shader inputs.
	bool has_vertex_data = mesh->vertices.buffer ? true : false;
	bool has_instance_data = mesh->instances.buffer ? true : false;
	SDL_GPUVertexAttribute* attributes = SDL_stack_alloc(SDL_GPUVertexAttribute, mesh->attribute_count);
	int attribute_count = 0;
	for (int i = 0; i < mesh->attribute_count; ++i) {
		SDL_GPUVertexAttribute* attr = attributes + attribute_count;
		int idx = shader->get_input_index(mesh->attributes[i].name);
		if (idx >= 0) {
			CF_ShaderInputFormat input_fmt = shader->input_formats[idx];
			CF_VertexFormat mesh_fmt = mesh->attributes[i].format;
			CF_ASSERT(s_is_compatible(input_fmt, mesh_fmt));
			if (has_vertex_data) {
				attr->buffer_slot = mesh->attributes[i].per_instance ? 1 : 0; // Slot in `vertex_buffer_descriptions` below.
			} else {
				attr->buffer_slot = 0;
			}
			attr->location = shader->input_locations[idx];
			attr->format = s_wrap(mesh->attributes[i].format);
			attr->offset = mesh->attributes[i].offset;
			++attribute_count;
		}
	}
	CF_ASSERT(attribute_count == shader->input_count);
	pip_info.vertex_input_state.num_vertex_attributes = attribute_count;
	pip_info.vertex_input_state.vertex_attributes = attributes;
	SDL_GPUVertexBufferDescription vertex_buffer_descriptions[2];
	int vertex_buffer_descriptions_count = 0;
	if (has_vertex_data) {
		vertex_buffer_descriptions[vertex_buffer_descriptions_count].slot = 0;
		vertex_buffer_descriptions[vertex_buffer_descriptions_count].pitch = mesh->vertices.stride;
		vertex_buffer_descriptions[vertex_buffer_descriptions_count].input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;
		vertex_buffer_descriptions[vertex_buffer_descriptions_count].instance_step_rate = 0;
		vertex_buffer_descriptions_count++;
	}
	if (has_instance_data) {
		int instance_slot = has_vertex_data ? 1 : 0;
		vertex_buffer_descriptions[vertex_buffer_descriptions_count].slot = instance_slot;
		vertex_buffer_descriptions[vertex_buffer_descriptions_count].pitch = mesh->instances.stride;
		vertex_buffer_descriptions[vertex_buffer_descriptions_count].input_rate = SDL_GPU_VERTEXINPUTRATE_INSTANCE;
		vertex_buffer_descriptions[vertex_buffer_descriptions_count].instance_step_rate = 0;
		vertex_buffer_descriptions_count++;
	}
	pip_info.vertex_input_state.num_vertex_buffers = vertex_buffer_descriptions_count;
	pip_info.vertex_input_state.vertex_buffer_descriptions = vertex_buffer_descriptions;

	pip_info.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_FILL;
	pip_info.rasterizer_state.cull_mode = s_wrap(state->cull_mode);
	pip_info.rasterizer_state.front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE;
	pip_info.rasterizer_state.depth_bias_constant_factor = state->depth_bias_constant_factor;
	pip_info.rasterizer_state.depth_bias_clamp = state->depth_bias_clamp;
	pip_info.rasterizer_state.depth_bias_slope_factor = state->depth_bias_slope_factor;
	pip_info.rasterizer_state.enable_depth_bias = state->enable_depth_bias;
	pip_info.rasterizer_state.enable_depth_clip = state->enable_depth_clip;
	pip_info.multisample_state.sample_count = (SDL_GPUSampleCount)g_ctx.canvas->sample_count;
	pip_info.multisample_state.sample_mask = 0;

	bool depth_test_requested = state->depth_write_enabled || state->depth_compare != CF_COMPARE_FUNCTION_ALWAYS;
	bool depth_test_enabled = (depth_texture != NULL) && depth_test_requested;
	bool stencil_capable = depth_texture ? s_texture_supports_stencil(depth_texture) : false;
	bool stencil_test_enabled = stencil_capable && state->stencil.enabled;
	pip_info.depth_stencil_state.enable_depth_test = depth_test_enabled;
	pip_info.depth_stencil_state.enable_depth_write = (depth_texture != NULL) && state->depth_write_enabled;
	pip_info.depth_stencil_state.compare_op = s_wrap(state->depth_compare);
	pip_info.depth_stencil_state.enable_stencil_test = stencil_test_enabled;
	pip_info.depth_stencil_state.back_stencil_state.fail_op = s_wrap(state->stencil.back.fail_op);
	pip_info.depth_stencil_state.back_stencil_state.pass_op = s_wrap(state->stencil.back.pass_op);
	pip_info.depth_stencil_state.back_stencil_state.depth_fail_op = s_wrap(state->stencil.back.depth_fail_op);
	pip_info.depth_stencil_state.back_stencil_state.compare_op = s_wrap(state->stencil.back.compare);
	pip_info.depth_stencil_state.front_stencil_state.fail_op = s_wrap(state->stencil.front.fail_op);
	pip_info.depth_stencil_state.front_stencil_state.pass_op = s_wrap(state->stencil.front.pass_op);
	pip_info.depth_stencil_state.front_stencil_state.depth_fail_op = s_wrap(state->stencil.front.depth_fail_op);
	pip_info.depth_stencil_state.front_stencil_state.compare_op = s_wrap(state->stencil.front.compare);
	pip_info.depth_stencil_state.compare_mask = state->stencil.read_mask;
	pip_info.depth_stencil_state.write_mask = state->stencil.write_mask;

	SDL_GPUGraphicsPipeline* pip = SDL_CreateGPUGraphicsPipeline(g_ctx.device, &pip_info);
	CF_ASSERT(pip);
	return pip;
}

static CF_PipelineKey s_make_pipeline_key(CF_RenderState* state, CF_MeshInternal* mesh, CF_ShaderInternal* shader)
{
	CF_PipelineKey key;
	CF_MEMSET(&key, 0, sizeof(key));

	// Canvas state.
	CF_TextureInternal* tex = (CF_TextureInternal*)g_ctx.canvas->cf_texture.id;
	key.color_format = tex->format;
	key.sample_count = (int)g_ctx.canvas->sample_count;
	CF_TextureInternal* depth_texture = g_ctx.canvas->depth_stencil ? (CF_TextureInternal*)g_ctx.canvas->cf_depth_stencil.id : NULL;
	if (depth_texture) {
		key.has_depth_stencil = true;
		key.depth_format = depth_texture->format;
	}

	// Material render state (copied field-by-field to avoid padding issues, but
	// CF_RenderState is fully zeroed by cf_render_state_defaults, so direct copy is safe).
	key.render_state = *state;

	// Vertex layout.
	bool has_vertex_data = mesh->vertices.buffer != NULL;
	key.vertex_stride = has_vertex_data ? mesh->vertices.stride : 0;
	key.instance_stride = mesh->instances.buffer ? mesh->instances.stride : 0;
	int attr_count = 0;
	for (int i = 0; i < mesh->attribute_count && attr_count < CF_MAX_SHADER_INPUTS; ++i) {
		int idx = shader->get_input_index(mesh->attributes[i].name);
		if (idx >= 0) {
			key.attrs[attr_count].location = shader->input_locations[idx];
			key.attrs[attr_count].format = mesh->attributes[i].format;
			key.attrs[attr_count].offset = mesh->attributes[i].offset;
			key.attrs[attr_count].per_instance = mesh->attributes[i].per_instance;
			attr_count++;
		}
	}
	key.attribute_count = attr_count;

	return key;
}

void cf_sdlgpu_apply_shader(CF_Shader shader_handle, CF_Material material_handle)
{
	CF_ASSERT(g_ctx.canvas);
	CF_ASSERT(g_ctx.canvas->mesh);
	CF_MeshInternal* mesh = g_ctx.canvas->mesh;
	CF_MaterialInternal* material = (CF_MaterialInternal*)material_handle.id;
	CF_ShaderInternal* shader = (CF_ShaderInternal*)shader_handle.id;
	CF_RenderState* state = &material->state;

	// Content-based pipeline cache lookup.
	CF_PipelineKey key = s_make_pipeline_key(state, mesh, shader);
	SDL_GPUGraphicsPipeline* pip = NULL;
	for (int i = 0; i < shader->pip_cache.count(); ++i) {
		if (CF_MEMCMP(&key, &shader->pip_cache[i].key, sizeof(key)) == 0) {
			pip = shader->pip_cache[i].pip;
			break;
		}
	}
	if (!pip) {
		pip = s_build_pipeline(shader, state, mesh);
		CF_Pipeline entry;
		entry.key = key;
		entry.pip = pip;
		shader->pip_cache.add(entry);
	}
	CF_ASSERT(pip);

	SDL_GPUCommandBuffer* cmd = g_ctx.cmd;
	CF_ASSERT(cmd);

	// Begin a new render pass if needed, or reuse the active one.
	SDL_GPURenderPass* pass = g_ctx.active_pass;
	if (!pass) {
		SDL_GPUColorTargetInfo pass_color_info;
		CF_MEMSET(&pass_color_info, 0, sizeof(pass_color_info));
		pass_color_info.texture = g_ctx.canvas->texture;
		pass_color_info.clear_color = { app->clear_color.r, app->clear_color.g, app->clear_color.b, app->clear_color.a };
		pass_color_info.load_op = g_ctx.canvas->clear ? SDL_GPU_LOADOP_CLEAR : SDL_GPU_LOADOP_LOAD;
		pass_color_info.cycle = g_ctx.canvas->clear ? true : false;
		if (g_ctx.canvas->sample_count == CF_SAMPLE_COUNT_1) {
			pass_color_info.store_op = SDL_GPU_STOREOP_STORE;
		} else {
			pass_color_info.store_op = SDL_GPU_STOREOP_RESOLVE_AND_STORE;
			pass_color_info.resolve_texture = g_ctx.canvas->resolve_texture;
		}

		// Always include depth-stencil if the canvas has one.
		SDL_GPUDepthStencilTargetInfo pass_depth_stencil_info;
		CF_MEMSET(&pass_depth_stencil_info, 0, sizeof(pass_depth_stencil_info));
		SDL_GPUDepthStencilTargetInfo* depth_stencil_ptr = NULL;
		if (g_ctx.canvas->depth_stencil) {
			pass_depth_stencil_info.texture = g_ctx.canvas->depth_stencil;
			pass_depth_stencil_info.clear_depth = app->clear_depth;
			pass_depth_stencil_info.clear_stencil = app->clear_stencil;
			pass_depth_stencil_info.load_op = g_ctx.canvas->clear ? SDL_GPU_LOADOP_CLEAR : SDL_GPU_LOADOP_LOAD;
			pass_depth_stencil_info.store_op = SDL_GPU_STOREOP_STORE;
			pass_depth_stencil_info.stencil_load_op = g_ctx.canvas->clear ? SDL_GPU_LOADOP_CLEAR : SDL_GPU_LOADOP_LOAD;
			pass_depth_stencil_info.stencil_store_op = SDL_GPU_STOREOP_STORE;
			pass_depth_stencil_info.cycle = pass_color_info.cycle;
			depth_stencil_ptr = &pass_depth_stencil_info;
		}

		pass = SDL_BeginGPURenderPass(cmd, &pass_color_info, 1, depth_stencil_ptr);
		CF_ASSERT(pass);
		g_ctx.active_pass = pass;
		g_ctx.canvas->pass = pass;
		g_ctx.canvas->clear = false;
	}

	SDL_BindGPUGraphicsPipeline(pass, pip);
	SDL_GPUBufferBinding bind[2];
	bool has_vertex_data = mesh->vertices.buffer != NULL;
	bool has_instance_data = mesh->instances.buffer != NULL;
	if (has_vertex_data && has_instance_data) {
		bind[0].buffer = mesh->vertices.buffer; bind[0].offset = 0;
		bind[1].buffer = mesh->instances.buffer; bind[1].offset = 0;
		SDL_BindGPUVertexBuffers(pass, 0, bind, 2);
	} else if (has_instance_data) {
		bind[0].buffer = mesh->instances.buffer; bind[0].offset = 0;
		SDL_BindGPUVertexBuffers(pass, 0, bind, 1);
	} else {
		bind[0].buffer = mesh->vertices.buffer; bind[0].offset = 0;
		SDL_BindGPUVertexBuffers(pass, 0, bind, 1);
	}

	if (mesh->indices.buffer) {
		SDL_GPUBufferBinding index_bind = {
			.buffer = mesh->indices.buffer,
			.offset = 0
		};
		SDL_BindGPUIndexBuffer(pass, &index_bind, mesh->indices.stride == 2 ? SDL_GPU_INDEXELEMENTSIZE_16BIT : SDL_GPU_INDEXELEMENTSIZE_32BIT);
	}

	// Bind VS images to their respective slots.
	int vs_sampler_count = shader->vs_image_names.count();
	if (vs_sampler_count > 0) {
		SDL_GPUTextureSamplerBinding* vs_sampler_bindings = SDL_stack_alloc(SDL_GPUTextureSamplerBinding, vs_sampler_count);
		int found_vs_image_count = 0;
		for (int i = 0; found_vs_image_count < vs_sampler_count && i < material->vs.textures.count(); ++i) {
			const char* image_name = material->vs.textures[i].name;
			for (int j = 0; j < shader->vs_image_names.size(); ++j) {
				if (shader->vs_image_names[j] == image_name) {
					if (g_ctx.sampler_override) {
						vs_sampler_bindings[j].sampler = g_ctx.sampler_override;
					} else {
						vs_sampler_bindings[j].sampler = ((CF_TextureInternal*)material->vs.textures[i].handle.id)->sampler;
					}
					vs_sampler_bindings[j].texture = ((CF_TextureInternal*)material->vs.textures[i].handle.id)->tex;
					found_vs_image_count++;
				}
			}
		}
		CF_ASSERT(found_vs_image_count == vs_sampler_count);
		SDL_BindGPUVertexSamplers(pass, 0, vs_sampler_bindings, (Uint32)found_vs_image_count);
	}

	// Bind FS images to their respective slots.
	int fs_sampler_count = shader->fs_image_names.count();
	if (fs_sampler_count > 0) {
		SDL_GPUTextureSamplerBinding* fs_sampler_bindings = SDL_stack_alloc(SDL_GPUTextureSamplerBinding, fs_sampler_count);
		int found_fs_image_count = 0;
		for (int i = 0; found_fs_image_count < fs_sampler_count && i < material->fs.textures.count(); ++i) {
			const char* image_name = material->fs.textures[i].name;
			for (int j = 0; j < shader->fs_image_names.size(); ++j) {
				if (shader->fs_image_names[j] == image_name) {
					if (g_ctx.sampler_override) {
						fs_sampler_bindings[j].sampler = g_ctx.sampler_override;
					} else {
						fs_sampler_bindings[j].sampler = ((CF_TextureInternal*)material->fs.textures[i].handle.id)->sampler;
					}
					fs_sampler_bindings[j].texture = ((CF_TextureInternal*)material->fs.textures[i].handle.id)->tex;
					found_fs_image_count++;
				}
			}
		}
		CF_ASSERT(found_fs_image_count == fs_sampler_count);
		SDL_BindGPUFragmentSamplers(pass, 0, fs_sampler_bindings, (Uint32)found_fs_image_count);
	}

	// Clear sampler override after use.
	g_ctx.sampler_override = NULL;

	// Copy over uniform data.
	s_copy_uniforms(cmd, &material->block_arena, shader, &material->vs, true);
	s_copy_uniforms(cmd, &material->block_arena, shader, &material->fs, false);

	SDL_SetGPUStencilReference(pass, state->stencil.reference);
}

void cf_sdlgpu_draw_elements()
{
	CF_MeshInternal* mesh = g_ctx.canvas->mesh;
	if (mesh->instances.buffer) {
		if (mesh->indices.buffer) {
			SDL_DrawGPUIndexedPrimitives(g_ctx.canvas->pass, mesh->indices.element_count, mesh->instances.element_count, 0, 0, 0);
		} else {
			SDL_DrawGPUPrimitives(g_ctx.canvas->pass, mesh->vertices.element_count, mesh->instances.element_count, 0, 0);
		}
	} else {
		if (mesh->indices.buffer) {
			SDL_DrawGPUIndexedPrimitives(g_ctx.canvas->pass, mesh->indices.element_count, 1, 0, 0, 0);
		} else {
			SDL_DrawGPUPrimitives(g_ctx.canvas->pass, mesh->vertices.element_count, 1, 0, 0);
		}
	}
	app->draw_call_count++;
}

void* cf_sdlgpu_create_draw_sampler(CF_Filter filter)
{
	SDL_GPUSamplerCreateInfo sampler_info = SDL_GPUSamplerCreateInfoDefaults();
	sampler_info.min_filter = s_wrap(filter);
	sampler_info.mag_filter = s_wrap(filter);
	SDL_GPUSampler* sampler = SDL_CreateGPUSampler(g_ctx.device, &sampler_info);
	return (void*)sampler;
}

void cf_sdlgpu_destroy_draw_sampler(void* sampler)
{
	if (sampler) {
		SDL_ReleaseGPUSampler(g_ctx.device, (SDL_GPUSampler*)sampler);
	}
}

void cf_sdlgpu_set_sampler_override(void* sampler)
{
	g_ctx.sampler_override = (SDL_GPUSampler*)sampler;
}

//--------------------------------------------------------------------------------------------------
// Compute shaders.

struct CF_ComputeShaderInternal
{
	SDL_GPUComputePipeline* pipeline;

	// Reflection data for name-matching uniforms from material.
	int cs_uniform_block_count;
	int cs_block_sizes[CF_MAX_UNIFORM_BLOCK_COUNT];
	Cute::Array<CF_UniformBlockMember> cs_uniform_block_members[CF_MAX_UNIFORM_BLOCK_COUNT];

	// Sampler names + binding slots for name-matching textures from material.
	Cute::Array<const char*> cs_sampler_names;
	Cute::Array<int> cs_sampler_binding_slots;

	// Resource counts (from SPIRV reflection, needed for pipeline creation).
	int num_samplers;
	int num_readonly_storage_textures;
	int num_readonly_storage_buffers;
	int num_readwrite_storage_textures;
	int num_readwrite_storage_buffers;
	int num_uniform_buffers;

	// Hot-reload.
	const char* path; // NULL if not file-based.

	CF_INLINE int cs_index(const char* name, int block_index)
	{
		for (int i = 0; i < cs_uniform_block_members[block_index].size(); ++i) {
			if (cs_uniform_block_members[block_index][i].name == name) return i;
		}
		return -1;
	}
};

struct CF_StorageBufferInternal
{
	SDL_GPUBuffer* buffer;
	SDL_GPUTransferBuffer* transfer_buffer;
	int size;
};

CF_ComputeShader cf_sdlgpu_make_compute_shader_from_bytecode(CF_ShaderBytecode bytecode)
{
	CF_ComputeShaderInternal* cs = CF_NEW(CF_ComputeShaderInternal);
	CF_MEMSET(cs, 0, sizeof(*cs));

	const CF_ShaderInfo* info = &bytecode.shader_info;

	// Load sampler reflection data.
	for (int i = 0; i < info->num_images; ++i) {
		cs->cs_sampler_names.add(sintern(info->image_names[i]));
		cs->cs_sampler_binding_slots.add(info->image_binding_slots ? info->image_binding_slots[i] : i);
	}

	// Load uniform block reflection data.
	cs->cs_uniform_block_count = info->num_uniforms;
	const CF_ShaderUniformMemberInfo* member_infos = info->uniform_members;
	for (int i = 0; i < info->num_uniforms; ++i) {
		const CF_ShaderUniformInfo* block_info = &info->uniforms[i];
		int block_index = block_info->block_index;
		cs->cs_block_sizes[block_index] = block_info->block_size;

		const char* block_name = sintern(block_info->block_name);
		for (int j = 0; j < block_info->num_members; ++j) {
			const CF_ShaderUniformMemberInfo* member_info = &member_infos[j];
			CF_UniformBlockMember block_member;
			block_member.name = sintern(member_info->name);
			block_member.block_name = block_name;
			block_member.type = s_uniform_type(member_info->type);
			CF_ASSERT(block_member.type != CF_UNIFORM_TYPE_UNKNOWN);
			block_member.array_element_count = member_info->array_length;
			block_member.size = s_uniform_size(block_member.type) * member_info->array_length;
			block_member.offset = member_info->offset;
			cs->cs_uniform_block_members[block_index].add(block_member);
		}
		member_infos += block_info->num_members;
	}

	// Store resource counts.
	cs->num_samplers = info->num_samplers;
	cs->num_readonly_storage_textures = info->num_storage_textures;
	cs->num_readonly_storage_buffers = info->num_storage_buffers;
	cs->num_readwrite_storage_textures = 0;
	cs->num_readwrite_storage_buffers = 0;
	cs->num_uniform_buffers = info->num_uniforms;

	// Create the compute pipeline.
	if (SDL_GetGPUShaderFormats(g_ctx.device) == SDL_GPU_SHADERFORMAT_SPIRV) {
		SDL_GPUComputePipelineCreateInfo pip_info = {};
		pip_info.code = bytecode.content;
		pip_info.code_size = bytecode.size;
		pip_info.entrypoint = "main";
		pip_info.format = SDL_GPU_SHADERFORMAT_SPIRV;
		pip_info.num_samplers = (Uint32)cs->num_samplers;
		pip_info.num_readonly_storage_textures = (Uint32)cs->num_readonly_storage_textures;
		pip_info.num_readonly_storage_buffers = (Uint32)cs->num_readonly_storage_buffers;
		pip_info.num_readwrite_storage_textures = (Uint32)cs->num_readwrite_storage_textures;
		pip_info.num_readwrite_storage_buffers = (Uint32)cs->num_readwrite_storage_buffers;
		pip_info.num_uniform_buffers = (Uint32)cs->num_uniform_buffers;
		pip_info.threadcount_x = 0;
		pip_info.threadcount_y = 0;
		pip_info.threadcount_z = 0;
		cs->pipeline = SDL_CreateGPUComputePipeline(g_ctx.device, &pip_info);
	} else {
		SDL_ShaderCross_ComputePipelineMetadata metadata = {};
		metadata.num_samplers = (Uint32)cs->num_samplers;
		metadata.num_readonly_storage_textures = (Uint32)cs->num_readonly_storage_textures;
		metadata.num_readonly_storage_buffers = (Uint32)cs->num_readonly_storage_buffers;
		metadata.num_readwrite_storage_textures = (Uint32)cs->num_readwrite_storage_textures;
		metadata.num_readwrite_storage_buffers = (Uint32)cs->num_readwrite_storage_buffers;
		metadata.num_uniform_buffers = (Uint32)cs->num_uniform_buffers;
		metadata.threadcount_x = 0;
		metadata.threadcount_y = 0;
		metadata.threadcount_z = 0;

		SDL_ShaderCross_SPIRV_Info spirvInfo;
		CF_MEMSET(&spirvInfo, 0, sizeof(spirvInfo));
		spirvInfo.bytecode = bytecode.content;
		spirvInfo.bytecode_size = bytecode.size;
		spirvInfo.entrypoint = "main";
		spirvInfo.shader_stage = SDL_SHADERCROSS_SHADERSTAGE_COMPUTE;
		spirvInfo.enable_debug = false;
		spirvInfo.name = "compute.glsl";
		spirvInfo.props = SDL_CreateProperties();
		cs->pipeline = SDL_ShaderCross_CompileComputePipelineFromSPIRV(g_ctx.device, &spirvInfo, &metadata);
		SDL_DestroyProperties(spirvInfo.props);
	}
	CF_ASSERT(cs->pipeline);

	CF_ComputeShader result;
	result.id = (uint64_t)cs;
	return result;
}

void cf_sdlgpu_destroy_compute_shader(CF_ComputeShader shader)
{
	CF_ComputeShaderInternal* cs = (CF_ComputeShaderInternal*)shader.id;
	if (!cs) return;
	SDL_ReleaseGPUComputePipeline(g_ctx.device, cs->pipeline);
	CF_FREE(cs);
}

CF_StorageBuffer cf_sdlgpu_make_storage_buffer(CF_StorageBufferParams params)
{
	CF_StorageBufferInternal* sb = CF_NEW(CF_StorageBufferInternal);
	CF_MEMSET(sb, 0, sizeof(*sb));
	sb->size = params.size;

	SDL_GPUBufferUsageFlags usage = 0;
	if (params.compute_readable) usage |= SDL_GPU_BUFFERUSAGE_COMPUTE_STORAGE_READ;
	if (params.compute_writable) usage |= SDL_GPU_BUFFERUSAGE_COMPUTE_STORAGE_WRITE;
	if (params.graphics_readable) usage |= SDL_GPU_BUFFERUSAGE_GRAPHICS_STORAGE_READ;

	SDL_GPUBufferCreateInfo buf_info = {};
	buf_info.usage = usage;
	buf_info.size = (Uint32)params.size;
	sb->buffer = SDL_CreateGPUBuffer(g_ctx.device, &buf_info);

	SDL_GPUTransferBufferCreateInfo tbuf_info = {};
	tbuf_info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
	tbuf_info.size = (Uint32)params.size;
	sb->transfer_buffer = SDL_CreateGPUTransferBuffer(g_ctx.device, &tbuf_info);

	CF_StorageBuffer result;
	result.id = (uint64_t)sb;
	return result;
}

void cf_sdlgpu_update_storage_buffer(CF_StorageBuffer buffer, const void* data, int size)
{
	CF_StorageBufferInternal* sb = (CF_StorageBufferInternal*)buffer.id;
	s_end_active_pass();

	// Resize if necessary.
	if (size > sb->size) {
		SDL_ReleaseGPUBuffer(g_ctx.device, sb->buffer);
		SDL_ReleaseGPUTransferBuffer(g_ctx.device, sb->transfer_buffer);

		int new_size = size * 2;
		sb->size = new_size;

		// Infer usage flags from the old buffer -- just re-create with same flags.
		// For simplicity, use READ since we're uploading.
		SDL_GPUBufferCreateInfo buf_info = {};
		buf_info.usage = SDL_GPU_BUFFERUSAGE_COMPUTE_STORAGE_READ;
		buf_info.size = (Uint32)new_size;
		sb->buffer = SDL_CreateGPUBuffer(g_ctx.device, &buf_info);

		SDL_GPUTransferBufferCreateInfo tbuf_info = {};
		tbuf_info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
		tbuf_info.size = (Uint32)new_size;
		sb->transfer_buffer = SDL_CreateGPUTransferBuffer(g_ctx.device, &tbuf_info);
	}

	// Map, copy, unmap.
	void* p = SDL_MapGPUTransferBuffer(g_ctx.device, sb->transfer_buffer, true);
	CF_MEMCPY(p, data, size);
	SDL_UnmapGPUTransferBuffer(g_ctx.device, sb->transfer_buffer);

	// Upload via copy pass.
	SDL_GPUCommandBuffer* cmd = g_ctx.cmd ? g_ctx.cmd : SDL_AcquireGPUCommandBuffer(g_ctx.device);
	SDL_GPUCopyPass* pass = SDL_BeginGPUCopyPass(cmd);
	SDL_GPUTransferBufferLocation location;
	location.offset = 0;
	location.transfer_buffer = sb->transfer_buffer;
	SDL_GPUBufferRegion region;
	region.buffer = sb->buffer;
	region.offset = 0;
	region.size = size;
	SDL_UploadToGPUBuffer(pass, &location, &region, true);
	SDL_EndGPUCopyPass(pass);
	if (!g_ctx.cmd) SDL_SubmitGPUCommandBuffer(cmd);
}

void cf_sdlgpu_destroy_storage_buffer(CF_StorageBuffer buffer)
{
	CF_StorageBufferInternal* sb = (CF_StorageBufferInternal*)buffer.id;
	if (!sb) return;
	SDL_ReleaseGPUBuffer(g_ctx.device, sb->buffer);
	SDL_ReleaseGPUTransferBuffer(g_ctx.device, sb->transfer_buffer);
	CF_FREE(sb);
}

void cf_sdlgpu_dispatch_compute(CF_ComputeShader shader, CF_Material material_handle, CF_ComputeDispatch dispatch)
{
	CF_ComputeShaderInternal* cs = (CF_ComputeShaderInternal*)shader.id;
	CF_MaterialInternal* material = (CF_MaterialInternal*)material_handle.id;

	s_end_active_pass();

	SDL_GPUCommandBuffer* cmd = g_ctx.cmd;
	CF_ASSERT(cmd);

	// Build RW texture bindings for pass creation.
	SDL_GPUStorageTextureReadWriteBinding* rw_tex_bindings = NULL;
	if (dispatch.rw_texture_count > 0) {
		rw_tex_bindings = SDL_stack_alloc(SDL_GPUStorageTextureReadWriteBinding, dispatch.rw_texture_count);
		for (int i = 0; i < dispatch.rw_texture_count; ++i) {
			CF_MEMSET(rw_tex_bindings + i, 0, sizeof(SDL_GPUStorageTextureReadWriteBinding));
			CF_TextureInternal* tex = (CF_TextureInternal*)dispatch.rw_textures[i].id;
			rw_tex_bindings[i].texture = tex->tex;
		}
	}

	// Build RW buffer bindings for pass creation.
	SDL_GPUStorageBufferReadWriteBinding* rw_buf_bindings = NULL;
	if (dispatch.rw_buffer_count > 0) {
		rw_buf_bindings = SDL_stack_alloc(SDL_GPUStorageBufferReadWriteBinding, dispatch.rw_buffer_count);
		for (int i = 0; i < dispatch.rw_buffer_count; ++i) {
			CF_MEMSET(rw_buf_bindings + i, 0, sizeof(SDL_GPUStorageBufferReadWriteBinding));
			CF_StorageBufferInternal* sb = (CF_StorageBufferInternal*)dispatch.rw_buffers[i].id;
			rw_buf_bindings[i].buffer = sb->buffer;
		}
	}

	// Begin compute pass with RW resources.
	SDL_GPUComputePass* pass = SDL_BeginGPUComputePass(
		cmd,
		rw_tex_bindings,
		(Uint32)dispatch.rw_texture_count,
		rw_buf_bindings,
		(Uint32)dispatch.rw_buffer_count
	);
	CF_ASSERT(pass);

	// Bind the pipeline.
	SDL_BindGPUComputePipeline(pass, cs->pipeline);

	// Bind sampled textures from material (name-matched against cs_sampler_names).
	int sampler_count = cs->cs_sampler_names.count();
	if (sampler_count > 0) {
		SDL_GPUTextureSamplerBinding* sampler_bindings = SDL_stack_alloc(SDL_GPUTextureSamplerBinding, sampler_count);
		int found_count = 0;
		for (int i = 0; found_count < sampler_count && i < material->cs.textures.count(); ++i) {
			const char* image_name = material->cs.textures[i].name;
			for (int j = 0; j < cs->cs_sampler_names.size(); ++j) {
				if (cs->cs_sampler_names[j] == image_name) {
					sampler_bindings[j].sampler = ((CF_TextureInternal*)material->cs.textures[i].handle.id)->sampler;
					sampler_bindings[j].texture = ((CF_TextureInternal*)material->cs.textures[i].handle.id)->tex;
					found_count++;
				}
			}
		}
		CF_ASSERT(found_count == sampler_count);
		SDL_BindGPUComputeSamplers(pass, 0, sampler_bindings, (Uint32)found_count);
	}

	// Bind readonly storage textures.
	if (dispatch.ro_texture_count > 0) {
		SDL_GPUTexture** ro_textures = SDL_stack_alloc(SDL_GPUTexture*, dispatch.ro_texture_count);
		for (int i = 0; i < dispatch.ro_texture_count; ++i) {
			CF_TextureInternal* tex = (CF_TextureInternal*)dispatch.ro_textures[i].id;
			ro_textures[i] = tex->tex;
		}
		SDL_BindGPUComputeStorageTextures(pass, 0, ro_textures, (Uint32)dispatch.ro_texture_count);
	}

	// Bind readonly storage buffers.
	if (dispatch.ro_buffer_count > 0) {
		SDL_GPUBuffer** ro_buffers = SDL_stack_alloc(SDL_GPUBuffer*, dispatch.ro_buffer_count);
		for (int i = 0; i < dispatch.ro_buffer_count; ++i) {
			CF_StorageBufferInternal* sb = (CF_StorageBufferInternal*)dispatch.ro_buffers[i].id;
			ro_buffers[i] = sb->buffer;
		}
		SDL_BindGPUComputeStorageBuffers(pass, 0, ro_buffers, (Uint32)dispatch.ro_buffer_count);
	}

	// Push uniforms from material (name-matched against cs_uniform_block_members).
	{
		void* ub_ptrs[CF_MAX_UNIFORM_BLOCK_COUNT] = { };
		int ub_sizes[CF_MAX_UNIFORM_BLOCK_COUNT] = { };
		for (int block_index = 0; block_index < cs->cs_uniform_block_count; ++block_index) {
			for (int i = 0; i < material->cs.uniforms.count(); ++i) {
				CF_Uniform uniform = material->cs.uniforms[i];
				int idx = cs->cs_index(uniform.name, block_index);
				if (idx >= 0) {
					if (!ub_ptrs[block_index]) {
						int size = cs->cs_block_sizes[block_index];
						void* block = cf_arena_alloc(&material->block_arena, size);
						CF_MEMSET(block, 0, size);
						ub_ptrs[block_index] = block;
						ub_sizes[block_index] = size;
					}
					int offset = cs->cs_uniform_block_members[block_index][idx].offset;
					void* dst = (void*)(((uintptr_t)ub_ptrs[block_index]) + offset);
					CF_MEMCPY(dst, uniform.data, uniform.size);
				}
			}
		}
		for (int i = 0; i < CF_MAX_UNIFORM_BLOCK_COUNT; ++i) {
			if (ub_ptrs[i]) {
				SDL_PushGPUComputeUniformData(cmd, i, ub_ptrs[i], (uint32_t)ub_sizes[i]);
			}
		}
		cf_arena_reset(&material->block_arena);
	}

	// Dispatch.
	SDL_DispatchGPUCompute(pass, (Uint32)dispatch.group_count_x, (Uint32)dispatch.group_count_y, (Uint32)dispatch.group_count_z);
	SDL_EndGPUComputePass(pass);
}

#endif
