#include "cute_shader.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unordered_set>
#include <unordered_map>
#include <glslang/Public/ShaderLang.h>
#include <glslang/Public/ResourceLimits.h>
#include <StandAlone/DirStackFileIncluder.h>
#include <SPIRV/GlslangToSpv.h>

#define CUTE_SHADER_GLSL_VERSION 450

namespace cute_shader {

struct BuiltinInclude {
	const char* content;
	size_t size;
};

class Includer: public glslang::TShader::Includer {
public:
	Includer(bool automatic_include_guard): automatic_include_guard(automatic_include_guard) {
	}

	void add_include_dir(const char *path) {
		dir_stack_includer.pushExternalLocalDirectory(path);
	}

	void add_builtin_include(const char* name, const char* content) {
		builtin_includes.insert({ name, { content, strlen(content) }});
	}

	IncludeResult* includeSystem(const char* header_name, const char* includer_name, size_t inclusion_depth) override {
		// Include guard has to be done against full path so we resolve first
		// and guard later
		auto builtin_include = builtin_includes.find(header_name);
		IncludeResult* result = nullptr;
		if (builtin_include == builtin_includes.end()) {
			result = dir_stack_includer.includeSystem(header_name, includer_name, inclusion_depth);
		} else {
			std::string resolved_name = "builtin:";
			resolved_name += header_name;
			printf("Include %s\n", header_name);
			result = new IncludeResult(
				resolved_name,
				builtin_include->second.content,
				builtin_include->second.size,
				nullptr
			);
		}

		return include_guard(result);
	}

	IncludeResult* includeLocal(const char* header_name, const char* includer_name, size_t inclusion_depth) override {
		return include_guard(dir_stack_includer.includeLocal(header_name, includer_name, inclusion_depth));
	}

	void releaseInclude(IncludeResult* result) override {
		if (result == nullptr) { return; }

		if (result->userData != nullptr) {
			dir_stack_includer.releaseInclude(result);
		} else {
			delete result;
		}
	}
private:
	IncludeResult* include_guard(IncludeResult* include) {
		if (include == nullptr || !automatic_include_guard) { return include; }

		if (included_files.contains(include->headerName)) {
			IncludeResult* guarded = new IncludeResult(
				include->headerName,
				"",
				0,
				nullptr
			);
			releaseInclude(include);
			return guarded;
		} else {
			included_files.insert(include->headerName);
			return include;
		}
	}

	bool automatic_include_guard;
	DirStackFileIncluder dir_stack_includer;
	std::unordered_set<std::string> included_files;
	std::unordered_map<std::string, BuiltinInclude> builtin_includes;
};

}

static cute_shader_result_t
cute_shader_failure(const char* message, const char* info_log, const char* debug_log) {
	int msg_size = snprintf(NULL, 0, "%s\n%s\n\n%s\n", message, info_log, debug_log);
	// TODO: use allocator?
	char* full_msg = (char*)malloc(msg_size + 1);
	snprintf(full_msg, msg_size, "%s\n%s\n\n%s\n", message, info_log, debug_log);
	full_msg[msg_size] = '\0';

	cute_shader_result_t result;
	result.success = false;
	result.bytecode = NULL;
	result.bytecode_size = 0;
	result.error_message = full_msg;
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
	shader.setPreamble(preamble.c_str());

	// Setup include
	cute_shader::Includer includer(config.automatic_include_guard);
	for (int i = 0; i < config.num_builtin_includes; ++i) {
		includer.add_builtin_include(
			config.builtin_includes[i].name, config.builtin_includes[i].content
		);
	}
	for (int i = 0; i < config.num_include_dirs; ++i) {
		includer.add_include_dir(config.include_dirs[i]);
	}

	// Preprocess
	shader.setStrings(&source, 1);
	std::string preprocessed_source;
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

	size_t bytecode_size = sizeof(uint32_t) * spirv.size();
	void* bytecode = malloc(bytecode_size);
	memcpy(bytecode, spirv.data(), bytecode_size);
	cute_shader_result_t result;
	result.success = true;
	result.bytecode = bytecode;
	result.bytecode_size = bytecode_size;
	result.error_message = NULL;
	return result;
}

void
cute_shader_free_result(cute_shader_result_t result) {
	free(result.bytecode);
	free((char*)result.error_message);
}
