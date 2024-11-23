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
#define SDL_GPU_SHADERCROSS_IMPLEMENTATION
#define SDL_GPU_SHADERCROSS_STATIC
#include <SDL_gpu_shadercross/SDL_gpu_shadercross.h>

struct CF_CanvasInternal;
static CF_CanvasInternal* s_canvas = NULL;
static CF_CanvasInternal* s_default_canvas = NULL;

#include <float.h>

using namespace Cute;

//--------------------------------------------------------------------------------------------------
// SDL_GPU wrapping implementation of cute_graphics.h.
// ...Variety of enum converters/struct initializers are in cute_graphics_internal.h.

CF_INLINE CF_UniformType s_uniform_type(CF_ShaderInfoDataType type)
{
	switch (type) {
	case CF_SHADER_INFO_TYPE_UNKNOWN: return CF_UNIFORM_TYPE_UNKNOWN;
	case CF_SHADER_INFO_TYPE_FLOAT:   return CF_UNIFORM_TYPE_FLOAT;
	case CF_SHADER_INFO_TYPE_FLOAT2:  return CF_UNIFORM_TYPE_FLOAT2;
	case CF_SHADER_INFO_TYPE_FLOAT4:  return CF_UNIFORM_TYPE_FLOAT4;
	case CF_SHADER_INFO_TYPE_SINT:    return CF_UNIFORM_TYPE_INT;
	case CF_SHADER_INFO_TYPE_SINT2:   return CF_UNIFORM_TYPE_INT2;
	case CF_SHADER_INFO_TYPE_SINT4:   return CF_UNIFORM_TYPE_INT4;
	case CF_SHADER_INFO_TYPE_MAT4:    return CF_UNIFORM_TYPE_MAT4;
	default: return CF_UNIFORM_TYPE_UNKNOWN;
	}
}

CF_INLINE CF_ShaderInputFormat s_wrap(CF_ShaderInfoDataType type)
{
	switch (type) {
	case CF_SHADER_INFO_TYPE_UNKNOWN: return CF_SHADER_INPUT_FORMAT_UNKNOWN;
	case CF_SHADER_INFO_TYPE_UINT:    return CF_SHADER_INPUT_FORMAT_UINT;
	case CF_SHADER_INFO_TYPE_SINT:    return CF_SHADER_INPUT_FORMAT_INT;
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

CF_BackendType cf_query_backend()
{
	SDL_GPUShaderFormat format = SDL_GetGPUShaderFormats(app->device);
	switch (format) {
	case SDL_GPU_SHADERFORMAT_INVALID:  return CF_BACKEND_TYPE_INVALID;
	case SDL_GPU_SHADERFORMAT_PRIVATE:  return CF_BACKEND_TYPE_PRIVATE;
	case SDL_GPU_SHADERFORMAT_SPIRV:    return CF_BACKEND_TYPE_VULKAN;
	case SDL_GPU_SHADERFORMAT_DXBC:     return CF_BACKEND_TYPE_D3D11;
	case SDL_GPU_SHADERFORMAT_DXIL:     return CF_BACKEND_TYPE_D3D12;
	case SDL_GPU_SHADERFORMAT_MSL:      // Fall through.
        case SDL_GPU_SHADERFORMAT_METALLIB: return CF_BACKEND_TYPE_METAL;
	default: return CF_BACKEND_TYPE_INVALID;
	}
}

bool cf_texture_supports_format(CF_PixelFormat format, CF_TextureUsageBits usage)
{
	return SDL_GPUTextureSupportsFormat(
			app->device,
			s_wrap(format),
			SDL_GPU_TEXTURETYPE_2D,
			usage
			);
}

CF_TextureParams cf_texture_defaults(int w, int h)
{
	CF_TextureParams params;
	params.pixel_format = CF_PIXEL_FORMAT_R8G8B8A8_UNORM;
	params.filter = CF_FILTER_LINEAR;
	params.usage = CF_TEXTURE_USAGE_SAMPLER_BIT;
	params.wrap_u = CF_WRAP_MODE_REPEAT;
	params.wrap_v = CF_WRAP_MODE_REPEAT;
	params.width = w;
	params.height = h;
	params.stream = false;
	return params;
}

CF_INLINE bool s_is_depth(CF_PixelFormat format)
{
	return format >= CF_PIXEL_FORMAT_D16_UNORM;
}

CF_Texture cf_make_texture(CF_TextureParams params)
{
	SDL_GPUTextureCreateInfo tex_info = SDL_GPUTextureCreateInfoDefaults(params.width, params.height);
	tex_info.width = (Uint32)params.width;
	tex_info.height = (Uint32)params.height;
	tex_info.format = s_wrap(params.pixel_format);
	tex_info.usage = params.usage;
	SDL_GPUTexture* tex = SDL_CreateGPUTexture(app->device, &tex_info);
	CF_ASSERT(tex);
	if (!tex) return { 0 };

	SDL_GPUSampler* sampler = NULL;
	// Depth/stencil textures don't need their own sampler, as the associated color
	// texture in the owning canvas already has a sampler attached.
	if (!s_is_depth(params.pixel_format)) {
		SDL_GPUSamplerCreateInfo sampler_info = SDL_GPUSamplerCreateInfoDefaults();
		sampler_info.min_filter = s_wrap(params.filter);
		sampler_info.mag_filter = s_wrap(params.filter);
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
		SDL_GPUTransferBuffer* buf = SDL_CreateGPUTransferBuffer(app->device, &tbuf_info);
	}

	CF_TextureInternal* tex_internal = CF_NEW(CF_TextureInternal);
	tex_internal->w = params.width;
	tex_internal->h = params.height;
	tex_internal->filter = sampler ? s_wrap(params.filter) : SDL_GPU_FILTER_NEAREST;
	tex_internal->tex = tex;
	tex_internal->buf = buf;
	tex_internal->sampler = sampler;
	tex_internal->format = tex_info.format;
	CF_Texture result;
	result.id = { (uint64_t)tex_internal };
	return result;
}

void cf_destroy_texture(CF_Texture texture_handle)
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

void cf_texture_update(CF_Texture texture_handle, void* data, int size)
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

uint64_t cf_texture_handle(CF_Texture texture)
{
	return (uint64_t)((CF_TextureInternal*)texture.id)->tex;
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

static cute_shader_vfs_t s_cute_shader_vfs = {
	.read_file_content = s_cute_shader_vfs_read,
	.free_file_content = s_cute_shader_vfs_free,
};
#endif

CF_ShaderBytecode cf_compile_shader_to_bytecode_internal(const char* shader_src, CF_ShaderStage cf_stage, const char* user_shd)
{
#ifdef CF_RUNTIME_SHADER_COMPILATION
	cute_shader_stage_t stage = CUTE_SHADER_STAGE_VERTEX;
	switch (cf_stage) {
	default: CF_ASSERT(false); break; // No valid stage provided.
	case CF_SHADER_STAGE_VERTEX: stage = CUTE_SHADER_STAGE_VERTEX; break;
	case CF_SHADER_STAGE_FRAGMENT: stage = CUTE_SHADER_STAGE_FRAGMENT; break;
	}

	// Setup builtin includes
	int num_builtin_includes = sizeof(s_builtin_includes) / sizeof(s_builtin_includes[0]);
	dyna cute_shader_file_t builtin_includes[sizeof(s_builtin_includes) / sizeof(s_builtin_includes[0]) + 1];
	// Use user shader as stub if provided
	for (int i = 0; i < num_builtin_includes; ++i) {
		builtin_includes[i] =  s_builtin_includes[i];
	}
	cute_shader_file_t shader_stub;
	shader_stub.name = "shader_stub.shd";
	shader_stub.content = user_shd != NULL ? user_shd : s_shader_stub;
	builtin_includes[num_builtin_includes++] = shader_stub;

	int num_include_dirs = 0;
	const char* include_dirs[1];
	if (app->shader_directory_set) {
		include_dirs[num_include_dirs++] = app->shader_directory.c_str();
	}

	cute_shader_config_t config = {
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

	cute_shader_result_t result = cute_shader_compile(shader_src, stage, config);
	if (result.success) {
		CF_ShaderBytecode bytecode = {
			.content = (uint8_t*)result.bytecode,
			.size = result.bytecode_size,
			.info = result.info,
		};
		return bytecode;
	} else {
		fprintf(stderr, "%s\n", result.error_message);
		cute_shader_free_result(result);

		CF_ShaderBytecode bytecode = { 0 };
		return bytecode;
	}
#else
	fprintf(stderr, "CF was not built with runtime shader compilation enabled\n");

	CF_ShaderBytecode bytecode = { 0 };
	return bytecode;
#endif
}

CF_ShaderBytecode cf_compile_shader_to_bytecode(const char* shader_src, CF_ShaderStage cf_stage)
{
	return cf_compile_shader_to_bytecode_internal(shader_src, cf_stage, NULL);
}

void cf_free_bytecode(CF_ShaderBytecode bytecode)
{
#ifdef CF_RUNTIME_SHADER_COMPILATION
	cute_shader_result_t compile_result = {
		.bytecode = (void*)bytecode.content,
		.info = bytecode.info,
	};
	cute_shader_free_result(compile_result);
#endif
}

static SDL_GPUShader* s_compile(CF_ShaderInternal* shader_internal, CF_ShaderBytecode bytecode, CF_ShaderStage stage)
{
	bool vs = stage == CF_SHADER_STAGE_VERTEX ? true : false;

	// Load reflection info

	for (int i = 0; i < bytecode.info.num_images; ++i) {
		shader_internal->image_names.add(sintern(bytecode.info.image_names[i]));
	}

	shader_internal->uniform_block_count = bytecode.info.num_uniforms;
	const CF_ShaderUniformMemberInfo* member_infos = bytecode.info.uniform_members;
	for (int i = 0; i < bytecode.info.num_uniforms; ++i) {
		const CF_ShaderUniformInfo* block_info = &bytecode.info.uniforms[i];
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
		CF_ASSERT(bytecode.info.num_inputs <= CF_MAX_SHADER_INPUTS); // Increase `CF_MAX_SHADER_INPUTS`, or refactor the shader with less vertex attributes.
		shader_internal->input_count = bytecode.info.num_inputs;
		for (int i = 0; i < bytecode.info.num_inputs; ++i) {
			CF_ShaderInputInfo* input = &bytecode.info.inputs[i];
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
	shaderCreateInfo.num_samplers = bytecode.info.num_samplers;
	shaderCreateInfo.num_storage_textures = bytecode.info.num_storage_textures;
	shaderCreateInfo.num_storage_buffers = bytecode.info.num_storage_buffers;
	shaderCreateInfo.num_uniform_buffers = bytecode.info.num_uniforms;
	SDL_GPUShader* sdl_shader = NULL;
	if (SDL_GetGPUShaderFormats(app->device) == SDL_GPU_SHADERFORMAT_SPIRV) {
		sdl_shader = (SDL_GPUShader*)SDL_CreateGPUShader(app->device, &shaderCreateInfo);
	} else {
		sdl_shader = (SDL_GPUShader*)SDL_ShaderCross_CompileGraphicsShaderFromSPIRV(app->device, &shaderCreateInfo);
	}
	CF_ASSERT(sdl_shader);
	return sdl_shader;
}

CF_Shader cf_make_shader_from_bytecode(CF_ShaderBytecode vertex_bytecode, CF_ShaderBytecode fragment_bytecode)
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
	CF_ShaderBytecode vs_bytecode = cf_compile_shader_to_bytecode_internal(vs_src, CF_SHADER_STAGE_VERTEX, user_shd);
	if (vs_bytecode.content == NULL) {
		CF_Shader result = { 0 };
		return result;
	}
	CF_ShaderBytecode fs_bytecode = cf_compile_shader_to_bytecode_internal(fs_src, CF_SHADER_STAGE_FRAGMENT, user_shd);
	if (fs_bytecode.content == NULL) {
		cf_free_bytecode(vs_bytecode);
		CF_Shader result = { 0 };
		return result;
	}

	// Create the actual shader object.
	CF_Shader shader = cf_make_shader_from_bytecode(vs_bytecode, fs_bytecode);
	cf_free_bytecode(vs_bytecode);
	cf_free_bytecode(fs_bytecode);
	return shader;
}

void cf_load_internal_shaders()
{
#ifdef CF_RUNTIME_SHADER_COMPILATION
	cute_shader_init();

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

CF_Shader cf_make_shader(const char* vertex_path, const char* fragment_path)
{
	// Make sure each file can be found.
	const char* vs = fs_read_entire_file_to_memory_and_nul_terminate(vertex_path);
	const char* fs = fs_read_entire_file_to_memory_and_nul_terminate(fragment_path);
	CF_ASSERT(vs);
	CF_ASSERT(fs);
	return s_compile(vs, fs);
}

CF_Shader cf_make_shader_from_source(const char* vertex_src, const char* fragment_src)
{
	return s_compile(vertex_src, fragment_src);
}

void cf_destroy_shader(CF_Shader shader_handle)
{
	// Draw shaders automatically have blit shaders generated, so clean that up as well,
	// if it exists. See `cf_make_draw_shader`.
	CF_Shader* blit = (CF_Shader*)draw->draw_shd_to_blit_shd.try_get(shader_handle.id);
	if (blit) {
		draw->draw_shd_to_blit_shd.remove(shader_handle.id);
		cf_destroy_shader(*blit);
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

void cf_clear_canvas(CF_Canvas canvas_handle)
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
		.cycle = true,
		.clear_stencil = 0,
	};
	SDL_GPURenderPass* renderPass = SDL_BeginGPURenderPass(cmd, &color_info, 1, canvas->depth_stencil ? &depth_stencil_info : NULL);
	SDL_EndGPURenderPass(renderPass);
	canvas->clear = false;

	if (!app->cmd) SDL_SubmitGPUCommandBuffer(cmd);
}

CF_CanvasParams cf_canvas_defaults(int w, int h)
{
	CF_CanvasParams params;
	if (w == 0 || h == 0) {
		params.name = NULL;
		params.target = { };
		params.depth_stencil_target = { };
	} else {
		params.name = NULL;
		params.target = cf_texture_defaults(w, h);
		params.target.usage |= CF_TEXTURE_USAGE_COLOR_TARGET_BIT;
		params.depth_stencil_enable = false;
		params.depth_stencil_target = cf_texture_defaults(w, h);
		params.depth_stencil_target.pixel_format = CF_PIXEL_FORMAT_D32_FLOAT_S8_UINT;
    if (cf_texture_supports_format(CF_PIXEL_FORMAT_D24_UNORM_S8_UINT, CF_TEXTURE_USAGE_DEPTH_STENCIL_TARGET_BIT)) {
      params.depth_stencil_target.pixel_format = CF_PIXEL_FORMAT_D24_UNORM_S8_UINT;
    }
		params.depth_stencil_target.usage = CF_TEXTURE_USAGE_DEPTH_STENCIL_TARGET_BIT;
	}
	return params;
}

CF_Canvas cf_make_canvas(CF_CanvasParams params)
{
	CF_CanvasInternal* canvas = (CF_CanvasInternal*)CF_CALLOC(sizeof(CF_CanvasInternal));
	if (params.target.width > 0 && params.target.height > 0) {
		canvas->w = params.target.width;
		canvas->h = params.target.height;
		canvas->cf_texture = cf_make_texture(params.target);
		if (canvas->cf_texture.id) {
			canvas->texture = ((CF_TextureInternal*)canvas->cf_texture.id)->tex;
			canvas->sampler = ((CF_TextureInternal*)canvas->cf_texture.id)->sampler;
		}
		if (params.depth_stencil_enable) {
			canvas->cf_depth_stencil = cf_make_texture(params.depth_stencil_target);
			if (canvas->cf_depth_stencil.id) {
				canvas->depth_stencil = ((CF_TextureInternal*)canvas->cf_depth_stencil.id)->tex;
			}
		} else {
			canvas->cf_depth_stencil = { 0 };
		}
	} else {
		return { 0 };
	}
	CF_Canvas result;
	result.id = (uint64_t)canvas;
	return result;
}

void cf_destroy_canvas(CF_Canvas canvas_handle)
{
	CF_CanvasInternal* canvas = (CF_CanvasInternal*)canvas_handle.id;
	cf_destroy_texture(canvas->cf_texture);
	if (canvas->depth_stencil) cf_destroy_texture(canvas->cf_depth_stencil);
	CF_FREE(canvas);
}

CF_Texture cf_canvas_get_target(CF_Canvas canvas_handle)
{
	CF_CanvasInternal* canvas = (CF_CanvasInternal*)canvas_handle.id;
	return canvas->cf_texture;
}

CF_Texture cf_canvas_get_depth_stencil_target(CF_Canvas canvas_handle)
{
	CF_CanvasInternal* canvas = (CF_CanvasInternal*)canvas_handle.id;
	return canvas->cf_depth_stencil;
}

CF_Mesh cf_make_mesh(int vertex_buffer_size, const CF_VertexAttribute* attributes, int attribute_count, int vertex_stride)
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

void cf_mesh_set_index_buffer(CF_Mesh mesh_handle, int index_buffer_size_in_bytes, int index_bit_count)
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

void cf_mesh_set_instance_buffer(CF_Mesh mesh_handle, int instance_buffer_size_in_bytes, int instance_stride)
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

void cf_destroy_mesh(CF_Mesh mesh_handle)
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

void cf_mesh_update_vertex_data(CF_Mesh mesh_handle, void* data, int count)
{
	CF_MeshInternal* mesh = (CF_MeshInternal*)mesh_handle.id;
	CF_ASSERT(mesh->attribute_count);
	s_update_buffer(&mesh->vertices, count, data, count * mesh->vertices.stride, SDL_GPU_BUFFERUSAGE_VERTEX);
}

void cf_mesh_update_index_data(CF_Mesh mesh_handle, void* data, int count)
{
	CF_MeshInternal* mesh = (CF_MeshInternal*)mesh_handle.id;
	s_update_buffer(&mesh->indices, count, data, count * mesh->indices.stride, SDL_GPU_BUFFERUSAGE_INDEX);
}

void cf_mesh_update_instance_data(CF_Mesh mesh_handle, void* data, int count)
{
	CF_MeshInternal* mesh = (CF_MeshInternal*)mesh_handle.id;
	s_update_buffer(&mesh->instances, count, data, count * mesh->instances.stride, SDL_GPU_BUFFERUSAGE_VERTEX);
}

CF_RenderState cf_render_state_defaults()
{
	CF_RenderState state;
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
	return state;
}

CF_Material cf_make_material()
{
	CF_MaterialInternal* material = CF_NEW(CF_MaterialInternal);
	material->uniform_arena = cf_make_arena(4, 1024);
	material->block_arena = cf_make_arena(4, 1024);
	material->state = cf_render_state_defaults();
	CF_Material result = { (uint64_t)material };
	return result;
}

void cf_destroy_material(CF_Material material_handle)
{
	CF_MaterialInternal* material = (CF_MaterialInternal*)material_handle.id;
	cf_arena_reset(&material->uniform_arena);
	cf_arena_reset(&material->block_arena);
	material->~CF_MaterialInternal();
	CF_FREE(material);
}

void cf_material_set_render_state(CF_Material material_handle, CF_RenderState render_state)
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

void cf_material_set_texture_vs(CF_Material material_handle, const char* name, CF_Texture texture)
{
	CF_MaterialInternal* material = (CF_MaterialInternal*)material_handle.id;
	name = sintern(name);
	s_material_set_texture(material, &material->vs, name, texture);
}

void cf_material_set_texture_fs(CF_Material material_handle, const char* name, CF_Texture texture)
{
	CF_MaterialInternal* material = (CF_MaterialInternal*)material_handle.id;
	name = sintern(name);
	s_material_set_texture(material, &material->fs, name, texture);
}

void cf_material_clear_textures(CF_Material material_handle)
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

void cf_material_set_uniform_vs(CF_Material material_handle, const char* name, void* data, CF_UniformType type, int array_length)
{
	CF_MaterialInternal* material = (CF_MaterialInternal*)material_handle.id;
	name = sintern(name);
	s_material_set_uniform(&material->uniform_arena, &material->vs, sintern("uniform_block"), name, data, type, array_length);
}

void cf_material_set_uniform_vs_internal(CF_Material material_handle, const char* block_name, const char* name, void* data, CF_UniformType type, int array_length)
{
	CF_MaterialInternal* material = (CF_MaterialInternal*)material_handle.id;
	name = sintern(name);
	s_material_set_uniform(&material->uniform_arena, &material->vs, sintern(block_name), name, data, type, array_length);
}

void cf_material_set_uniform_fs(CF_Material material_handle, const char* name, void* data, CF_UniformType type, int array_length)
{
	CF_MaterialInternal* material = (CF_MaterialInternal*)material_handle.id;
	name = sintern(name);
	s_material_set_uniform(&material->uniform_arena, &material->fs, sintern("uniform_block"), name, data, type, array_length);
}

void cf_material_set_uniform_fs_internal(CF_Material material_handle, const char* block_name, const char* name, void* data, CF_UniformType type, int array_length)
{
	CF_MaterialInternal* material = (CF_MaterialInternal*)material_handle.id;
	name = sintern(name);
	s_material_set_uniform(&material->uniform_arena, &material->fs, sintern(block_name), name, data, type, array_length);
}

void cf_material_clear_uniforms(CF_Material material_handle)
{
	CF_MaterialInternal* material = (CF_MaterialInternal*)material_handle.id;
	arena_reset(&material->uniform_arena);
	material->vs.uniforms.clear();
	material->fs.uniforms.clear();
}

void cf_clear_color(float red, float green, float blue, float alpha)
{
	app->clear_color = make_color(red, green, blue, alpha);
}

void cf_clear_depth_stencil(float depth, uint32_t stencil)
{
	app->clear_depth = depth;
	app->clear_stencil = stencil;
}

void cf_apply_canvas(CF_Canvas canvas_handle, bool clear)
{
	CF_CanvasInternal* canvas = (CF_CanvasInternal*)canvas_handle.id;
	CF_ASSERT(canvas);
	s_canvas = canvas;
	s_canvas->clear = clear;
}

void cf_apply_viewport(int x, int y, int w, int h)
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

void cf_apply_scissor(int x, int y, int w, int h)
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

void cf_apply_stencil_reference(int reference)
{
  CF_ASSERT(s_canvas);
  CF_ASSERT(s_canvas->pass);
  SDL_SetGPUStencilReference(s_canvas->pass, reference);
}

void cf_apply_blend_constants(float r, float g, float b, float a)
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

void cf_apply_mesh(CF_Mesh mesh_handle)
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
	for (int block_index = 0; block_index < shd->uniform_block_count; ++block_index) {
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
	SDL_GPUColorTargetDescription color_info;
	CF_MEMSET(&color_info, 0, sizeof(color_info));
	CF_ASSERT(s_canvas->texture);
	color_info.format = ((CF_TextureInternal*)s_canvas->cf_texture.id)->format;
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
	pip_info.target_info.num_color_targets = 1;
	pip_info.target_info.color_target_descriptions = &color_info;
	pip_info.vertex_shader = shader->vs;
	pip_info.fragment_shader = shader->fs;
	pip_info.target_info.has_depth_stencil_target = state->depth_write_enabled;
	if (s_canvas->cf_depth_stencil.id) {
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
		vertex_buffer_descriptions[vertex_buffer_descriptions_count].instance_step_rate = 1;
		vertex_buffer_descriptions_count++;
	}
	pip_info.vertex_input_state.num_vertex_buffers = vertex_buffer_descriptions_count;
	pip_info.vertex_input_state.vertex_buffer_descriptions = vertex_buffer_descriptions;

	pip_info.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
	pip_info.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_FILL;
	pip_info.rasterizer_state.cull_mode = s_wrap(state->cull_mode);
	pip_info.rasterizer_state.front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE;
	pip_info.rasterizer_state.enable_depth_bias = false;
	pip_info.rasterizer_state.depth_bias_constant_factor = 0;
	pip_info.rasterizer_state.depth_bias_clamp = 0;
	pip_info.rasterizer_state.depth_bias_slope_factor = 0;
	pip_info.multisample_state.sample_count = SDL_GPU_SAMPLECOUNT_1;
	pip_info.multisample_state.sample_mask = 0xFFFF;

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

void cf_apply_shader(CF_Shader shader_handle, CF_Material material_handle)
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
		if (pip_cache.material == material && pip_cache.mesh == mesh) {
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
		shader->pip_cache.add({ material, pip, mesh });
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
	pass_color_info.store_op = SDL_GPU_STOREOP_STORE;
	pass_color_info.cycle = s_canvas->clear ? true : false;
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
	SDL_GPURenderPass* pass = SDL_BeginGPURenderPass(cmd, &pass_color_info, 1, s_canvas->depth_stencil ? &pass_depth_stencil_info : NULL);
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
				sampler_bindings[found_image_count].sampler = ((CF_TextureInternal*)material->fs.textures[i].handle.id)->sampler;
				sampler_bindings[found_image_count].texture = ((CF_TextureInternal*)material->fs.textures[i].handle.id)->tex;
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

void cf_draw_elements()
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

void cf_commit()
{
	SDL_EndGPURenderPass(s_canvas->pass);
}
