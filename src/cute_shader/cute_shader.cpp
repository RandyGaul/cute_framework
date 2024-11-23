#include "cute_shader.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unordered_set>
#include <unordered_map>
#include <glslang/Public/ShaderLang.h>
#include <glslang/Public/ResourceLimits.h>
#include <SPIRV/GlslangToSpv.h>

#define CUTE_SHADER_GLSL_VERSION 450

namespace cute_shader {

static char s_empty = 0;

static char* libc_read_file_content(const char* path, size_t* len, void* context) {
	(void)context;
	FILE* file = fopen(path, "rb");
	if (file == NULL) { return NULL; }

	if (fseek(file, 0, SEEK_END) != 0) {
		fclose(file);
		return NULL;
	}
	long size = ftell(file);
	if (size < 0) {
		fclose(file);
		return NULL;
	}

	// malloc(0) can return NULL which is the same as error
	if (size == 0) {
		if (len != NULL) { *len = 0; }
		fclose(file);
		return &s_empty;
	}
	if (fseek(file, 0, SEEK_SET) != 0) {
		fclose(file);
		return NULL;
	}

	char* content = (char*)malloc(size);
	fread(content, (size_t)size, 1, file);
	if (ferror(file) != 0) {
		fclose(file);
		return NULL;
	}

	if (len != 0) { *len = (size_t)size; }
	return content;
}

static void libc_free_file_content(char* content, void* context) {
	if (content != &s_empty) {
		free(content);
	}
}

static cute_shader_vfs_t libc_vfs = {
	.read_file_content = libc_read_file_content,
	.free_file_content = libc_free_file_content,
};

struct BuiltinInclude {
	const char* content;
	size_t size;
};

class Includer: public glslang::TShader::Includer {
public:
	Includer(const cute_shader_config_t& config)
		: automatic_include_guard(config.automatic_include_guard)
		, vfs(config.vfs != NULL ? config.vfs : &libc_vfs)
		, num_include_dirs(config.num_include_dirs)
		, include_dirs(config.include_dirs)
	{
		for (int i = 0; i < config.num_builtin_includes; ++i) {
			cute_shader_file_t file = config.builtin_includes[i];
			builtin_includes.insert({
				file.name,
				{ file.content, strlen(file.content) }
			});
		}
	}

	IncludeResult* includeSystem(const char* header_name, const char* includer_name, size_t inclusion_depth) override {
		std::string include_name = header_name;

		// Since all includes are system include, we can resolve include guard immediately
		if (automatic_include_guard && included_files.contains(include_name)) {
			return new IncludeResult(include_name, "", 0, nullptr);
		}

		auto builtin_include = builtin_includes.find(include_name);
		IncludeResult* result = nullptr;
		if (builtin_include == builtin_includes.end()) {
			for (int i = 0; i < num_include_dirs; ++i) {
				std::string file_path = include_dirs[i];
				file_path += "/";
				file_path += include_name;
				size_t len;
				char* content = vfs->read_file_content(file_path.c_str(), &len, vfs->context);
				if (content != NULL) {
					if (automatic_include_guard) {
						included_files.insert(include_name);
					}
					return new IncludeResult(include_name, content, len, content);
				}
			}

			return nullptr;
		} else {
			if (automatic_include_guard) {
				included_files.insert(include_name);
			}
			return new IncludeResult(
				include_name,
				builtin_include->second.content,
				builtin_include->second.size,
				nullptr
			);
		}
	}

	IncludeResult* includeLocal(const char* header_name, const char* includer_name, size_t inclusion_depth) override {
		// Treat all includes as system include
		return nullptr;
	}

	void releaseInclude(IncludeResult* result) override {
		if (result == nullptr) { return; }

		if (result->userData != nullptr) {
			vfs->free_file_content((char*)result->userData, vfs->context);
		}

		delete result;
	}
private:
	bool automatic_include_guard;
	int num_include_dirs;
	const char** include_dirs;
	std::unordered_set<std::string> included_files;
	std::unordered_map<std::string, BuiltinInclude> builtin_includes;
	cute_shader_vfs_t* vfs;
};

}

static cute_shader_result_t
cute_shader_failure(const char* message, const char* info_log, const char* debug_log) {
	int msg_size = snprintf(NULL, 0, "%s\n%s\n\n%s\n", message, info_log, debug_log);
	// TODO: use allocator?
	char* full_msg = (char*)malloc(msg_size + 1);
	snprintf(full_msg, msg_size, "%s\n%s\n\n%s\n", message, info_log, debug_log);
	full_msg[msg_size] = '\0';

	cute_shader_result_t result = {
		.success = false,
		.error_message = full_msg,
	};
	return result;
}

void
cute_shader_init(void) {
	glslang::InitializeProcess();
}

void
cute_shader_cleanup(void) {
	glslang::FinalizeProcess();
}

cute_shader_result_t
cute_shader_compile(
	const char* source,
	cute_shader_stage_t stage,
	cute_shader_config_t config
) {
	EShLanguage glslang_stage = EShLangVertex;
	switch (stage) {
		case CUTE_SHADER_STAGE_VERTEX: glslang_stage = EShLangVertex; break;
		case CUTE_SHADER_STAGE_FRAGMENT: glslang_stage = EShLangFragment; break;
	}

	glslang::TShader shader(glslang_stage);
	shader.setEnvInput(glslang::EShSourceGlsl, glslang_stage, glslang::EShClientVulkan, CUTE_SHADER_GLSL_VERSION);
	shader.setEnvClient(glslang::EShClientVulkan, glslang::EShTargetVulkan_1_3);
	shader.setEnvTarget(glslang::EShTargetSpv, glslang::EShTargetSpv_1_3);
	shader.setEntryPoint("main");
	shader.setSourceEntryPoint("main");
	shader.setAutoMapLocations(true);
	shader.setAutoMapBindings(true);

	// Generate preamble for builtin defines
	std::string preamble;
	preamble += "#extension GL_ARB_shading_language_include : require\n";
	for (int i = 0; i < config.num_builtin_defines; ++i) {
		preamble += "#define ";
		preamble += config.builtin_defines[i].name;
		preamble += " ";
		preamble += config.builtin_defines[i].value;
		preamble += "\n";
	}
	preamble += "#line 1 0\n";
	shader.setPreamble(preamble.c_str());

	// Preprocess
	shader.setStrings(&source, 1);
	std::string preprocessed_source;
	cute_shader::Includer includer(config);
	if (!shader.preprocess(
		GetDefaultResources(),
		CUTE_SHADER_GLSL_VERSION,
		ENoProfile,
		false,
		false,
		EShMsgDefault,
		&preprocessed_source,
		includer
	)) {
		return cute_shader_failure(
			"Preprocessing failed",
			shader.getInfoLog(),
			shader.getInfoDebugLog()
		);
	}

	// Parse
	const char* new_source = preprocessed_source.c_str();
	shader.setStrings(&new_source, 1);
	if (!shader.parse(
		GetDefaultResources(),
		CUTE_SHADER_GLSL_VERSION,
		false,
		EShMsgDefault
	)) {
		return cute_shader_failure(
			"Parsing failed",
			shader.getInfoLog(),
			shader.getInfoDebugLog()
		);
	}

	// Link
	glslang::TProgram program;
	program.addShader(&shader);
	if (!program.link(EShMsgDefault)) {
		return cute_shader_failure(
			"Linking failed",
			program.getInfoLog(),
			program.getInfoDebugLog()
		);
	}

	std::vector<uint32_t> spirv;
	glslang::SpvOptions options;
	options.generateDebugInfo = false;
	options.stripDebugInfo = false;
	options.disableOptimizer = false;
	options.optimizeSize = false;
	options.disassemble = false;
	options.validate = false;
	glslang::GlslangToSpv(*program.getIntermediate(glslang_stage), spirv, &options);

	char* preprocessed_source_copy = NULL;
	size_t preprocessed_source_size = 0;
	if (config.return_preprocessed_source) {
		preprocessed_source_size = preprocessed_source.size();
		preprocessed_source_copy = (char*)malloc(preprocessed_source_size + 1);
		memcpy(preprocessed_source_copy, preprocessed_source.c_str(), preprocessed_source_size);
		preprocessed_source_copy[preprocessed_source_size] = '\0';
	}

	size_t bytecode_size = sizeof(uint32_t) * spirv.size();
	void* bytecode = malloc(bytecode_size);
	memcpy(bytecode, spirv.data(), bytecode_size);
	cute_shader_result_t result = {
		.success = true,
		.bytecode = bytecode,
		.bytecode_size = bytecode_size,
		.preprocessed_source = preprocessed_source_copy,
		.preprocessed_source_size = preprocessed_source_size,
	};
	return result;
}

void
cute_shader_free_result(cute_shader_result_t result) {
	free(result.bytecode);
	free((char*)result.preprocessed_source);
	free((char*)result.error_message);
}
