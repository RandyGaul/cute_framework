/*
	Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

#ifndef CF_GRAPHICS_INTERNAL_H
#define CF_GRAPHICS_INTERNAL_H

#include <cute_app.h>
#include <cute_array.h>
#include <cute_result.h>
#include <cute_graphics.h>
#include <cute_input.h>
#include <cute_image.h>
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
		return vertex_format == CF_VERTEX_FORMAT_UBYTE4_NORM || vertex_format == CF_VERTEX_FORMAT_UBYTE4
			|| vertex_format == CF_VERTEX_FORMAT_USHORT4 || vertex_format == CF_VERTEX_FORMAT_UINT4;

	case CF_SHADER_INPUT_FORMAT_IVEC4:
		return vertex_format == CF_VERTEX_FORMAT_SHORT4 || vertex_format == CF_VERTEX_FORMAT_SHORT4_NORM
			|| vertex_format == CF_VERTEX_FORMAT_INT4;

	case CF_SHADER_INPUT_FORMAT_IVEC2:
		return vertex_format == CF_VERTEX_FORMAT_SHORT2 || vertex_format == CF_VERTEX_FORMAT_SHORT2_NORM
			|| vertex_format == CF_VERTEX_FORMAT_INT2;

	case CF_SHADER_INPUT_FORMAT_UVEC2:
		return vertex_format == CF_VERTEX_FORMAT_HALF2
			|| vertex_format == CF_VERTEX_FORMAT_USHORT2 || vertex_format == CF_VERTEX_FORMAT_UINT2;

	// Not supported.
	case CF_SHADER_INPUT_FORMAT_UVEC3:
	case CF_SHADER_INPUT_FORMAT_IVEC3:
	case CF_SHADER_INPUT_FORMAT_UNKNOWN:
	default:
		return false;
	}
}

CF_INLINE CF_UniformType s_uniform_type(CF_ShaderInfoDataType type)
{
	switch (type) {
	case CF_SHADER_INFO_TYPE_UNKNOWN: return CF_UNIFORM_TYPE_UNKNOWN;
	case CF_SHADER_INFO_TYPE_FLOAT:   return CF_UNIFORM_TYPE_FLOAT;
	case CF_SHADER_INFO_TYPE_FLOAT2:  return CF_UNIFORM_TYPE_FLOAT2;
	case CF_SHADER_INFO_TYPE_FLOAT3:  return CF_UNIFORM_TYPE_FLOAT3;
	case CF_SHADER_INFO_TYPE_FLOAT4:  return CF_UNIFORM_TYPE_FLOAT4;
	case CF_SHADER_INFO_TYPE_SINT:	return CF_UNIFORM_TYPE_INT;
	case CF_SHADER_INFO_TYPE_SINT2:   return CF_UNIFORM_TYPE_INT2;
	case CF_SHADER_INFO_TYPE_SINT4:   return CF_UNIFORM_TYPE_INT4;
	case CF_SHADER_INFO_TYPE_MAT4:	return CF_UNIFORM_TYPE_MAT4;
	default: return CF_UNIFORM_TYPE_UNKNOWN;
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
	CF_MaterialState cs;
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

CF_INLINE bool cf_pixel_format_has_stencil(CF_PixelFormat format)
{
	switch (format) {
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
CF_Shader cf_make_draw_shader_from_source_internal(const char* src, const char* src_name = NULL);
CF_Shader cf_make_draw_shader_from_bytecode_internal(CF_ShaderBytecode bytecode);
CF_Shader cf_make_draw_blit_shader_internal(const char* path);
void cf_destroy_shader_internal(CF_Shader shader_handle);
CF_Shader cf_make_draw_blit_shader_from_source_internal(const char* src, const char* src_name = NULL);
CF_Shader cf_make_draw_blit_shader_from_bytecode_internal(CF_ShaderBytecode bytecode);
CF_Shader cf_make_shader_from_source_internal(const char* vs_src, const char* fs_src, const char* user_shd = NULL, const char* user_shd_name = NULL);
// Rebuilds the SDF command pipelines (draw/tile/count/gather) with new custom_shapes.shd
// content; compile-then-swap, returns false (old pipelines intact) on any failure.
bool cf_recompile_draw_pipelines(const char* custom_shapes_src);
void cf_shader_watch();
// Swaps the internal contents of two shaders (hot-reload: user-held handles keep
// pointing at the same internals, which get fresh guts).
void cf_shader_swap_contents(CF_Shader a, CF_Shader b);
void cf_compute_shader_swap_contents(CF_ComputeShader a, CF_ComputeShader b);

#ifndef CF_EMSCRIPTEN

CF_Result cf_sdlgpu_init(const char* device_name, bool debug, CF_BackendType* backend_type);
// True when the device consumes DXBC but not SPIR-V (i.e. D3D12): runtime
// shader compilation then emits HLSL for the FXC -> DXBC path.
bool cf_sdlgpu_wants_hlsl();
// True when the device consumes MSL but not SPIR-V (i.e. Metal): runtime
// shader compilation then emits MSL source.
bool cf_sdlgpu_wants_msl();
SDL_GPUDevice* cf_sdlgpu_get_device();
SDL_GPUTexture* cf_sdlgpu_get_swapchain_texture();
SDL_GPUCommandBuffer* cf_sdlgpu_get_command_buffer();
void cf_sdlgpu_attach(SDL_Window* window);
bool cf_sdlgpu_supports_msaa(int sample_count);
void cf_sdlgpu_flush();
void cf_sdlgpu_gpu_sync();
bool cf_sdlgpu_set_present_mode(CF_PresentMode mode);
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
void cf_gles_gpu_sync();
bool cf_gles_set_present_mode(CF_PresentMode mode);
void cf_gles_begin_frame();
void cf_gles_blit_canvas(CF_Canvas canvas);
void cf_gles_end_frame();
void cf_gles_cleanup();

// Draw system sampler override API.
// Used by the draw system to dynamically switch between nearest/linear filtering.
// Dispatch shims are defined in cute_graphics.cpp via CF_DISPATCH_SHIM macros.
void* cf_create_draw_sampler(CF_Filter filter);
void cf_destroy_draw_sampler(void* sampler);
void cf_set_sampler_override(void* sampler);

// cf_apply_vs/fs_storage_buffers, cf_push/pop_gpu_label, and cf_draw_elements_instanced
// used to be declared here as internal-only; they are public in cute_graphics.h now.
void cf_current_canvas_size(int* w, int* h);

// Region-granular texture ops used by the draw layer's atlas cache to rebuild atlas pages
// GPU-side (repacks become texture->texture copies instead of CPU pixel re-fetch + upload).
// Dispatch shims live in cute_graphics.cpp, implemented in both backends. 2D textures only.
// Coordinates are in pixels, row 0 being the first row of uploaded pixel data.
void cf_texture_update_region(CF_Texture texture, int x, int y, int w, int h, void* pixels);
void cf_texture_copy_region(CF_Texture dst, int dst_x, int dst_y, CF_Texture src, int src_x, int src_y, int w, int h);

// Mesh introspection for the draw3d layer (cute_draw3d.cpp): whether an attribute with this
// name exists (used to detect its reserved instance attributes), and the mesh's instance
// stride (0 when no instance buffer is set -- a nonzero stride on a mesh without the reserved
// attributes marks user-owned instancing, the draw3d escape hatch).
bool cf_mesh_has_vertex_attribute(CF_Mesh mesh, const char* name);
int cf_mesh_instance_stride(CF_Mesh mesh);
// Whether the draw3d layer augmented this mesh (appended its reserved instance attributes and
// installed its own instance buffer). Stored on the mesh itself so a destroyed handle reused
// for a new mesh can never inherit a stale classification -- the escape-hatch test depends on
// this: a mesh with an instance buffer that draw3d did NOT install is user-owned instancing
// and must never be augmented or have its instance buffer overwritten.
bool cf_mesh_draw3d_augmented(CF_Mesh mesh);
void cf_mesh_set_draw3d_augmented(CF_Mesh mesh);

// Standalone per-instance vertex buffers for the draw3d layer's baked draw lists: uploaded
// once at bake, then bound in place of the applied mesh's own instance buffer for exactly one
// draw via the override (set it before cf_apply_shader; it clears itself after the next
// cf_draw_elements). The mesh must still declare its per-instance attributes -- the override
// only swaps which buffer feeds them, so strides must match the mesh's instance stride.
uint64_t cf_make_instance_buffer(int size_in_bytes, int stride);
void cf_update_instance_buffer(uint64_t handle, void* data, int count);
void cf_destroy_instance_buffer(uint64_t handle);
// `offset_bytes` positions the binding within the buffer, so many commands can share one
// per-flush staging buffer (each binds its own slice). Must be a multiple of the stride.
void cf_apply_instance_buffer_override(uint64_t handle, int count, int offset_bytes);

#endif // CF_GRAPHICS_INTERNAL_H
