#include <cute.h>
using namespace Cute;

#include <glslang/Public/ShaderLang.h>
#include <glslang/Public/ResourceLimits.h>
#include <SPIRV/GlslangToSpv.h>
#define SDL_GPU_SHADERCROSS_IMPLEMENTATION
#define SDL_GPU_SHADERCROSS_STATIC
#include <SDL_gpu_shadercross/SDL_gpu_shadercross.h>
#include <SDL_gpu_shadercross/spirv.h>
#include <SPIRV-Reflect/spirv_reflect.h>

#include <internal/cute_app_internal.h>

// This isn't really a sample, but a scratch pad for the CF author to experiment.

/**
 * @enum     CF_ShaderStage
 * @category graphics
 * @brief    
 * @related  
 */
#define CF_SHADER_STAGE_DEFS \
	/* @entry */ \
	CF_ENUM(SHADER_STAGE_VERTEX,   0) \
	/* @entry */ \
	CF_ENUM(SHADER_STAGE_FRAGMENT, 1) \
	/* @entry */ \
	CF_ENUM(SHADER_STAGE_GEOMETRY, 2) \
	/* @entry */ \
	CF_ENUM(SHADER_STAGE_COMPUTE,  3) \
	/* @entry */ \
	CF_ENUM(SHADER_STAGE_COUNT,    4) \
	/* @end */

typedef enum CF_ShaderStage
{
	#define CF_ENUM(K, V) CF_##K = V,
	CF_SHADER_STAGE_DEFS
	#undef CF_ENUM
} CF_ShaderStage;

dyna uint32_t* cf_compile_shader_to_bytecode(const char* shader_src, CF_ShaderStage cf_stage)
{
	EShLanguage stage = EShLangVertex;
	switch (cf_stage) {
	default: CF_ASSERT(false); break; // No valid stage provided.
	case CF_SHADER_STAGE_VERTEX: stage = EShLangVertex; break;
	case CF_SHADER_STAGE_FRAGMENT: stage = EShLangFragment; break;
	case CF_SHADER_STAGE_GEOMETRY: stage = EShLangGeometry; break;
	case CF_SHADER_STAGE_COMPUTE: stage = EShLangCompute; break;
	}

	glslang::TShader shader(stage);

	const char* shader_strings[1];
	shader_strings[0] = shader_src;
	shader.setStrings(shader_strings, 1);

	shader.setEnvInput(glslang::EShSourceGlsl, stage, glslang::EShClientVulkan, 450);
	shader.setEnvClient(glslang::EShClientVulkan, glslang::EShTargetVulkan_1_2);
	shader.setEnvTarget(glslang::EShTargetSpv, glslang::EShTargetSpv_1_6);
	shader.setEntryPoint("main");
	shader.setSourceEntryPoint("main");
	shader.setAutoMapLocations(true);
	shader.setAutoMapBindings(true);

	if (!shader.parse(GetDefaultResources(), 450, false, EShMsgDefault)) {
		fprintf(stderr, "GLSL parsing failed...\n");
		fprintf(stderr, "%s\n\n%s\n", shader.getInfoLog(), shader.getInfoDebugLog());
		return NULL;
	}

	glslang::TProgram program;
	program.addShader(&shader);

	if (!program.link(EShMsgDefault)) {
		fprintf(stderr, "GLSL linking failed...\n");
		fprintf(stderr, "%s\n\n%s\n", program.getInfoLog(), program.getInfoDebugLog());
		return NULL;
	}

	std::vector<uint32_t> spirv;
	glslang::SpvOptions options;
	options.generateDebugInfo = false;
	options.stripDebugInfo = false;
	options.disableOptimizer = false;
	options.optimizeSize = false;
	options.disassemble = false;
	options.validate = false;
	glslang::GlslangToSpv(*program.getIntermediate(stage), spirv, &options);

	dyna uint32_t* bytecode = NULL;
	afit(bytecode, (int)spirv.size());
	CF_MEMCPY(bytecode, spirv.data(), sizeof(uint32_t) * spirv.size());
	alen(bytecode) = (int)spirv.size();

	return bytecode;
}

const char* spv_type_string(const SpvReflectTypeDescription* type_desc)
{
	switch (type_desc->op) {
		case SpvOpTypeVoid: return "void";
		case SpvOpTypeBool: return "bool";
		case SpvOpTypeInt:
			return type_desc->traits.numeric.scalar.signedness ? "int" : "uint";
		case SpvOpTypeFloat: return "float";
		case SpvOpTypeVector:
			if (type_desc->type_flags & SPV_REFLECT_TYPE_FLAG_INT) {
			} else {
				switch (type_desc->traits.numeric.vector.component_count) {
					case 2: return "vec2";
					case 3: return "vec3";
					case 4: return "vec4";
					default: return "unknown";
				}
			}
		case SpvOpTypeMatrix:
			return "mat";
		case SpvOpTypeStruct:
			return "struct";
		case SpvOpTypeArray:
			return "array";
		default:
			return "unknown";
	}
}

int main(int argc, char* argv[])
{
	int w = 640;
	int h = 480;
	make_app("Development Scratch", 0, 0, 0, w, h, APP_OPTIONS_WINDOW_POS_CENTERED, argv[0]);
	cf_clear_color(0, 0, 0, 0);

	glslang::InitializeProcess();

	const char* shader_src = R"(
		#version 450
		layout (location = 0) out vec4 v_color;

		layout(binding = 0) uniform fs_params {
			vec4 u_color;
		};

		void main()
		{
			vec2 pos;

			if (gl_VertexIndex == 0)
			{
				pos = vec2(-1, -1);
				v_color = vec4(1, 0, 0, 1);
			}
			else if (gl_VertexIndex == 1)
			{
				pos = vec2(1, -1);
				v_color = vec4(0, 1, 0, 1);
			}
			else if (gl_VertexIndex == 2)
			{
				pos = vec2(0, 1);
				v_color = vec4(0, 0, 1, 1);
			}

			v_color = u_color;
			gl_Position = vec4(pos, 0, 1) * u_color;
		})";

	dyna uint32_t* bytecode = cf_compile_shader_to_bytecode(shader_src, CF_SHADER_STAGE_VERTEX);

	SpvReflectShaderModule module;
	SpvReflectResult result = spvReflectCreateShaderModule(asize(bytecode) * sizeof(uint32_t), bytecode, &module);

	// Enumerate descriptor bindings (uniforms)
	uint32_t binding_count = 0;
	result = spvReflectEnumerateDescriptorBindings(&module, &binding_count, nullptr);

	SpvReflectDescriptorBinding** bindings = NULL;
	afit(bindings, (int)binding_count);
	alen(bindings) = binding_count;
	result = spvReflectEnumerateDescriptorBindings(&module, &binding_count, bindings);

	int samplerCount = 0;
	int storageTextureCount = 0;
	int storageBufferCount = 0;
	int uniformBufferCount = 0;

	// Check if the name is correctly captured
	for (int i = 0; i < (int)binding_count; ++i) {
		SpvReflectDescriptorBinding* binding = bindings[i];
		
		switch (binding->descriptor_type) {
			case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLER: samplerCount++; break;
			case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_IMAGE: storageTextureCount++; break;
			case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER: storageBufferCount++; break;
			case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER: uniformBufferCount++; break;
		}

		// Check if the binding is a uniform block
		if (binding->descriptor_type == SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER) {
			printf("Uniform Block: %s\n", binding->name);

			// Enumerate the members of the uniform block
			uint32_t member_count = binding->block.member_count;
			for (uint32_t j = 0; j < member_count; ++j) {
				SpvReflectBlockVariable* member = &(binding->block.members[j]);

				// Print member name and type
				printf("  Member: %s\n", member->name);
				printf("  Type: %s\n", spv_type_string(member->type_description));
				printf("  Size: %d bytes\n", member->size);
			}
		}
	}
	afree(bindings);

	SDL_GpuShaderCreateInfo shaderCreateInfo = {};
	shaderCreateInfo.codeSize = asize(bytecode) * sizeof(*bytecode);
	shaderCreateInfo.code = (uint8_t*)bytecode;
	shaderCreateInfo.entryPointName = "main";
	shaderCreateInfo.format = SDL_GPU_SHADERFORMAT_DXBC;
	shaderCreateInfo.stage = SDL_GPU_SHADERSTAGE_VERTEX;
	shaderCreateInfo.samplerCount = samplerCount;
	shaderCreateInfo.storageTextureCount = storageTextureCount;
	shaderCreateInfo.storageBufferCount = storageBufferCount;
	shaderCreateInfo.uniformBufferCount = uniformBufferCount;

	// @NOTE " vs < in SDL_gpu_shadercross
	// @NOTE Why is this returning void*?
	// @NOTE newCreateInfo.code = (const uint8_t*)blob->lpVtbl->GetBufferPointer(blob);
	// @NOTE newCreateInfo.code = (const uint8_t*)blob->lpVtbl->GetBufferPointer(blob);
	// @QUESTION Can I just call SDL_GpuCreateShader if I'm targeting Vulkan? It's weird and annoying to have to if-else to SDL_GpuCreateShader.
	// @NOTE spirvcross_dll -- Add an option to SFH in case the user has static linked SPIRV-Cross.
	SDL_GpuShader* shader = (SDL_GpuShader*)SDL_CompileFromSPIRV(app->dev, &shaderCreateInfo, false);

	while (app_is_running()) {
		app_update();

		app_draw_onto_screen();
	}

	glslang::FinalizeProcess();
	destroy_app();

	return 0;
}

#include <SPIRV-Reflect/spirv_reflect.c>
