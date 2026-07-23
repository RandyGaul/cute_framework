// This is a standalone shader compiler for Cute Framework, used to precompile shaders into
// cross-platform bytecode blobs, either as direct binary files, or C-headers.
//
// See: https://randygaul.github.io/cute_framework/topics/shader_compilation

#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>
#include "cute_shader.h"
#include "builtin_shaders.h"

#define FLAG_INCLUDE "-I"
#define FLAG_HELP "--help"
#define FLAG_HEADER_OUT "-oheader="
#define FLAG_BYTECODE_OUT "-obytecode="
#define FLAG_VARNAME "-varname="
#define FLAG_TYPE "-type="
#define FLAG_NOGLES "-nogles"
#define FLAG_VERBOSE "-verbose"
#define FLAG_INVALID "-"
#define MAX_INCLUDES 64
#define HEADER_LINE_SIZE 16

typedef enum
{
	SHADER_TYPE_VERTEX,
	SHADER_TYPE_FRAGMENT,
	SHADER_TYPE_COMPUTE,
	SHADER_TYPE_DRAW,
} shader_type_t;

static const char* parse_flag(const char* arg, const char* flag_name)
{
	size_t flag_len = strlen(flag_name);
	if (strncmp(flag_name, arg, flag_len) == 0) {
		return arg + flag_len;
	} else {
		return NULL;
	}
}

static char* read_file(const char* path)
{
	errno = 0;
	FILE* file = fopen(path, "rb");
	if (file == NULL) {
		return NULL;
	}

	if (fseek(file, 0, SEEK_END) != 0) {
		fclose(file);
		return NULL;
	}
	long size = ftell(file);
	if (size < 0) {
		fclose(file);
		return NULL;
	}

	if (fseek(file, 0, SEEK_SET) != 0) {
		fclose(file);
		return NULL;
	}
	char* content = (char*)malloc(size + 1);
	fread(content, size, 1, file);
	if (ferror(file)) {
		free(content);
		fclose(file);
		return NULL;
	}
	content[size] = 0;
	fclose(file);
	return content;
}

static bool write_bytecode(
	FILE* file,
	CF_ShaderCompilerResult compile_result,
	const char* var_name,
	const char* suffix
)
{
	const uint8_t* content = compile_result.bytecode.content;

	// Write preprocessed shader as comment (-verbose only; it dominates header size).
	if (compile_result.preprocessed_source) {
		fprintf(file, "/*\n");
		fprintf(file, "%.*s\n", (int)compile_result.preprocessed_source_size, compile_result.preprocessed_source);
		fprintf(file, "*/\n");
	}

	// Write the bytecode.
	fprintf(file, "#ifndef __EMSCRIPTEN__\n");
	fprintf(file, "static const uint8_t %s%s_content[%zu] = {", var_name, suffix, compile_result.bytecode.size);
	for (size_t i = 0; i < compile_result.bytecode.size; ++i) {
		if ((i % HEADER_LINE_SIZE) == 0) {
			fprintf(file, "\n\t");
		}
		fprintf(file, "0x%02X,", content[i]);
	}
	fprintf(file, "\n};\n");
	fprintf(file, "#endif\n");

	// Write GLSL 300 (absent with -nogles: the bytecode then only works on
	// non-GLES backends).
	if (compile_result.bytecode.glsl300_src) {
		fprintf(file, "static const char %s%s_glsl300_src[%zu] =\n\"", var_name, suffix, compile_result.bytecode.glsl300_src_size + 1);
		for (size_t i = 0; i < compile_result.bytecode.glsl300_src_size; ++i) {
			char ch = compile_result.bytecode.glsl300_src[i];
			if (ch == '\n') {  // Escape new line as \n and start an actual new line
				fprintf(file, "\\n\"\n\"");
			} else {
				fprintf(file, "%c", ch);
			}
		}
		fprintf(file, "\";\n");
	} else {
		fprintf(file, "#define %s%s_glsl300_src NULL\n", var_name, suffix);
	}

	// Write HLSL (for D3D12, compiled to DXBC by the system FXC at runtime).
	if (compile_result.bytecode.hlsl_src) {
		fprintf(file, "static const char %s%s_hlsl_src[%zu] =\n\"", var_name, suffix, compile_result.bytecode.hlsl_src_size + 1);
		for (size_t i = 0; i < compile_result.bytecode.hlsl_src_size; ++i) {
			char ch = compile_result.bytecode.hlsl_src[i];
			if (ch == '\n') {  // Escape new line as \n and start an actual new line
				fprintf(file, "\\n\"\n\"");
			} else {
				fprintf(file, "%c", ch);
			}
		}
		fprintf(file, "\";\n");
	} else {
		fprintf(file, "#define %s%s_hlsl_src NULL\n", var_name, suffix);
	}

	// Write MSL (for Metal, compiled by the OS at runtime).
	if (compile_result.bytecode.msl_src) {
		fprintf(file, "static const char %s%s_msl_src[%zu] =\n\"", var_name, suffix, compile_result.bytecode.msl_src_size + 1);
		for (size_t i = 0; i < compile_result.bytecode.msl_src_size; ++i) {
			char ch = compile_result.bytecode.msl_src[i];
			if (ch == '\n') {  // Escape new line as \n and start an actual new line
				fprintf(file, "\\n\"\n\"");
			} else if (ch == '"') {
				fprintf(file, "\\\"");
			} else {
				fprintf(file, "%c", ch);
			}
		}
		fprintf(file, "\";\n");
	} else {
		fprintf(file, "#define %s%s_msl_src NULL\n", var_name, suffix);
	}

	// Write reflection info.
	const CF_ShaderInfo* shader_info = &compile_result.bytecode.shader_info;

	if (shader_info->num_images > 0) {
		fprintf(file, "static const char* %s%s_image_names[%d] = {\n   ", var_name, suffix, shader_info->num_images);
		for (int i = 0; i < shader_info->num_images; ++i) {
			fprintf(file, " \"%s\",", shader_info->image_names[i]);
		}
		fprintf(file, "\n};\n");
		fprintf(file, "static int %s%s_image_binding_slots[%d] = {", var_name, suffix, shader_info->num_images);
		for (int i = 0; i < shader_info->num_images; ++i) {
			fprintf(file, " %d,", shader_info->image_binding_slots[i]);
		}
		fprintf(file, " };\n");
	} else {
		fprintf(file, "#define %s%s_image_names NULL\n", var_name, suffix);
		fprintf(file, "#define %s%s_image_binding_slots NULL\n", var_name, suffix);
	}

	if (shader_info->num_uniforms > 0) {
		fprintf(file, "static CF_ShaderUniformInfo %s%s_uniforms[%d] = {\n", var_name, suffix, shader_info->num_uniforms);
		for (int i = 0; i < shader_info->num_uniforms; ++i) {
			fprintf(file, "\t{\n");
			fprintf(file, "\t\t.block_name = \"%s\",\n", shader_info->uniforms[i].block_name);
			fprintf(file, "\t\t.block_index = %d,\n", shader_info->uniforms[i].block_index);
			fprintf(file, "\t\t.block_size = %d,\n", shader_info->uniforms[i].block_size);
			fprintf(file, "\t\t.num_members = %d,\n", shader_info->uniforms[i].num_members);
			fprintf(file, "\t},\n");
		}
		fprintf(file, "};\n");
	} else {
		fprintf(file, "#define %s%s_uniforms NULL\n", var_name, suffix);
	}

	if (shader_info->num_uniform_members > 0) {
		fprintf(file, "static CF_ShaderUniformMemberInfo %s%s_uniform_members[%d] = {\n", var_name, suffix, shader_info->num_uniform_members);
		for (int i = 0; i < shader_info->num_uniform_members; ++i) {
			fprintf(file, "\t{\n");
			fprintf(file, "\t\t.name = \"%s\",\n", shader_info->uniform_members[i].name);
			fprintf(file, "\t\t.type = %s,\n", cf_shader_info_data_type_to_string(shader_info->uniform_members[i].type));
			fprintf(file, "\t\t.offset = %d,\n", shader_info->uniform_members[i].offset);
			fprintf(file, "\t\t.array_length = %d,\n", shader_info->uniform_members[i].array_length);
			fprintf(file, "\t},\n");
		}
		fprintf(file, "};\n");
	} else {
		fprintf(file, "#define %s%s_uniform_members NULL\n", var_name, suffix);
	}

	if (shader_info->num_inputs > 0) {
		fprintf(file, "static CF_ShaderInputInfo %s%s_inputs[%d] = {\n", var_name, suffix, shader_info->num_inputs);
		for (int i = 0; i < shader_info->num_inputs; ++i) {
			fprintf(file, "\t{\n");
			fprintf(file, "\t\t.name = \"%s\",\n", shader_info->inputs[i].name);
			fprintf(file, "\t\t.location = %d,\n", shader_info->inputs[i].location);
			fprintf(file, "\t\t.format = %s,\n", cf_shader_info_data_type_to_string(shader_info->inputs[i].format));
			fprintf(file, "\t},\n");
		}
		fprintf(file, "};\n");
	} else {
		fprintf(file, "#define %s%s_inputs NULL\n", var_name, suffix);
	}

	return ferror(file) == 0;
}

static bool write_bytecode_struct_contents(
	FILE* file,
	CF_ShaderCompilerResult compile_result,
	const char* var_name,
	const char* suffix,
	int tabs
)
{
	const CF_ShaderInfo* shader_info = &compile_result.bytecode.shader_info;

#define TABS() fprintf(file, "%s", tabs == 0 ? "" : (tabs == 1 ? "\t" : (tabs == 2 ? "\t\t" : (tabs == 3 ? "\t\t\t" : "\t\t\t\t"))))

	// Write the struct.
	fprintf(file, "#ifndef __EMSCRIPTEN__\n");
	TABS(); fprintf(file, ".content = %s%s_content,\n", var_name, suffix);
	TABS(); fprintf(file, ".size = %zu,\n", compile_result.bytecode.size);
	fprintf(file, "#endif\n");
	TABS(); fprintf(file, ".glsl300_src = %s%s_glsl300_src,\n", var_name, suffix);
	TABS(); fprintf(file, ".glsl300_src_size = %zu,\n", compile_result.bytecode.glsl300_src_size);
	TABS(); fprintf(file, ".hlsl_src = %s%s_hlsl_src,\n", var_name, suffix);
	TABS(); fprintf(file, ".hlsl_src_size = %zu,\n", compile_result.bytecode.hlsl_src_size);
	TABS(); fprintf(file, ".msl_src = %s%s_msl_src,\n", var_name, suffix);
	TABS(); fprintf(file, ".msl_src_size = %zu,\n", compile_result.bytecode.msl_src_size);
	TABS(); fprintf(file, ".shader_info = {\n");
	TABS(); fprintf(file, "\t.num_samplers = %d,\n", shader_info->num_samplers);
	TABS(); fprintf(file, "\t.num_storage_textures = %d,\n", shader_info->num_storage_textures);
	TABS(); fprintf(file, "\t.num_storage_buffers = %d,\n", shader_info->num_storage_buffers);
	TABS(); fprintf(file, "\t.num_readwrite_storage_textures = %d,\n", shader_info->num_readwrite_storage_textures);
	TABS(); fprintf(file, "\t.num_readwrite_storage_buffers = %d,\n", shader_info->num_readwrite_storage_buffers);
	TABS(); fprintf(file, "\t.num_images = %d,\n", shader_info->num_images);
	TABS(); fprintf(file, "\t.image_names = %s%s_image_names,\n", var_name, suffix);
	TABS(); fprintf(file, "\t.image_binding_slots = %s%s_image_binding_slots,\n", var_name, suffix);
	TABS(); fprintf(file, "\t.num_uniforms = %d,\n", shader_info->num_uniforms);
	TABS(); fprintf(file, "\t.uniforms = %s%s_uniforms,\n", var_name, suffix);
	TABS(); fprintf(file, "\t.num_uniform_members = %d,\n", shader_info->num_uniform_members);
	TABS(); fprintf(file, "\t.uniform_members = %s%s_uniform_members,\n", var_name, suffix);
	TABS(); fprintf(file, "\t.num_inputs = %d,\n", shader_info->num_inputs);
	TABS(); fprintf(file, "\t.inputs = %s%s_inputs,\n", var_name, suffix);
	TABS(); fprintf(file, "\t.local_size = { %d, %d, %d },\n", shader_info->local_size[0], shader_info->local_size[1], shader_info->local_size[2]);
	TABS(); fprintf(file, "},\n");

#undef TABS

	return ferror(file) == 0;
}


static bool write_bytecode_struct(
	FILE* file,
	CF_ShaderCompilerResult compile_result,
	const char* var_name,
	const char* suffix
)
{
	write_bytecode(file, compile_result, var_name, suffix);
	fprintf(file, "static const CF_ShaderBytecode %s%s = {\n", var_name, suffix);
	write_bytecode_struct_contents(file, compile_result, var_name, suffix, 1);
	fprintf(file, "};\n");

	return ferror(file) == 0;
}

static bool write_draw_bytecode_struct(
	FILE* file,
	CF_ShaderCompilerResult draw_result,
	CF_ShaderCompilerResult blit_result,
	const char* var_name
)
{
	fprintf(file, "static const CF_DrawShaderBytecode %s = {\n", var_name);
	fprintf(file, "\t.draw_shader = {\n");
	if (!write_bytecode_struct_contents(file, draw_result, var_name, "_draw", 2)) {
		fclose(file);
		return false;
	}
	fprintf(file, "\t},\n");
	fprintf(file, "\t.blit_shader = {\n");
	if (!write_bytecode_struct_contents(file, blit_result, var_name, "_blit", 2)) {
		fclose(file);
		return false;
	}
	fprintf(file, "\t},\n");
	fprintf(file, "};\n");

	return ferror(file) == 0;
}

static bool write_draw_header_file(
	const char* path,
	CF_ShaderCompilerResult draw_result,
	CF_ShaderCompilerResult blit_result,
	const char* var_name
)
{
	errno = 0;
	FILE* file = fopen(path, "wb");
	if (file == NULL) {
		return false;
	}

	fprintf(file, "#pragma once\n\n");

	// Write the constant.
	write_bytecode(file, draw_result, var_name, "_draw");
	write_bytecode(file, blit_result, var_name, "_blit");
	write_draw_bytecode_struct(file, draw_result, blit_result, var_name);

	if (ferror(file) != 0) {
		fclose(file);
		return false;
	}

	if (fflush(file) != 0) {
		fclose(file);
		return false;
	}

	return fclose(file) == 0;
}

static bool write_standalone_header_file(
	const char* path,
	CF_ShaderCompilerResult compile_result,
	const char* var_name
)
{
	errno = 0;
	FILE* file = fopen(path, "wb");
	if (file == NULL) {
		return false;
	}

	fprintf(file, "#pragma once\n\n");

	if (!write_bytecode_struct(file, compile_result, var_name, "")) {
		fclose(file);
		return false;
	}
	if (fflush(file) != 0) {
		fclose(file);
		return false;
	}

	return fclose(file) == 0;
}

static bool write_bytecode_file(
	const char* path,
	CF_ShaderCompilerResult compile_result
)
{
	errno = 0;
	FILE* file = fopen(path, "wb");
	if (file == NULL) { return false; }

	fwrite(compile_result.bytecode.content, compile_result.bytecode.size, 1, file);
	if (ferror(file) != 0) {
		fclose(file);
		return false;
	}
	if (fflush(file) != 0) {
		fclose(file);
		return false;
	}

	return fclose(file) == 0;
}

int main(int argc, const char* argv[])
{
	const char* input_path = NULL;
	const char* output_header_path = NULL;
	const char* output_bytecode_path = NULL;
	const char* var_name = NULL;
	int num_includes = 0;
	bool type_set = false;
	bool nogles = false;
	bool verbose = false;
	shader_type_t type = SHADER_TYPE_DRAW;
	const char* include_dirs[MAX_INCLUDES];

	for (int i = 1; i < argc; ++i) {
		const char* flag_value;
		const char* arg = argv[i];
		if ((flag_value = parse_flag(arg, FLAG_HELP)) != NULL) {
			fprintf(stderr,
				"Usage: cute-shaderc [options] <input>\n"
				"Compile GLSL into SPIRV bytecode and generate a C header for embedding.\n"
				"\n"
				"--help             Print this message.\n"
				"-I<dir>            Add directory to #include search path.\n"
				"-type=<type>       The shader type. Valid values are:\n"
				"                   * draw (default): Draw shader for `cf_make_draw_shader_from_bytecode`.\n"
				"                   * vertex: Standalone vertex shader for `cf_make_shader_from_bytecode`.\n"
				"                   * fragment: Standalone fragment shader for `cf_make_shader_from_bytecode`.\n"
				"-oheader=<file>    Where to write the C header file.\n"
				"                   Also requires -varname.\n"
				"-varname=<file>    The variable name inside the C header.\n"
				"-obytecode=<file>  (Optional) Where to write the raw SPIRV blob.\n"
			);
			return 0;
		} else if ((flag_value = parse_flag(arg, FLAG_INCLUDE)) != NULL) {
			if (num_includes <= MAX_INCLUDES) {
				include_dirs[num_includes++] = flag_value;
			} else {
				fprintf(stderr, "Too many includes\n");
				return 1;
			}
		} else if (strcmp(arg, FLAG_NOGLES) == 0) {
			nogles = true;
		} else if (strcmp(arg, FLAG_VERBOSE) == 0) {
			verbose = true;
		} else if ((flag_value = parse_flag(arg, FLAG_TYPE)) != NULL) {
			if (type_set) {
				fprintf(stderr, "%s can only be specified once\n", FLAG_TYPE);
				return 1;
			}

			if (strcmp(flag_value, "vertex") == 0) {
				type = SHADER_TYPE_VERTEX;
			} else if (strcmp(flag_value, "fragment") == 0) {
				type = SHADER_TYPE_FRAGMENT;
			} else if (strcmp(flag_value, "compute") == 0) {
				type = SHADER_TYPE_COMPUTE;
			} else if (strcmp(flag_value, "draw") == 0) {
				type = SHADER_TYPE_DRAW;
			} else {
				fprintf(stderr, "Invalid shader type: %s\n", flag_value);
				return 1;
			}
			type_set = true;
		} else if ((flag_value = parse_flag(arg, FLAG_HEADER_OUT)) != NULL) {
			if (output_header_path == NULL) {
				output_header_path = flag_value;
			} else {
				fprintf(stderr, "%s can only be specified once\n", FLAG_HEADER_OUT);
				return 1;
			}
		} else if ((flag_value = parse_flag(arg, FLAG_VARNAME)) != NULL) {
			if (var_name == NULL) {
				var_name = flag_value;
			} else {
				fprintf(stderr, "%s can only be specified once\n", FLAG_VARNAME);
				return 1;
			}
		} else if ((flag_value = parse_flag(arg, FLAG_BYTECODE_OUT)) != NULL) {
			if (output_bytecode_path == NULL) {
				output_bytecode_path = flag_value;
			} else {
				fprintf(stderr, "%s can only be specified once\n", FLAG_BYTECODE_OUT);
				return 1;
			}
		} else if ((flag_value = parse_flag(arg, FLAG_INVALID)) != NULL) {
			fprintf(stderr, "Invalid option: %s\n", arg);
			return 1;
		} else {
			if (input_path == NULL) {
				input_path = arg;
			} else {
				fprintf(stderr, "Please specify only one input file\n");
				return 1;
			}
		}
	}

	{
		if (input_path == NULL) {
			fprintf(stderr, "Please specify an input, use --help for more info\n");
			fprintf(stderr, "You can also visit https://randygaul.github.io/cute_framework/topics/shader_compilation\n");
			return 1;
		}

		if (output_header_path != NULL && var_name == NULL) {
			fprintf(stderr, "%s also requires %s\n", FLAG_HEADER_OUT, FLAG_VARNAME);
			return 1;
		}

		if (
			!(type == SHADER_TYPE_VERTEX || type == SHADER_TYPE_FRAGMENT || type == SHADER_TYPE_COMPUTE)
			&& output_bytecode_path != NULL
		) {
			fprintf(stderr, "%s is only valid for shader of type 'vertex', 'fragment', or 'compute'\n", FLAG_BYTECODE_OUT);
			return 1;
		}
	}

	CF_ShaderCompilerFile builtin_includes[sizeof(s_builtin_includes) / sizeof(s_builtin_includes[0]) + 1];
	int num_builtin_includes = sizeof(s_builtin_includes) / sizeof(s_builtin_includes[0]);
	for (int i = 0; i < num_builtin_includes; ++i) {
		builtin_includes[i] = s_builtin_includes[i];
	}

	CF_ShaderCompilerConfig config = {
		.num_builtin_includes = num_builtin_includes,
		.num_include_dirs = num_includes,
		.include_dirs = include_dirs,
		.automatic_include_guard = true,
	};

	int return_code = 1;
	char* input_content = NULL;
	input_content = read_file(input_path);
	if (input_content == NULL) {
		perror("Error while reading input");
		goto end;
	}

	if (type == SHADER_TYPE_DRAW) {
		builtin_includes[num_builtin_includes++] = {
			.name = "shader_stub.shd",
			.content = input_content,
		};

		int num_defines = 0;
		CF_ShaderCompilerDefine defines[2];

		CF_ShaderCompilerConfig config = {
			.num_builtin_defines = 0,
			.builtin_defines = defines,

			.num_builtin_includes = num_builtin_includes,
			.builtin_includes = builtin_includes,

			.num_include_dirs = num_includes,
			.include_dirs = include_dirs,

			.automatic_include_guard = true,
			.return_preprocessed_source = verbose,

			// The SSBO draw flavor cannot transpile to GLSL 300 es; the GLES flavor's
			// transpile is grafted in below.
			.skip_glsl300 = true,
		};

		// The payload storage buffer binds right after the stub's last sampler.
		// Scan the preprocessed stub so comments/macros can't fool the scan.
		char payload_binding_str[16];
		int payload_binding = 1;
		{
			char* preprocessed = cute_shader_preprocess(input_content, config);
			if (preprocessed) {
				payload_binding = cf_compute_payload_binding(preprocessed);
				free(preprocessed);
			}
		}
		snprintf(payload_binding_str, sizeof(payload_binding_str), "%d", payload_binding);
		defines[num_defines++] = { "CF_PAYLOAD_BINDING", payload_binding_str };
		config.num_builtin_defines = num_defines;

		CF_ShaderCompilerResult draw_shader_result = cute_shader_compile(
			s_draw_fs,
			CUTE_SHADER_STAGE_FRAGMENT,
			config
		);
		if (!draw_shader_result.success) {
			fprintf(stderr, "%s\n", draw_shader_result.error_message);
			cute_shader_free_result(draw_shader_result);
			goto end;
		}

		// GLES3/WebGL2 runs the texel-fetch flavor of the draw shader (CF_GLES
		// define); compile the same stub against it and graft its GLSL 300 es
		// output into the primary bytecode. Skipped entirely with -nogles.
		CF_ShaderCompilerResult draw_gles_result = { 0 };
		if (!nogles) {
			config.skip_glsl300 = false;
			config.skip_hlsl = true; // Only the GLSL 300 output is grafted from this flavor.
			config.skip_msl = true;
			defines[num_defines++] = { "CF_GLES", "1" };
			config.num_builtin_defines = num_defines;
			draw_gles_result = cute_shader_compile(
				s_draw_fs,
				CUTE_SHADER_STAGE_FRAGMENT,
				config
			);
			if (!draw_gles_result.success) {
				fprintf(stderr, "%s\n", draw_gles_result.error_message);
				cute_shader_free_result(draw_shader_result);
				cute_shader_free_result(draw_gles_result);
				goto end;
			}
			draw_shader_result.bytecode.glsl300_src = draw_gles_result.bytecode.glsl300_src;
			draw_shader_result.bytecode.glsl300_src_size = draw_gles_result.bytecode.glsl300_src_size;
			// The blit shader below wants its GLSL 300 too; drop the CF_GLES define
			// (blit has no flavors) but keep transpilation on.
			config.num_builtin_defines = --num_defines;
			config.skip_hlsl = false;
			config.skip_msl = false;
		} else {
			config.skip_glsl300 = true;
		}

		CF_ShaderCompilerResult blit_shader_result = cute_shader_compile(
			s_blit_fs,
			CUTE_SHADER_STAGE_FRAGMENT,
			config
		);
		if (!blit_shader_result.success) {
			fprintf(stderr, "%s\n", blit_shader_result.error_message);
			cute_shader_free_result(draw_shader_result);
			cute_shader_free_result(draw_gles_result);
			cute_shader_free_result(blit_shader_result);
			goto end;
		}

		bool wrote_ok = true;
		if (output_header_path != NULL) {
			wrote_ok = write_draw_header_file(
				output_header_path,
				draw_shader_result, blit_shader_result,
				var_name
			);
			if (!wrote_ok) perror("Error while writing header");
		}

		// The grafted glsl300 pointer belongs to draw_gles_result; detach before freeing.
		draw_shader_result.bytecode.glsl300_src = NULL;
		draw_shader_result.bytecode.glsl300_src_size = 0;
		cute_shader_free_result(draw_shader_result);
		cute_shader_free_result(draw_gles_result);
		cute_shader_free_result(blit_shader_result);
		if (!wrote_ok) goto end;
	} else {
		builtin_includes[num_builtin_includes++] = {
			.name = "shader_stub.shd",
			.content = s_shader_stub,
		};

		CF_ShaderCompilerConfig config = {
			.num_builtin_includes = num_builtin_includes,
			.builtin_includes = builtin_includes,

			.num_include_dirs = num_includes,
			.include_dirs = include_dirs,

			.automatic_include_guard = true,
			.return_preprocessed_source = verbose,
			.skip_glsl300 = nogles,
		};

		CF_ShaderCompilerStage compile_stage = CUTE_SHADER_STAGE_FRAGMENT;
		if (type == SHADER_TYPE_VERTEX) compile_stage = CUTE_SHADER_STAGE_VERTEX;
		else if (type == SHADER_TYPE_COMPUTE) compile_stage = CUTE_SHADER_STAGE_COMPUTE;

		CF_ShaderCompilerResult result = cute_shader_compile(
			input_content,
			compile_stage,
			config
		);
		if (!result.success) {
			fprintf(stderr, "%s\n", result.error_message);
			cute_shader_free_result(result);
			goto end;
		}

		if (output_header_path != NULL) {
			if (!write_standalone_header_file(
				output_header_path,
				result,
				var_name
			)) {
				perror("Error while writing header");
				cute_shader_free_result(result);
				goto end;
			}
		}

		if (output_bytecode_path != NULL) {
			if (!write_bytecode_file(output_bytecode_path, result)) {
				perror("Error while writing bytecode");
				cute_shader_free_result(result);
				goto end;
			}
		}

		cute_shader_free_result(result);
	}

	return_code = 0;
end:
	if (input_content != NULL) { free(input_content); }

	return return_code;
}
