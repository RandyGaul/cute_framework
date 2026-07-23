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

#include "cute/ckit.h"
#define CUTE_SPIRV_IMPLEMENTATION
#include "cute/cute_spirv.h"

#include "cute_shader.h"

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
	char* content = (char*)malloc(size + 1);
	size_t num_read = fread(content, 1, size, file);
	fclose(file);
	if (num_read != (size_t)size) {
		free(content);
		return NULL;
	}
	content[size] = '\0';
	*len = (size_t)size;
	return content;
}

static void s_libc_free_file_content(char* content, void* context)
{
	(void)context;
	free(content);
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
// Failure helper: matches the "header\ndetail" message shape of the glslang path.

static CF_ShaderCompilerResult s_failure(const char* header, const char* detail)
{
	char* msg = sfmake("%s\n%s", header, detail ? detail : "");
	CF_ShaderCompilerResult result;
	memset(&result, 0, sizeof(result));
	result.success = false;
	result.error_message = strdup(msg);
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
		out = strdup(r.preprocessed);
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
	// GLSL 300 es transpilation (skipped for compute -- GLES 3.0 has no compute support).
	opts.emit_glsl300 = stage != CUTE_SHADER_STAGE_COMPUTE && !config.skip_glsl300;
	// HLSL SM 5.1 transpilation, for D3D12 via the system FXC.
	opts.emit_hlsl = !config.skip_hlsl;
	// MSL transpilation, for Metal (the OS compiles the source at runtime).
	opts.emit_msl = !config.skip_msl;

	CSPV_Result r = cspv_compile_ex(source, cspv_stage, &opts);

	afree(defines);
	for (int i = 0; i < (int)asize(include_ctx.owned); ++i) sfree(include_ctx.owned[i]);
	afree(include_ctx.owned);

	if (!r.success) {
		CF_ShaderCompilerResult result = s_failure("Shader compilation failed", r.error_message);
		cspv_free(&r);
		return result;
	}

	// Bytecode blob (plain malloc; freed with free() by cute_shader_free_result).
	size_t bytecode_size = r.word_count * sizeof(uint32_t);
	void* bytecode = malloc(bytecode_size);
	memcpy(bytecode, r.spirv, bytecode_size);

	// GLSL 300 es source, from cute_spirv's transpiler backend (requested via
	// opts.emit_glsl300 above; NULL when skipped).
	char* glsl300_src = NULL;
	size_t glsl300_src_size = 0;
	if (r.glsl300) {
		glsl300_src_size = strlen(r.glsl300);
		glsl300_src = (char*)malloc(glsl300_src_size + 1);
		memcpy(glsl300_src, r.glsl300, glsl300_src_size + 1);
	}

	// HLSL source likewise.
	char* hlsl_src = NULL;
	size_t hlsl_src_size = 0;
	if (r.hlsl) {
		hlsl_src_size = strlen(r.hlsl);
		hlsl_src = (char*)malloc(hlsl_src_size + 1);
		memcpy(hlsl_src, r.hlsl, hlsl_src_size + 1);
	}

	// MSL source likewise.
	char* msl_src = NULL;
	size_t msl_src_size = 0;
	if (r.msl) {
		msl_src_size = strlen(r.msl);
		msl_src = (char*)malloc(msl_src_size + 1);
		memcpy(msl_src, r.msl, msl_src_size + 1);
	}

	// Reflection: map CSPV_Reflection to CF_ShaderInfo. Arrays are malloc'd (freed by
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
		image_names = (const char**)malloc(sizeof(char*) * num_images);
		image_binding_slots = (int*)malloc(sizeof(int) * num_images);
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
		uniforms = (CF_ShaderUniformInfo*)malloc(sizeof(CF_ShaderUniformInfo) * num_uniforms);
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
		uniform_members = (CF_ShaderUniformMemberInfo*)malloc(sizeof(CF_ShaderUniformMemberInfo) * num_uniform_members);
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
		inputs = (CF_ShaderInputInfo*)malloc(sizeof(CF_ShaderInputInfo) * num_inputs);
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
		preprocessed_copy = (char*)malloc(preprocessed_size + 1);
		memcpy(preprocessed_copy, r.preprocessed, preprocessed_size + 1);
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
	result.preprocessed_source = preprocessed_copy;
	result.preprocessed_source_size = preprocessed_size;
	return result;
}

void cute_shader_free_result(CF_ShaderCompilerResult result)
{
	// Reflection names are interned strings (immortal) -- only the arrays are freed.
	CF_ShaderInfo* shader_info = &result.bytecode.shader_info;
	free(shader_info->inputs);
	free(shader_info->uniform_members);
	free(shader_info->uniforms);
	free(shader_info->image_names);
	free(shader_info->image_binding_slots);

	free((void*)result.bytecode.glsl300_src);
	free((void*)result.bytecode.hlsl_src);
	free((void*)result.bytecode.msl_src);
	free((void*)result.bytecode.content);
	free((char*)result.preprocessed_source);
	free((char*)result.error_message);
}
