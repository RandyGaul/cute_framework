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

#include "cute_shader.h"
#include "builtin_shaders.h"
#include "data/builtin_shaders_bytecode.h"

#include <float.h>

using namespace Cute;

static void s_shader_directory_recursive(CF_Path path)
{
	Array<CF_Path> dir = CF_Directory::enumerate(app->shader_directory + path);
	for (int i = 0; i < dir.size(); ++i) {
		CF_Path p = app->shader_directory + path + dir[i];
		if (p.is_directory()) {
			s_shader_directory_recursive(path + dir[i]);
		} else {
			CF_Stat stat;
			fs_stat(p, &stat);
			String ext = p.ext();
			if (ext == ".vs" || ext == ".fs" || ext == ".shd" || ext == ".c_shd") {
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
			s_shader_watch_recursive(path + dir[i]);
		} else {
			CF_Stat stat;
			fs_stat(p, &stat);
			String ext = p.ext();
			if (ext == ".vs" || ext == ".fs" || ext == ".shd" || ext == ".c_shd") {
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

static CF_ShaderBytecode cf_compile_shader_to_bytecode_internal(const char* shader_src, CF_ShaderStage cf_stage, const char* user_shd)
{
#ifdef CF_RUNTIME_SHADER_COMPILATION
	CF_ShaderCompilerStage stage = CUTE_SHADER_STAGE_VERTEX;
	switch (cf_stage) {
	default: CF_ASSERT(false); break; // No valid stage provided.
	case CF_SHADER_STAGE_VERTEX: stage = CUTE_SHADER_STAGE_VERTEX; break;
	case CF_SHADER_STAGE_FRAGMENT: stage = CUTE_SHADER_STAGE_FRAGMENT; break;
	case CF_SHADER_STAGE_COMPUTE: stage = CUTE_SHADER_STAGE_COMPUTE; break;
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

CF_Shader cf_make_shader_from_source_internal(const char* vs_src, const char* fs_src, const char* user_shd)
{
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

void cf_load_internal_shaders()
{
#ifdef CF_RUNTIME_SHADER_COMPILATION
	cute_shader_init();

	// Compile built-in shaders.
	app->draw_shader = cf_make_shader_from_source_internal(s_draw_vs, s_draw_fs, NULL);
	app->blit_shader = cf_make_shader_from_source_internal(s_blit_vs, s_blit_fs, NULL);
#else
	app->draw_shader = cf_make_shader_from_bytecode(s_draw_vs_bytecode, s_draw_fs_bytecode);
	app->blit_shader = cf_make_shader_from_bytecode(s_blit_vs_bytecode, s_blit_fs_bytecode);
#endif
}

void cf_destroy_shader_internal(CF_Shader shader_handle);

void cf_unload_internal_shaders()
{
	cf_destroy_shader_internal(app->draw_shader);
	cf_destroy_shader_internal(app->blit_shader);
#ifdef CF_RUNTIME_SHADER_COMPILATION
	cute_shader_cleanup();
#endif
}

void cf_destroy_shader(CF_Shader shader_handle)
{
	// Draw shaders automatically have blit shaders generated, so clean that up as well,
	// if it exists. See `cf_make_draw_shader`.
	CF_Shader* blit = (CF_Shader*)s_draw->draw_shd_to_blit_shd.try_get(shader_handle.id);
	if (blit) {
		cf_destroy_shader(*blit);
		s_draw->draw_shd_to_blit_shd.remove(shader_handle.id);
	}

	cf_destroy_shader_internal(shader_handle);
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
	return cf_make_shader_from_source_internal(s_draw_vs, s_draw_fs, src);
}

CF_Shader cf_make_draw_shader_from_bytecode_internal(CF_ShaderBytecode bytecode)
{
	return cf_make_shader_from_bytecode(s_draw_vs_bytecode, bytecode);
}

CF_Shader cf_make_draw_blit_shader_from_source_internal(const char* src)
{
	return cf_make_shader_from_source_internal(s_blit_vs, s_blit_fs, src);
}

CF_Shader cf_make_draw_blit_shader_from_bytecode_internal(CF_ShaderBytecode bytecode)
{
	return cf_make_shader_from_bytecode(s_blit_vs_bytecode, bytecode);
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

//--------------------------------------------------------------------------------------------------
// Backend-agnostic functions

CF_BackendType cf_query_backend()
{
	return app->gfx_backend_type;
}

CF_TextureParams cf_texture_defaults(int w, int h)
{
	CF_TextureParams params = { };
	params.pixel_format = CF_PIXEL_FORMAT_R8G8B8A8_UNORM;
	params.usage = CF_TEXTURE_USAGE_SAMPLER_BIT;
	params.filter = CF_FILTER_LINEAR;
	params.wrap_u = CF_WRAP_MODE_REPEAT;
	params.wrap_v = CF_WRAP_MODE_REPEAT;
	params.mip_filter = CF_MIP_FILTER_LINEAR;
	params.width = w;
	params.height = h;
	params.mip_count = 0;
	params.allocate_mipmaps = false;
	params.mip_lod_bias = 0.0f;
	params.max_anisotropy = 1.0f;
	params.stream = false;
	return params;
}

bool cf_texture_supports_format(CF_PixelFormat format, CF_TextureUsageBits usage);

CF_CanvasParams cf_canvas_defaults(int w, int h)
{
	CF_CanvasParams params = { 0 };
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
		params.depth_stencil_target.pixel_format = CF_PIXEL_FORMAT_D16_UNORM;
		if (cf_texture_supports_format(CF_PIXEL_FORMAT_D24_UNORM_S8_UINT, CF_TEXTURE_USAGE_DEPTH_STENCIL_TARGET_BIT)) {
			params.depth_stencil_target.pixel_format = CF_PIXEL_FORMAT_D24_UNORM_S8_UINT;
		}
		params.depth_stencil_target.usage = CF_TEXTURE_USAGE_DEPTH_STENCIL_TARGET_BIT;
	}
	return params;
}

CF_RenderState cf_render_state_defaults()
{
	CF_RenderState state = { };
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

CF_Material cf_make_material()
{
	CF_MaterialInternal* material = CF_NEW(CF_MaterialInternal);
	material->uniform_arena = cf_make_arena(4, CF_KB * 16);
	material->block_arena = cf_make_arena(4, CF_KB * 16);
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

void cf_material_set_texture_cs(CF_Material material_handle, const char* name, CF_Texture texture)
{
	CF_MaterialInternal* material = (CF_MaterialInternal*)material_handle.id;
	name = sintern(name);
	s_material_set_texture(material, &material->cs, name, texture);
}

void cf_material_clear_textures(CF_Material material_handle)
{
	CF_MaterialInternal* material = (CF_MaterialInternal*)material_handle.id;
	material->vs.textures.clear();
	material->fs.textures.clear();
	material->cs.textures.clear();
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

void cf_material_set_uniform_cs(CF_Material material_handle, const char* name, void* data, CF_UniformType type, int array_length)
{
	CF_MaterialInternal* material = (CF_MaterialInternal*)material_handle.id;
	name = sintern(name);
	s_material_set_uniform(&material->uniform_arena, &material->cs, sintern("uniform_block"), name, data, type, array_length);
}

void cf_material_clear_uniforms(CF_Material material_handle)
{
	CF_MaterialInternal* material = (CF_MaterialInternal*)material_handle.id;
	arena_reset(&material->uniform_arena);
	material->vs.uniforms.clear();
	material->fs.uniforms.clear();
	material->cs.uniforms.clear();
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

CF_Shader cf_make_shader(const char* vertex_path, const char* fragment_path)
{
	// Make sure each file can be found.
	char* vs = fs_read_entire_file_to_memory_and_nul_terminate(vertex_path);
	char* fs = fs_read_entire_file_to_memory_and_nul_terminate(fragment_path);
	CF_ASSERT(vs);
	CF_ASSERT(fs);
	CF_Shader shader = cf_make_shader_from_source(vs, fs);
	CF_FREE(vs);
	CF_FREE(fs);
	return shader;
}

CF_Shader cf_make_shader_from_source(const char* vertex_src, const char* fragment_src)
{
	return cf_make_shader_from_source_internal(vertex_src, fragment_src, NULL);
}

CF_ComputeShader cf_make_compute_shader(const char* path)
{
	CF_Path p = CF_Path("/") + path;
	const char* path_s = sintern(p);
	CF_ShaderFileInfo* info = app->shader_file_infos.try_find(path_s);
	if (!info) return { 0 };
	char* shd = fs_read_entire_file_to_memory_and_nul_terminate(info->path);
	if (!shd) return { 0 };
	CF_ComputeShader result = cf_make_compute_shader_from_source(shd);
	cf_free(shd);
	return result;
}

CF_ComputeShader cf_make_compute_shader_from_source(const char* src)
{
	CF_ShaderBytecode bytecode = cf_compile_shader_to_bytecode_internal(src, CF_SHADER_STAGE_COMPUTE, NULL);
	if (bytecode.content == NULL) {
		CF_ComputeShader result = { 0 };
		return result;
	}
	CF_ComputeShader shader = cf_make_compute_shader_from_bytecode(bytecode);
	cf_free_shader_bytecode(bytecode);
	return shader;
}

//--------------------------------------------------------------------------------------------------
// Backend dispatch shims.

#ifdef CF_EMSCRIPTEN

#define CF_DISPATCH_SHIM(RETURN_TYPE, OP, ARGUMENTS, ...) \
	RETURN_TYPE cf_gles_##OP ARGUMENTS; \
	RETURN_TYPE cf_##OP ARGUMENTS { \
		return cf_gles_##OP(__VA_ARGS__); \
	}

#define CF_DISPATCH_SHIM_VOID(OP, ARGUMENTS, ...) \
	void cf_gles_##OP ARGUMENTS; \
	void cf_##OP ARGUMENTS { \
		cf_gles_##OP(__VA_ARGS__); \
	}

#else

#define CF_DISPATCH_SHIM(RETURN_TYPE, OP, ARGUMENTS, ...) \
	RETURN_TYPE cf_sdlgpu_##OP ARGUMENTS; \
	RETURN_TYPE cf_gles_##OP ARGUMENTS; \
	RETURN_TYPE cf_##OP ARGUMENTS { \
		if (app->gfx_backend_type == CF_BACKEND_TYPE_GLES3) { \
			return cf_gles_##OP(__VA_ARGS__); \
		} else { \
			return cf_sdlgpu_##OP(__VA_ARGS__); \
		} \
	}

#define CF_DISPATCH_SHIM_VOID(OP, ARGUMENTS, ...) \
	void cf_sdlgpu_##OP ARGUMENTS; \
	void cf_gles_##OP ARGUMENTS; \
	void cf_##OP ARGUMENTS { \
		if (app->gfx_backend_type == CF_BACKEND_TYPE_GLES3) { \
			cf_gles_##OP(__VA_ARGS__); \
		} else { \
			cf_sdlgpu_##OP(__VA_ARGS__); \
		} \
	}

#endif

CF_DISPATCH_SHIM(bool, texture_supports_format, (CF_PixelFormat format, CF_TextureUsageBits usage), format, usage)
CF_DISPATCH_SHIM(bool, query_pixel_format, (CF_PixelFormat format, CF_PixelFormatOp op), format, op)

CF_DISPATCH_SHIM(CF_Texture, make_texture, (CF_TextureParams params), params)
CF_DISPATCH_SHIM_VOID(destroy_texture, (CF_Texture texture_handle), texture_handle)
CF_DISPATCH_SHIM_VOID(texture_update, (CF_Texture texture_handle, void* data, int size), texture_handle, data, size)
CF_DISPATCH_SHIM_VOID(texture_update_mip, (CF_Texture texture_handle, void* data, int size, int mip_level), texture_handle, data, size, mip_level)
CF_DISPATCH_SHIM_VOID(generate_mipmaps, (CF_Texture texture_handle), texture_handle)
CF_DISPATCH_SHIM(uint64_t, texture_handle, (CF_Texture texture), texture)
CF_DISPATCH_SHIM(uint64_t, texture_binding_handle, (CF_Texture texture), texture)

CF_DISPATCH_SHIM(CF_Canvas, make_canvas, (CF_CanvasParams params), params)
CF_DISPATCH_SHIM_VOID(destroy_canvas, (CF_Canvas canvas_handle), canvas_handle)
CF_DISPATCH_SHIM(CF_Texture, canvas_get_target, (CF_Canvas canvas_handle), canvas_handle)
CF_DISPATCH_SHIM(CF_Texture, canvas_get_depth_stencil_target, (CF_Canvas canvas_handle), canvas_handle)
CF_DISPATCH_SHIM_VOID(canvas_get_size, (CF_Canvas canvas_handle, int* w, int* h), canvas_handle, w, h)
CF_DISPATCH_SHIM_VOID(clear_canvas, (CF_Canvas canvas_handle), canvas_handle)
CF_DISPATCH_SHIM_VOID(apply_canvas, (CF_Canvas canvas_handle, bool clear), canvas_handle, clear)

CF_DISPATCH_SHIM_VOID(apply_viewport, (int x, int y, int w, int h), x, y, w, h)
CF_DISPATCH_SHIM_VOID(apply_scissor, (int x, int y, int w, int h), x, y, w, h)
CF_DISPATCH_SHIM_VOID(apply_stencil_reference, (int reference), reference)
CF_DISPATCH_SHIM_VOID(apply_blend_constants, (float r, float g, float b, float a), r, g, b, a)

CF_DISPATCH_SHIM(CF_Mesh, make_mesh, (int vertex_buffer_size, const CF_VertexAttribute* attributes, int attribute_count, int vertex_stride), vertex_buffer_size, attributes, attribute_count, vertex_stride)
CF_DISPATCH_SHIM_VOID(mesh_set_index_buffer, (CF_Mesh mesh_handle, int index_buffer_size_in_bytes, int index_bit_count), mesh_handle, index_buffer_size_in_bytes, index_bit_count)
CF_DISPATCH_SHIM_VOID(mesh_set_instance_buffer, (CF_Mesh mesh_handle, int instance_buffer_size_in_bytes, int instance_stride), mesh_handle, instance_buffer_size_in_bytes, instance_stride)
CF_DISPATCH_SHIM_VOID(destroy_mesh, (CF_Mesh mesh_handle), mesh_handle)
CF_DISPATCH_SHIM_VOID(mesh_update_vertex_data, (CF_Mesh mesh_handle, void* data, int count), mesh_handle, data, count)
CF_DISPATCH_SHIM_VOID(mesh_update_index_data, (CF_Mesh mesh_handle, void* data, int count), mesh_handle, data, count)
CF_DISPATCH_SHIM_VOID(mesh_update_instance_data, (CF_Mesh mesh_handle, void* data, int count), mesh_handle, data, count)
CF_DISPATCH_SHIM_VOID(apply_mesh, (CF_Mesh mesh_handle), mesh_handle)

CF_DISPATCH_SHIM(CF_Shader, make_shader_from_bytecode, (CF_ShaderBytecode vertex_bytecode, CF_ShaderBytecode fragment_bytecode), vertex_bytecode, fragment_bytecode)
CF_DISPATCH_SHIM_VOID(destroy_shader_internal, (CF_Shader shader_handle), shader_handle)
CF_DISPATCH_SHIM_VOID(apply_shader, (CF_Shader shader_handle, CF_Material material_handle), shader_handle, material_handle)

void cf_sdlgpu_draw_elements();
void cf_gles_draw_elements();
void cf_draw_elements()
{
	if (app->gfx_backend_type == CF_BACKEND_TYPE_GLES3) {
		cf_gles_draw_elements();
	} else {
#ifndef CF_EMSCRIPTEN
		cf_sdlgpu_draw_elements();
#endif
	}
}

CF_DISPATCH_SHIM(CF_ComputeShader, make_compute_shader_from_bytecode, (CF_ShaderBytecode bytecode), bytecode)
CF_DISPATCH_SHIM_VOID(destroy_compute_shader, (CF_ComputeShader shader), shader)
CF_DISPATCH_SHIM(CF_StorageBuffer, make_storage_buffer, (CF_StorageBufferParams params), params)
CF_DISPATCH_SHIM_VOID(update_storage_buffer, (CF_StorageBuffer buffer, const void* data, int size), buffer, data, size)
CF_DISPATCH_SHIM_VOID(destroy_storage_buffer, (CF_StorageBuffer buffer), buffer)
CF_DISPATCH_SHIM_VOID(dispatch_compute, (CF_ComputeShader shader, CF_Material material, CF_ComputeDispatch dispatch), shader, material, dispatch)

CF_DISPATCH_SHIM(void*, create_draw_sampler, (CF_Filter filter), filter)
CF_DISPATCH_SHIM_VOID(destroy_draw_sampler, (void* sampler), sampler)
CF_DISPATCH_SHIM_VOID(set_sampler_override, (void* sampler), sampler)
