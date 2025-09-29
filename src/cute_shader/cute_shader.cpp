// This file hides a lot of glslang, and related, dependencies. It's a bit gnarly, but
// totally temporary until SDL_Gpu gets its shader tools up and going.
//
// See: https://randygaul.github.io/cute_framework/topics/shader_compilation

#include "cute_shader.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <glslang/Public/ShaderLang.h>
#include <glslang/Public/ResourceLimits.h>
#include <SPIRV/GlslangToSpv.h>
#include <SDL3_shadercross/spirv.h>
#include <SPIRV-Reflect/spirv_reflect.h>

spvc_context g_spvc_context = NULL;

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
	fclose(file);
	return content;
}

static void libc_free_file_content(char* content, void* context) {
	if (content != &s_empty) {
		free(content);
	}
}

static CF_ShaderCompilerVfs libc_vfs = {
	.read_file_content = libc_read_file_content,
	.free_file_content = libc_free_file_content,
};

struct BuiltinInclude
{
	const char* content;
	size_t size;
};

class Includer: public glslang::TShader::Includer {
public:
	Includer(const CF_ShaderCompilerConfig& config)
		: automatic_include_guard(config.automatic_include_guard)
		, vfs(config.vfs != NULL ? config.vfs : &libc_vfs)
		, num_include_dirs(config.num_include_dirs)
		, include_dirs(config.include_dirs)
	{
		for (int i = 0; i < config.num_builtin_includes; ++i) {
			CF_ShaderCompilerFile file = config.builtin_includes[i];
			builtin_includes.insert({
				file.name,
				{ file.content, strlen(file.content) }
			});
		}
	}

	IncludeResult* includeSystem(const char* header_name, const char* includer_name, size_t inclusion_depth) override
	{
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

	IncludeResult* includeLocal(const char* header_name, const char* includer_name, size_t inclusion_depth) override
	{
		// Treat all includes as system include
		return nullptr;
	}

	void releaseInclude(IncludeResult* result) override
	{
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
	CF_ShaderCompilerVfs* vfs;
};

static CF_ShaderInfoDataType s_uniform_vector_type(SpvReflectTypeDescription* type_desc)
{
	if (type_desc->traits.numeric.scalar.width == 32) {
		if (type_desc->traits.numeric.scalar.signedness == 0) {
			switch (type_desc->traits.numeric.vector.component_count) {
				case 2: return CF_SHADER_INFO_TYPE_FLOAT2;
				case 3: return CF_SHADER_INFO_TYPE_FLOAT3;
				case 4: return CF_SHADER_INFO_TYPE_FLOAT4;
				default: return CF_SHADER_INFO_TYPE_UNKNOWN;
			}
		} else {
			switch (type_desc->traits.numeric.vector.component_count) {
				case 2: return CF_SHADER_INFO_TYPE_SINT2;
				case 3: return CF_SHADER_INFO_TYPE_SINT3;
				case 4: return CF_SHADER_INFO_TYPE_SINT4;
				default: return CF_SHADER_INFO_TYPE_UNKNOWN;
			}
		}
	}
	return CF_SHADER_INFO_TYPE_UNKNOWN;
}

static CF_ShaderInfoDataType s_uniform_type(SpvReflectTypeDescription* type_desc)
{
	switch (type_desc->op) {
	case SpvOpTypeFloat: return CF_SHADER_INFO_TYPE_FLOAT;
	case SpvOpTypeInt: return CF_SHADER_INFO_TYPE_SINT;
	case SpvOpTypeVector: return s_uniform_vector_type(type_desc);
	case SpvOpTypeMatrix:
		if (type_desc->traits.numeric.matrix.column_count == 4 && type_desc->traits.numeric.matrix.row_count == 4)
			return CF_SHADER_INFO_TYPE_MAT4;
		break;
	case SpvOpTypeArray:
		if (type_desc->type_flags & SPV_REFLECT_TYPE_FLAG_VECTOR) {
			return s_uniform_vector_type(type_desc);
		} else if (type_desc->type_flags & SPV_REFLECT_TYPE_FLAG_MATRIX) {
			if (type_desc->traits.numeric.matrix.column_count == 4 && type_desc->traits.numeric.matrix.row_count == 4 && (type_desc->type_flags & SPV_REFLECT_TYPE_FLAG_FLOAT))
				return CF_SHADER_INFO_TYPE_MAT4;
			break;
		} else if (type_desc->type_flags & SPV_REFLECT_TYPE_FLAG_INT) {
			return CF_SHADER_INFO_TYPE_SINT;
		} else if (type_desc->type_flags & SPV_REFLECT_TYPE_FLAG_FLOAT) {
			return CF_SHADER_INFO_TYPE_FLOAT;
		}
		break;
	default:
		return CF_SHADER_INFO_TYPE_UNKNOWN;
	}
	return CF_SHADER_INFO_TYPE_UNKNOWN;
}

static CF_ShaderInfoDataType s_wrap(SpvReflectFormat format)
{
	switch (format) {
	case SPV_REFLECT_FORMAT_UNDEFINED:           return CF_SHADER_INFO_TYPE_UNKNOWN;
	case SPV_REFLECT_FORMAT_R32_UINT:            return CF_SHADER_INFO_TYPE_UINT;
	case SPV_REFLECT_FORMAT_R32_SINT:            return CF_SHADER_INFO_TYPE_SINT;
	case SPV_REFLECT_FORMAT_R32_SFLOAT:          return CF_SHADER_INFO_TYPE_FLOAT;
	case SPV_REFLECT_FORMAT_R32G32_UINT:         return CF_SHADER_INFO_TYPE_UINT2;
	case SPV_REFLECT_FORMAT_R32G32_SINT:         return CF_SHADER_INFO_TYPE_SINT2;
	case SPV_REFLECT_FORMAT_R32G32_SFLOAT:       return CF_SHADER_INFO_TYPE_FLOAT2;
	case SPV_REFLECT_FORMAT_R32G32B32_UINT:      return CF_SHADER_INFO_TYPE_UINT3;
	case SPV_REFLECT_FORMAT_R32G32B32_SINT:      return CF_SHADER_INFO_TYPE_SINT3;
	case SPV_REFLECT_FORMAT_R32G32B32_SFLOAT:    return CF_SHADER_INFO_TYPE_FLOAT3;
	case SPV_REFLECT_FORMAT_R32G32B32A32_UINT:   return CF_SHADER_INFO_TYPE_UINT4;
	case SPV_REFLECT_FORMAT_R32G32B32A32_SINT:   return CF_SHADER_INFO_TYPE_SINT4;
	case SPV_REFLECT_FORMAT_R32G32B32A32_SFLOAT: return CF_SHADER_INFO_TYPE_FLOAT4;
	default: return CF_SHADER_INFO_TYPE_UNKNOWN;
	}
}

}

static CF_ShaderCompilerResult cute_shader_failure(const char* message, const char* info_log, const char* debug_log)
{
	int msg_size = snprintf(NULL, 0, "%s\n%s\n\n%s\n", message, info_log, debug_log);
	// TODO: use allocator?
	char* full_msg = (char*)malloc(msg_size + 1);
	snprintf(full_msg, msg_size, "%s\n%s\n\n%s\n", message, info_log, debug_log);
	full_msg[msg_size] = '\0';

	CF_ShaderCompilerResult result = {
		.success = false,
		.error_message = full_msg,
	};
	return result;
}

void cute_shader_init(void)
{
	glslang::InitializeProcess();
	if (spvc_context_create(&g_spvc_context) != SPVC_SUCCESS) {
		g_spvc_context = NULL;
	}
}

void cute_shader_cleanup(void)
{
	if (g_spvc_context) {
		spvc_context_destroy(g_spvc_context);
		g_spvc_context = NULL;
	}
	glslang::FinalizeProcess();
}

CF_ShaderCompilerResult cute_shader_compile(const char* source, CF_ShaderCompilerStage stage, CF_ShaderCompilerConfig config)
{
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

	// Reflection
	int num_samplers = 0;
	int num_storage_textures = 0;
	int num_storage_buffers = 0;
	int num_images;
	const char** image_names = NULL;
	int num_uniforms;
	CF_ShaderUniformInfo* uniforms = NULL;
	int num_uniform_members;
	CF_ShaderUniformMemberInfo* uniform_members = NULL;
	uint32_t num_inputs = 0;
	CF_ShaderInputInfo* inputs = NULL;
	{
		std::vector<const char*> image_names_vec;
		std::vector<CF_ShaderUniformInfo> uniforms_vec;
		std::vector<CF_ShaderUniformMemberInfo> uniform_members_vec;

		SpvReflectShaderModule module;
		spvReflectCreateShaderModule(bytecode_size, bytecode, &module);

		uint32_t num_bindings = 0;
		spvReflectEnumerateDescriptorBindings(&module, &num_bindings, nullptr);
		SpvReflectDescriptorBinding** bindings = (SpvReflectDescriptorBinding**)malloc(num_bindings * sizeof(SpvReflectDescriptorBinding*));
		spvReflectEnumerateDescriptorBindings(&module, &num_bindings, bindings);
		for (uint32_t binding_index = 0; binding_index < num_bindings; ++binding_index) {
			SpvReflectDescriptorBinding* binding = bindings[binding_index];

			switch (binding->descriptor_type) {
			case SPV_REFLECT_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
			{
				image_names_vec.push_back(strdup(binding->name));
			}    // Fall-thru.
			case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLER: ++num_samplers; break;
			case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_IMAGE: ++num_storage_textures; break;
			case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER: ++num_storage_buffers; break;
			case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
			{
				// Grab information about the uniform block.
				CF_ShaderUniformInfo uniform;
				uniform.block_index = binding->binding;
				uniform.block_size = binding->block.size;
				uniform.block_name = strdup(binding->type_description->type_name);
				uniform.num_members = binding->block.member_count;
				uniforms_vec.push_back(uniform);

				for (uint32_t member_index = 0; member_index < binding->block.member_count; ++member_index) {
					const SpvReflectBlockVariable* reflected_member = &binding->block.members[member_index];

					CF_ShaderUniformMemberInfo uniform_member;
					uniform_member.name = strdup(reflected_member->name);
					uniform_member.type = cute_shader::s_uniform_type(reflected_member->type_description);
					uniform_member.offset = (int)reflected_member->offset;

					uniform_member.array_length = 1;
					if (
						reflected_member->type_description->type_flags & SPV_REFLECT_TYPE_FLAG_ARRAY
						&& reflected_member->type_description->traits.array.dims_count > 0
					) {
						uniform_member.array_length = (int)reflected_member->type_description->traits.array.dims[0];
					}

					uniform_members_vec.push_back(uniform_member);
				}
			} break;
			default: break;
			}
		}
		free(bindings);

		// Prepare the returned reflection info
		num_images = (int)image_names_vec.size();
		if (num_images > 0) {
			image_names = (const char**)malloc(sizeof(char*) * num_images);
			memcpy(image_names, image_names_vec.data(), sizeof(char*) * num_images);
		}

		num_uniforms = (int)uniforms_vec.size();
		if (num_uniforms > 0) {
			uniforms = (CF_ShaderUniformInfo*)malloc(sizeof(CF_ShaderUniformInfo) * num_uniforms);
			memcpy(uniforms, uniforms_vec.data(), sizeof(CF_ShaderUniformInfo) * num_uniforms);
		}

		num_uniform_members = (int)uniform_members_vec.size();
		if (num_uniform_members > 0) {
			uniform_members = (CF_ShaderUniformMemberInfo*)malloc(sizeof(CF_ShaderUniformMemberInfo) * num_uniform_members);
			memcpy(uniform_members, uniform_members_vec.data(), sizeof(CF_ShaderUniformMemberInfo) * num_uniform_members);
		}

		// Gather up type information on shader inputs.
		if (stage == CUTE_SHADER_STAGE_VERTEX) {
			spvReflectEnumerateInputVariables(&module, &num_inputs, nullptr);
			SpvReflectInterfaceVariable** reflected_inputs = (SpvReflectInterfaceVariable**)malloc(num_inputs * sizeof(SpvReflectInterfaceVariable*));
			spvReflectEnumerateInputVariables(&module, &num_inputs, reflected_inputs);
			inputs = (CF_ShaderInputInfo*)malloc(sizeof(CF_ShaderInputInfo) * num_inputs);
			for (uint32_t input_index = 0; input_index < num_inputs; ++input_index) {
				SpvReflectInterfaceVariable* reflected_input = reflected_inputs[input_index];

				CF_ShaderInputInfo* input = &inputs[input_index];
				input->name = strdup(reflected_input->name);
				input->location = reflected_input->location;
				input->format = cute_shader::s_wrap(reflected_input->format);
			}
			free(reflected_inputs);
		}

		spvReflectDestroyShaderModule(&module);
	}

	CF_ShaderCompilerResult result = {
		.success = true,

		.bytecode = {
			.content = (uint8_t*)bytecode,
			.size = bytecode_size,

			.shader_info = {
				.num_samplers = num_samplers,
				.num_storage_textures = num_storage_textures,
				.num_storage_buffers = num_storage_buffers,

				.num_images = num_images,
				.image_names = image_names,

				.num_uniforms = num_uniforms,
				.uniforms = uniforms,

				.num_uniform_members = num_uniform_members,
				.uniform_members = uniform_members,

				.num_inputs = (int)num_inputs,
				.inputs = inputs,
			},
		},

		.preprocessed_source = preprocessed_source_copy,
		.preprocessed_source_size = preprocessed_source_size,
	};
	return result;
}

void cute_shader_free_result(CF_ShaderCompilerResult result)
{
	CF_ShaderInfo* shader_info = &result.bytecode.shader_info;
	for (int i = 0; i < shader_info->num_inputs; ++i) {
		free((char*)shader_info->inputs[i].name);
	}
	free(shader_info->inputs);

	for (int i = 0; i < shader_info->num_uniform_members; ++i) {
		free((char*)shader_info->uniform_members[i].name);
	}
	free(shader_info->uniform_members);

	for (int i = 0; i < shader_info->num_uniforms; ++i) {
		free((char*)shader_info->uniforms[i].block_name);
	}
	free(shader_info->uniforms);

	for (int i = 0; i < shader_info->num_images; ++i) {
		free((char*)shader_info->image_names[i]);
	}
	free(shader_info->image_names);

	free((void*)result.bytecode.content);
	free((char*)result.preprocessed_source);
	free((char*)result.error_message);
}

#include <SPIRV-Reflect/spirv_reflect.c>
