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
#define FLAG_VARNAME "-varname="
#define FLAG_TYPE "-type="
#define FLAG_INVALID "-"
#define MAX_INCLUDES 64
#define HEADER_LINE_SIZE 16

#define PARSE_FLAG(ARG, FLAG) parse_flag(ARG, FLAG, sizeof(FLAG) - 1)

typedef enum {
	SHADER_TYPE_VERTEX,
	SHADER_TYPE_FRAGMENT,
	SHADER_TYPE_DRAW,
	SHADER_TYPE_BUILTIN,
} shader_type_t;

static const char* parse_flag(const char* arg, const char* flag_name, size_t flag_len) {
	if (strncmp(flag_name, arg, flag_len) == 0) {
		return arg + flag_len;
	} else {
		return NULL;
	}
}

static char* read_file(const char* path) {
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
	fclose(file);
	return content;
}

static bool write_bytecode_struct(
	FILE* file,
	cute_shader_result_t compile_result,
	const char* var_name,
	const char* suffix
) {
	const uint8_t* content = (const uint8_t*)compile_result.bytecode;
	fprintf(file, "static const CF_ShaderBytecode %s%s = {\n", var_name, suffix);
	fprintf(file, "    .content = {");
	for (size_t i = 0; i < compile_result.bytecode_size; ++i) {
		if ((i % HEADER_LINE_SIZE) == 0) {
			fprintf(file, "\n       ");
		}
		fprintf(file, " 0x%02X,", content[i]);
	}
	fprintf(file, "\n    },\n");
	fprintf(file, "    .size = %zu,\n", compile_result.bytecode_size);
	fprintf(file, "};\n");

	return ferror(file) == 0;
}

static bool write_draw_header_file(
	const char* path,
	cute_shader_result_t draw_result,
	cute_shader_result_t blit_result,
	int argc, const char* argv[],
	const char* var_name
) {
	errno = 0;
	FILE* file = fopen(path, "wb");
	if (file == NULL) {
		return false;
	}

	fprintf(file, "#pragma once\n\n");
	// Write the command for reference
	fprintf(file, "//");
	for (int i = 0; i < argc; ++i) {
		fprintf(file, " %s", argv[i]);
	}
	fprintf(file, "\n\n");

	// Write the constant
	if (!write_bytecode_struct(file, draw_result, var_name, "_draw")) {
		fclose(file);
		return false;
	}
	if (!write_bytecode_struct(file, blit_result, var_name, "_blit")) {
		fclose(file);
		return false;
	}

	fprintf(file, "static const CF_DrawShaderBytecode %s = {\n", var_name);
	fprintf(file, "    .draw_shader = %s_draw,\n", var_name);
	fprintf(file, "    .blit_shader = %s_blit,\n", var_name);
	fprintf(file, "};\n");
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
	cute_shader_result_t compile_result,
	int argc, const char* argv[],
	const char* var_name
) {
	errno = 0;
	FILE* file = fopen(path, "wb");
	if (file == NULL) {
		return false;
	}

	fprintf(file, "#pragma once\n\n");
	// Write the command for reference
	fprintf(file, "//");
	for (int i = 0; i < argc; ++i) {
		fprintf(file, " %s", argv[i]);
	}
	fprintf(file, "\n\n");
	// Write the constant
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

int main(int argc, const char* argv[]) {
	const char* input_path = NULL;
	const char* output_header_path = NULL;
	const char* var_name = NULL;
	int num_includes = 0;
	bool type_set = false;
	shader_type_t type = SHADER_TYPE_DRAW;
	const char* include_dirs[MAX_INCLUDES];

	for (int i = 1; i < argc; ++i) {
		const char* flag_value;
		const char* arg = argv[i];
		if ((flag_value = PARSE_FLAG(arg, FLAG_HELP)) != NULL) {
			fprintf(stderr,
				"Usage: cute-shaderc [options] <input>\n"
				"Compile GLSL into SPIRV bytecode and generate a header.\n"
				"\n"
				"\n"
				"--help             Print this message.\n"
				"-I<dir>            Add directory to #include search path.\n"
				"-type=<type>       The shader type.\n"
				"                   Valid values are:\n"
				"                   * draw (default): Draw shader for cf_make_draw_shader_from_bytecode.\n"
				"                   * vertex: Standalone vertex shader for cf_make_shader_from_bytecode.\n"
				"                   * fragment: Standalone fragment shader for cf_make_shader_from_bytecode.\n"
				"-oheader=<file>    Where to write the C header file.\n"
				"                   Also requires -varname.\n"
				"-varname=<file>    The variable name inside the C header.\n"
			);
			return 0;
		} else if ((flag_value = PARSE_FLAG(arg, FLAG_INCLUDE)) != NULL) {
			if (num_includes <= MAX_INCLUDES) {
				include_dirs[num_includes++] = flag_value;
			} else {
				fprintf(stderr, "Too many includes\n");
				return 1;
			}
		} else if ((flag_value = PARSE_FLAG(arg, FLAG_TYPE)) != NULL) {
			if (type_set) {
				fprintf(stderr, "%s can only be specified once\n", FLAG_TYPE);
				return 1;
			}

			if (strcmp(flag_value, "vertex") == 0) {
				type = SHADER_TYPE_VERTEX;
			} else if (strcmp(flag_value, "fragment") == 0) {
				type = SHADER_TYPE_FRAGMENT;
			} else if (strcmp(flag_value, "draw") == 0) {
				type = SHADER_TYPE_DRAW;
			} else {
				fprintf(stderr, "Invalid shader type: %s\n", flag_value);
				return 1;
			}
			type_set = true;
		} else if ((flag_value = PARSE_FLAG(arg, FLAG_HEADER_OUT)) != NULL) {
			if (output_header_path == NULL) {
				output_header_path = flag_value;
			} else {
				fprintf(stderr, "%s can only be specified once\n", FLAG_HEADER_OUT);
				return 1;
			}
		} else if ((flag_value = PARSE_FLAG(arg, FLAG_VARNAME)) != NULL) {
			if (var_name == NULL) {
				var_name = flag_value;
			} else {
				fprintf(stderr, "%s can only be specified once\n", FLAG_VARNAME);
				return 1;
			}
		} else if ((flag_value = PARSE_FLAG(arg, FLAG_INVALID)) != NULL) {
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

	if (input_path == NULL) {
		fprintf(stderr, "Please specify an input\n");
		return 1;
	}

	if (output_header_path != NULL && var_name == NULL) {
		fprintf(stderr, "%s also requires %s\n", FLAG_HEADER_OUT, FLAG_VARNAME);
		return 1;
	}

	cute_shader_init();

	cute_shader_file_t builtin_includes[sizeof(s_builtin_includes) / sizeof(s_builtin_includes[0]) + 1];
	int num_builtin_includes = sizeof(s_builtin_includes) / sizeof(s_builtin_includes[0]);
	for (int i = 0; i < num_builtin_includes; ++i) {
		builtin_includes[i] = s_builtin_includes[i];
	}

	cute_shader_config_t config = {
		.num_builtin_includes = num_builtin_includes,
		.num_include_dirs = num_includes,
		.include_dirs = include_dirs,
		.automatic_include_guard = true,
	};

	int return_code = 1;
	char* input_content = read_file(input_path);
	if (input_content == NULL) {
		perror("Error while reading input");
		goto end;
	}

	if (type == SHADER_TYPE_DRAW) {
		builtin_includes[num_builtin_includes++] = {
			.name = "shader_stub.shd",
			.content = input_content,
		};

		cute_shader_config_t config = {
			.num_builtin_includes = num_builtin_includes,
			.builtin_includes = builtin_includes,

			.num_include_dirs = num_includes,
			.include_dirs = include_dirs,

			.automatic_include_guard = true,
		};

		cute_shader_result_t draw_shader_result = cute_shader_compile(
			s_draw_fs,
			CUTE_SHADER_STAGE_FRAGMENT,
			config
		);
		if (!draw_shader_result.success) {
			fprintf(stderr, "%s\n", draw_shader_result.error_message);
			cute_shader_free_result(draw_shader_result);
			goto end;
		}
		cute_shader_result_t blit_shader_result = cute_shader_compile(
			s_blit_fs,
			CUTE_SHADER_STAGE_FRAGMENT,
			config
		);
		if (!blit_shader_result.success) {
			fprintf(stderr, "%s\n", blit_shader_result.error_message);
			cute_shader_free_result(draw_shader_result);
			cute_shader_free_result(blit_shader_result);
			goto end;
		}

		if (output_header_path != NULL) {
			if (!write_draw_header_file(
				output_header_path,
				draw_shader_result, blit_shader_result,
				argc, argv,
				var_name
			)) {
				perror("Error while writing header");
				cute_shader_free_result(draw_shader_result);
				cute_shader_free_result(blit_shader_result);
				goto end;
			}
		}

		cute_shader_free_result(draw_shader_result);
		cute_shader_free_result(blit_shader_result);
	} else {
		builtin_includes[num_builtin_includes++] = {
			.name = "shader_stub.shd",
			.content = s_shader_stub,
		};

		cute_shader_config_t config = {
			.num_builtin_includes = num_builtin_includes,
			.builtin_includes = builtin_includes,

			.num_include_dirs = num_includes,
			.include_dirs = include_dirs,

			.automatic_include_guard = true,
		};

		cute_shader_result_t result = cute_shader_compile(
			input_content,
			type == SHADER_TYPE_FRAGMENT
				? CUTE_SHADER_STAGE_FRAGMENT
				: CUTE_SHADER_STAGE_VERTEX,
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
				argc, argv,
				var_name
			)) {
				perror("Error while writing header");
				cute_shader_free_result(result);
				goto end;
			}
		}

		cute_shader_free_result(result);
	}

	return_code = 0;
end:
	if (input_content != NULL) { free(input_content); }

	cute_shader_cleanup();
	return return_code;
}
