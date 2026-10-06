/*
	Cute Framework
	Copyright (C) 2026 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info

	Implements the cute_shader.h API on top of cute_spirv.h, CF's own GLSL -> SPIR-V
	compiler (see docs/topics/glsl_support.md for the supported subset). The GLSL
	ES 300 output consumed by the GLES/WebGL2 backend also comes from cute_spirv's
	transpiler backend -- fully dependency-free.

	ckit's implementation is expected to come from another TU (cute_ckit.cpp inside
	CF, or a dedicated TU for standalone tools like cute-shaderc).
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <cute_alloc.h>
#include "cute/ckit.h"
#define CUTE_SPIRV_IMPLEMENTATION
#include "cute/cute_spirv.h"

#include "cute_shader.h"

static char* s_cf_strdup(const char* s)
{
	size_t len = strlen(s) + 1;
	char* copy = (char*)cf_alloc(len);
	memcpy(copy, s, len);
	return copy;
}

// Matches cspv_wg_format: unknown formats emit as rgba8unorm. CF_PixelFormat has no r32ui.
static CF_PixelFormat s_pixel_format_from_spirv(int image_format)
{
	switch (image_format) {
	case 1:  return CF_PIXEL_FORMAT_R32G32B32A32_FLOAT;
	case 2:  return CF_PIXEL_FORMAT_R16G16B16A16_FLOAT;
	case 3:  return CF_PIXEL_FORMAT_R32_FLOAT;
	case 6:  return CF_PIXEL_FORMAT_R32G32_FLOAT;
	case 7:  return CF_PIXEL_FORMAT_R16G16_FLOAT;
	case 9:  return CF_PIXEL_FORMAT_R16_FLOAT;
	case 32: return CF_PIXEL_FORMAT_R8G8B8A8_UINT;
	case 33: return CF_PIXEL_FORMAT_INVALID;
	default: return CF_PIXEL_FORMAT_R8G8B8A8_UNORM;
	}
}

//--------------------------------------------------------------------------------------------------
// Default filesystem VFS (mirrors cute_shader.cpp's libc vfs).

static char* s_libc_read_file_content(const char* path, size_t* len, void* context)
{
	(void)context;
	FILE* file = fopen(path, "rb");
	if (!file) return NULL;
	fseek(file, 0, SEEK_END);
	long size = ftell(file);
	fseek(file, 0, SEEK_SET);
	char* content = (char*)cf_alloc(size + 1);
	size_t num_read = fread(content, 1, size, file);
	fclose(file);
	if (num_read != (size_t)size) {
		cf_free(content);
		return NULL;
	}
	content[size] = '\0';
	*len = (size_t)size;
	return content;
}

static void s_libc_free_file_content(char* content, void* context)
{
	(void)context;
	cf_free(content);
}

static CF_ShaderCompilerVfs s_libc_vfs = {
	s_libc_read_file_content,
	s_libc_free_file_content,
	NULL,
};

//--------------------------------------------------------------------------------------------------
// Include resolution: builtin includes first, then include dirs through the VFS.
// VFS content is copied into ckit strings owned for the duration of the compile,
// since VFS buffers are not guaranteed to be null-terminated.

typedef struct CF_CspvIncludeCtx
{
	const CF_ShaderCompilerConfig* config;
	CF_ShaderCompilerVfs* vfs;
	CK_DYNA char** owned; // ckit strings freed after the compile.
} CF_CspvIncludeCtx;

static const char* s_display_name(const char* path, void* user)
{
	CF_CspvIncludeCtx* ctx = (CF_CspvIncludeCtx*)user;
	if (ctx->config->shader_stub_display_name && sequ(path, "shader_stub.shd")) {
		return ctx->config->shader_stub_display_name;
	}
	return NULL;
}

static const char* s_resolve_include(const char* path, void* user)
{
	CF_CspvIncludeCtx* ctx = (CF_CspvIncludeCtx*)user;
	const CF_ShaderCompilerConfig* config = ctx->config;

	for (int i = 0; i < config->num_builtin_includes; ++i) {
		if (sequ(config->builtin_includes[i].name, path)) {
			return config->builtin_includes[i].content;
		}
	}

	for (int i = 0; i < config->num_include_dirs; ++i) {
		char* full_path = sfmake("%s/%s", config->include_dirs[i], path);
		size_t len = 0;
		char* content = ctx->vfs->read_file_content(full_path, &len, ctx->vfs->context);
		sfree(full_path);
		if (content) {
			char* copy = NULL;
			sappend_range(copy, content, content + len);
			ctx->vfs->free_file_content(content, ctx->vfs->context);
			apush(ctx->owned, copy);
			return copy;
		}
	}

	return NULL;
}

//--------------------------------------------------------------------------------------------------
// Failure helper: error messages take a "header\ndetail" shape.

static CF_ShaderCompilerResult s_failure(const char* header, const char* detail)
{
	char* msg = sfmake("%s\n%s", header, detail ? detail : "");
	CF_ShaderCompilerResult result;
	memset(&result, 0, sizeof(result));
	result.success = false;
	result.error_message = s_cf_strdup(msg);
	sfree(msg);
	return result;
}

//--------------------------------------------------------------------------------------------------

char* cute_shader_preprocess(const char* source, CF_ShaderCompilerConfig config)
{
	CK_DYNA CSPV_Define* defines = NULL;
	for (int i = 0; i < config.num_builtin_defines; ++i) {
		CSPV_Define d;
		d.name = config.builtin_defines[i].name;
		d.value = config.builtin_defines[i].value;
		apush(defines, d);
	}

	CF_CspvIncludeCtx include_ctx;
	memset(&include_ctx, 0, sizeof(include_ctx));
	include_ctx.config = &config;
	include_ctx.vfs = config.vfs ? config.vfs : &s_libc_vfs;

	CSPV_Options opts;
	memset(&opts, 0, sizeof(opts));
	opts.num_defines = (int)asize(defines);
	opts.defines = defines;
	opts.include_resolve = s_resolve_include;
	opts.user = &include_ctx;
	opts.preprocess_only = true;

	CSPV_Result r = cspv_compile_ex(source, CSPV_STAGE_FRAGMENT, &opts);

	afree(defines);
	for (int i = 0; i < (int)asize(include_ctx.owned); ++i) sfree(include_ctx.owned[i]);
	afree(include_ctx.owned);

	char* out = NULL;
	if (r.success && r.preprocessed) {
		out = s_cf_strdup(r.preprocessed);
	}
	cspv_free(&r);
	return out;
}

CF_ShaderCompilerResult cute_shader_compile(const char* source, CF_ShaderCompilerStage stage, CF_ShaderCompilerConfig config)
{
	CSPV_Stage cspv_stage = CSPV_STAGE_VERTEX;
	switch (stage) {
	case CUTE_SHADER_STAGE_VERTEX: cspv_stage = CSPV_STAGE_VERTEX; break;
	case CUTE_SHADER_STAGE_FRAGMENT: cspv_stage = CSPV_STAGE_FRAGMENT; break;
	case CUTE_SHADER_STAGE_COMPUTE: cspv_stage = CSPV_STAGE_COMPUTE; break;
	}

	// Defines.
	CK_DYNA CSPV_Define* defines = NULL;
	for (int i = 0; i < config.num_builtin_defines; ++i) {
		CSPV_Define d;
		d.name = config.builtin_defines[i].name;
		d.value = config.builtin_defines[i].value;
		apush(defines, d);
	}

	CF_CspvIncludeCtx include_ctx;
	memset(&include_ctx, 0, sizeof(include_ctx));
	include_ctx.config = &config;
	include_ctx.vfs = config.vfs ? config.vfs : &s_libc_vfs;

	CSPV_Options opts;
	memset(&opts, 0, sizeof(opts));
	opts.num_defines = (int)asize(defines);
	opts.defines = defines;
	opts.include_resolve = s_resolve_include;
	opts.user = &include_ctx;
	opts.return_preprocessed = config.return_preprocessed_source;
	opts.display_name = s_display_name;
	// GLSL 300 es transpilation. A compute shader becomes the GLES backend's capture-pass
	// fragment shader (GLES3 has no compute; see cf_gles_dispatch_compute).
	opts.emit_glsl300 = !config.skip_glsl300;
	// HLSL SM 5.1 transpilation, for D3D12 via the system FXC.
	opts.emit_hlsl = !config.skip_hlsl;
	// MSL transpilation, for Metal (the OS compiles the source at runtime).
	opts.emit_msl = !config.skip_msl;
	// WGSL transpilation, for WebGPU.
	opts.emit_wgsl = !config.skip_wgsl;

	CSPV_Result r = cspv_compile_ex(source, cspv_stage, &opts);

	afree(defines);
	for (int i = 0; i < (int)asize(include_ctx.owned); ++i) sfree(include_ctx.owned[i]);
	afree(include_ctx.owned);

	if (!r.success) {
		CF_ShaderCompilerResult result = s_failure("Shader compilation failed", r.error_message);
		cspv_free(&r);
		return result;
	}

	// Bytecode blob (cf_alloc'd; freed with cf_free() by cute_shader_free_result).
	size_t bytecode_size = r.word_count * sizeof(uint32_t);
	void* bytecode = cf_alloc(bytecode_size);
	memcpy(bytecode, r.spirv, bytecode_size);

	// GLSL 300 es source, from cute_spirv's transpiler backend (requested via
	// opts.emit_glsl300 above; NULL when skipped).
	char* glsl300_src = NULL;
	size_t glsl300_src_size = 0;
	if (r.glsl300) {
		glsl300_src_size = strlen(r.glsl300);
		glsl300_src = (char*)cf_alloc(glsl300_src_size + 1);
		memcpy(glsl300_src, r.glsl300, glsl300_src_size + 1);
	}

	// HLSL source likewise.
	char* hlsl_src = NULL;
	size_t hlsl_src_size = 0;
	if (r.hlsl) {
		hlsl_src_size = strlen(r.hlsl);
		hlsl_src = (char*)cf_alloc(hlsl_src_size + 1);
		memcpy(hlsl_src, r.hlsl, hlsl_src_size + 1);
	}

	// MSL source likewise.
	char* msl_src = NULL;
	size_t msl_src_size = 0;
	if (r.msl) {
		msl_src_size = strlen(r.msl);
		msl_src = (char*)cf_alloc(msl_src_size + 1);
		memcpy(msl_src, r.msl, msl_src_size + 1);
	}

	// WGSL source likewise.
	char* wgsl_src = NULL;
	size_t wgsl_src_size = 0;
	if (r.wgsl) {
		wgsl_src_size = strlen(r.wgsl);
		wgsl_src = (char*)cf_alloc(wgsl_src_size + 1);
		memcpy(wgsl_src, r.wgsl, wgsl_src_size + 1);
	}

	// Reflection: map CSPV_Reflection to CF_ShaderInfo. Arrays are cf_alloc'd (freed by
	// cute_shader_free_result); names are interned strings from the compiler, which
	// are immortal -- no copies, and free_result must not free them.
	CSPV_Reflection* rf = &r.reflection;

	int num_samplers = (int)asize(rf->samplers);
	int num_storage_textures = 0;
	int num_readwrite_storage_textures = 0;
	for (int i = 0; i < (int)asize(rf->storage_images); ++i) {
		if (rf->storage_images[i].readonly) ++num_storage_textures;
		else ++num_readwrite_storage_textures;
	}
	int num_storage_buffers = 0;
	int num_readwrite_storage_buffers = 0;
	for (int i = 0; i < (int)asize(rf->storage_buffers); ++i) {
		if (rf->storage_buffers[i].readonly) ++num_storage_buffers;
		else ++num_readwrite_storage_buffers;
	}

	// Combined samplers, sorted by binding so array index matches the SDL_GPU slot.
	int num_images = num_samplers;
	const char** image_names = NULL;
	int* image_binding_slots = NULL;
	if (num_images > 0) {
		image_names = (const char**)cf_alloc(sizeof(char*) * num_images);
		image_binding_slots = (int*)cf_alloc(sizeof(int) * num_images);
		for (int i = 0; i < num_images; ++i) {
			image_names[i] = rf->samplers[i].name;
			image_binding_slots[i] = rf->samplers[i].binding;
		}
		for (int i = 0; i < num_images - 1; ++i) {
			for (int j = i + 1; j < num_images; ++j) {
				if (image_binding_slots[j] < image_binding_slots[i]) {
					const char* tn = image_names[i]; image_names[i] = image_names[j]; image_names[j] = tn;
					int tb = image_binding_slots[i]; image_binding_slots[i] = image_binding_slots[j]; image_binding_slots[j] = tb;
				}
			}
		}
	}

	int num_uniforms = (int)asize(rf->uniform_blocks);
	CF_ShaderUniformInfo* uniforms = NULL;
	if (num_uniforms > 0) {
		uniforms = (CF_ShaderUniformInfo*)cf_alloc(sizeof(CF_ShaderUniformInfo) * num_uniforms);
		for (int i = 0; i < num_uniforms; ++i) {
			uniforms[i].block_name = rf->uniform_blocks[i].name;
			uniforms[i].block_index = rf->uniform_blocks[i].binding;
			uniforms[i].block_size = rf->uniform_blocks[i].size;
			uniforms[i].num_members = rf->uniform_blocks[i].num_members;
		}
	}

	int num_uniform_members = (int)asize(rf->uniform_members);
	CF_ShaderUniformMemberInfo* uniform_members = NULL;
	if (num_uniform_members > 0) {
		uniform_members = (CF_ShaderUniformMemberInfo*)cf_alloc(sizeof(CF_ShaderUniformMemberInfo) * num_uniform_members);
		for (int i = 0; i < num_uniform_members; ++i) {
			uniform_members[i].name = rf->uniform_members[i].name;
			uniform_members[i].type = (CF_ShaderInfoDataType)rf->uniform_members[i].type;
			uniform_members[i].offset = rf->uniform_members[i].offset;
			uniform_members[i].array_length = rf->uniform_members[i].array_length;
		}
	}

	int num_inputs = (int)asize(rf->inputs);
	CF_ShaderInputInfo* inputs = NULL;
	if (num_inputs > 0) {
		inputs = (CF_ShaderInputInfo*)cf_alloc(sizeof(CF_ShaderInputInfo) * num_inputs);
		for (int i = 0; i < num_inputs; ++i) {
			inputs[i].name = rf->inputs[i].name;
			inputs[i].location = rf->inputs[i].location;
			inputs[i].format = (CF_ShaderInfoDataType)rf->inputs[i].type;
		}
	}

	char* preprocessed_copy = NULL;
	size_t preprocessed_size = 0;
	if (config.return_preprocessed_source && r.preprocessed) {
		preprocessed_size = slen(r.preprocessed);
		preprocessed_copy = (char*)cf_alloc(preprocessed_size + 1);
		memcpy(preprocessed_copy, r.preprocessed, preprocessed_size + 1);
	}

	// Storage images and buffers by name (the GLES backend binds them by name), and the
	// compute emulation's write sites.
	int num_storage_image_infos = (int)asize(rf->storage_images);
	CF_ShaderResourceInfo* storage_image_infos = NULL;
	if (num_storage_image_infos > 0) {
		storage_image_infos = (CF_ShaderResourceInfo*)cf_alloc(sizeof(CF_ShaderResourceInfo) * num_storage_image_infos);
		for (int i = 0; i < num_storage_image_infos; ++i) {
			storage_image_infos[i].name = rf->storage_images[i].name;
			storage_image_infos[i].set = rf->storage_images[i].set;
			storage_image_infos[i].binding = rf->storage_images[i].binding;
			storage_image_infos[i].readonly = rf->storage_images[i].readonly;
		}
	}
	int num_storage_buffer_infos = (int)asize(rf->storage_buffers);
	CF_ShaderResourceInfo* storage_buffer_infos = NULL;
	if (num_storage_buffer_infos > 0) {
		storage_buffer_infos = (CF_ShaderResourceInfo*)cf_alloc(sizeof(CF_ShaderResourceInfo) * num_storage_buffer_infos);
		for (int i = 0; i < num_storage_buffer_infos; ++i) {
			storage_buffer_infos[i].name = rf->storage_buffers[i].name;
			storage_buffer_infos[i].set = rf->storage_buffers[i].set;
			storage_buffer_infos[i].binding = rf->storage_buffers[i].binding;
			storage_buffer_infos[i].readonly = rf->storage_buffers[i].readonly;
		}
	}
	int num_write_sites = 0;
	CF_ShaderWriteSite* write_sites = NULL;
	num_write_sites = r.glsl300 ? (int)asize(rf->write_sites) : 0;
	if (num_write_sites > 0) {
		write_sites = (CF_ShaderWriteSite*)cf_alloc(sizeof(CF_ShaderWriteSite) * num_write_sites);
		for (int i = 0; i < num_write_sites; ++i) {
			write_sites[i].kind = rf->write_sites[i].kind == CSPV_WRITE_IMAGE ? CF_SHADER_WRITE_KIND_IMAGE : CF_SHADER_WRITE_KIND_BUFFER;
			write_sites[i].name = rf->write_sites[i].name;
			write_sites[i].set = rf->write_sites[i].set;
			write_sites[i].binding = rf->write_sites[i].binding;
			write_sites[i].words = rf->write_sites[i].words;
		}
	}

	// The WGSL bind groups (see CF_ShaderWgslBinding / CF_ShaderWgslSplit).
	int num_wgsl_bindings = r.wgsl ? (int)asize(rf->wgsl_bindings) : 0;
	CF_ShaderWgslBinding* wgsl_bindings = NULL;
	if (num_wgsl_bindings > 0) {
		wgsl_bindings = (CF_ShaderWgslBinding*)cf_alloc(sizeof(CF_ShaderWgslBinding) * num_wgsl_bindings);
		for (int i = 0; i < num_wgsl_bindings; ++i) {
			const CSPV_WgslBinding* b = rf->wgsl_bindings + i;
			wgsl_bindings[i].name = b->name;
			wgsl_bindings[i].set = b->set;
			wgsl_bindings[i].slot = b->slot;
			wgsl_bindings[i].binding = b->binding;
			switch (b->kind) {
			case CSPV_WGSL_SAMPLED_TEXTURE: wgsl_bindings[i].kind = CF_SHADER_WGSL_BINDING_KIND_SAMPLED_TEXTURE; break;
			case CSPV_WGSL_SAMPLER: wgsl_bindings[i].kind = CF_SHADER_WGSL_BINDING_KIND_SAMPLER; break;
			case CSPV_WGSL_STORAGE_TEXTURE: wgsl_bindings[i].kind = CF_SHADER_WGSL_BINDING_KIND_STORAGE_TEXTURE; break;
			case CSPV_WGSL_STORAGE_BUFFER: wgsl_bindings[i].kind = CF_SHADER_WGSL_BINDING_KIND_STORAGE_BUFFER; break;
			case CSPV_WGSL_UNIFORM_BUFFER: wgsl_bindings[i].kind = CF_SHADER_WGSL_BINDING_KIND_UNIFORM_BUFFER; break;
			default: wgsl_bindings[i].kind = CF_SHADER_WGSL_BINDING_KIND_SPLIT_LOAD_TEXTURE; break;
			}
			switch (b->dim) {
			case CSPV_WGSL_DIM_CUBE: wgsl_bindings[i].dimension = CF_TEXTURE_TYPE_CUBE; break;
			case CSPV_WGSL_DIM_3D: wgsl_bindings[i].dimension = CF_TEXTURE_TYPE_3D; break;
			case CSPV_WGSL_DIM_2D_ARRAY: wgsl_bindings[i].dimension = CF_TEXTURE_TYPE_2D_ARRAY; break;
			default: wgsl_bindings[i].dimension = CF_TEXTURE_TYPE_2D; break;
			}
			switch (b->sample_type) {
			case CSPV_WGSL_SAMPLE_UINT: wgsl_bindings[i].sample_type = CF_SHADER_WGSL_SAMPLE_TYPE_UINT; break;
			case CSPV_WGSL_SAMPLE_DEPTH: wgsl_bindings[i].sample_type = CF_SHADER_WGSL_SAMPLE_TYPE_DEPTH; break;
			default: wgsl_bindings[i].sample_type = CF_SHADER_WGSL_SAMPLE_TYPE_FLOAT; break;
			}
			wgsl_bindings[i].multisampled = false;
			bool texel_format = b->kind == CSPV_WGSL_STORAGE_TEXTURE || b->kind == CSPV_WGSL_SPLIT_LOAD_TEXTURE;
			wgsl_bindings[i].storage_format = texel_format ? s_pixel_format_from_spirv(b->image_format) : CF_PIXEL_FORMAT_INVALID;
			switch (b->access) {
			case CSPV_WGSL_ACCESS_WRITE: wgsl_bindings[i].storage_access = CF_SHADER_WGSL_ACCESS_WRITE; break;
			case CSPV_WGSL_ACCESS_READ_WRITE: wgsl_bindings[i].storage_access = CF_SHADER_WGSL_ACCESS_READ_WRITE; break;
			default: wgsl_bindings[i].storage_access = CF_SHADER_WGSL_ACCESS_READ; break;
			}
			wgsl_bindings[i].comparison = b->comparison;
		}
	}
	int num_wgsl_splits = r.wgsl ? (int)asize(rf->wgsl_splits) : 0;
	CF_ShaderWgslSplit* wgsl_splits = NULL;
	if (num_wgsl_splits > 0) {
		wgsl_splits = (CF_ShaderWgslSplit*)cf_alloc(sizeof(CF_ShaderWgslSplit) * num_wgsl_splits);
		for (int i = 0; i < num_wgsl_splits; ++i) {
			wgsl_splits[i].name = rf->wgsl_splits[i].name;
			wgsl_splits[i].set = rf->wgsl_splits[i].set;
			wgsl_splits[i].store_binding = rf->wgsl_splits[i].store_binding;
			wgsl_splits[i].load_binding = rf->wgsl_splits[i].load_binding;
		}
	}

	// Captured before cspv_free wipes the result.
	int local_size[3] = { r.reflection.local_size[0], r.reflection.local_size[1], r.reflection.local_size[2] };

	cspv_free(&r);

	CF_ShaderCompilerResult result;
	memset(&result, 0, sizeof(result));
	result.success = true;
	result.bytecode.content = (uint8_t*)bytecode;
	result.bytecode.size = bytecode_size;
	result.bytecode.glsl300_src = glsl300_src;
	result.bytecode.glsl300_src_size = glsl300_src_size;
	result.bytecode.hlsl_src = hlsl_src;
	result.bytecode.hlsl_src_size = hlsl_src_size;
	result.bytecode.msl_src = msl_src;
	result.bytecode.msl_src_size = msl_src_size;
	result.bytecode.shader_info.local_size[0] = local_size[0];
	result.bytecode.shader_info.local_size[1] = local_size[1];
	result.bytecode.shader_info.local_size[2] = local_size[2];
	result.bytecode.shader_info.num_samplers = num_samplers;
	result.bytecode.shader_info.num_storage_textures = num_storage_textures;
	result.bytecode.shader_info.num_storage_buffers = num_storage_buffers;
	result.bytecode.shader_info.num_readwrite_storage_textures = num_readwrite_storage_textures;
	result.bytecode.shader_info.num_readwrite_storage_buffers = num_readwrite_storage_buffers;
	result.bytecode.shader_info.num_images = num_images;
	result.bytecode.shader_info.image_names = image_names;
	result.bytecode.shader_info.image_binding_slots = image_binding_slots;
	result.bytecode.shader_info.num_uniforms = num_uniforms;
	result.bytecode.shader_info.uniforms = uniforms;
	result.bytecode.shader_info.num_uniform_members = num_uniform_members;
	result.bytecode.shader_info.uniform_members = uniform_members;
	result.bytecode.shader_info.num_inputs = num_inputs;
	result.bytecode.shader_info.inputs = inputs;
	result.bytecode.shader_info.num_storage_image_infos = num_storage_image_infos;
	result.bytecode.shader_info.storage_image_infos = storage_image_infos;
	result.bytecode.shader_info.num_storage_buffer_infos = num_storage_buffer_infos;
	result.bytecode.shader_info.storage_buffer_infos = storage_buffer_infos;
	result.bytecode.shader_info.num_write_sites = num_write_sites;
	result.bytecode.shader_info.write_sites = write_sites;
	result.bytecode.shader_info.num_wgsl_bindings = num_wgsl_bindings;
	result.bytecode.shader_info.wgsl_bindings = wgsl_bindings;
	result.bytecode.shader_info.num_wgsl_splits = num_wgsl_splits;
	result.bytecode.shader_info.wgsl_splits = wgsl_splits;
	result.bytecode.wgsl_src = wgsl_src;
	result.bytecode.wgsl_src_size = wgsl_src_size;
	result.preprocessed_source = preprocessed_copy;
	result.preprocessed_source_size = preprocessed_size;
	return result;
}

void cute_shader_free_result(CF_ShaderCompilerResult result)
{
	// Reflection names are interned strings (immortal) -- only the arrays are freed.
	CF_ShaderInfo* shader_info = &result.bytecode.shader_info;
	cf_free(shader_info->inputs);
	cf_free(shader_info->uniform_members);
	cf_free(shader_info->uniforms);
	cf_free(shader_info->image_names);
	cf_free(shader_info->image_binding_slots);
	cf_free(shader_info->storage_image_infos);
	cf_free(shader_info->storage_buffer_infos);
	cf_free(shader_info->write_sites);
	cf_free(shader_info->wgsl_bindings);
	cf_free(shader_info->wgsl_splits);

	cf_free((void*)result.bytecode.glsl300_src);
	cf_free((void*)result.bytecode.hlsl_src);
	cf_free((void*)result.bytecode.msl_src);
	cf_free((void*)result.bytecode.wgsl_src);
	cf_free((void*)result.bytecode.content);
	cf_free((char*)result.preprocessed_source);
	cf_free((char*)result.error_message);
}
