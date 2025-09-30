/*
	Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

#ifndef CF_GRAPHICS_INTERNAL_H
#define CF_GRAPHICS_INTERNAL_H

#include <cute_array.h>
#include <cute_result.h>
#include <cute_graphics.h>
#include <SDL3/SDL_gpu.h>

#define CF_MAX_UNIFORM_BLOCK_COUNT (4)

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
	case CF_SHADER_INPUT_FORMAT_INT:
		return vertex_format == CF_VERTEX_FORMAT_INT;

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

CF_INLINE int s_uniform_size(CF_UniformType type)
{
	switch (type) {
	case CF_UNIFORM_TYPE_FLOAT:  return 4;
	case CF_UNIFORM_TYPE_FLOAT2: return 8;
	case CF_UNIFORM_TYPE_FLOAT3: return 12;
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

//--------------------------------------------------------------------------------------------------
// Helpers used by both SDL_Gpu and GL backends.

CF_INLINE bool cf_pixel_format_is_depth(CF_PixelFormat format)
{
	switch (format) {
	case CF_PIXEL_FORMAT_D16_UNORM:
	case CF_PIXEL_FORMAT_D24_UNORM:
	case CF_PIXEL_FORMAT_D32_FLOAT:
	case CF_PIXEL_FORMAT_D24_UNORM_S8_UINT:
	case CF_PIXEL_FORMAT_D32_FLOAT_S8_UINT:
		return true;
	default:
		return false;
	}
}

CF_INLINE bool cf_pixel_format_has_alpha(CF_PixelFormat format)
{
	switch (format) {
	case CF_PIXEL_FORMAT_A8_UNORM:
	case CF_PIXEL_FORMAT_R8G8B8A8_UNORM:
	case CF_PIXEL_FORMAT_R16G16B16A16_UNORM:
	case CF_PIXEL_FORMAT_R10G10B10A2_UNORM:
	case CF_PIXEL_FORMAT_B5G5R5A1_UNORM:
	case CF_PIXEL_FORMAT_B4G4R4A4_UNORM:
	case CF_PIXEL_FORMAT_B8G8R8A8_UNORM:
	case CF_PIXEL_FORMAT_R8G8B8A8_SNORM:
	case CF_PIXEL_FORMAT_R16G16B16A16_SNORM:
	case CF_PIXEL_FORMAT_R16G16B16A16_FLOAT:
	case CF_PIXEL_FORMAT_R32G32B32A32_FLOAT:
	case CF_PIXEL_FORMAT_R8G8B8A8_UINT:
	case CF_PIXEL_FORMAT_R16G16B16A16_UINT:
	case CF_PIXEL_FORMAT_R8G8B8A8_INT:
	case CF_PIXEL_FORMAT_R16G16B16A16_INT:
	case CF_PIXEL_FORMAT_R8G8B8A8_UNORM_SRGB:
	case CF_PIXEL_FORMAT_B8G8R8A8_UNORM_SRGB:
	case CF_PIXEL_FORMAT_BC1_RGBA_UNORM:
	case CF_PIXEL_FORMAT_BC2_RGBA_UNORM:
	case CF_PIXEL_FORMAT_BC3_RGBA_UNORM:
	case CF_PIXEL_FORMAT_BC7_RGBA_UNORM:
	case CF_PIXEL_FORMAT_BC1_RGBA_UNORM_SRGB:
	case CF_PIXEL_FORMAT_BC2_RGBA_UNORM_SRGB:
	case CF_PIXEL_FORMAT_BC3_RGBA_UNORM_SRGB:
	case CF_PIXEL_FORMAT_BC7_RGBA_UNORM_SRGB:
		return true;
	default:
		return false;
	}
}

CF_INLINE bool s_is_depth(CF_PixelFormat format)
{
	return format >= CF_PIXEL_FORMAT_D16_UNORM;
}

void cf_load_internal_shaders();
void cf_unload_internal_shaders();
CF_Shader cf_make_draw_shader_internal(const char* path);
CF_Shader cf_make_draw_shader_from_source_internal(const char* src);
CF_Shader cf_make_draw_shader_from_bytecode_internal(CF_ShaderBytecode bytecode);
CF_Shader cf_make_draw_blit_shader_internal(const char* path);
CF_Shader cf_make_draw_blit_shader_from_source_internal(const char* src);
CF_Shader cf_make_draw_blit_shader_from_bytecode_internal(CF_ShaderBytecode bytecode);
CF_Shader cf_make_shader_from_source_internal(const char* vs_src, const char* fs_src, const char* user_shd = NULL);
void cf_canvas_get_size(CF_Canvas canvas, int* w, int* h);
void cf_shader_watch();

#ifndef CF_EMSCRIPTEN

CF_Result cf_sdlgpu_init(const char* device_name, bool debug, CF_BackendType* backend_type);
SDL_GPUDevice* cf_sdlgpu_get_device();
SDL_GPUTexture* cf_sdlgpu_get_swapchain_texture();
SDL_GPUCommandBuffer* cf_sdlgpu_get_command_buffer();
void cf_sdlgpu_attach(SDL_Window* window);
bool cf_sdlgpu_supports_msaa(int sample_count);
void cf_sdlgpu_flush();
void cf_sdlgpu_set_vsync(bool true_turn_on_vsync);
void cf_sdlgpu_set_vsync_mailbox(bool true_turn_on_vsync);
void cf_sdlgpu_begin_frame();
void cf_sdlgpu_blit_canvas(CF_Canvas canvas);
void cf_sdlgpu_end_frame();
void cf_sdlgpu_cleanup();

#endif

CF_Result cf_gles_init(bool debug);
SDL_GLContext cf_gles_get_gl_context();
void cf_gles_attach(SDL_Window* window);
bool cf_gles_supports_msaa(int sample_count);
void cf_gles_flush();
void cf_gles_set_vsync(bool true_turn_on_vsync);
void cf_gles_begin_frame();
void cf_gles_blit_canvas(CF_Canvas canvas);
void cf_gles_present();
void cf_gles_end_frame();
void cf_gles_cleanup();

#endif // CF_GRAPHICS_INTERNAL_H
