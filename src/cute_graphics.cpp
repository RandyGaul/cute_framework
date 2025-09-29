/*
	Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

#include <cute_defines.h>
#include <cute_c_runtime.h>
#include <cute_graphics.h>
#include <cute_file_system.h>

#include <internal/cute_alloc_internal.h>
#include <internal/cute_app_internal.h>
#include <internal/cute_graphics_internal.h>

#include "cute_shader/cute_shader.h"
#include "cute_shader/builtin_shaders.h"
#include "data/builtin_shaders_bytecode.h"
#include <SDL3_shadercross/SDL_shadercross.h>
#include <spirv_cross_c.h>

spvc_context g_spvc_context = NULL;

struct CF_CanvasInternal;
static CF_CanvasInternal* s_canvas = NULL;
static CF_CanvasInternal* s_default_canvas = NULL;

#include <float.h>

using namespace Cute;

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

//--------------------------------------------------------------------------------------------------
// SDL_Gpu implementation of cute_graphics.h.

CF_INLINE CF_UniformType s_uniform_type(CF_ShaderInfoDataType type)
{
	SDL_GL_CreateContext(app->window);

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

CF_INLINE CF_ShaderInputFormat s_wrap(CF_ShaderInfoDataType type)
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

CF_BackendType sdlgpu_query_backend()
{
	SDL_GPUShaderFormat format = SDL_GetGPUShaderFormats(app->device);
	switch (format) {
	case SDL_GPU_SHADERFORMAT_INVALID:  return CF_BACKEND_TYPE_INVALID;
	case SDL_GPU_SHADERFORMAT_PRIVATE:  return CF_BACKEND_TYPE_PRIVATE;
	case SDL_GPU_SHADERFORMAT_SPIRV:	return CF_BACKEND_TYPE_VULKAN;
	case SDL_GPU_SHADERFORMAT_DXBC:	 return CF_BACKEND_TYPE_D3D11;
	case SDL_GPU_SHADERFORMAT_DXIL:	 return CF_BACKEND_TYPE_D3D12;
	case SDL_GPU_SHADERFORMAT_MSL:	  // Fall through.
	case SDL_GPU_SHADERFORMAT_METALLIB: // Fall through.
	case SDL_GPU_SHADERFORMAT_MSL | SDL_GPU_SHADERFORMAT_METALLIB: return CF_BACKEND_TYPE_METAL;
	default: return CF_BACKEND_TYPE_INVALID;
	}
}


bool sdlgpu_texture_supports_format(CF_PixelFormat format, CF_TextureUsageBits usage)
{
	return SDL_GPUTextureSupportsFormat(
		app->device,
		s_wrap(format),
		SDL_GPU_TEXTURETYPE_2D,
		usage
	);
}

bool sdlgpu_query_pixel_format(CF_PixelFormat format, CF_PixelFormatOp op)
{
	switch (op) {
	case CF_PIXELFORMAT_OP_NEAREST_FILTER:
	case CF_PIXELFORMAT_OP_BILINEAR_FILTER:
		return sdlgpu_texture_supports_format(format, CF_TEXTURE_USAGE_SAMPLER_BIT);
	case CF_PIXELFORMAT_OP_RENDER_TARGET:
		return sdlgpu_texture_supports_format(format, CF_TEXTURE_USAGE_COLOR_TARGET_BIT);
	case CF_PIXELFORMAT_OP_ALPHA_BLENDING:
		return sdlgpu_texture_supports_format(format, CF_TEXTURE_USAGE_COLOR_TARGET_BIT) && cf_pixel_format_has_alpha(format);
	case CF_PIXELFORMAT_OP_MSAA:
		if (cf_pixel_format_is_depth(format)) return sdlgpu_texture_supports_format(format, CF_TEXTURE_USAGE_DEPTH_STENCIL_TARGET_BIT);
		return sdlgpu_texture_supports_format(format, CF_TEXTURE_USAGE_COLOR_TARGET_BIT);
	case CF_PIXELFORMAT_OP_DEPTH:
		return sdlgpu_texture_supports_format(format, CF_TEXTURE_USAGE_DEPTH_STENCIL_TARGET_BIT);
	default:
		return false;
	}
}

CF_TextureParams sdlgpu_texture_defaults(int w, int h)
{
CF_TextureParams params;
	params.pixel_format = CF_PIXEL_FORMAT_R8G8B8A8_UNORM;
	params.usage = CF_TEXTURE_USAGE_SAMPLER_BIT;
	params.filter = CF_FILTER_LINEAR;
	params.wrap_u = CF_WRAP_MODE_REPEAT;
	params.wrap_v = CF_WRAP_MODE_REPEAT;
	params.mip_filter = CF_MIP_FILTER_LINEAR;
	params.width = w;
	params.height = h;
	params.mip_count = 0;
	params.generate_mipmaps = false;
	params.mip_lod_bias = 0.0f;
	params.max_anisotropy = 1.0f;
	params.stream = false;
	return params;
}

CF_INLINE bool s_is_depth(CF_PixelFormat format)
{
	return format >= CF_PIXEL_FORMAT_D16_UNORM;
}

static CF_Texture s_make_texture(CF_TextureParams params, CF_SampleCount sample_count)
{
	SDL_GPUTextureCreateInfo tex_info = SDL_GPUTextureCreateInfoDefaults(params.width, params.height);
	tex_info.width = (Uint32)params.width;
	tex_info.height = (Uint32)params.height;
	tex_info.format = s_wrap(params.pixel_format);

	// Not allowed to sample from MSAA textures.
	tex_info.usage = sample_count == CF_SAMPLE_COUNT_1 ? params.usage : (params.usage & ~(SDL_GPU_TEXTUREUSAGE_SAMPLER));

	tex_info.sample_count = (SDL_GPUSampleCount)sample_count;
	if (params.generate_mipmaps) {
		tex_info.num_levels = params.mip_count > 0
			? (Uint32)params.mip_count
			: (Uint32)(1 + (int)CF_FLOORF(CF_LOG2F((float)cf_max(params.width, params.height))));
	}

	SDL_GPUTexture* tex = SDL_CreateGPUTexture(app->device, &tex_info);
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
		sampler = SDL_CreateGPUSampler(app->device, &sampler_info);
		CF_ASSERT(sampler);
		if (!sampler) {
			SDL_ReleaseGPUTexture(app->device, tex);
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
		buf = SDL_CreateGPUTransferBuffer(app->device, &tbuf_info);
	}

	CF_TextureInternal* tex_internal = CF_NEW(CF_TextureInternal);
	tex_internal->w = params.width;
	tex_internal->h = params.height;
	tex_internal->filter = sampler ? s_wrap(params.filter) : SDL_GPU_FILTER_NEAREST;
	tex_internal->tex = tex;
	tex_internal->buf = buf;
	tex_internal->sampler = sampler;
	tex_internal->format = tex_info.format;
	tex_internal->binding.texture = tex;
	tex_internal->binding.sampler = sampler;
	CF_Texture result;
	result.id = (uint64_t)(uintptr_t)tex_internal;
	return result;
}

CF_Texture sdlgpu_make_texture(CF_TextureParams params)
{
	return s_make_texture(params, CF_SAMPLE_COUNT_1);
}

void sdlgpu_destroy_texture(CF_Texture texture_handle)
{
	CF_TextureInternal* tex = (CF_TextureInternal*)texture_handle.id;
	SDL_ReleaseGPUTexture(app->device, tex->tex);
	if (tex->sampler) SDL_ReleaseGPUSampler(app->device, tex->sampler);
	if (tex->buf) SDL_ReleaseGPUTransferBuffer(app->device, tex->buf);
	CF_FREE(tex);
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

void sdlgpu_texture_update(CF_Texture texture_handle, void* data, int size)
{
	CF_TextureInternal* tex = (CF_TextureInternal*)texture_handle.id;

	// Copy bytes over to the driver.
	SDL_GPUTransferBuffer* buf = tex->buf;
	if (!buf) {
		SDL_GPUTransferBufferCreateInfo tbuf_info = {
			.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
			.size = (Uint32)size,
			.props = 0,
		};
		buf = SDL_CreateGPUTransferBuffer(app->device, &tbuf_info);
	}
	void* p = SDL_MapGPUTransferBuffer(app->device, buf, true);
	CF_MEMCPY(p, data, size);
	SDL_UnmapGPUTransferBuffer(app->device, buf);

	// Tell the driver to upload the bytes to the GPU.
	SDL_GPUCommandBuffer* cmd = app->cmd ? app->cmd : SDL_AcquireGPUCommandBuffer(app->device);
	SDL_GPUCopyPass* pass = SDL_BeginGPUCopyPass(cmd);
	SDL_GPUTextureTransferInfo src;
	src.transfer_buffer = buf;
	src.offset = 0;
	src.pixels_per_row = tex->w;
	src.rows_per_layer = tex->h;
	SDL_GPUTextureRegion dst = SDL_GPUTextureRegionDefaults(tex, tex->w, tex->h);
	SDL_UploadToGPUTexture(pass, &src, &dst, true);
	SDL_EndGPUCopyPass(pass);
	if (!tex->buf) SDL_ReleaseGPUTransferBuffer(app->device, buf);
	if (!app->cmd) SDL_SubmitGPUCommandBuffer(cmd);
}

void sdlgpu_texture_update_mip(CF_Texture texture_handle, void* data, int size, int mip_level)
{
	CF_TextureInternal* tex = (CF_TextureInternal*)texture_handle.id;

	// Create a temporary transfer buffer if needed.
	SDL_GPUTransferBuffer* buf = tex->buf;
	if (!buf) {
		SDL_GPUTransferBufferCreateInfo tbuf_info = {
			.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
			.size = (Uint32)size,
			.props = 0,
		};
		buf = SDL_CreateGPUTransferBuffer(app->device, &tbuf_info);
	}

	// Copy data into the transfer buffer
	void* p = SDL_MapGPUTransferBuffer(app->device, buf, true);
	CF_MEMCPY(p, data, size);
	SDL_UnmapGPUTransferBuffer(app->device, buf);

	// Compute dimensions for the mip level.
	int w = cf_max(tex->w >> mip_level, 1);
	int h = cf_max(tex->h >> mip_level, 1);

	// Tell the driver to upload the bytes to the GPU.
	SDL_GPUCommandBuffer* cmd = app->cmd ? app->cmd : SDL_AcquireGPUCommandBuffer(app->device);
	SDL_GPUCopyPass* pass = SDL_BeginGPUCopyPass(cmd);
	SDL_GPUTextureTransferInfo src;
	src.transfer_buffer = buf;
	src.offset = 0;
	src.pixels_per_row = w;
	src.rows_per_layer = h;
	SDL_GPUTextureRegion dst = SDL_GPUTextureRegionDefaults(tex, w, h);
	dst.mip_level = (Uint32)mip_level;
	SDL_UploadToGPUTexture(pass, &src, &dst, true);
	SDL_EndGPUCopyPass(pass);
	if (!tex->buf) SDL_ReleaseGPUTransferBuffer(app->device, buf);
	if (!app->cmd) SDL_SubmitGPUCommandBuffer(cmd);
}

void sdlgpu_generate_mipmaps(CF_Texture texture_handle)
{
	CF_TextureInternal* tex = (CF_TextureInternal*)texture_handle.id;
	SDL_GPUCommandBuffer* cmd = app->cmd ? app->cmd : SDL_AcquireGPUCommandBuffer(app->device);
	SDL_GenerateMipmapsForGPUTexture(cmd, tex->tex);
	if (!app->cmd) SDL_SubmitGPUCommandBuffer(cmd);
}

uint64_t sdlgpu_texture_handle(CF_Texture texture)
{
	return (uint64_t)((CF_TextureInternal*)texture.id)->tex;
}

uint64_t sdlgpu_texture_binding_handle(CF_Texture texture)
{
	return (uint64_t)&((CF_TextureInternal*)texture.id)->binding;
}

static void s_shader_directory_recursive(CF_Path path)
{
	Array<CF_Path> dir = CF_Directory::enumerate(app->shader_directory + path);
	for (int i = 0; i < dir.size(); ++i) {
		CF_Path p = app->shader_directory + path + dir[i];
		if (p.is_directory()) {
			s_shader_directory_recursive(p);
		} else {
			CF_Stat stat;
			fs_stat(p, &stat);
			String ext = p.ext();
			if (ext == ".vs" || ext == ".fs" || ext == ".shd") {
				// Exclude app->shader_directory for easier lookups.
				// e.g. app->shader_directory is "/shaders" and contains
				// "/shaders/my_shader.shd", the user needs to only reference it by:
				// "my_shader.shd".
				CF_ShaderFileInfo info;
				info.stat = stat;
				info.path = sintern(p);
				const char* key = sintern(path + dir[i]);
				app->shader_file_infos.add(key, info);
			}
		}
	}
}

void cf_shader_directory(const char* path)
{
	CF_ASSERT(!app->shader_directory_set);
	if (app->shader_directory_set) return;
	app->shader_directory_set = true;
	app->shader_directory = path;
	s_shader_directory_recursive("/");
}

void cf_shader_on_changed(void (*on_changed_fn)(const char* path, void* udata), void* udata)
{
	app->on_shader_changed_fn = on_changed_fn;
	app->on_shader_changed_udata = udata;
}

static void s_shader_watch_recursive(CF_Path path)
{
	Array<CF_Path> dir = CF_Directory::enumerate(app->shader_directory + path);
	for (int i = 0; i < dir.size(); ++i) {
		CF_Path p = app->shader_directory + path + dir[i];
		if (p.is_directory()) {
			s_shader_directory_recursive(p);
		} else {
			CF_Stat stat;
			fs_stat(p, &stat);
			String ext = p.ext();
			if (ext == ".vs" || ext == ".fs" || ext == ".shd") {
				const char* key = sintern(path + dir[i]);
				CF_ShaderFileInfo& info = app->shader_file_infos.find(key);
				if (info.stat.last_modified_time < stat.last_modified_time) {
					info.stat.last_modified_time = stat.last_modified_time;
					app->on_shader_changed_fn(key, app->on_shader_changed_udata);
				}
			}
		}
	}
}

void cf_shader_watch()
{
	if (!app->on_shader_changed_fn) return;
	s_shader_watch_recursive("/");
}

#ifdef CF_RUNTIME_SHADER_COMPILATION
static char* s_cute_shader_vfs_read(const char* path, size_t* len, void* context) {
	CF_UNUSED(context);
	return (char*)fs_read_entire_file_to_memory(path, len);
}

static void s_cute_shader_vfs_free(char* content, void* context) {
	CF_UNUSED(context);
	cf_free(content);
}

static CF_ShaderCompilerVfs s_cute_shader_vfs = {
	.read_file_content = s_cute_shader_vfs_read,
	.free_file_content = s_cute_shader_vfs_free,
};
#endif

CF_ShaderBytecode cf_compile_shader_to_bytecode_internal(const char* shader_src, CF_ShaderStage cf_stage, const char* user_shd)
{
#ifdef CF_RUNTIME_SHADER_COMPILATION
	CF_ShaderCompilerStage stage = CUTE_SHADER_STAGE_VERTEX;
	switch (cf_stage) {
	default: CF_ASSERT(false); break; // No valid stage provided.
	case CF_SHADER_STAGE_VERTEX: stage = CUTE_SHADER_STAGE_VERTEX; break;
	case CF_SHADER_STAGE_FRAGMENT: stage = CUTE_SHADER_STAGE_FRAGMENT; break;
	}

	// Setup builtin includes
	int num_builtin_includes = sizeof(s_builtin_includes) / sizeof(s_builtin_includes[0]);
	CF_ShaderCompilerFile builtin_includes[sizeof(s_builtin_includes) / sizeof(s_builtin_includes[0]) + 1];
	// Use user shader as stub if provided
	for (int i = 0; i < num_builtin_includes; ++i) {
		builtin_includes[i] =  s_builtin_includes[i];
	}
	CF_ShaderCompilerFile shader_stub;
	shader_stub.name = "shader_stub.shd";
	shader_stub.content = user_shd != NULL ? user_shd : s_shader_stub;
	builtin_includes[num_builtin_includes++] = shader_stub;

	int num_include_dirs = 0;
	const char* include_dirs[1];
	if (app->shader_directory_set) {
		include_dirs[num_include_dirs++] = app->shader_directory.c_str();
	}

	CF_ShaderCompilerConfig config = {
		.num_builtin_defines = 0,
		.builtin_defines = NULL,

		.num_builtin_includes = num_builtin_includes,
		.builtin_includes = builtin_includes,

		.num_include_dirs = num_include_dirs,
		.include_dirs = include_dirs,

		.automatic_include_guard = true,
		.return_preprocessed_source = false,

		.vfs = &s_cute_shader_vfs,
	};

	CF_ShaderCompilerResult result = cute_shader_compile(shader_src, stage, config);
	if (result.success) {
		return result.bytecode;
	} else {
		fprintf(stderr, "%s\n", result.error_message);
		cute_shader_free_result(result);

		CF_ShaderBytecode bytecode = { 0 };
		return bytecode;
	}
#else
	fprintf(stderr, "CF was built with CF_RUNTIME_SHADER_COMPILATION=OFF\n");

	CF_ShaderBytecode bytecode = { 0 };
	return bytecode;
#endif
}

CF_ShaderBytecode cf_compile_shader_to_bytecode(const char* shader_src, CF_ShaderStage cf_stage)
{
	return cf_compile_shader_to_bytecode_internal(shader_src, cf_stage, NULL);
}

void cf_free_shader_bytecode(CF_ShaderBytecode bytecode)
{
#ifdef CF_RUNTIME_SHADER_COMPILATION
	CF_ShaderCompilerResult compile_result = {
		.bytecode = bytecode,
	};
	cute_shader_free_result(compile_result);
#endif
}

static SDL_GPUShader* s_compile(CF_ShaderInternal* shader_internal, CF_ShaderBytecode bytecode, CF_ShaderStage stage)
{
	bool vs = stage == CF_SHADER_STAGE_VERTEX ? true : false;

	// Load reflection info
	const CF_ShaderInfo* shader_info = &bytecode.shader_info;

	for (int i = 0; i < shader_info->num_images; ++i) {
		shader_internal->image_names.add(sintern(shader_info->image_names[i]));
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
	if (SDL_GetGPUShaderFormats(app->device) == SDL_GPU_SHADERFORMAT_SPIRV) {
		sdl_shader = (SDL_GPUShader*)SDL_CreateGPUShader(app->device, &shaderCreateInfo);
	} else {
#ifndef CF_EMSCRIPTEN
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
		sdl_shader = (SDL_GPUShader*)SDL_ShaderCross_CompileGraphicsShaderFromSPIRV(app->device, &spirvInfo, &metadata);
#endif
	}
	CF_ASSERT(sdl_shader);
	return sdl_shader;
}

CF_Shader sdlgpu_make_shader_from_bytecode(CF_ShaderBytecode vertex_bytecode, CF_ShaderBytecode fragment_bytecode)
{
	CF_ShaderInternal* shader_internal = CF_NEW(CF_ShaderInternal);
	CF_MEMSET(shader_internal, 0, sizeof(*shader_internal));

	shader_internal->vs = s_compile(shader_internal, vertex_bytecode, CF_SHADER_STAGE_VERTEX);
	shader_internal->fs = s_compile(shader_internal, fragment_bytecode, CF_SHADER_STAGE_FRAGMENT);
	CF_ASSERT(shader_internal->vs);
	CF_ASSERT(shader_internal->fs);

	CF_Shader result;
	result.id = { (uint64_t)shader_internal };
	return result;
}

static CF_Shader s_compile(const char* vs_src, const char* fs_src, bool builtin = false, const char* user_shd = NULL)
{
	// TODO: builtin flag is redundant
	// Compile to bytecode.
	CF_ShaderBytecode vs_bytecode = cf_compile_shader_to_bytecode_internal(vs_src, CF_SHADER_STAGE_VERTEX, NULL);
	if (vs_bytecode.content == NULL) {
		CF_Shader result = { 0 };
		return result;
	}
	CF_ShaderBytecode fs_bytecode = cf_compile_shader_to_bytecode_internal(fs_src, CF_SHADER_STAGE_FRAGMENT, user_shd);
	if (fs_bytecode.content == NULL) {
		cf_free_shader_bytecode(vs_bytecode);
		CF_Shader result = { 0 };
		return result;
	}

	// Create the actual shader object.
	CF_Shader shader = cf_make_shader_from_bytecode(vs_bytecode, fs_bytecode);
	cf_free_shader_bytecode(vs_bytecode);
	cf_free_shader_bytecode(fs_bytecode);
	return shader;
}

void cf_load_internal_shaders()
{
#ifdef CF_RUNTIME_SHADER_COMPILATION
	cute_shader_init();

	if (spvc_context_create(&g_spvc_context) != SPVC_SUCCESS) {
		g_spvc_context = NULL;
	}

	// Compile built-in shaders.
	app->draw_shader = s_compile(s_draw_vs, s_draw_fs, true, NULL);
	app->basic_shader = s_compile(s_basic_vs, s_basic_fs, true, NULL);
	app->backbuffer_shader = s_compile(s_backbuffer_vs, s_backbuffer_fs, true, NULL);
	app->blit_shader = s_compile(s_blit_vs, s_blit_fs, true, NULL);
#else
	app->draw_shader = cf_make_shader_from_bytecode(s_draw_vs_bytecode, s_draw_fs_bytecode);
	app->basic_shader = cf_make_shader_from_bytecode(s_basic_vs_bytecode, s_basic_fs_bytecode);
	app->backbuffer_shader = cf_make_shader_from_bytecode(s_backbuffer_vs_bytecode, s_backbuffer_fs_bytecode);
	app->blit_shader = cf_make_shader_from_bytecode(s_blit_vs_bytecode, s_blit_fs_bytecode);
#endif
}

void cf_unload_internal_shaders()
{
	cf_destroy_shader(app->draw_shader);
	cf_destroy_shader(app->basic_shader);
	cf_destroy_shader(app->backbuffer_shader);
#ifdef CF_RUNTIME_SHADER_COMPILATION
	cute_shader_cleanup();
#endif

	if (g_spvc_context) {
		spvc_context_destroy(g_spvc_context);
		g_spvc_context = NULL;
	}
}

// Create a user shader by injecting their `shader` function into CF's draw shader.
CF_Shader cf_make_draw_shader_internal(const char* path)
{
	CF_Path p = CF_Path("/") + path;
	const char* path_s = sintern(p);
	CF_ShaderFileInfo info = app->shader_file_infos.find(path_s);
	if (!info.path) return { 0 };
	char* shd = fs_read_entire_file_to_memory_and_nul_terminate(info.path);
	if (!shd) return { 0 };
	CF_Shader result = cf_make_draw_shader_from_source_internal(shd);
	cf_free(shd);
	return result;
}

// Create a user shader by injecting their `shader` function into CF's draw shader.
CF_Shader cf_make_draw_blit_shader_internal(const char* path)
{
	CF_Path p = CF_Path("/") + path;
	const char* path_s = sintern(p);
	CF_ShaderFileInfo info = app->shader_file_infos.find(path_s);
	if (!info.path) return { 0 };
	char* shd = fs_read_entire_file_to_memory_and_nul_terminate(info.path);
	if (!shd) return { 0 };
	CF_Shader result = cf_make_draw_blit_shader_from_source_internal(shd);
	cf_free(shd);
	return result;
}

CF_Shader cf_make_draw_shader_from_source_internal(const char* src)
{
	return s_compile(s_draw_vs, s_draw_fs, true, src);
}

CF_Shader cf_make_draw_shader_from_bytecode_internal(CF_ShaderBytecode bytecode)
{
	return cf_make_shader_from_bytecode(s_draw_vs_bytecode, bytecode);
}

CF_Shader cf_make_draw_blit_shader_from_source_internal(const char* src)
{
	return s_compile(s_blit_vs, s_blit_fs, true, src);
}

CF_Shader cf_make_draw_blit_shader_from_bytecode_internal(CF_ShaderBytecode bytecode)
{
	return cf_make_shader_from_bytecode(s_blit_vs_bytecode, bytecode);
}

CF_Shader sdlgpu_make_shader(const char* vertex_path, const char* fragment_path)
{
	// Make sure each file can be found.
	const char* vs = fs_read_entire_file_to_memory_and_nul_terminate(vertex_path);
	const char* fs = fs_read_entire_file_to_memory_and_nul_terminate(fragment_path);
	CF_ASSERT(vs);
	CF_ASSERT(fs);
	return s_compile(vs, fs);
}

CF_Shader sdlgpu_make_shader_from_source(const char* vertex_src, const char* fragment_src)
{
	return s_compile(vertex_src, fragment_src);
}

void sdlgpu_destroy_shader(CF_Shader shader_handle)
{
	// Draw shaders automatically have blit shaders generated, so clean that up as well,
	// if it exists. See `cf_make_draw_shader`.
	CF_Shader* blit = (CF_Shader*)draw->draw_shd_to_blit_shd.try_get(shader_handle.id);
	if (blit) {
		cf_destroy_shader(*blit);
		draw->draw_shd_to_blit_shd.remove(shader_handle.id);
	}

	CF_ShaderInternal* shd = (CF_ShaderInternal*)shader_handle.id;
	SDL_ReleaseGPUShader(app->device, shd->vs);
	SDL_ReleaseGPUShader(app->device, shd->fs);
	for (int i = 0; i < shd->pip_cache.count(); ++i) {
		SDL_ReleaseGPUGraphicsPipeline(app->device, shd->pip_cache[i].pip);
	}
	shd->~CF_ShaderInternal();
	CF_FREE(shd);
}

void sdlgpu_clear_canvas(CF_Canvas canvas_handle)
{
	CF_CanvasInternal* canvas = (CF_CanvasInternal*)canvas_handle.id;
	SDL_GPUCommandBuffer* cmd = app->cmd ? app->cmd : SDL_AcquireGPUCommandBuffer(app->device);

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

	if (!app->cmd) SDL_SubmitGPUCommandBuffer(cmd);
}

CF_CanvasParams sdlgpu_canvas_defaults(int w, int h)
{
	CF_CanvasParams params = { 0 };
	if (w == 0 || h == 0) {
		params.name = NULL;
		params.target = { };
		params.depth_stencil_target = { };
	} else {
		params.name = NULL;
				params.target = sdlgpu_texture_defaults(w, h);
		params.target.usage |= CF_TEXTURE_USAGE_COLOR_TARGET_BIT;
		params.depth_stencil_enable = false;
				params.depth_stencil_target = sdlgpu_texture_defaults(w, h);
		params.depth_stencil_target.pixel_format = CF_PIXEL_FORMAT_D16_UNORM;
				if (sdlgpu_texture_supports_format(CF_PIXEL_FORMAT_D24_UNORM_S8_UINT, CF_TEXTURE_USAGE_DEPTH_STENCIL_TARGET_BIT)) {
			params.depth_stencil_target.pixel_format = CF_PIXEL_FORMAT_D24_UNORM_S8_UINT;
		}
		params.depth_stencil_target.usage = CF_TEXTURE_USAGE_DEPTH_STENCIL_TARGET_BIT;
	}
	return params;
}

CF_Canvas sdlgpu_make_canvas(CF_CanvasParams params)
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
						canvas->cf_resolve_texture = sdlgpu_make_texture(params.target);
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

void sdlgpu_destroy_canvas(CF_Canvas canvas_handle)
{
	CF_CanvasInternal* canvas = (CF_CanvasInternal*)canvas_handle.id;
		sdlgpu_destroy_texture(canvas->cf_texture);
		if (canvas->resolve_texture) sdlgpu_destroy_texture(canvas->cf_resolve_texture);
		if (canvas->depth_stencil) sdlgpu_destroy_texture(canvas->cf_depth_stencil);
	CF_FREE(canvas);
}

CF_Texture sdlgpu_canvas_get_target(CF_Canvas canvas_handle)
{
	CF_CanvasInternal* canvas = (CF_CanvasInternal*)canvas_handle.id;
	return canvas->resolve_texture ? canvas->cf_resolve_texture : canvas->cf_texture;
}

CF_Texture sdlgpu_canvas_get_depth_stencil_target(CF_Canvas canvas_handle)
{
	CF_CanvasInternal* canvas = (CF_CanvasInternal*)canvas_handle.id;
	return canvas->cf_depth_stencil;
}

CF_Mesh sdlgpu_make_mesh(int vertex_buffer_size, const CF_VertexAttribute* attributes, int attribute_count, int vertex_stride)
{
	CF_MeshInternal* mesh = (CF_MeshInternal*)CF_CALLOC(sizeof(CF_MeshInternal));
	mesh->vertices.size = vertex_buffer_size;
	if (vertex_buffer_size) {
		SDL_GPUBufferCreateInfo buf_info = {
			.usage = SDL_GPU_BUFFERUSAGE_VERTEX,
			.size = (Uint32)vertex_buffer_size,
			.props = 0,
		};
		mesh->vertices.buffer = SDL_CreateGPUBuffer(app->device, &buf_info);
		SDL_GPUTransferBufferCreateInfo tbuf_info = {
			.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
			.size = (Uint32)vertex_buffer_size,
			.props = 0,
		};
		mesh->vertices.transfer_buffer = SDL_CreateGPUTransferBuffer(app->device, &tbuf_info);
	}
	attribute_count = min(attribute_count, CF_MESH_MAX_VERTEX_ATTRIBUTES);
	mesh->attribute_count = attribute_count;
	mesh->vertices.stride = vertex_stride;
	for (int i = 0; i < attribute_count; ++i) {
		mesh->attributes[i] = attributes[i];
		mesh->attributes[i].name = sintern(attributes[i].name);
	}
	CF_Mesh result = { (uint64_t)mesh };
	return result;
}

void sdlgpu_mesh_set_index_buffer(CF_Mesh mesh_handle, int index_buffer_size_in_bytes, int index_bit_count)
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
	mesh->indices.buffer = SDL_CreateGPUBuffer(app->device, &buf_info);
	SDL_GPUTransferBufferCreateInfo tbuf_info = {
		.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
		.size = (Uint32)index_buffer_size_in_bytes,
		.props = 0,
	};
	mesh->indices.transfer_buffer = SDL_CreateGPUTransferBuffer(app->device, &tbuf_info);
}

void sdlgpu_mesh_set_instance_buffer(CF_Mesh mesh_handle, int instance_buffer_size_in_bytes, int instance_stride)
{
	CF_MeshInternal* mesh = (CF_MeshInternal*)mesh_handle.id;
	mesh->instances.size = instance_buffer_size_in_bytes;
	mesh->instances.stride = instance_stride;
	SDL_GPUBufferCreateInfo buf_info = {
		.usage = SDL_GPU_BUFFERUSAGE_VERTEX,
		.size = (Uint32)instance_buffer_size_in_bytes,
		.props = 0,
	};
	mesh->instances.buffer = SDL_CreateGPUBuffer(app->device, &buf_info);
	SDL_GPUTransferBufferCreateInfo tbuf_info = {
		.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
		.size = (Uint32)instance_buffer_size_in_bytes,
		.props = 0,
	};
	mesh->instances.transfer_buffer = SDL_CreateGPUTransferBuffer(app->device, &tbuf_info);
}

void sdlgpu_destroy_mesh(CF_Mesh mesh_handle)
{
	CF_MeshInternal* mesh = (CF_MeshInternal*)mesh_handle.id;
	if (mesh->vertices.buffer) {
		SDL_ReleaseGPUBuffer(app->device, mesh->vertices.buffer);
		SDL_ReleaseGPUTransferBuffer(app->device, mesh->vertices.transfer_buffer);
	}
	if (mesh->indices.buffer) {
		SDL_ReleaseGPUBuffer(app->device, mesh->indices.buffer);
		SDL_ReleaseGPUTransferBuffer(app->device, mesh->indices.transfer_buffer);
	}
	if (mesh->instances.buffer) {
		SDL_ReleaseGPUBuffer(app->device, mesh->instances.buffer);
		SDL_ReleaseGPUTransferBuffer(app->device, mesh->instances.transfer_buffer);
	}
	CF_FREE(mesh);
}

static void s_update_buffer(CF_Buffer* buffer, int element_count, void* data, int size, SDL_GPUBufferUsageFlags flags)
{
	// Resize buffer if necessary.
	if (size > buffer->size) {
		SDL_ReleaseGPUBuffer(app->device, buffer->buffer);
		SDL_ReleaseGPUTransferBuffer(app->device, buffer->transfer_buffer);

		int new_size = size * 2;
		buffer->size = new_size;
		SDL_GPUBufferCreateInfo buf_info = {
			.usage = flags,
			.size = (Uint32)new_size,
			.props = 0,
		};
		buffer->buffer = SDL_CreateGPUBuffer(app->device, &buf_info);
		SDL_GPUTransferBufferCreateInfo tbuf_info = {
			.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
			.size = (Uint32)new_size,
			.props = 0,
		};
		buffer->transfer_buffer = SDL_CreateGPUTransferBuffer(app->device, &tbuf_info);
	}

	// Copy vertices over to the driver.
	CF_ASSERT(size <= buffer->size);
	void* p = SDL_MapGPUTransferBuffer(app->device, buffer->transfer_buffer, true);
	CF_MEMCPY(p, data, size);
	SDL_UnmapGPUTransferBuffer(app->device, buffer->transfer_buffer);
	buffer->element_count = element_count;

	// Submit the upload command to the GPU.
	SDL_GPUCommandBuffer* cmd = app->cmd ? app->cmd : SDL_AcquireGPUCommandBuffer(app->device);
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
	if (!app->cmd) SDL_SubmitGPUCommandBuffer(cmd);
}

void sdlgpu_mesh_update_vertex_data(CF_Mesh mesh_handle, void* data, int count)
{
	CF_MeshInternal* mesh = (CF_MeshInternal*)mesh_handle.id;
	CF_ASSERT(mesh->attribute_count);
	s_update_buffer(&mesh->vertices, count, data, count * mesh->vertices.stride, SDL_GPU_BUFFERUSAGE_VERTEX);
}

void sdlgpu_mesh_update_index_data(CF_Mesh mesh_handle, void* data, int count)
{
	CF_MeshInternal* mesh = (CF_MeshInternal*)mesh_handle.id;
	s_update_buffer(&mesh->indices, count, data, count * mesh->indices.stride, SDL_GPU_BUFFERUSAGE_INDEX);
}

void sdlgpu_mesh_update_instance_data(CF_Mesh mesh_handle, void* data, int count)
{
	CF_MeshInternal* mesh = (CF_MeshInternal*)mesh_handle.id;
	s_update_buffer(&mesh->instances, count, data, count * mesh->instances.stride, SDL_GPU_BUFFERUSAGE_VERTEX);
}

CF_RenderState sdlgpu_render_state_defaults()
{
	CF_RenderState state;
	state.primitive_type = CF_PRIMITIVE_TYPE_TRIANGLELIST;
	state.blend.enabled = true;
	state.cull_mode = CF_CULL_MODE_NONE;
	state.blend.pixel_format = CF_PIXEL_FORMAT_R8G8B8A8_UNORM;
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
	state.depth_bias_constant_factor = 0;
	state.depth_bias_clamp = 0;
	state.depth_bias_slope_factor = 0;
	state.enable_depth_bias = false;
	state.enable_depth_clip = true;
	return state;
}

CF_Material sdlgpu_make_material()
{
	CF_MaterialInternal* material = CF_NEW(CF_MaterialInternal);
	material->uniform_arena = cf_make_arena(4, CF_KB * 16);
	material->block_arena = cf_make_arena(4, CF_KB * 16);
	material->state = cf_render_state_defaults();
	CF_Material result = { (uint64_t)material };
	return result;
}

void sdlgpu_destroy_material(CF_Material material_handle)
{
	CF_MaterialInternal* material = (CF_MaterialInternal*)material_handle.id;
	cf_arena_reset(&material->uniform_arena);
	cf_arena_reset(&material->block_arena);
	material->~CF_MaterialInternal();
	CF_FREE(material);
}

void sdlgpu_material_set_render_state(CF_Material material_handle, CF_RenderState render_state)
{
	CF_MaterialInternal* material = (CF_MaterialInternal*)material_handle.id;
	if (CF_MEMCMP(&material->state, &render_state, sizeof(material->state))) {
		material->state = render_state;
		material->dirty = true;
	}
}

static void s_material_set_texture(CF_MaterialInternal* material, CF_MaterialState* state, const char* name, CF_Texture texture)
{
	bool found = false;
	for (int i = 0; i < state->textures.count(); ++i) {
		if (state->textures[i].name == name) {
			state->textures[i].handle = texture;
			found = true;
			break;
		}
	}
	if (!found) {
		CF_MaterialTex tex;
		tex.name = name;
		tex.handle = texture;
		state->textures.add(tex);
		material->dirty = true;
	}
}

void sdlgpu_material_set_texture_vs(CF_Material material_handle, const char* name, CF_Texture texture)
{
	CF_MaterialInternal* material = (CF_MaterialInternal*)material_handle.id;
	name = sintern(name);
	s_material_set_texture(material, &material->vs, name, texture);
}

void sdlgpu_material_set_texture_fs(CF_Material material_handle, const char* name, CF_Texture texture)
{
	CF_MaterialInternal* material = (CF_MaterialInternal*)material_handle.id;
	name = sintern(name);
	s_material_set_texture(material, &material->fs, name, texture);
}

void sdlgpu_material_clear_textures(CF_Material material_handle)
{
	CF_MaterialInternal* material = (CF_MaterialInternal*)material_handle.id;
	material->vs.textures.clear();
	material->fs.textures.clear();
	material->dirty = true;
}

static void s_material_set_uniform(CF_Arena* arena, CF_MaterialState* state, const char* block_name, const char* name, void* data, CF_UniformType type, int array_length)
{
	if (array_length <= 0) array_length = 1;
	CF_Uniform* uniform = NULL;
	for (int i = 0; i < state->uniforms.count(); ++i) {
		CF_Uniform* u = state->uniforms + i;
		if (u->block_name == block_name && u->name == name) {
			uniform = u;
			break;
		}
	}
	int size = s_uniform_size(type) * array_length;
	if (!uniform) {
		uniform = &state->uniforms.add();
		uniform->name = name;
		uniform->block_name = block_name;
		uniform->data = cf_arena_alloc(arena, size);
		uniform->size = size;
		uniform->type = type;
		uniform->array_length = array_length;
	}
	CF_ASSERT(uniform->type == type);
	CF_ASSERT(uniform->array_length == array_length);
	CF_MEMCPY(uniform->data, data, size);
}

void sdlgpu_material_set_uniform_vs(CF_Material material_handle, const char* name, void* data, CF_UniformType type, int array_length)
{
	CF_MaterialInternal* material = (CF_MaterialInternal*)material_handle.id;
	name = sintern(name);
	s_material_set_uniform(&material->uniform_arena, &material->vs, sintern("uniform_block"), name, data, type, array_length);
}

void sdlgpu_material_set_uniform_vs_internal(CF_Material material_handle, const char* block_name, const char* name, void* data, CF_UniformType type, int array_length)
{
	CF_MaterialInternal* material = (CF_MaterialInternal*)material_handle.id;
	name = sintern(name);
	s_material_set_uniform(&material->uniform_arena, &material->vs, sintern(block_name), name, data, type, array_length);
}

void sdlgpu_material_set_uniform_fs(CF_Material material_handle, const char* name, void* data, CF_UniformType type, int array_length)
{
	CF_MaterialInternal* material = (CF_MaterialInternal*)material_handle.id;
	name = sintern(name);
	s_material_set_uniform(&material->uniform_arena, &material->fs, sintern("uniform_block"), name, data, type, array_length);
}

void sdlgpu_material_set_uniform_fs_internal(CF_Material material_handle, const char* block_name, const char* name, void* data, CF_UniformType type, int array_length)
{
	CF_MaterialInternal* material = (CF_MaterialInternal*)material_handle.id;
	name = sintern(name);
	s_material_set_uniform(&material->uniform_arena, &material->fs, sintern(block_name), name, data, type, array_length);
}

void sdlgpu_material_clear_uniforms(CF_Material material_handle)
{
	CF_MaterialInternal* material = (CF_MaterialInternal*)material_handle.id;
	arena_reset(&material->uniform_arena);
	material->vs.uniforms.clear();
	material->fs.uniforms.clear();
}

void sdlgpu_clear_color(float red, float green, float blue, float alpha)
{
	app->clear_color = make_color(red, green, blue, alpha);
}

void sdlgpu_clear_depth_stencil(float depth, uint32_t stencil)
{
	app->clear_depth = depth;
	app->clear_stencil = stencil;
}

void sdlgpu_apply_canvas(CF_Canvas canvas_handle, bool clear)
{
	CF_CanvasInternal* canvas = (CF_CanvasInternal*)canvas_handle.id;
	CF_ASSERT(canvas);
	s_canvas = canvas;
	s_canvas->clear = clear;
}

void sdlgpu_apply_viewport(int x, int y, int w, int h)
{
	CF_ASSERT(s_canvas);
	CF_ASSERT(s_canvas->pass);
	SDL_GPUViewport viewport;
	viewport.x = (float)x;
	viewport.y = (float)y;
	viewport.w = (float)w;
	viewport.h = (float)h;
	viewport.min_depth = 0;
	viewport.max_depth = 1;
	SDL_SetGPUViewport(s_canvas->pass, &viewport);
}

void sdlgpu_apply_scissor(int x, int y, int w, int h)
{
	CF_ASSERT(s_canvas);
	CF_ASSERT(s_canvas->pass);
	SDL_Rect scissor;
	scissor.x = x;
	scissor.y = y;
	scissor.w = w;
	scissor.h = h;
	SDL_SetGPUScissor(s_canvas->pass, &scissor);
}

void sdlgpu_apply_stencil_reference(int reference)
{
  CF_ASSERT(s_canvas);
  CF_ASSERT(s_canvas->pass);
  SDL_SetGPUStencilReference(s_canvas->pass, reference);
}

void sdlgpu_apply_blend_constants(float r, float g, float b, float a)
{
  CF_ASSERT(s_canvas);
  CF_ASSERT(s_canvas->pass);
  SDL_FColor color;
  color.r = r;
  color.g = g;
  color.b = b;
  color.a = a;
  SDL_SetGPUBlendConstants(s_canvas->pass, color);
}

void sdlgpu_apply_mesh(CF_Mesh mesh_handle)
{
	CF_ASSERT(s_canvas);
	CF_MeshInternal* mesh = (CF_MeshInternal*)mesh_handle.id;
	s_canvas->mesh = mesh;
}

static void s_copy_uniforms(SDL_GPUCommandBuffer* cmd, CF_Arena* arena, CF_ShaderInternal* shd, CF_MaterialState* mstate, bool vs)
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

static SDL_GPUGraphicsPipeline* s_build_pipeline(CF_ShaderInternal* shader, CF_RenderState* state, CF_MeshInternal* mesh)
{
	CF_TextureInternal* tex = (CF_TextureInternal*)s_canvas->cf_texture.id;
	SDL_GPUColorTargetDescription color_info;
	CF_MEMSET(&color_info, 0, sizeof(color_info));
	CF_ASSERT(s_canvas->texture);
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
	if (s_canvas->cf_depth_stencil.id && state->depth_write_enabled) {
		pip_info.target_info.depth_stencil_format = ((CF_TextureInternal*)s_canvas->cf_depth_stencil.id)->format;
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
		vertex_buffer_descriptions[vertex_buffer_descriptions_count].slot = 1;
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
	pip_info.multisample_state.sample_count = (SDL_GPUSampleCount)s_canvas->sample_count;
	pip_info.multisample_state.sample_mask = 0;

	pip_info.depth_stencil_state.enable_depth_test = state->depth_write_enabled;
	pip_info.depth_stencil_state.enable_depth_write = state->depth_write_enabled;
	pip_info.depth_stencil_state.compare_op = s_wrap(state->depth_compare);
	pip_info.depth_stencil_state.enable_stencil_test = state->stencil.enabled;
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

	SDL_GPUGraphicsPipeline* pip = SDL_CreateGPUGraphicsPipeline(app->device, &pip_info);
	CF_ASSERT(pip);
	return pip;
}

void sdlgpu_apply_shader(CF_Shader shader_handle, CF_Material material_handle)
{
	CF_ASSERT(s_canvas);
	CF_ASSERT(s_canvas->mesh);
	CF_MeshInternal* mesh = s_canvas->mesh;
	CF_MaterialInternal* material = (CF_MaterialInternal*)material_handle.id;
	CF_ShaderInternal* shader = (CF_ShaderInternal*)shader_handle.id;
	CF_RenderState* state = &material->state;

	// Cache the pipeline to avoid create/release each frame.
	// ...Build a new one if the material marks itself as dirty.
	SDL_GPUGraphicsPipeline* pip = NULL;
	bool found = false;
	for (int i = 0; i < shader->pip_cache.count(); ++i) {
		CF_Pipeline pip_cache = shader->pip_cache[i];
		if (pip_cache.material == material &&
			pip_cache.mesh == mesh &&
			s_canvas->sample_count == (CF_SampleCount)pip_cache.sample_count) {
			found = true;
			if (material->dirty) {
				material->dirty = false;
				pip = s_build_pipeline(shader, state, mesh);
				if (pip_cache.pip) {
					SDL_ReleaseGPUGraphicsPipeline(app->device, pip_cache.pip);
				}
				shader->pip_cache[i].pip = pip;
			} else {
				pip = pip_cache.pip;
			}
		}
	}
	if (!found) {
		pip = s_build_pipeline(shader, state, mesh);
		shader->pip_cache.add({ (SDL_GPUSampleCount)s_canvas->sample_count, material, pip, mesh });
		material->dirty = false;
	}
	CF_ASSERT(pip);

	SDL_GPUCommandBuffer* cmd = app->cmd;
	CF_ASSERT(cmd);
	s_canvas->pip = pip;

	SDL_GPUColorTargetInfo pass_color_info;
	CF_MEMSET(&pass_color_info, 0, sizeof(pass_color_info));
	pass_color_info.texture = s_canvas->texture;
	pass_color_info.clear_color = { app->clear_color.r, app->clear_color.g, app->clear_color.b, app->clear_color.a };
	pass_color_info.load_op = s_canvas->clear ? SDL_GPU_LOADOP_CLEAR : SDL_GPU_LOADOP_LOAD;
	pass_color_info.cycle = s_canvas->clear ? true : false;
	if (s_canvas->sample_count == CF_SAMPLE_COUNT_1) {
		pass_color_info.store_op = SDL_GPU_STOREOP_STORE;
	} else {
		pass_color_info.store_op = SDL_GPU_STOREOP_RESOLVE_AND_STORE;
		pass_color_info.resolve_texture = s_canvas->resolve_texture;
	}

	SDL_GPUDepthStencilTargetInfo pass_depth_stencil_info;
	CF_MEMSET(&pass_depth_stencil_info, 0, sizeof(pass_depth_stencil_info));
	pass_depth_stencil_info.texture = s_canvas->depth_stencil;
	if (s_canvas->depth_stencil) {
		pass_depth_stencil_info.clear_depth = app->clear_depth;
		pass_depth_stencil_info.clear_stencil = app->clear_stencil;
		pass_depth_stencil_info.load_op = s_canvas->clear ? SDL_GPU_LOADOP_CLEAR : SDL_GPU_LOADOP_LOAD;
		pass_depth_stencil_info.store_op = SDL_GPU_STOREOP_STORE;
		pass_depth_stencil_info.stencil_load_op = s_canvas->clear ? SDL_GPU_LOADOP_CLEAR : SDL_GPU_LOADOP_LOAD;
		pass_depth_stencil_info.stencil_store_op = SDL_GPU_STOREOP_DONT_CARE;
		pass_depth_stencil_info.cycle = pass_color_info.cycle;
	}
	SDL_GPUDepthStencilTargetInfo* depth_stencil_ptr = state->depth_write_enabled && s_canvas->depth_stencil ? &pass_depth_stencil_info : NULL;
	SDL_GPURenderPass* pass = SDL_BeginGPURenderPass(cmd, &pass_color_info, 1, depth_stencil_ptr);
	CF_ASSERT(pass);
	s_canvas->pass = pass;
	SDL_BindGPUGraphicsPipeline(pass, pip);
	SDL_GPUBufferBinding bind[2];
	bind[0].buffer = mesh->vertices.buffer;
	bind[0].offset = 0;
	bind[1].buffer = mesh->instances.buffer;
	bind[1].offset = 0;
	SDL_BindGPUVertexBuffers(pass, 0, bind, mesh->instances.buffer ? 2 : 1);

	if (mesh->indices.buffer) {
		SDL_GPUBufferBinding index_bind = {
			.buffer = mesh->indices.buffer,
			.offset = 0
		};
		SDL_BindGPUIndexBuffer(pass, &index_bind, mesh->indices.stride == 2 ? SDL_GPU_INDEXELEMENTSIZE_16BIT : SDL_GPU_INDEXELEMENTSIZE_32BIT);
	}
	// @TODO Storage/compute.

	// Bind images to all their respective slots.
	int sampler_count = shader->image_names.count();
	SDL_GPUTextureSamplerBinding* sampler_bindings = SDL_stack_alloc(SDL_GPUTextureSamplerBinding, sampler_count);
	int found_image_count = 0;
	for (int i = 0; found_image_count < sampler_count && i < material->fs.textures.count(); ++i) {
		const char* image_name = material->fs.textures[i].name;
		for (int j = 0; j < shader->image_names.size(); ++j) {
			if (shader->image_names[j] == image_name) {
				sampler_bindings[j].sampler = ((CF_TextureInternal*)material->fs.textures[i].handle.id)->sampler;
				sampler_bindings[j].texture = ((CF_TextureInternal*)material->fs.textures[i].handle.id)->tex;
				found_image_count++;
			}
		}
	}
	CF_ASSERT(found_image_count == sampler_count);
	SDL_BindGPUFragmentSamplers(pass, 0, sampler_bindings, (Uint32)found_image_count);

	// Copy over uniform data.
	s_copy_uniforms(cmd, &material->block_arena, shader, &material->vs, true);
	s_copy_uniforms(cmd, &material->block_arena, shader, &material->fs, false);

	SDL_SetGPUStencilReference(pass, state->stencil.reference);

	// Prevent the same canvas from clearing itself more than once.
	s_canvas->clear = false;
}

void sdlgpu_draw_elements()
{
	CF_MeshInternal* mesh = s_canvas->mesh;
	if (mesh->instances.buffer) {
		if (mesh->indices.buffer) {
			SDL_DrawGPUIndexedPrimitives(s_canvas->pass, mesh->indices.element_count, mesh->instances.element_count, 0, 0, 0);
		} else {
			SDL_DrawGPUPrimitives(s_canvas->pass, mesh->vertices.element_count, mesh->instances.element_count, 0, 0);
		}
	} else {
		if (mesh->indices.buffer) {
			SDL_DrawGPUIndexedPrimitives(s_canvas->pass, mesh->indices.element_count, 1, 0, 0, 0);
		} else {
			SDL_DrawGPUPrimitives(s_canvas->pass, mesh->vertices.element_count, 1, 0, 0);
		}
	}
	app->draw_call_count++;
}

void sdlgpu_commit()
{
	SDL_EndGPURenderPass(s_canvas->pass);
}

//--------------------------------------------------------------------------------------------------
// OpenGL ES 3.0 implementation of cute_graphics.h.

#ifdef CF_EMSCRIPTEN
#	include <GLES3/gl3.h>
#else
#	include <glad/glad.h>
#endif


struct CF_GL_PixelFormatInfo
{
	CF_PixelFormat format;
	GLenum internal_fmt;
	GLenum upload_fmt;
	GLenum upload_type;
	uint32_t caps;
	bool has_alpha;
	bool is_depth;
	bool has_stencil;
	bool is_integer;
	const char* required_extension;
};

enum
{
	CF_GL_FMT_CAP_SAMPLE = 0x1,
	CF_GL_FMT_CAP_LINEAR = 0x2,
	CF_GL_FMT_CAP_COLOR = 0x4,
	CF_GL_FMT_CAP_ALPHA = 0x8,
	CF_GL_FMT_CAP_MSAA = 0x10,
	CF_GL_FMT_CAP_DEPTH = 0x20,
	CF_GL_FMT_CAP_STENCIL = 0x40,
};

#ifndef GL_BGRA8_EXT
#define GL_BGRA8_EXT 0
#endif

static CF_GL_PixelFormatInfo g_gl_pixel_formats[] =
{
{ CF_PIXEL_FORMAT_A8_UNORM, GL_NONE, GL_NONE, GL_NONE, 0, true, false, false, false, NULL },
	{ CF_PIXEL_FORMAT_R8_UNORM, GL_R8, GL_RED, GL_UNSIGNED_BYTE, 0, false, false, false, false, NULL },
	{ CF_PIXEL_FORMAT_R8G8_UNORM, GL_RG8, GL_RG, GL_UNSIGNED_BYTE, 0, false, false, false, false, NULL },
	{ CF_PIXEL_FORMAT_R8G8B8A8_UNORM, GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE, 0, true, false, false, false, NULL },
	{ CF_PIXEL_FORMAT_R16_UNORM, GL_NONE, GL_NONE, GL_NONE, 0, false, false, false, false, NULL },
	{ CF_PIXEL_FORMAT_R16G16_UNORM, GL_NONE, GL_NONE, GL_NONE, 0, false, false, false, false, NULL },
	{ CF_PIXEL_FORMAT_R16G16B16A16_UNORM, GL_NONE, GL_NONE, GL_NONE, 0, true, false, false, false, NULL },
	{ CF_PIXEL_FORMAT_R10G10B10A2_UNORM, GL_RGB10_A2, GL_RGBA, GL_UNSIGNED_INT_2_10_10_10_REV, 0, true, false, false, false, NULL },
	{ CF_PIXEL_FORMAT_B5G6R5_UNORM, GL_NONE, GL_NONE, GL_NONE, 0, false, false, false, false, NULL },
	{ CF_PIXEL_FORMAT_B5G5R5A1_UNORM, GL_NONE, GL_NONE, GL_NONE, 0, true, false, false, false, NULL },
	{ CF_PIXEL_FORMAT_B4G4R4A4_UNORM, GL_NONE, GL_NONE, GL_NONE, 0, true, false, false, false, NULL },
	{ CF_PIXEL_FORMAT_B8G8R8A8_UNORM, GL_BGRA8_EXT, GL_BGRA, GL_UNSIGNED_BYTE, 0, true, false, false, false, "GL_EXT_texture_format_BGRA8888" },
	{ CF_PIXEL_FORMAT_BC1_RGBA_UNORM, GL_NONE, GL_NONE, GL_NONE, 0, true, false, false, false, NULL },
	{ CF_PIXEL_FORMAT_BC2_RGBA_UNORM, GL_NONE, GL_NONE, GL_NONE, 0, true, false, false, false, NULL },
	{ CF_PIXEL_FORMAT_BC3_RGBA_UNORM, GL_NONE, GL_NONE, GL_NONE, 0, true, false, false, false, NULL },
	{ CF_PIXEL_FORMAT_BC4_R_UNORM, GL_NONE, GL_NONE, GL_NONE, 0, false, false, false, false, NULL },
	{ CF_PIXEL_FORMAT_BC5_RG_UNORM, GL_NONE, GL_NONE, GL_NONE, 0, false, false, false, false, NULL },
	{ CF_PIXEL_FORMAT_BC7_RGBA_UNORM, GL_NONE, GL_NONE, GL_NONE, 0, true, false, false, false, NULL },
	{ CF_PIXEL_FORMAT_BC6H_RGB_FLOAT, GL_NONE, GL_NONE, GL_NONE, 0, false, false, false, false, NULL },
	{ CF_PIXEL_FORMAT_BC6H_RGB_UFLOAT, GL_NONE, GL_NONE, GL_NONE, 0, false, false, false, false, NULL },
	{ CF_PIXEL_FORMAT_R8_SNORM, GL_R8_SNORM, GL_RED, GL_BYTE, 0, false, false, false, false, NULL },
	{ CF_PIXEL_FORMAT_R8G8_SNORM, GL_RG8_SNORM, GL_RG, GL_BYTE, 0, false, false, false, false, NULL },
	{ CF_PIXEL_FORMAT_R8G8B8A8_SNORM, GL_RGBA8_SNORM, GL_RGBA, GL_BYTE, 0, true, false, false, false, NULL },
	{ CF_PIXEL_FORMAT_R16_SNORM, GL_R16_SNORM, GL_RED, GL_SHORT, 0, false, false, false, false, NULL },
	{ CF_PIXEL_FORMAT_R16G16_SNORM, GL_RG16_SNORM, GL_RG, GL_SHORT, 0, false, false, false, false, NULL },
	{ CF_PIXEL_FORMAT_R16G16B16A16_SNORM, GL_RGBA16_SNORM, GL_RGBA, GL_SHORT, 0, true, false, false, false, NULL },
	{ CF_PIXEL_FORMAT_R16_FLOAT, GL_R16F, GL_RED, GL_HALF_FLOAT, 0, false, false, false, false, NULL },
	{ CF_PIXEL_FORMAT_R16G16_FLOAT, GL_RG16F, GL_RG, GL_HALF_FLOAT, 0, false, false, false, false, NULL },
	{ CF_PIXEL_FORMAT_R16G16B16A16_FLOAT, GL_RGBA16F, GL_RGBA, GL_HALF_FLOAT, 0, true, false, false, false, NULL },
	{ CF_PIXEL_FORMAT_R32_FLOAT, GL_R32F, GL_RED, GL_FLOAT, 0, false, false, false, false, NULL },
	{ CF_PIXEL_FORMAT_R32G32_FLOAT, GL_RG32F, GL_RG, GL_FLOAT, 0, false, false, false, false, NULL },
	{ CF_PIXEL_FORMAT_R32G32B32A32_FLOAT, GL_RGBA32F, GL_RGBA, GL_FLOAT, 0, true, false, false, false, NULL },
	{ CF_PIXEL_FORMAT_R11G11B10_UFLOAT, GL_R11F_G11F_B10F, GL_RGB, GL_UNSIGNED_INT_10F_11F_11F_REV, 0, false, false, false, false, NULL },
	{ CF_PIXEL_FORMAT_R8_UINT, GL_R8UI, GL_RED_INTEGER, GL_UNSIGNED_BYTE, 0, false, false, false, true, NULL },
	{ CF_PIXEL_FORMAT_R8G8_UINT, GL_RG8UI, GL_RG_INTEGER, GL_UNSIGNED_BYTE, 0, false, false, false, true, NULL },
	{ CF_PIXEL_FORMAT_R8G8B8A8_UINT, GL_RGBA8UI, GL_RGBA_INTEGER, GL_UNSIGNED_BYTE, 0, true, false, false, true, NULL },
	{ CF_PIXEL_FORMAT_R16_UINT, GL_R16UI, GL_RED_INTEGER, GL_UNSIGNED_SHORT, 0, false, false, false, true, NULL },
	{ CF_PIXEL_FORMAT_R16G16_UINT, GL_RG16UI, GL_RG_INTEGER, GL_UNSIGNED_SHORT, 0, false, false, false, true, NULL },
	{ CF_PIXEL_FORMAT_R16G16B16A16_UINT, GL_RGBA16UI, GL_RGBA_INTEGER, GL_UNSIGNED_SHORT, 0, true, false, false, true, NULL },
	{ CF_PIXEL_FORMAT_R8_INT, GL_R8I, GL_RED_INTEGER, GL_BYTE, 0, false, false, false, true, NULL },
	{ CF_PIXEL_FORMAT_R8G8_INT, GL_RG8I, GL_RG_INTEGER, GL_BYTE, 0, false, false, false, true, NULL },
	{ CF_PIXEL_FORMAT_R8G8B8A8_INT, GL_RGBA8I, GL_RGBA_INTEGER, GL_BYTE, 0, true, false, false, true, NULL },
	{ CF_PIXEL_FORMAT_R16_INT, GL_R16I, GL_RED_INTEGER, GL_SHORT, 0, false, false, false, true, NULL },
	{ CF_PIXEL_FORMAT_R16G16_INT, GL_RG16I, GL_RG_INTEGER, GL_SHORT, 0, false, false, false, true, NULL },
	{ CF_PIXEL_FORMAT_R16G16B16A16_INT, GL_RGBA16I, GL_RGBA_INTEGER, GL_SHORT, 0, true, false, false, true, NULL },
	{ CF_PIXEL_FORMAT_R8G8B8A8_UNORM_SRGB, GL_SRGB8_ALPHA8, GL_RGBA, GL_UNSIGNED_BYTE, 0, true, false, false, false, NULL },
	{ CF_PIXEL_FORMAT_B8G8R8A8_UNORM_SRGB, GL_NONE, GL_NONE, GL_NONE, 0, true, false, false, false, NULL },
	{ CF_PIXEL_FORMAT_BC1_RGBA_UNORM_SRGB, GL_NONE, GL_NONE, GL_NONE, 0, true, false, false, false, NULL },
	{ CF_PIXEL_FORMAT_BC2_RGBA_UNORM_SRGB, GL_NONE, GL_NONE, GL_NONE, 0, true, false, false, false, NULL },
	{ CF_PIXEL_FORMAT_BC3_RGBA_UNORM_SRGB, GL_NONE, GL_NONE, GL_NONE, 0, true, false, false, false, NULL },
	{ CF_PIXEL_FORMAT_BC7_RGBA_UNORM_SRGB, GL_NONE, GL_NONE, GL_NONE, 0, true, false, false, false, NULL },
	{ CF_PIXEL_FORMAT_D16_UNORM, GL_DEPTH_COMPONENT16, GL_DEPTH_COMPONENT, GL_UNSIGNED_SHORT, 0, false, true, false, false, NULL },
	{ CF_PIXEL_FORMAT_D24_UNORM, GL_DEPTH_COMPONENT24, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, 0, false, true, false, false, NULL },
	{ CF_PIXEL_FORMAT_D32_FLOAT, GL_DEPTH_COMPONENT32F, GL_DEPTH_COMPONENT, GL_FLOAT, 0, false, true, false, false, NULL },
	{ CF_PIXEL_FORMAT_D24_UNORM_S8_UINT, GL_DEPTH24_STENCIL8, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, 0, false, true, true, false, NULL },
#if defined(GL_DEPTH32F_STENCIL8)
	{ CF_PIXEL_FORMAT_D32_FLOAT_S8_UINT, GL_DEPTH32F_STENCIL8, GL_DEPTH_STENCIL, GL_FLOAT_32_UNSIGNED_INT_24_8_REV, 0, false, true, true, false, NULL },
#else
	{ CF_PIXEL_FORMAT_D32_FLOAT_S8_UINT, GL_NONE, GL_NONE, GL_NONE, 0, false, true, true, false, NULL },
#endif
};

static bool opengl_has_extension(const char* name)
{
	if (!name) return true;
	GLint count = 0;
	glGetIntegerv(GL_NUM_EXTENSIONS, &count);
	for (GLint i = 0; i < count; ++i) {
		const char* ext = (const char*)glGetStringi(GL_EXTENSIONS, (GLuint)i);
		if (ext && CF_STRCMP(ext, name) == 0) return true;
	}
	return false;
}

static CF_GL_PixelFormatInfo* opengl_find_pixel_format_info(CF_PixelFormat format)
{
	for (size_t i = 0; i < CF_ARRAY_SIZE(g_gl_pixel_formats); ++i) {
		if (g_gl_pixel_formats[i].format == format) return &g_gl_pixel_formats[i];
	}
	return NULL;
}

void opengl_load_format_caps()
{
	static bool s_caps_initialized = false;
	if (s_caps_initialized) return;
	s_caps_initialized = true;

	for (size_t i = 0; i < CF_ARRAY_SIZE(g_gl_pixel_formats); ++i) {
		CF_GL_PixelFormatInfo& info = g_gl_pixel_formats[i];
		info.caps = 0;

		if (info.internal_fmt == GL_NONE) continue;
		if (!opengl_has_extension(info.required_extension)) continue;

		// GL 3.3 core does not support querying these properties.
		// Mark as unsupported by default.
		// You can hardcode a conservative set of assumptions here if desired.

		// Example conservative assumptions:
		if (!info.is_integer) info.caps |= CF_GL_FMT_CAP_SAMPLE; // can sample
		if (!info.is_integer) info.caps |= CF_GL_FMT_CAP_LINEAR; // linear filter on normalized formats

		if (info.has_alpha)   info.caps |= CF_GL_FMT_CAP_ALPHA;
		if (info.is_depth)    info.caps |= CF_GL_FMT_CAP_DEPTH;
		if (info.has_stencil) info.caps |= CF_GL_FMT_CAP_STENCIL;

		// Don’t set MSAA capability (GL_SAMPLES query not available in 3.3 core).
		// If you want MSAA support, you’ll need to test by actually creating
		// a multisampled renderbuffer and checking for errors.
	}
}

CF_BackendType opengl_query_backend()
{
	return CF_BACKEND_TYPE_PRIVATE;
}

char* opengl_transpile_spirv_to_glsl(const CF_ShaderBytecode& bytecode)
{
	if (!bytecode.content || !bytecode.size) return NULL;
	if (!g_spvc_context) return NULL;

	spvc_context context = g_spvc_context;

	spvc_parsed_ir ir = NULL;
	spvc_result result = spvc_context_parse_spirv(
		context,
		(const uint32_t*)bytecode.content,
		(size_t)(bytecode.size / sizeof(uint32_t)),
		&ir
	);
	if (result != SPVC_SUCCESS) {
		spvc_context_release_allocations(context);
		return NULL;
	}

	spvc_compiler compiler = NULL;
	result = spvc_context_create_compiler(context, SPVC_BACKEND_GLSL, ir, SPVC_CAPTURE_MODE_TAKE_OWNERSHIP, &compiler);
	if (result != SPVC_SUCCESS) {
		spvc_context_release_allocations(context);
		return NULL;
	}

	spvc_compiler_options options = NULL;
	if (spvc_compiler_create_compiler_options(compiler, &options) == SPVC_SUCCESS) {
#ifdef CF_EMSCRIPTEN
		spvc_compiler_options_set_uint(options, SPVC_COMPILER_OPTION_GLSL_VERSION, 300);
		spvc_compiler_options_set_bool(options, SPVC_COMPILER_OPTION_GLSL_ES, SPVC_TRUE);
#else
		spvc_compiler_options_set_uint(options, SPVC_COMPILER_OPTION_GLSL_VERSION, 330);
		spvc_compiler_options_set_bool(options, SPVC_COMPILER_OPTION_GLSL_ES, SPVC_FALSE);
#endif
		spvc_compiler_options_set_bool(options, SPVC_COMPILER_OPTION_GLSL_SEPARATE_SHADER_OBJECTS, SPVC_TRUE);
		spvc_compiler_install_compiler_options(compiler, options);
	}

	const char* source = NULL;
	result = spvc_compiler_compile(compiler, &source);
	char* output = NULL;
	if (result == SPVC_SUCCESS && source) {
		size_t len = CF_STRLEN(source);
		output = (char*)CF_ALLOC(len + 1);
		CF_MEMCPY(output, source, len + 1);
	}

	spvc_context_release_allocations(context);
	return output;
}

CF_INLINE GLenum opengl_wrap_filter(CF_Filter f)
{
	switch (f) { default:
	case CF_FILTER_NEAREST: return GL_NEAREST;
	case CF_FILTER_LINEAR:  return GL_LINEAR;
	}
}

CF_INLINE GLenum opengl_wrap_mip(CF_MipFilter m, bool has_mips)
{
	if (!has_mips) return GL_NEAREST; // min filter without mips
	switch (m) { default:
	case CF_MIP_FILTER_NEAREST: return GL_NEAREST_MIPMAP_NEAREST;
	case CF_MIP_FILTER_LINEAR:  return GL_LINEAR_MIPMAP_LINEAR;
	}
}

CF_INLINE GLenum opengl_wrap_wrap(CF_WrapMode w)
{
	switch (w) { default:
	case CF_WRAP_MODE_CLAMP_TO_EDGE:   return GL_CLAMP_TO_EDGE;
	case CF_WRAP_MODE_REPEAT:          return GL_REPEAT;
	case CF_WRAP_MODE_MIRRORED_REPEAT: return GL_MIRRORED_REPEAT;
	}
}

CF_INLINE GLenum opengl_internal_format(CF_PixelFormat f)
{
	CF_GL_PixelFormatInfo* info = opengl_find_pixel_format_info(f);
	return info ? info->internal_fmt : GL_NONE;
}

CF_INLINE GLenum opengl_upload_format(CF_PixelFormat f)
{
	CF_GL_PixelFormatInfo* info = opengl_find_pixel_format_info(f);
	return info ? info->upload_fmt : GL_NONE;
}

CF_INLINE GLenum opengl_upload_type(CF_PixelFormat f)
{
	CF_GL_PixelFormatInfo* info = opengl_find_pixel_format_info(f);
	return info ? info->upload_type : GL_NONE;
}

CF_INLINE GLenum opengl_primitive(CF_PrimitiveType p)
{
	switch (p) { default:
	case CF_PRIMITIVE_TYPE_TRIANGLELIST:  return GL_TRIANGLES;
	case CF_PRIMITIVE_TYPE_TRIANGLESTRIP: return GL_TRIANGLE_STRIP;
	case CF_PRIMITIVE_TYPE_LINELIST:      return GL_LINES;
	case CF_PRIMITIVE_TYPE_LINESTRIP:     return GL_LINE_STRIP;
	}
}

CF_INLINE GLenum opengl_compare(CF_CompareFunction c)
{
	switch (c) { default:
	case CF_COMPARE_FUNCTION_ALWAYS:                return GL_ALWAYS;
	case CF_COMPARE_FUNCTION_NEVER:                 return GL_NEVER;
	case CF_COMPARE_FUNCTION_LESS_THAN:             return GL_LESS;
	case CF_COMPARE_FUNCTION_EQUAL:                 return GL_EQUAL;
	case CF_COMPARE_FUNCTION_NOT_EQUAL:             return GL_NOTEQUAL;
	case CF_COMPARE_FUNCTION_LESS_THAN_OR_EQUAL:    return GL_LEQUAL;
	case CF_COMPARE_FUNCTION_GREATER_THAN:          return GL_GREATER;
	case CF_COMPARE_FUNCTION_GREATER_THAN_OR_EQUAL: return GL_GEQUAL;
	}
}

CF_INLINE GLenum opengl_cull_mode(CF_CullMode m)
{
	switch (m) { default:
	case CF_CULL_MODE_NONE:  return 0;
	case CF_CULL_MODE_FRONT: return GL_FRONT;
	case CF_CULL_MODE_BACK:  return GL_BACK;
	}
}

CF_INLINE GLenum opengl_blend_op(CF_BlendOp op)
{
	switch (op) { default:
	case CF_BLEND_OP_ADD:              return GL_FUNC_ADD;
	case CF_BLEND_OP_SUBTRACT:         return GL_FUNC_SUBTRACT;
	case CF_BLEND_OP_REVERSE_SUBTRACT: return GL_FUNC_REVERSE_SUBTRACT;
	case CF_BLEND_OP_MIN:              return GL_MIN;
	case CF_BLEND_OP_MAX:              return GL_MAX;
	}
}

CF_INLINE GLenum opengl_blend_factor(CF_BlendFactor f)
{
	switch (f) { default:
	case CF_BLENDFACTOR_ZERO:                     return GL_ZERO;
	case CF_BLENDFACTOR_ONE:                      return GL_ONE;
	case CF_BLENDFACTOR_SRC_COLOR:                return GL_SRC_COLOR;
	case CF_BLENDFACTOR_ONE_MINUS_SRC_COLOR:      return GL_ONE_MINUS_SRC_COLOR;
	case CF_BLENDFACTOR_DST_COLOR:                return GL_DST_COLOR;
	case CF_BLENDFACTOR_ONE_MINUS_DST_COLOR:      return GL_ONE_MINUS_DST_COLOR;
	case CF_BLENDFACTOR_SRC_ALPHA:                return GL_SRC_ALPHA;
	case CF_BLENDFACTOR_ONE_MINUS_SRC_ALPHA:      return GL_ONE_MINUS_SRC_ALPHA;
	case CF_BLENDFACTOR_DST_ALPHA:                return GL_DST_ALPHA;
	case CF_BLENDFACTOR_ONE_MINUS_DST_ALPHA:      return GL_ONE_MINUS_DST_ALPHA;
	case CF_BLENDFACTOR_CONSTANT_COLOR:           return GL_CONSTANT_COLOR;
	case CF_BLENDFACTOR_ONE_MINUS_CONSTANT_COLOR: return GL_ONE_MINUS_CONSTANT_COLOR;
	case CF_BLENDFACTOR_SRC_ALPHA_SATURATE:       return GL_SRC_ALPHA_SATURATE;
	}
}

struct CF_GL_TextureInternal
{
	int w = 0, h = 0;
	GLuint id = 0;
	GLenum internal_fmt = GL_NONE;
	GLenum upload_fmt   = GL_NONE;
	GLenum upload_type  = GL_NONE;
	bool has_mips = false;
	GLint min_filter = GL_LINEAR;
	GLint mag_filter = GL_LINEAR;
	GLint wrap_u = GL_REPEAT, wrap_v = GL_REPEAT;
};

struct CF_GL_Buffer
{
	GLuint id = 0;
	int size = 0;
	int stride = 0;
	int count = 0;
};

struct CF_GL_MeshInternal
{
	GLuint vao = 0;
	CF_GL_Buffer vbo;
	CF_GL_Buffer ibo;
	CF_GL_Buffer instance;
	int index_count = 0;

	int attribute_count = 0;
	CF_VertexAttribute attributes[CF_MESH_MAX_VERTEX_ATTRIBUTES];
};

struct CF_GL_ShaderInternal
{
	GLuint prog = 0;
	GLuint ubo = 0;
	GLuint ubo_index = GL_INVALID_INDEX;
	GLuint ubo_binding = 0; // binding point

	// lazy texture bindings by name
	struct TexBinding { const char* name; GLint loc; GLint unit; };
	Cute::Array<TexBinding> fs_textures;
};

struct CF_GL_MaterialInternal
{
	CF_RenderState state{};
	CF_MaterialState vs;
	CF_MaterialState fs;
	CF_Arena uniform_arena;
};

struct CF_GL_CanvasInternal
{
	int w = 0, h = 0;
	GLuint fbo = 0;
	GLuint color = 0; // texture
	GLuint depth = 0; // renderbuffer if present
	CF_Texture cf_color{}; // handle back to CF
};

static CF_GL_CanvasInternal* s_opengl_canvas = NULL;

static void opengl_apply_sampler_params(CF_GL_TextureInternal* t, const CF_TextureParams& p)
{
	opengl_load_format_caps();
	CF_GL_PixelFormatInfo* info = opengl_find_pixel_format_info(p.pixel_format);
	uint32_t caps = info ? info->caps : 0;
	t->has_mips = p.generate_mipmaps || p.mip_count > 1;
	GLenum min_filter = opengl_wrap_mip(p.mip_filter, t->has_mips);
	GLenum mag_filter = opengl_wrap_filter(p.filter);
	if (!(caps & CF_GL_FMT_CAP_LINEAR)) {
		min_filter = t->has_mips ? GL_NEAREST_MIPMAP_NEAREST : GL_NEAREST;
		mag_filter = GL_NEAREST;
	}
	t->min_filter = min_filter;
	t->mag_filter = mag_filter;
	t->wrap_u = opengl_wrap_wrap(p.wrap_u);
	t->wrap_v = opengl_wrap_wrap(p.wrap_v);
	
	glBindTexture(GL_TEXTURE_2D, t->id);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, t->min_filter);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, t->mag_filter);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, t->wrap_u);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, t->wrap_v);
	glBindTexture(GL_TEXTURE_2D, 0);
}

CF_TextureParams opengl_texture_defaults(int w, int h)
{
	CF_TextureParams p{};
	p.pixel_format = CF_PIXEL_FORMAT_R8G8B8A8_UNORM;
	p.usage = CF_TEXTURE_USAGE_SAMPLER_BIT;
	p.filter = CF_FILTER_LINEAR;
	p.wrap_u = CF_WRAP_MODE_REPEAT; p.wrap_v = CF_WRAP_MODE_REPEAT;
	p.mip_filter = CF_MIP_FILTER_LINEAR;
	p.width = w; p.height = h;
	return p;
}

bool opengl_texture_supports_format(CF_PixelFormat format, CF_TextureUsageBits usage)
{
	opengl_load_format_caps();
	CF_GL_PixelFormatInfo* info = opengl_find_pixel_format_info(format);
	if (!info || info->internal_fmt == GL_NONE) return false;
		uint32_t caps = info->caps;
		if (!caps) return false;
		if (usage & (CF_TEXTURE_USAGE_GRAPHICS_STORAGE_READ_BIT | CF_TEXTURE_USAGE_COMPUTE_STORAGE_READ_BIT | CF_TEXTURE_USAGE_COMPUTE_STORAGE_WRITE_BIT)) return false;
		if ((usage & CF_TEXTURE_USAGE_SAMPLER_BIT) && !(caps & CF_GL_FMT_CAP_SAMPLE)) return false;
		if ((usage & CF_TEXTURE_USAGE_COLOR_TARGET_BIT) && !(caps & CF_GL_FMT_CAP_COLOR)) return false;
		if (usage & CF_TEXTURE_USAGE_DEPTH_STENCIL_TARGET_BIT) {
		if (!info->is_depth) return false;
		if (!(caps & CF_GL_FMT_CAP_DEPTH)) return false;
		if (info->has_stencil && !(caps & CF_GL_FMT_CAP_STENCIL)) return false;
	}
	return true;
}

bool opengl_query_pixel_format(CF_PixelFormat format, CF_PixelFormatOp op)
{
	opengl_load_format_caps();
	CF_GL_PixelFormatInfo* info = opengl_find_pixel_format_info(format);
	if (!info || info->internal_fmt == GL_NONE) return false;
	uint32_t caps = info->caps;
	if (!caps) return false;

	switch (op) {
	case CF_PIXELFORMAT_OP_NEAREST_FILTER:
		return (caps & CF_GL_FMT_CAP_SAMPLE) != 0;

	case CF_PIXELFORMAT_OP_BILINEAR_FILTER:
		return (caps & CF_GL_FMT_CAP_SAMPLE) && (caps & CF_GL_FMT_CAP_LINEAR);

	case CF_PIXELFORMAT_OP_RENDER_TARGET:
		return (caps & CF_GL_FMT_CAP_COLOR) != 0;

	case CF_PIXELFORMAT_OP_ALPHA_BLENDING:
		if (!(caps & CF_GL_FMT_CAP_COLOR)) return false;
		if (!(caps & CF_GL_FMT_CAP_ALPHA)) return false;
		return !info->is_integer;

	case CF_PIXELFORMAT_OP_MSAA:
		if (info->is_depth) return (caps & CF_GL_FMT_CAP_MSAA) && (caps & CF_GL_FMT_CAP_DEPTH);
		return (caps & CF_GL_FMT_CAP_MSAA) && (caps & CF_GL_FMT_CAP_COLOR);

	case CF_PIXELFORMAT_OP_DEPTH:
		return (caps & CF_GL_FMT_CAP_DEPTH) != 0;

	default:
		return false;
	}
}

CF_Texture opengl_make_texture(CF_TextureParams params)
{
	if (!opengl_texture_supports_format(params.pixel_format, (CF_TextureUsageBits)params.usage)) {
		CF_ASSERT(!"Unsupported pixel format for OpenGL backend.");
		return CF_Texture{};
	}

	CF_GL_PixelFormatInfo* info = opengl_find_pixel_format_info(params.pixel_format);
	if (!info || info->internal_fmt == GL_NONE) return CF_Texture{};

	auto* t = CF_NEW(CF_GL_TextureInternal);
	t->w = params.width;
	t->h = params.height;
	t->internal_fmt = info->internal_fmt;
	t->upload_fmt   = info->upload_fmt;
	t->upload_type  = info->upload_type;
	if (!info->is_depth && (t->upload_fmt == GL_NONE || t->upload_type == GL_NONE)) {
		CF_FREE(t);
		return CF_Texture{};
	}

	glGenTextures(1, &t->id);
	if (!t->id) {
		CF_FREE(t);
		return CF_Texture{};
	}

	glBindTexture(GL_TEXTURE_2D, t->id);
	glTexImage2D(GL_TEXTURE_2D, 0, t->internal_fmt, t->w, t->h, 0, t->upload_fmt, t->upload_type, NULL);
	opengl_apply_sampler_params(t, params);
	if (params.generate_mipmaps) glGenerateMipmap(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, 0);

	return CF_Texture{ (uint64_t)(uintptr_t)t };
}

void opengl_destroy_texture(CF_Texture tex)
{
	if (!tex.id) return;
	auto* t = (CF_GL_TextureInternal*)(uintptr_t)tex.id;
	if (t->id) glDeleteTextures(1, &t->id);
	CF_FREE(t);
}

void opengl_texture_update(CF_Texture tex, void* data, int /*size*/)
{
	auto* t = (CF_GL_TextureInternal*)(uintptr_t)tex.id;
	glBindTexture(GL_TEXTURE_2D, t->id);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, t->w, t->h, t->upload_fmt, t->upload_type, data);
	if (t->has_mips) glGenerateMipmap(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, 0);
}

void opengl_texture_update_mip(CF_Texture tex, void* data, int /*size*/, int mip)
{
	auto* t = (CF_GL_TextureInternal*)(uintptr_t)tex.id;
	int w = cf_max(t->w >> mip, 1);
	int h = cf_max(t->h >> mip, 1);
	glBindTexture(GL_TEXTURE_2D, t->id);
	glTexSubImage2D(GL_TEXTURE_2D, mip, 0, 0, w, h, t->upload_fmt, t->upload_type, data);
	glBindTexture(GL_TEXTURE_2D, 0);
}

void opengl_generate_mipmaps(CF_Texture tex)
{
	auto* t = (CF_GL_TextureInternal*)(uintptr_t)tex.id;
	glBindTexture(GL_TEXTURE_2D, t->id);
	glGenerateMipmap(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, 0);
}

uint64_t opengl_texture_handle(CF_Texture t) { return t.id; }
uint64_t opengl_texture_binding_handle(CF_Texture t) { return t.id; }

CF_CanvasParams opengl_canvas_defaults(int w, int h)
{
	CF_CanvasParams p;
	CF_MEMSET(&p, 0, sizeof(p));
	if (w == 0 || h == 0) return p;
	p.target = opengl_texture_defaults(w, h);
	p.target.usage |= CF_TEXTURE_USAGE_COLOR_TARGET_BIT;
	p.depth_stencil_enable = false;
	p.depth_stencil_target = opengl_texture_defaults(w, h);
	p.depth_stencil_target.pixel_format = CF_PIXEL_FORMAT_D16_UNORM;
	p.depth_stencil_target.usage = CF_TEXTURE_USAGE_DEPTH_STENCIL_TARGET_BIT;
	p.sample_count = CF_SAMPLE_COUNT_1;
	return p;
}

static GLuint opengl_make_depth_renderbuffer(const CF_TextureParams& p)
{
	opengl_load_format_caps();
	CF_GL_PixelFormatInfo* info = opengl_find_pixel_format_info(p.pixel_format);
	if (!info || info->internal_fmt == GL_NONE || !info->is_depth) return 0;
	if (!(info->caps & CF_GL_FMT_CAP_DEPTH)) return 0;
	if (info->has_stencil && !(info->caps & CF_GL_FMT_CAP_STENCIL)) return 0;
	
	GLuint rbo = 0;
	glGenRenderbuffers(1, &rbo);
	glBindRenderbuffer(GL_RENDERBUFFER, rbo);
	glRenderbufferStorage(GL_RENDERBUFFER, info->internal_fmt, p.width, p.height);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);
	return rbo;
}

CF_Canvas opengl_make_canvas(CF_CanvasParams params)
{
	if (!opengl_texture_supports_format(params.target.pixel_format, (CF_TextureUsageBits)params.target.usage)) {
		CF_ASSERT(!"Unsupported color target format for OpenGL backend.");
		return CF_Canvas{};
	}
	if (params.depth_stencil_enable) {
		if (!opengl_texture_supports_format(params.depth_stencil_target.pixel_format, (CF_TextureUsageBits)params.depth_stencil_target.usage)) {
			CF_ASSERT(!"Unsupported depth/stencil format for OpenGL backend.");
			return CF_Canvas{};
		}
	}

	auto* c = CF_NEW(CF_GL_CanvasInternal);
	c->w = params.target.width;
	c->h = params.target.height;

	// color
	CF_Texture color = opengl_make_texture(params.target);
	c->cf_color = color;
	if (!color.id) {
		CF_FREE(c);
		return CF_Canvas{};
	}
	c->color = ((CF_GL_TextureInternal*)(uintptr_t)color.id)->id;

	// depth/stencil (renderbuffer)
	if (params.depth_stencil_enable) {
		c->depth = opengl_make_depth_renderbuffer(params.depth_stencil_target);
		if (!c->depth) {
			opengl_destroy_texture(color);
			CF_FREE(c);
			return CF_Canvas{};
		}
	}

	glGenFramebuffers(1, &c->fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, c->fbo);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, c->color, 0);
	if (c->depth) {
		if (params.depth_stencil_target.pixel_format == CF_PIXEL_FORMAT_D24_UNORM_S8_UINT)
			glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, c->depth);
		else
			glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, c->depth);
	}
	CF_ASSERT(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	return CF_Canvas{ (uint64_t)(uintptr_t)c };
}

void opengl_destroy_canvas(CF_Canvas ch)
{
	if (!ch.id) return;
	auto* c = (CF_GL_CanvasInternal*)(uintptr_t)ch.id;
	if (c->depth) glDeleteRenderbuffers(1, &c->depth);
	if (c->fbo) glDeleteFramebuffers(1, &c->fbo);
	opengl_destroy_texture(c->cf_color);
	CF_FREE(c);
}

CF_Texture opengl_canvas_get_target(CF_Canvas ch)
{
	auto* c = (CF_GL_CanvasInternal*)(uintptr_t)ch.id;
	return c->cf_color;
}

CF_Texture opengl_canvas_get_depth_stencil_target(CF_Canvas) { return CF_Texture{}; }

void opengl_clear_canvas(CF_Canvas ch)
{
	auto* c = (CF_GL_CanvasInternal*)(uintptr_t)ch.id;
	glBindFramebuffer(GL_FRAMEBUFFER, c->fbo);
	GLbitfield bits = GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT;
	glClearColor(app->clear_color.r, app->clear_color.g, app->clear_color.b, app->clear_color.a);
	glClearDepthf(app->clear_depth);
	glClearStencil((GLint)app->clear_stencil);
	glClear(bits);
	// leave bound for subsequent draws
}

// Bind canvas and optionally clear immediately.
void opengl_apply_canvas(CF_Canvas ch, bool clear)
{
	auto* c = (CF_GL_CanvasInternal*)(uintptr_t)ch.id;
	s_opengl_canvas = c;
	glBindFramebuffer(GL_FRAMEBUFFER, c->fbo);
	if (clear) opengl_clear_canvas(ch);
}

void opengl_apply_viewport(int x, int y, int w, int h) { glViewport(x, y, w, h); }
void opengl_apply_scissor(int x, int y, int w, int h) { glEnable(GL_SCISSOR_TEST); glScissor(x, y, w, h); }
void opengl_apply_stencil_reference(int reference) { glStencilFuncSeparate(GL_FRONT_AND_BACK, GL_ALWAYS, reference, 0xFF); }
void opengl_apply_blend_constants(float r, float g, float b, float a) { glBlendColor(r,g,b,a); }

CF_Mesh opengl_make_mesh(int vertex_buffer_size, const CF_VertexAttribute* attributes, int attribute_count, int vertex_stride)
{
	auto* m = CF_NEW(CF_GL_MeshInternal);
	glGenVertexArrays(1, &m->vao);
	glGenBuffers(1, &m->vbo.id);

	m->vbo.size = vertex_buffer_size;
	m->vbo.stride = vertex_stride;
	m->attribute_count = cf_min(attribute_count, CF_MESH_MAX_VERTEX_ATTRIBUTES);
	for (int i = 0; i < m->attribute_count; ++i) {
		m->attributes[i] = attributes[i];
		m->attributes[i].name = sintern(attributes[i].name);
	}

	glBindVertexArray(m->vao);
	glBindBuffer(GL_ARRAY_BUFFER, m->vbo.id);
	glBufferData(GL_ARRAY_BUFFER, vertex_buffer_size, NULL, GL_DYNAMIC_DRAW);
	glBindVertexArray(0);

	return CF_Mesh{ (uint64_t)(uintptr_t)m };
}

void opengl_mesh_set_index_buffer(CF_Mesh mh, int index_buffer_size_in_bytes, int index_bit_count)
{
	auto* m = (CF_GL_MeshInternal*)(uintptr_t)mh.id;
	if (!m->ibo.id) glGenBuffers(1, &m->ibo.id);
	CF_ASSERT(index_bit_count == 16 || index_bit_count == 32);
	m->ibo.size = index_buffer_size_in_bytes;
	m->ibo.stride = index_bit_count / 8;
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m->ibo.id);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, index_buffer_size_in_bytes, NULL, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void opengl_mesh_set_instance_buffer(CF_Mesh mh, int instance_buffer_size_in_bytes, int instance_stride)
{
	auto* m = (CF_GL_MeshInternal*)(uintptr_t)mh.id;
	if (!m->instance.id) glGenBuffers(1, &m->instance.id);
	m->instance.size = instance_buffer_size_in_bytes;
	m->instance.stride = instance_stride;
	glBindBuffer(GL_ARRAY_BUFFER, m->instance.id);
	glBufferData(GL_ARRAY_BUFFER, instance_buffer_size_in_bytes, NULL, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void opengl_mesh_update_vertex_data(CF_Mesh mh, void* verts, int vertex_count)
{
	auto* m = (CF_GL_MeshInternal*)(uintptr_t)mh.id;
	GLsizeiptr bytes = vertex_count * m->vbo.stride;
	glBindBuffer(GL_ARRAY_BUFFER, m->vbo.id);
	if (bytes > m->vbo.size) {
		glBufferData(GL_ARRAY_BUFFER, bytes, verts, GL_DYNAMIC_DRAW);
		m->vbo.size = (int)bytes;
	} else {
		glBufferSubData(GL_ARRAY_BUFFER, 0, bytes, verts);
	}
	m->vbo.count = vertex_count;
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void opengl_mesh_update_index_data(CF_Mesh mh, void* indices, int index_count)
{
	auto* m = (CF_GL_MeshInternal*)(uintptr_t)mh.id;
	m->index_count = index_count;
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m->ibo.id);
	int stride = m->ibo.stride ? m->ibo.stride : sizeof(uint16_t);
	GLsizeiptr bytes = index_count * stride;
	if (bytes > m->ibo.size) {
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, bytes, indices, GL_DYNAMIC_DRAW);
		m->ibo.size = (int)bytes;
	} else {
		glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, bytes, indices);
	}
	m->ibo.count = index_count;
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void opengl_mesh_update_instance_data(CF_Mesh mh, void* instances, int instance_count)
{
	auto* m = (CF_GL_MeshInternal*)(uintptr_t)mh.id;
	if (!m->instance.id) return;
	glBindBuffer(GL_ARRAY_BUFFER, m->instance.id);
	GLsizeiptr bytes = instance_count * m->instance.stride;
	if (bytes > m->instance.size) {
		glBufferData(GL_ARRAY_BUFFER, bytes, instances, GL_DYNAMIC_DRAW);
		m->instance.size = (int)bytes;
	} else {
		glBufferSubData(GL_ARRAY_BUFFER, 0, bytes, instances);
	}
	m->instance.count = instance_count;
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void opengl_destroy_mesh(CF_Mesh mh)
{
	if (!mh.id) return;
	auto* m = (CF_GL_MeshInternal*)(uintptr_t)mh.id;
	if (m->ibo.id) glDeleteBuffers(1, &m->ibo.id);
	if (m->vbo.id) glDeleteBuffers(1, &m->vbo.id);
	if (m->instance.id) glDeleteBuffers(1, &m->instance.id);
	if (m->vao)	glDeleteVertexArrays(1, &m->vao);
	CF_FREE(m);
}

struct CF_GL_ShaderAndMaterial
{
	CF_GL_ShaderInternal* sh = NULL;
	CF_GL_MaterialInternal* ma = NULL;
	CF_GL_MeshInternal* me = NULL;
};

CF_GL_ShaderAndMaterial s_opengl_bindings;

void opengl_apply_mesh(CF_Mesh mh)
{
	auto* m = (CF_GL_MeshInternal*)(uintptr_t)mh.id;
	s_opengl_bindings.me = m;
	glBindVertexArray(m->vao);
	glBindBuffer(GL_ARRAY_BUFFER, m->vbo.id);
	if (m->ibo.id) glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m->ibo.id);
}

static GLuint opengl_compile_shader(GLenum stage, const char* src)
{
	GLuint s = glCreateShader(stage);
	glShaderSource(s, 1, &src, NULL);
	glCompileShader(s);
	GLint ok = GL_FALSE;
	glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
	if (!ok) {
		char log[4096]; GLsizei len=0;
		glGetShaderInfoLog(s, sizeof(log), &len, log);
		fprintf(stderr, "GLSL compile error:\n%.*s\n", (int)len, log);
	}
	return s;
}

static GLuint opengl_link_program(GLuint vs, GLuint fs)
{
	GLuint p = glCreateProgram();
	glAttachShader(p, vs);
	glAttachShader(p, fs);
	glLinkProgram(p);
	GLint ok = GL_FALSE;
	glGetProgramiv(p, GL_LINK_STATUS, &ok);
	if (!ok) {
		char log[4096]; GLsizei len=0;
		glGetProgramInfoLog(p, sizeof(log), &len, log);
		fprintf(stderr, "GLSL link error:\n%.*s\n", (int)len, log);
	}
	glDetachShader(p, vs); glDetachShader(p, fs);
	glDeleteShader(vs); glDeleteShader(fs);
	return p;
}

static CF_Shader opengl_make_shader_es(const char* vs_src, const char* fs_src)
{
	auto* sh = CF_NEW(CF_GL_ShaderInternal);
	GLuint vs = opengl_compile_shader(GL_VERTEX_SHADER,   vs_src);
	GLuint fs = opengl_compile_shader(GL_FRAGMENT_SHADER, fs_src);
	sh->prog = opengl_link_program(vs, fs);

	// Optional UBO named "uniform_block"
	sh->ubo_index = glGetUniformBlockIndex(sh->prog, "uniform_block");
	if (sh->ubo_index != GL_INVALID_INDEX) {
		glGenBuffers(1, &sh->ubo);
		glBindBuffer(GL_UNIFORM_BUFFER, sh->ubo);
		glBufferData(GL_UNIFORM_BUFFER, 4 * 1024, NULL, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_UNIFORM_BUFFER, 0);
		sh->ubo_binding = 0;
		glUniformBlockBinding(sh->prog, sh->ubo_index, sh->ubo_binding);
	}

	return CF_Shader{ (uint64_t)(uintptr_t)sh };
}

CF_Shader opengl_make_shader_from_source(const char* vertex_src, const char* fragment_src)
{
	return opengl_make_shader_es(vertex_src, fragment_src);
}

CF_Shader opengl_make_shader(const char* vs_path, const char* fs_path)
{
	const char* vs = fs_read_entire_file_to_memory_and_nul_terminate(vs_path);
	const char* fs = fs_read_entire_file_to_memory_and_nul_terminate(fs_path);
	CF_ASSERT(vs && fs);
	return opengl_make_shader_es(vs, fs);
}

CF_Shader opengl_make_shader_from_bytecode(CF_ShaderBytecode vertex_bytecode, CF_ShaderBytecode fragment_bytecode)
{
#if !defined(CF_EMSCRIPTEN)
	char* vs_src = opengl_transpile_spirv_to_glsl(vertex_bytecode);
	char* fs_src = opengl_transpile_spirv_to_glsl(fragment_bytecode);
	if (!vs_src || !fs_src) {
		if (vs_src) CF_FREE(vs_src);
		if (fs_src) CF_FREE(fs_src);
		return CF_Shader{};
	}

	CF_Shader shader = opengl_make_shader_from_source(vs_src, fs_src);
	CF_FREE(vs_src);
	CF_FREE(fs_src);
	return shader;
#else
	CF_UNUSED(vertex_bytecode);
	CF_UNUSED(fragment_bytecode);
	return CF_Shader{};
#endif
}

void opengl_destroy_shader(CF_Shader sh)
{
	if (!sh.id) return;
	auto* s = (CF_GL_ShaderInternal*)(uintptr_t)sh.id;
	if (s->ubo) glDeleteBuffers(1, &s->ubo);
	if (s->prog) glDeleteProgram(s->prog);
	CF_FREE(s);
}

CF_Material opengl_make_material()
{
	auto* m = CF_NEW(CF_GL_MaterialInternal);
	m->state = CF_RenderState{};
	m->uniform_arena = make_arena(4, CF_KB * 16);
	return CF_Material{ (uint64_t)(uintptr_t)m };
}

void opengl_destroy_material(CF_Material mh)
{
	if (!mh.id) return;
	auto* m = (CF_GL_MaterialInternal*)(uintptr_t)mh.id;
	destroy_arena(&m->uniform_arena);
	CF_FREE(m);
}

int s_uniform_type_size(CF_UniformType t)
{
	switch (t) {
	case CF_UNIFORM_TYPE_FLOAT:  return 4;
	case CF_UNIFORM_TYPE_FLOAT2: return 8;
	case CF_UNIFORM_TYPE_FLOAT3: return 12;
	case CF_UNIFORM_TYPE_FLOAT4: return 16;
	case CF_UNIFORM_TYPE_INT:    return 4;
	case CF_UNIFORM_TYPE_INT2:   return 8;
	case CF_UNIFORM_TYPE_INT4:   return 16;
	case CF_UNIFORM_TYPE_MAT4:   return 64;
	default: return 0;
	}
}

void s_mat_set_uniform(CF_GL_MaterialInternal* mi, CF_MaterialState* st, const char* block, const char* name, void* data, CF_UniformType type, int array_len)
{
	int size = s_uniform_type_size(type) * array_len;
	CF_Uniform* u = NULL;
	for (int i = 0; i < st->uniforms.count(); ++i) {
		if (st->uniforms[i].block_name == block && st->uniforms[i].name == name) { u = &st->uniforms[i]; break; }
	}
	if (!u) {
		u = &st->uniforms.add();
		u->block_name = sintern(block);
		u->name = sintern(name);
		u->size = size;
		u->type = type;
		u->array_length = array_len;
		u->data = cf_arena_alloc(&mi->uniform_arena, size);
	}
	CF_ASSERT(u->size == size);
	CF_MEMCPY(u->data, data, size);
}

void opengl_material_set_uniform_vs(CF_Material m, const char* name, void* data, CF_UniformType type, int array_len)
{
	auto* mi = (CF_GL_MaterialInternal*)(uintptr_t)m.id;
	s_mat_set_uniform(mi, &mi->vs, "uniform_block", name, data, type, array_len);
}

void opengl_material_set_uniform_vs_internal(CF_Material m, const char* block, const char* name, void* data, CF_UniformType type, int array_len)
{
	auto* mi = (CF_GL_MaterialInternal*)(uintptr_t)m.id;
	s_mat_set_uniform(mi, &mi->vs, block, name, data, type, array_len);
}

void opengl_material_set_uniform_fs(CF_Material m, const char* name, void* data, CF_UniformType type, int array_len)
{
	auto* mi = (CF_GL_MaterialInternal*)(uintptr_t)m.id;
	s_mat_set_uniform(mi, &mi->fs, "uniform_block", name, data, type, array_len);
}

void opengl_material_set_uniform_fs_internal(CF_Material m, const char* block, const char* name, void* data, CF_UniformType type, int array_len)
{
	auto* mi = (CF_GL_MaterialInternal*)(uintptr_t)m.id;
	s_mat_set_uniform(mi, &mi->fs, block, name, data, type, array_len);
}

void opengl_material_clear_uniforms(CF_Material m)
{
	auto* mi = (CF_GL_MaterialInternal*)(uintptr_t)m.id;
	mi->vs.uniforms.clear();
	mi->fs.uniforms.clear();
	arena_reset(&mi->uniform_arena);
}

void opengl_material_set_texture_fs(CF_Material m, const char* name, CF_Texture t)
{
	auto* mi = (CF_GL_MaterialInternal*)(uintptr_t)m.id;
	CF_MaterialTex mt{ sintern(name), t };
	mi->fs.textures.add(mt);
}

void opengl_material_set_texture_vs(CF_Material m, const char* name, CF_Texture t)
{
	auto* mi = (CF_GL_MaterialInternal*)(uintptr_t)m.id;
	CF_MaterialTex mt{ sintern(name), t };
	mi->vs.textures.add(mt);
}

void opengl_material_clear_textures(CF_Material m)
{
	auto* mi = (CF_GL_MaterialInternal*)(uintptr_t)m.id;
	mi->vs.textures.clear();
	mi->fs.textures.clear();
}

void opengl_material_set_render_state(CF_Material m, CF_RenderState s)
{
	auto* mi = (CF_GL_MaterialInternal*)(uintptr_t)m.id;
	mi->state = s;
}

static void opengl_upload_uniform_block(CF_GL_ShaderInternal* sh, CF_GL_MaterialInternal* mi)
{
	if (sh->ubo_index == GL_INVALID_INDEX) return;

	// Compute total size (naive pack: VS then FS, in the order they were added).
	size_t total = 0;
	for (int pass = 0; pass < 2; ++pass) {
		const auto& list = (pass == 0) ? mi->vs.uniforms : mi->fs.uniforms;
		for (int i = 0; i < list.count(); ++i) total += (size_t)list[i].size;
	}
	if (total == 0) {
		// Nothing to upload; still ensure UBO is bound to the binding point.
		glBindBufferBase(GL_UNIFORM_BUFFER, sh->ubo_binding, sh->ubo);
		return;
	}

	// Single temporary blob; manual writes.
	uint8_t* blob = (uint8_t*)CF_ALLOC(total);
	uint8_t* write = blob;
	for (int pass = 0; pass < 2; ++pass) {
		const auto& list = (pass == 0) ? mi->vs.uniforms : mi->fs.uniforms;
		for (int i = 0; i < list.count(); ++i) {
			const CF_Uniform& u = list[i];
			CF_MEMCPY(write, u.data, u.size);
			write += u.size;
		}
	}

	// Upload to UBO.
	glBindBuffer(GL_UNIFORM_BUFFER, sh->ubo);
	GLint cur = 0;
	glGetBufferParameteriv(GL_UNIFORM_BUFFER, GL_BUFFER_SIZE, &cur);
	if ((GLint)total > cur) {
		glBufferData(GL_UNIFORM_BUFFER, (GLsizeiptr)total, NULL, GL_DYNAMIC_DRAW);
	}
	glBufferSubData(GL_UNIFORM_BUFFER, 0, (GLsizeiptr)total, blob);
	glBindBufferBase(GL_UNIFORM_BUFFER, sh->ubo_binding, sh->ubo);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);

	CF_FREE(blob);
}

void opengl_apply_shader(CF_Shader sh, CF_Material m)
{
	s_opengl_bindings.sh = (CF_GL_ShaderInternal*)(uintptr_t)sh.id;
	s_opengl_bindings.ma = (CF_GL_MaterialInternal*)(uintptr_t)m.id;

	glUseProgram(s_opengl_bindings.sh->prog);

	// render state
	auto& rs = s_opengl_bindings.ma->state;
	// cull
	if (rs.cull_mode == CF_CULL_MODE_NONE) glDisable(GL_CULL_FACE);
	else { glEnable(GL_CULL_FACE); glCullFace(opengl_cull_mode(rs.cull_mode)); }
	// depth
	if (rs.depth_write_enabled || rs.depth_compare != CF_COMPARE_FUNCTION_ALWAYS) {
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(opengl_compare(rs.depth_compare));
		glDepthMask(rs.depth_write_enabled ? GL_TRUE : GL_FALSE);
	} else {
		glDisable(GL_DEPTH_TEST);
	}
	// blend
	if (rs.blend.enabled) {
		glEnable(GL_BLEND);
		glColorMask(rs.blend.write_R_enabled, rs.blend.write_G_enabled, rs.blend.write_B_enabled, rs.blend.write_A_enabled);
		glBlendEquationSeparate(opengl_blend_op(rs.blend.rgb_op), opengl_blend_op(rs.blend.alpha_op));
		glBlendFuncSeparate(opengl_blend_factor(rs.blend.rgb_src_blend_factor),
							opengl_blend_factor(rs.blend.rgb_dst_blend_factor),
							opengl_blend_factor(rs.blend.alpha_src_blend_factor),
							opengl_blend_factor(rs.blend.alpha_dst_blend_factor));
	} else {
		glDisable(GL_BLEND);
		glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);
	}

	// uniforms
	opengl_upload_uniform_block(s_opengl_bindings.sh, s_opengl_bindings.ma);

	// textures (FS)
	GLint unit = 0;
	for (int i = 0; i < s_opengl_bindings.ma->fs.textures.count(); ++i) {
		const char* name = s_opengl_bindings.ma->fs.textures[i].name;
		auto* tex = (CF_GL_TextureInternal*)(uintptr_t)s_opengl_bindings.ma->fs.textures[i].handle.id;
		if (!tex) continue;
		GLint loc = glGetUniformLocation(s_opengl_bindings.sh->prog, name);
		if (loc >= 0) {
			glActiveTexture(GL_TEXTURE0 + unit);
			glBindTexture(GL_TEXTURE_2D, tex->id);
			glUniform1i(loc, unit);
			++unit;
		}
	}

	// vertex attribs (match by name)
	CF_GL_MeshInternal* me = s_opengl_bindings.me;
	if (me) {
		glBindVertexArray(me->vao);
		if (me->ibo.id) glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, me->ibo.id);
		for (int i = 0; i < me->attribute_count; ++i) {
			const auto& a = me->attributes[i];
			GLint loc = glGetAttribLocation(s_opengl_bindings.sh->prog, a.name);
			if (loc < 0) continue;

			const bool per_instance = a.per_instance;
			CF_GL_Buffer& buf = per_instance ? me->instance : me->vbo;
			if (!buf.id) continue;

			GLenum type = GL_FLOAT; GLint comps = 4; GLboolean norm = GL_FALSE;
			switch (a.format) {
				case CF_VERTEX_FORMAT_FLOAT:  type=GL_FLOAT; comps=1; break;
				case CF_VERTEX_FORMAT_FLOAT2: type=GL_FLOAT; comps=2; break;
				case CF_VERTEX_FORMAT_FLOAT3: type=GL_FLOAT; comps=3; break;
				case CF_VERTEX_FORMAT_FLOAT4: type=GL_FLOAT; comps=4; break;
				case CF_VERTEX_FORMAT_BYTE4_NORM: type=GL_BYTE; comps=4; norm=GL_TRUE; break;
				case CF_VERTEX_FORMAT_UBYTE4_NORM: type=GL_UNSIGNED_BYTE; comps=4; norm=GL_TRUE; break;
				case CF_VERTEX_FORMAT_SHORT2: type=GL_SHORT; comps=2; break;
				case CF_VERTEX_FORMAT_SHORT2_NORM: type=GL_SHORT; comps=2; norm=GL_TRUE; break;
				case CF_VERTEX_FORMAT_SHORT4: type=GL_SHORT; comps=4; break;
				case CF_VERTEX_FORMAT_SHORT4_NORM: type=GL_SHORT; comps=4; norm=GL_TRUE; break;
				case CF_VERTEX_FORMAT_USHORT2: type=GL_UNSIGNED_SHORT; comps=2; break;
				case CF_VERTEX_FORMAT_USHORT2_NORM: type=GL_UNSIGNED_SHORT; comps=2; norm=GL_TRUE; break;
				case CF_VERTEX_FORMAT_USHORT4: type=GL_UNSIGNED_SHORT; comps=4; break;
				case CF_VERTEX_FORMAT_USHORT4_NORM: type=GL_UNSIGNED_SHORT; comps=4; norm=GL_TRUE; break;
				default: break;
			}
			glBindBuffer(GL_ARRAY_BUFFER, buf.id);
			glEnableVertexAttribArray((GLuint)loc);
			glVertexAttribPointer((GLuint)loc, comps, type, norm, buf.stride, (const void*)(intptr_t)a.offset);
			glVertexAttribDivisor((GLuint)loc, per_instance ? 1 : 0);
		}
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}
}

void opengl_bind_mesh_for_shader(CF_Mesh mh)
{
	s_opengl_bindings.me = (CF_GL_MeshInternal*)(uintptr_t)mh.id;
	opengl_apply_mesh(mh);
}

void opengl_draw_arrays(CF_PrimitiveType prim, int first, int count)
{
	glDrawArrays(opengl_primitive(prim), first, count);
}

void opengl_draw_elements()
{
	auto* me = s_opengl_bindings.me;
	auto* ma = s_opengl_bindings.ma;
	if (!me || !ma) return;

	GLenum prim = opengl_primitive(ma->state.primitive_type);
	int instance_count = (me->instance.id && me->instance.count > 0) ? me->instance.count : 0;

	if (me->ibo.id && me->index_count > 0) {
		GLenum elem = (me->ibo.stride == 2) ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT;
		if (instance_count > 0) {
			glDrawElementsInstanced(prim, me->index_count, elem, NULL, instance_count);
		} else {
			glDrawElements(prim, me->index_count, elem, NULL);
		}
	} else if (me->vbo.count > 0) {
		if (instance_count > 0) {
			glDrawArraysInstanced(prim, 0, me->vbo.count, instance_count);
		} else {
			glDrawArrays(prim, 0, me->vbo.count);
		}
	}

	++app->draw_call_count;
}

void opengl_commit()
{
	// Unbind to keep state clean for the next backend
	glBindVertexArray(0);
	glUseProgram(0);
	if (s_opengl_canvas) glBindFramebuffer(GL_FRAMEBUFFER, 0);
	s_opengl_canvas = NULL;
}

void opengl_clear_color(float r, float g, float b, float a) { app->clear_color = make_color(r,g,b,a); }
void opengl_clear_depth_stencil(float depth, uint32_t stencil) { app->clear_depth = depth; app->clear_stencil = stencil; }

CF_RenderState opengl_render_state_defaults()
{
	CF_RenderState rs{};
	rs.primitive_type = CF_PRIMITIVE_TYPE_TRIANGLELIST;
	rs.cull_mode = CF_CULL_MODE_BACK;
	rs.depth_compare = CF_COMPARE_FUNCTION_ALWAYS;
	rs.depth_write_enabled = false;

	rs.blend.enabled = true;
	rs.blend.pixel_format = CF_PIXEL_FORMAT_R8G8B8A8_UNORM;
	rs.blend.write_R_enabled = rs.blend.write_G_enabled = rs.blend.write_B_enabled = rs.blend.write_A_enabled = true;
	rs.blend.rgb_op = CF_BLEND_OP_ADD;
	rs.blend.rgb_src_blend_factor = CF_BLENDFACTOR_ONE;
	rs.blend.rgb_dst_blend_factor = CF_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
	rs.blend.alpha_op = CF_BLEND_OP_ADD;
	rs.blend.alpha_src_blend_factor = CF_BLENDFACTOR_ONE;
	rs.blend.alpha_dst_blend_factor = CF_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
	return rs;
}

//--------------------------------------------------------------------------------------------------
// Backend dispatch shims.

CF_BackendType cf_query_backend()
{
	CF_BackendType type = CF_BACKEND_TYPE_INVALID;
	if (app->use_sdlgpu) type = sdlgpu_query_backend();
	if (app->use_opengl) type = opengl_query_backend();
	return type;
}


bool cf_texture_supports_format(CF_PixelFormat format, CF_TextureUsageBits usage)
{
	bool supported = false;
	if (app->use_sdlgpu) supported |= sdlgpu_texture_supports_format(format, usage);
	if (app->use_opengl) supported |= opengl_texture_supports_format(format, usage);
	return supported;
}

bool cf_query_pixel_format(CF_PixelFormat format, CF_PixelFormatOp op)
{
	bool supported = false;
	if (app->use_sdlgpu) supported |= sdlgpu_query_pixel_format(format, op);
	if (app->use_opengl) supported |= opengl_query_pixel_format(format, op);
	return supported;
}

CF_TextureParams cf_texture_defaults(int w, int h)
{
CF_TextureParams params{};
if (app->use_sdlgpu) params = sdlgpu_texture_defaults(w, h);
	if (app->use_opengl) params = opengl_texture_defaults(w, h);
	return params;
}

CF_Texture cf_make_texture(CF_TextureParams params)
{
	CF_Texture texture{};
	if (app->use_sdlgpu) texture = sdlgpu_make_texture(params);
	if (app->use_opengl) texture = opengl_make_texture(params);
	return texture;
}

void cf_destroy_texture(CF_Texture texture_handle)
{
	if (app->use_sdlgpu) sdlgpu_destroy_texture(texture_handle);
	if (app->use_opengl) opengl_destroy_texture(texture_handle);
}

void cf_texture_update(CF_Texture texture_handle, void* data, int size)
{
	if (app->use_sdlgpu) sdlgpu_texture_update(texture_handle, data, size);
	if (app->use_opengl) opengl_texture_update(texture_handle, data, size);
}

void cf_texture_update_mip(CF_Texture texture_handle, void* data, int size, int mip_level)
{
	if (app->use_sdlgpu) sdlgpu_texture_update_mip(texture_handle, data, size, mip_level);
	if (app->use_opengl) opengl_texture_update_mip(texture_handle, data, size, mip_level);
}

void cf_generate_mipmaps(CF_Texture texture_handle)
{
	if (app->use_sdlgpu) sdlgpu_generate_mipmaps(texture_handle);
	if (app->use_opengl) opengl_generate_mipmaps(texture_handle);
}

uint64_t cf_texture_handle(CF_Texture texture)
{
		uint64_t handle = 0;
		if (app->use_sdlgpu) handle = sdlgpu_texture_handle(texture);
		if (app->use_opengl) handle = opengl_texture_handle(texture);
		return handle;
}

uint64_t cf_texture_binding_handle(CF_Texture texture)
{
	uint64_t handle = 0;
	if (app->use_sdlgpu) handle = sdlgpu_texture_binding_handle(texture);
	if (app->use_opengl) handle = opengl_texture_binding_handle(texture);
	return handle;
}

CF_CanvasParams cf_canvas_defaults(int w, int h)
{
	CF_CanvasParams params{};
	if (app->use_sdlgpu) params = sdlgpu_canvas_defaults(w, h);
	if (app->use_opengl) params = opengl_canvas_defaults(w, h);
	return params;
}

CF_Canvas cf_make_canvas(CF_CanvasParams params)
{
	CF_Canvas canvas{};
	if (app->use_sdlgpu) canvas = sdlgpu_make_canvas(params);
	if (app->use_opengl) canvas = opengl_make_canvas(params);
	return canvas;
}

void cf_destroy_canvas(CF_Canvas canvas_handle)
{
	if (app->use_sdlgpu) sdlgpu_destroy_canvas(canvas_handle);
	if (app->use_opengl) opengl_destroy_canvas(canvas_handle);
}

CF_Texture cf_canvas_get_target(CF_Canvas canvas_handle)
{
	CF_Texture target{};
	if (app->use_sdlgpu) target = sdlgpu_canvas_get_target(canvas_handle);
	if (app->use_opengl) target = opengl_canvas_get_target(canvas_handle);
	return target;
}

CF_Texture cf_canvas_get_depth_stencil_target(CF_Canvas canvas_handle)
{
	CF_Texture target{};
	if (app->use_sdlgpu) target = sdlgpu_canvas_get_depth_stencil_target(canvas_handle);
	if (app->use_opengl) target = opengl_canvas_get_depth_stencil_target(canvas_handle);
	return target;
}

void cf_clear_canvas(CF_Canvas canvas_handle)
{
	if (app->use_sdlgpu) sdlgpu_clear_canvas(canvas_handle);
	if (app->use_opengl) opengl_clear_canvas(canvas_handle);
}

void cf_apply_canvas(CF_Canvas canvas_handle, bool clear)
{
	if (app->use_sdlgpu) sdlgpu_apply_canvas(canvas_handle, clear);
	if (app->use_opengl) opengl_apply_canvas(canvas_handle, clear);
}

void cf_apply_viewport(int x, int y, int w, int h)
{
	if (app->use_sdlgpu) sdlgpu_apply_viewport(x, y, w, h);
	if (app->use_opengl) opengl_apply_viewport(x, y, w, h);
}

void cf_apply_scissor(int x, int y, int w, int h)
{
	if (app->use_sdlgpu) sdlgpu_apply_scissor(x, y, w, h);
	if (app->use_opengl) opengl_apply_scissor(x, y, w, h);
}

void cf_apply_stencil_reference(int reference)
{
	if (app->use_sdlgpu) sdlgpu_apply_stencil_reference(reference);
	if (app->use_opengl) opengl_apply_stencil_reference(reference);
}

void cf_apply_blend_constants(float r, float g, float b, float a)
{
	if (app->use_sdlgpu) sdlgpu_apply_blend_constants(r, g, b, a);
	if (app->use_opengl) opengl_apply_blend_constants(r, g, b, a);
}

CF_Mesh cf_make_mesh(int vertex_buffer_size, const CF_VertexAttribute* attributes, int attribute_count, int vertex_stride)
{
	CF_Mesh mesh{};
	if (app->use_sdlgpu) mesh = sdlgpu_make_mesh(vertex_buffer_size, attributes, attribute_count, vertex_stride);
	if (app->use_opengl) mesh = opengl_make_mesh(vertex_buffer_size, attributes, attribute_count, vertex_stride);
	return mesh;
}

void cf_mesh_set_index_buffer(CF_Mesh mesh_handle, int index_buffer_size_in_bytes, int index_bit_count)
{
	if (app->use_sdlgpu) sdlgpu_mesh_set_index_buffer(mesh_handle, index_buffer_size_in_bytes, index_bit_count);
	if (app->use_opengl) opengl_mesh_set_index_buffer(mesh_handle, index_buffer_size_in_bytes, index_bit_count);
}

void cf_mesh_set_instance_buffer(CF_Mesh mesh_handle, int instance_buffer_size_in_bytes, int instance_stride)
{
	if (app->use_sdlgpu) sdlgpu_mesh_set_instance_buffer(mesh_handle, instance_buffer_size_in_bytes, instance_stride);
	if (app->use_opengl) opengl_mesh_set_instance_buffer(mesh_handle, instance_buffer_size_in_bytes, instance_stride);
}

void cf_destroy_mesh(CF_Mesh mesh_handle)
{
	if (app->use_sdlgpu) sdlgpu_destroy_mesh(mesh_handle);
	if (app->use_opengl) opengl_destroy_mesh(mesh_handle);
}

void cf_mesh_update_vertex_data(CF_Mesh mesh_handle, void* data, int count)
{
	if (app->use_sdlgpu) sdlgpu_mesh_update_vertex_data(mesh_handle, data, count);
	if (app->use_opengl) opengl_mesh_update_vertex_data(mesh_handle, data, count);
}

void cf_mesh_update_index_data(CF_Mesh mesh_handle, void* data, int count)
{
	if (app->use_sdlgpu) sdlgpu_mesh_update_index_data(mesh_handle, data, count);
	if (app->use_opengl) opengl_mesh_update_index_data(mesh_handle, data, count);
}

void cf_mesh_update_instance_data(CF_Mesh mesh_handle, void* data, int count)
{
	if (app->use_sdlgpu) sdlgpu_mesh_update_instance_data(mesh_handle, data, count);
	if (app->use_opengl) opengl_mesh_update_instance_data(mesh_handle, data, count);
}

CF_RenderState cf_render_state_defaults()
{
	CF_RenderState state{};
	if (app->use_sdlgpu) state = sdlgpu_render_state_defaults();
	if (app->use_opengl) state = opengl_render_state_defaults();
	return state;
}

CF_Material cf_make_material()
{
	CF_Material material{};
	if (app->use_sdlgpu) material = sdlgpu_make_material();
	if (app->use_opengl) material = opengl_make_material();
	return material;
}

void cf_destroy_material(CF_Material material_handle)
{
	if (app->use_sdlgpu) sdlgpu_destroy_material(material_handle);
	if (app->use_opengl) opengl_destroy_material(material_handle);
}

void cf_material_set_render_state(CF_Material material_handle, CF_RenderState render_state)
{
	if (app->use_sdlgpu) sdlgpu_material_set_render_state(material_handle, render_state);
	if (app->use_opengl) opengl_material_set_render_state(material_handle, render_state);
}

void cf_material_set_texture_vs(CF_Material material_handle, const char* name, CF_Texture texture)
{
	if (app->use_sdlgpu) sdlgpu_material_set_texture_vs(material_handle, name, texture);
	if (app->use_opengl) opengl_material_set_texture_vs(material_handle, name, texture);
}

void cf_material_set_texture_fs(CF_Material material_handle, const char* name, CF_Texture texture)
{
	if (app->use_sdlgpu) sdlgpu_material_set_texture_fs(material_handle, name, texture);
	if (app->use_opengl) opengl_material_set_texture_fs(material_handle, name, texture);
}

void cf_material_clear_textures(CF_Material material_handle)
{
	if (app->use_sdlgpu) sdlgpu_material_clear_textures(material_handle);
	if (app->use_opengl) opengl_material_clear_textures(material_handle);
}

void cf_material_set_uniform_vs(CF_Material material_handle, const char* name, void* data, CF_UniformType type, int array_length)
{
	if (app->use_sdlgpu) sdlgpu_material_set_uniform_vs(material_handle, name, data, type, array_length);
	if (app->use_opengl) opengl_material_set_uniform_vs(material_handle, name, data, type, array_length);
}

void cf_material_set_uniform_vs_internal(CF_Material material_handle, const char* block_name, const char* name, void* data, CF_UniformType type, int array_length)
{
	if (app->use_sdlgpu) sdlgpu_material_set_uniform_vs_internal(material_handle, block_name, name, data, type, array_length);
	if (app->use_opengl) opengl_material_set_uniform_vs_internal(material_handle, block_name, name, data, type, array_length);
}

void cf_material_set_uniform_fs(CF_Material material_handle, const char* name, void* data, CF_UniformType type, int array_length)
{
	if (app->use_sdlgpu) sdlgpu_material_set_uniform_fs(material_handle, name, data, type, array_length);
	if (app->use_opengl) opengl_material_set_uniform_fs(material_handle, name, data, type, array_length);
}

void cf_material_set_uniform_fs_internal(CF_Material material_handle, const char* block_name, const char* name, void* data, CF_UniformType type, int array_length)
{
	if (app->use_sdlgpu) sdlgpu_material_set_uniform_fs_internal(material_handle, block_name, name, data, type, array_length);
	if (app->use_opengl) opengl_material_set_uniform_fs_internal(material_handle, block_name, name, data, type, array_length);
}

void cf_material_clear_uniforms(CF_Material material_handle)
{
	if (app->use_sdlgpu) sdlgpu_material_clear_uniforms(material_handle);
	if (app->use_opengl) opengl_material_clear_uniforms(material_handle);
}

void cf_clear_color(float red, float green, float blue, float alpha)
{
	if (app->use_sdlgpu) sdlgpu_clear_color(red, green, blue, alpha);
	if (app->use_opengl) opengl_clear_color(red, green, blue, alpha);
}

void cf_clear_depth_stencil(float depth, uint32_t stencil)
{
	if (app->use_sdlgpu) sdlgpu_clear_depth_stencil(depth, stencil);
	if (app->use_opengl) opengl_clear_depth_stencil(depth, stencil);
}

void cf_apply_shader(CF_Shader shader_handle, CF_Material material_handle)
{
	if (app->use_sdlgpu) sdlgpu_apply_shader(shader_handle, material_handle);
	if (app->use_opengl) opengl_apply_shader(shader_handle, material_handle);
}

CF_Shader cf_make_shader(const char* vertex_path, const char* fragment_path)
{
	CF_Shader shader{};
	if (app->use_sdlgpu) shader = sdlgpu_make_shader(vertex_path, fragment_path);
	if (app->use_opengl) shader = opengl_make_shader(vertex_path, fragment_path);
	return shader;
}

CF_Shader cf_make_shader_from_source(const char* vertex_src, const char* fragment_src)
{
	CF_Shader shader{};
	if (app->use_sdlgpu) shader = sdlgpu_make_shader_from_source(vertex_src, fragment_src);
	if (app->use_opengl) shader = opengl_make_shader_from_source(vertex_src, fragment_src);
	return shader;
}

CF_Shader cf_make_shader_from_bytecode(CF_ShaderBytecode vertex_bytecode, CF_ShaderBytecode fragment_bytecode)
{
	CF_Shader shader{};
	if (app->use_sdlgpu) shader = sdlgpu_make_shader_from_bytecode(vertex_bytecode, fragment_bytecode);
	if (app->use_opengl) shader = opengl_make_shader_from_bytecode(vertex_bytecode, fragment_bytecode);
	return shader;
}

void cf_destroy_shader(CF_Shader shader_handle)
{
	if (app->use_sdlgpu) sdlgpu_destroy_shader(shader_handle);
	if (app->use_opengl) opengl_destroy_shader(shader_handle);
}

void cf_apply_mesh(CF_Mesh mesh_handle)
{
	if (app->use_sdlgpu) sdlgpu_apply_mesh(mesh_handle);
	if (app->use_opengl) opengl_apply_mesh(mesh_handle);
}

void cf_draw_elements()
{
	if (app->use_sdlgpu) sdlgpu_draw_elements();
	if (app->use_opengl) opengl_draw_elements();
}

void cf_commit()
{
	if (app->use_sdlgpu) sdlgpu_commit();
	if (app->use_opengl) opengl_commit();
}
