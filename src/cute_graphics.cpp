/*
	Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

#include <cute_defines.h>
#include <cute_c_runtime.h>
#include <cute_graphics.h>

#include <internal/cute_alloc_internal.h>
#include <internal/cute_app_internal.h>

#include <glslang/Public/ShaderLang.h>
#include <glslang/Public/ResourceLimits.h>
#include <SPIRV/GlslangToSpv.h>
#define SDL_GPU_SHADERCROSS_IMPLEMENTATION
#define SDL_GPU_SHADERCROSS_STATIC
#include <SDL_gpu_shadercross/SDL_gpu_shadercross.h>
#include <SDL_gpu_shadercross/spirv.h>
#include <SPIRV-Reflect/spirv_reflect.h>

struct CF_CanvasInternal;
static CF_CanvasInternal* s_canvas = NULL;
static CF_CanvasInternal* s_default_canvas = NULL;

#include <float.h>

using namespace Cute;

typedef enum CF_ShaderInputFormat
{
	CF_SHADER_INPUT_FORMAT_UNKNOWN,
	CF_SHADER_INPUT_FORMAT_UINT,
	CF_SHADER_INPUT_FORMAT_INT,
	CF_SHADER_INPUT_FORMAT_FLOAT,
	CF_SHADER_INPUT_FORMAT_UVEC2,
	CF_SHADER_INPUT_FORMAT_IVEC2,
	CF_SHADER_INPUT_FORMAT_VEC2,
	CF_SHADER_INPUT_FORMAT_UVEC3,
	CF_SHADER_INPUT_FORMAT_IVEC3,
	CF_SHADER_INPUT_FORMAT_VEC3,
	CF_SHADER_INPUT_FORMAT_UVEC4,
	CF_SHADER_INPUT_FORMAT_IVEC4,
	CF_SHADER_INPUT_FORMAT_VEC4,
} CF_ShaderInputFormat;

static int s_uniform_size(CF_UniformType type)
{
	switch (type) {
	case CF_UNIFORM_TYPE_FLOAT:  return 4;
	case CF_UNIFORM_TYPE_FLOAT2: return 8;
	case CF_UNIFORM_TYPE_FLOAT4: return 16;
	case CF_UNIFORM_TYPE_INT:    return 4;
	case CF_UNIFORM_TYPE_INT2:   return 8;
	case CF_UNIFORM_TYPE_INT4:   return 16;
	case CF_UNIFORM_TYPE_MAT4:   return 64;
	default:                     return 0;
	}
}

#define CF_MAX_SHADER_INPUTS (32)

struct CF_UniformBlockMember
{
	const char* name;
	CF_UniformType type;
	int array_element_count;
	int size; // In bytes. If an array, it's the size in bytes of the whole array.
	int offset;
};

struct CF_ShaderInternal
{
	SDL_GpuShader* vs;
	SDL_GpuShader* fs;
	int input_count;
	const char* input_names[CF_MAX_SHADER_INPUTS];
	int input_locations[CF_MAX_SHADER_INPUTS];
	CF_ShaderInputFormat input_formats[CF_MAX_SHADER_INPUTS];
	int vs_block_size;
	int fs_block_size;
	Array<CF_UniformBlockMember> fs_uniform_block_members;
	Array<CF_UniformBlockMember> vs_uniform_block_members;

	CF_INLINE int get_input_index(const char* name)
	{
		for (int i = 0; i < input_count; ++i) {
			if (input_names[i] == name) return i;
		}
		return -1;
	}

	CF_INLINE int fs_index(const char* name)
	{
		for (int i = 0; i < fs_uniform_block_members.size(); ++i) {
			if (fs_uniform_block_members[i].name == name) return i;
		}
		return -1;
	}

	CF_INLINE int vs_index(const char* name)
	{
		for (int i = 0; i < vs_uniform_block_members.size(); ++i) {
			if (vs_uniform_block_members[i].name == name) return i;
		}
		return -1;
	}
};

struct CF_Buffer
{
	bool was_appended;
	int element_count;
	int size;
	int offset;
	int stride;
	//sg_buffer handle;
};

struct CF_MeshInternal
{
	//sg_usage usage;
	bool need_vertex_sync;
	bool need_index_sync;
	bool need_instance_sync;
	CF_Buffer vertices;
	CF_Buffer indices;
	CF_Buffer instances;
	int attribute_count;
	CF_VertexAttribute attributes[CF_MESH_MAX_VERTEX_ATTRIBUTES];
};

struct CF_CanvasInternal
{
	CF_Texture cf_texture;
	CF_Texture cf_depth_stencil;
	SDL_GpuTexture* texture;
	SDL_GpuTexture* depth_stencil;
	CF_MeshInternal* mesh;

	SDL_GpuGraphicsPipeline* pip;
	SDL_GpuRenderPass* pass;
	SDL_GpuCommandBuffer* cmd;
};

CF_BackendType cf_query_backend()
{
	//sg_backend backend = sg_query_backend();
	return BACKEND_TYPE_D3D11;
}

bool cf_query_pixel_format(CF_PixelFormat format, CF_PixelFormatOp op)
{
	//sg_pixel_format sgfmt = SG_PIXELFORMAT_NONE;
	//switch (format) {
	//case CF_PIXELFORMAT_DEFAULT: sgfmt = _SG_PIXELFORMAT_DEFAULT; break;
	//default:                  sgfmt = s_wrap(format);          break;
	//}
	//
	//sg_pixelformat_info info = sg_query_pixelformat(sgfmt);
	//bool result = false;
	//
	//switch (op) {
	//case CF_PIXELFORMAT_OP_NEAREST_FILTER:  result = info.sample; break;
	//case CF_PIXELFORMAT_OP_BILINEAR_FILTER: result = info.filter; break;
	//case CF_PIXELFORMAT_OP_RENDER_TARGET:   result = info.render; break;
	//case CF_PIXELFORMAT_OP_ALPHA_BLENDING:  result = info.blend;  break;
	//case CF_PIXELFORMAT_OP_MSAA:            result = info.msaa;   break;
	//case CF_PIXELFORMAT_OP_DEPTH:           result = info.depth;  break;
	//}

	return false;
}

//bool cf_query_device_feature(CF_DeviceFeature feature)
//{
//	sg_features sgf = sg_query_features();
//	bool result = false;
//	switch (feature) {
//	case CF_DEVICE_FEATURE_TEXTURE_CLAMP:    result = sgf.image_clamp_to_border;       break;
//	}
//	return result;
//}

//int cf_query_resource_limit(CF_ResourceLimit resource_limit)
//{
//	sg_limits sgl = sg_query_limits();
//	int result = 0;
//	switch (resource_limit) {
//	case CF_RESOURCE_LIMIT_TEXTURE_DIMENSION:       result = sgl.max_image_size_2d; break;
//	case CF_RESOURCE_LIMIT_VERTEX_ATTRIBUTE_MAX:    result = sgl.max_vertex_attrs; break;
//	}
//	return result;
//}

CF_TextureParams cf_texture_defaults(int w, int h)
{
	CF_TextureParams params;
	params.pixel_format = CF_PIXELFORMAT_DEFAULT;
	params.filter = CF_FILTER_NEAREST;
	params.usage = CF_USAGE_TYPE_IMMUTABLE;
	params.wrap_u = CF_WRAP_MODE_DEFAULT;
	params.wrap_v = CF_WRAP_MODE_DEFAULT;
	params.width = w;
	params.height = h;
	params.render_target = false;
	params.initial_data = NULL;
	params.initial_data_size = 0;
	return params;
}

CF_Texture cf_make_texture(CF_TextureParams texture_params)
{
	SDL_GpuTextureCreateInfo tex_info;
	CF_MEMSET(&tex_info, 0, sizeof(tex_info));
	tex_info.width = (Uint32)texture_params.width;
	tex_info.height = (Uint32)texture_params.height;
	tex_info.sampleCount = SDL_GPU_SAMPLECOUNT_1;

	// WORKING HERE.

	SDL_GpuCreateTexture(app->device, &tex_info);
	//sg_image_desc desc;
	//CF_MEMSET(&desc, 0, sizeof(desc));
	//desc.type = SG_IMAGETYPE_2D;
	//desc.render_target = texture_params.render_target;
	//desc.width = texture_params.width;
	//desc.height = texture_params.height;
	//desc.num_slices = 0;
	//desc.num_mipmaps = 0;
	//desc.usage = s_wrap(texture_params.usage);
	//desc.pixel_format = s_wrap(texture_params.pixel_format);
	//desc.min_filter = s_wrap(texture_params.filter);
	//desc.mag_filter = s_wrap(texture_params.filter);
	//desc.wrap_u = s_wrap(texture_params.wrap_u);
	//desc.wrap_v = s_wrap(texture_params.wrap_v);
	//desc.wrap_w = s_wrap(texture_params.wrap_v);
	//desc.border_color = _SG_BORDERCOLOR_DEFAULT;
	//desc.max_anisotropy = 1;
	//desc.min_lod = 0;
	//desc.max_lod = FLT_MAX;
	//desc.data.subimage[0][0].ptr = texture_params.initial_data;
	//desc.data.subimage[0][0].size = texture_params.initial_data_size;
	//sg_image sgi = sg_make_image(desc);
	CF_Texture texture = { 0 };
	return texture;
}

void cf_destroy_texture(CF_Texture texture)
{
	//sg_image sgi = { (uint32_t)texture.id };
	//sg_destroy_image(sgi);
}

void cf_update_texture(CF_Texture texture, void* data, int size)
{
	//sg_image_data sgid = { };
	//sgid.subimage[0][0].ptr = data;
	//sgid.subimage[0][0].size = size;
	//sg_image sgi = { (uint32_t)texture.id };
	//sg_update_image(sgi, sgid);
}

const dyna uint8_t* cf_compile_shader_to_bytecode(const char* shader_src, CF_ShaderStage cf_stage)
{
	EShLanguage stage = EShLangVertex;
	switch (cf_stage) {
	default: CF_ASSERT(false); break; // No valid stage provided.
	case CF_SHADER_STAGE_VERTEX: stage = EShLangVertex; break;
	case CF_SHADER_STAGE_FRAGMENT: stage = EShLangFragment; break;
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

	dyna uint8_t* bytecode = NULL;
	int size = (int)(sizeof(uint32_t) * spirv.size());
	afit(bytecode, size);
	CF_MEMCPY(bytecode, spirv.data(), size);
	alen(bytecode) = size;

	return bytecode;
}

static CF_INLINE SDL_GpuShaderFormat s_wrap(CF_ShaderFormat format)
{
	switch (format) {
	case CF_SHADER_FORMAT_SECRET_NDA: return SDL_GPU_SHADERFORMAT_SECRET;
	case CF_SHADER_FORMAT_SPIRV: return SDL_GPU_SHADERFORMAT_SPIRV;
	case CF_SHADER_FORMAT_DXBC: return SDL_GPU_SHADERFORMAT_DXBC;
	case CF_SHADER_FORMAT_DXIL: return SDL_GPU_SHADERFORMAT_DXIL;
	case CF_SHADER_FORMAT_MSL: return SDL_GPU_SHADERFORMAT_MSL;
	default: return SDL_GPU_SHADERFORMAT_INVALID;
	}
}

static CF_INLINE SDL_GpuShaderStage s_wrap(CF_ShaderStage stage)
{
	switch (stage) {
	case CF_SHADER_STAGE_VERTEX: return SDL_GPU_SHADERSTAGE_VERTEX;
	case CF_SHADER_STAGE_FRAGMENT: return SDL_GPU_SHADERSTAGE_FRAGMENT;
	default: return SDL_GPU_SHADERSTAGE_VERTEX;
	}
}

static CF_ShaderInputFormat s_wrap(SpvReflectFormat format)
{
	switch (format) {
	case SPV_REFLECT_FORMAT_UNDEFINED:           return CF_SHADER_INPUT_FORMAT_UNKNOWN;
	case SPV_REFLECT_FORMAT_R32_UINT:            return CF_SHADER_INPUT_FORMAT_UINT;
	case SPV_REFLECT_FORMAT_R32_SINT:            return CF_SHADER_INPUT_FORMAT_INT;
	case SPV_REFLECT_FORMAT_R32_SFLOAT:          return CF_SHADER_INPUT_FORMAT_FLOAT;
	case SPV_REFLECT_FORMAT_R32G32_UINT:         return CF_SHADER_INPUT_FORMAT_UVEC2;
	case SPV_REFLECT_FORMAT_R32G32_SINT:         return CF_SHADER_INPUT_FORMAT_IVEC2;
	case SPV_REFLECT_FORMAT_R32G32_SFLOAT:       return CF_SHADER_INPUT_FORMAT_VEC2;
	case SPV_REFLECT_FORMAT_R32G32B32_UINT:      return CF_SHADER_INPUT_FORMAT_UVEC3;
	case SPV_REFLECT_FORMAT_R32G32B32_SINT:      return CF_SHADER_INPUT_FORMAT_IVEC3;
	case SPV_REFLECT_FORMAT_R32G32B32_SFLOAT:    return CF_SHADER_INPUT_FORMAT_VEC3;
	case SPV_REFLECT_FORMAT_R32G32B32A32_UINT:   return CF_SHADER_INPUT_FORMAT_UVEC4;
	case SPV_REFLECT_FORMAT_R32G32B32A32_SINT:   return CF_SHADER_INPUT_FORMAT_IVEC4;
	case SPV_REFLECT_FORMAT_R32G32B32A32_SFLOAT: return CF_SHADER_INPUT_FORMAT_VEC4;
	default: return CF_SHADER_INPUT_FORMAT_UNKNOWN;
	}
}

static CF_UniformType s_uniform_type(SpvReflectTypeDescription* type_desc)
{
	switch (type_desc->op) {
	case SpvOpTypeFloat: return CF_UNIFORM_TYPE_FLOAT;
	case SpvOpTypeInt: return CF_UNIFORM_TYPE_INT;
	case SpvOpTypeVector:
		if (type_desc->traits.numeric.scalar.width == 32) { // Check if it's a 32-bit type
			if (type_desc->traits.numeric.scalar.signedness == 0) { // unsigned int or float
				switch (type_desc->traits.numeric.vector.component_count) {
					case 2: return CF_UNIFORM_TYPE_FLOAT2; // Assuming vector is float
					case 4: return CF_UNIFORM_TYPE_FLOAT4; // Assuming vector is float
					default: return CF_UNIFORM_TYPE_UNKNOWN;
				}
			} else { // signed int
				switch (type_desc->traits.numeric.vector.component_count) {
					case 2: return CF_UNIFORM_TYPE_INT2;
					case 4: return CF_UNIFORM_TYPE_INT4;
					default: return CF_UNIFORM_TYPE_UNKNOWN;
				}
			}
		}
		break;
	case SpvOpTypeMatrix:
		if (type_desc->traits.numeric.matrix.column_count == 4 && type_desc->traits.numeric.matrix.row_count == 4)
			return CF_UNIFORM_TYPE_MAT4;
		break;
	default:
		return CF_UNIFORM_TYPE_UNKNOWN;
	}
	return CF_UNIFORM_TYPE_UNKNOWN;
}

static SDL_GpuShader* s_compile(CF_ShaderInternal* shader_internal, const dyna uint8_t* bytecode, CF_ShaderStage stage)
{
	bool vs = stage == CF_SHADER_STAGE_VERTEX ? true : false;
	SpvReflectShaderModule module;
	spvReflectCreateShaderModule(asize(bytecode), bytecode, &module);

	// Gather up counts for samplers/textures/buffers.
	// ...SDL_Gpu needs these counts.
	uint32_t binding_count = 0;
	spvReflectEnumerateDescriptorBindings(&module, &binding_count, nullptr);
	dyna SpvReflectDescriptorBinding** bindings = NULL;
	afit(bindings, (int)binding_count);
	alen(bindings) = binding_count;
	spvReflectEnumerateDescriptorBindings(&module, &binding_count, bindings);
	int sampler_count = 0;
	int storage_texture_count = 0;
	int storage_buffer_count = 0;
	int uniform_buffer_count = 0;
	for (int i = 0; i < (int)binding_count; ++i) {
		SpvReflectDescriptorBinding* binding = bindings[i];

		switch (binding->descriptor_type) {
			case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLER: sampler_count++; break;
			case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_IMAGE: storage_texture_count++; break;
			case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER: storage_buffer_count++; break;
			case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
			{
				uniform_buffer_count++;

				// Grab information about the uniform block.
				// ...This allows CF_Material to dynamically match uniforms to a shader.
				CF_ASSERT(sequ(binding->type_description->type_name, "uniform_block"));
				if (vs) {
					shader_internal->vs_block_size = binding->block.size;
				} else {
					shader_internal->fs_block_size = binding->block.size;
				}
				for (uint32_t i = 0; i < binding->block.member_count; ++i) {
					const SpvReflectBlockVariable* member = &binding->block.members[i];
					CF_UniformType uniform_type = s_uniform_type(member->type_description);
					CF_ASSERT(uniform_type != CF_UNIFORM_TYPE_UNKNOWN);
					int array_length = 1;
					if (member->type_description->type_flags & SPV_REFLECT_TYPE_FLAG_ARRAY && member->type_description->traits.array.dims_count > 0) {
						array_length = (int)member->type_description->traits.array.dims[0];
					}

					CF_UniformBlockMember block_member;
					block_member.name = sintern(member->name);
					block_member.type = uniform_type;
					block_member.array_element_count = array_length;
					block_member.size = s_uniform_size(block_member.type) * array_length;
					block_member.offset = (int)member->offset;
					if (vs) {
						shader_internal->vs_uniform_block_members.add(block_member);
					} else {
						shader_internal->fs_uniform_block_members.add(block_member);
					}
				}
			} break;
		}
	}
	afree(bindings);

	// Gather up type information on shader inputs.
	if (vs) {
		uint32_t input_count = 0;
		spvReflectEnumerateInputVariables(&module, &input_count, nullptr);
		CF_ASSERT(input_count <= CF_MAX_SHADER_INPUTS); // Increase `CF_MAX_SHADER_INPUTS`, or refactor the shader with less vertex attributes.
		shader_internal->input_count = input_count;
		dyna SpvReflectInterfaceVariable** inputs = NULL;
		afit(inputs, (int)input_count);
		alen(inputs) = (int)input_count;
		spvReflectEnumerateInputVariables(&module, &input_count, inputs);
		for (int i = 0; i < alen(inputs); ++i) {
			SpvReflectInterfaceVariable* input = inputs[i];

			shader_internal->input_names[i] = input->name;
			shader_internal->input_locations[i] = input->location;
			shader_internal->input_formats[i] = s_wrap(input->format);
		}
		afree(inputs);
	}

	// Create the actual shader.
	SDL_GpuShaderCreateInfo shaderCreateInfo = {};
	shaderCreateInfo.codeSize = asize(bytecode);
	shaderCreateInfo.code = bytecode;
	shaderCreateInfo.entryPointName = "main";
	shaderCreateInfo.format = SDL_GPU_SHADERFORMAT_SPIRV;
	shaderCreateInfo.stage = s_wrap(stage);
	shaderCreateInfo.samplerCount = sampler_count;
	shaderCreateInfo.storageTextureCount = storage_texture_count;
	shaderCreateInfo.storageBufferCount = storage_buffer_count;
	shaderCreateInfo.uniformBufferCount = uniform_buffer_count;
	SDL_GpuShader* sdl_shader = (SDL_GpuShader*)SDL_CompileFromSPIRV(app->device, &shaderCreateInfo, false);
	afree(bytecode);
	return sdl_shader;
}

CF_Shader cf_make_shader_from_bytecode(const dyna uint8_t* vertex_bytecode, const dyna uint8_t* fragment_bytecode)
{
	CF_ShaderInternal* shader_internal = CF_NEW(CF_ShaderInternal);
	CF_MEMSET(shader_internal, 0, sizeof(*shader_internal));

	shader_internal->vs = s_compile(shader_internal, vertex_bytecode, CF_SHADER_STAGE_VERTEX);
	shader_internal->fs = s_compile(shader_internal, fragment_bytecode, CF_SHADER_STAGE_FRAGMENT);

	CF_Shader result;
	result.id = { (uint64_t)shader_internal };
	return result;
}

CF_Shader cf_make_shader(CF_ShaderFormat format, const char* vertex, const char* fragment)
{
	const dyna uint8_t* vs_bytecode = cf_compile_shader_to_bytecode(vertex, CF_SHADER_STAGE_VERTEX);
	if (!vs_bytecode) {
		CF_Shader result = { 0 };
		return result;
	}

	const dyna uint8_t* fs_bytecode = cf_compile_shader_to_bytecode(fragment, CF_SHADER_STAGE_FRAGMENT);
	if (!fs_bytecode) {
		afree(vs_bytecode);
		CF_Shader result = { 0 };
		return result;
	}

	return cf_make_shader_from_bytecode(vs_bytecode, fs_bytecode);
}

void cf_destroy_shader(CF_Shader shader)
{
	CF_ShaderInternal* shader_internal = (CF_ShaderInternal*)shader.id;
	//sg_destroy_shader(shader_internal->shd);
	CF_FREE(shader_internal);
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
		params.target.render_target = true;
		params.depth_stencil_target = cf_texture_defaults(w, h);
		params.depth_stencil_target.pixel_format = CF_PIXELFORMAT_DEPTH_STENCIL;
		params.depth_stencil_target.render_target = true;
	}
	return params;
}

static void s_canvas_clear_settings(CF_CanvasInternal* canvas)
{
	//canvas->action.colors[0].load_action = SG_LOADACTION_CLEAR;
	//canvas->action.colors[0].clear_value.r = s_clear_red;
	//canvas->action.colors[0].clear_value.g = s_clear_green;
	//canvas->action.colors[0].clear_value.b = s_clear_blue;
	//canvas->action.colors[0].clear_value.a = s_clear_alpha;
	//canvas->action.depth.load_action = canvas->action.colors[0].load_action;
	//canvas->action.depth.clear_value = s_clear_depth;
	//canvas->action.stencil.load_action = canvas->action.colors[0].load_action;
	//canvas->action.stencil.clear_value = (uint8_t)(s_clear_stencil * 255.0f);
}

CF_Canvas cf_make_canvas(CF_CanvasParams canvas_params)
{
	CF_CanvasInternal* canvas = (CF_CanvasInternal*)CF_CALLOC(sizeof(CF_CanvasInternal));
	if (canvas_params.target.width > 0 && canvas_params.target.height > 0) {
		canvas->cf_texture = cf_make_texture(canvas_params.target);
		canvas->cf_depth_stencil = cf_make_texture(canvas_params.depth_stencil_target);

		//sg_pass_desc desc;
		//CF_MEMSET(&desc, 0, sizeof(desc));
		//desc.color_attachments[0].image = { (uint32_t)canvas->cf_texture.id };
		//desc.depth_stencil_attachment.image = { (uint32_t)canvas->cf_depth_stencil.id };
		//canvas->texture = desc.color_attachments[0].image;
		//canvas->depth_stencil = desc.depth_stencil_attachment.image;
		//desc.label = canvas_params.name;
		//canvas->pass = sg_make_pass(desc);
	} else {
		//canvas->cf_texture.id = CF_INVALID_HANDLE;
		//canvas->cf_depth_stencil.id = CF_INVALID_HANDLE;
		//canvas->texture.id = SG_INVALID_ID;
		//canvas->depth_stencil.id = SG_INVALID_ID;
		//canvas->pass_is_default = true;
	}
	CF_Canvas result;
	result.id = (uint64_t)canvas;
	return result;
}

void cf_destroy_canvas(CF_Canvas canvas_handle)
{
	CF_CanvasInternal* canvas = (CF_CanvasInternal*)canvas_handle.id;
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

uint64_t cf_canvas_get_backend_target_handle(CF_Canvas canvas_handle)
{
	CF_CanvasInternal* canvas = (CF_CanvasInternal*)canvas_handle.id;
	//return (uint64_t)canvas->texture.id;
	return 0;
}

uint64_t cf_canvas_get_backend_depth_stencil_handle(CF_Canvas canvas_handle)
{
	CF_CanvasInternal* canvas = (CF_CanvasInternal*)canvas_handle.id;
	//return (uint64_t)canvas->depth_stencil.id;
	return 0;
}

void cf_canvas_blit(CF_Canvas src, CF_V2 u0, CF_V2 v0, CF_Canvas dst, CF_V2 u1, CF_V2 v1)
{
	typedef struct Vertex
	{
		float x, y;
		float u, v;
	} Vertex;

	if (!app->canvas_blit_init) {
		app->canvas_blit_init = true;

		// Create a full-screen quad mesh.
		CF_Mesh blit_mesh = cf_make_mesh(USAGE_TYPE_STREAM, sizeof(Vertex) * 1024, 0, 0);
		CF_VertexAttribute attrs[2] = { 0 };
		attrs[0].name = "in_pos";
		attrs[0].format = CF_VERTEX_FORMAT_FLOAT2;
		attrs[0].offset = CF_OFFSET_OF(Vertex, x);
		attrs[1].name = "in_uv";
		attrs[1].format = CF_VERTEX_FORMAT_FLOAT2;
		attrs[1].offset = CF_OFFSET_OF(Vertex, u);
		cf_mesh_set_attributes(blit_mesh, attrs, CF_ARRAY_SIZE(attrs), sizeof(Vertex), 0);
		app->blit_mesh = blit_mesh;

		// Create material + shader for blitting.
		CF_Material blit_material = cf_make_material();
		cf_material_set_texture_fs(blit_material, "u_image", cf_canvas_get_target(src));
		CF_RenderState state = cf_render_state_defaults();
		state.blend.enabled = true;
		state.blend.rgb_src_blend_factor = CF_BLENDFACTOR_ONE;
		state.blend.rgb_dst_blend_factor = CF_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
		state.blend.rgb_op = CF_BLEND_OP_ADD;
		state.blend.alpha_src_blend_factor = CF_BLENDFACTOR_ONE;
		state.blend.alpha_dst_blend_factor = CF_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
		state.blend.alpha_op = CF_BLEND_OP_ADD;
		cf_material_set_render_state(blit_material, state);
		//CF_Shader blit_shader = CF_MAKE_SOKOL_SHADER(blit_shader);
		app->blit_material = blit_material;
		//app->blit_shader = blit_shader;
	}

	// UV (0,0) is top-left of the screen, while UV (1,1) is bottom right. We flip the y-axis for UVs to make the y-axis point up.
	// Coordinate (-1,1) is top left, while (1,-1) is bottom right.
	auto fill_quad = [](float x, float y, float sx, float sy, v2 u, v2 v, Vertex verts[6])
	{
		u.y = 1.0f - u.y;
		v.y = 1.0f - v.y;

		// Build a quad from (-1.0f,-1.0f) to (1.0f,1.0f).
		verts[0].x = -1.0f; verts[0].y =  1.0f; verts[0].u = u.x; verts[0].v = v.y;
		verts[1].x =  1.0f; verts[1].y = -1.0f; verts[1].u = v.x; verts[1].v = u.y;
		verts[2].x =  1.0f; verts[2].y =  1.0f; verts[2].u = v.x; verts[2].v = v.y;

		verts[3].x = -1.0f; verts[3].y =  1.0f; verts[3].u = u.x; verts[3].v = v.y;
		verts[4].x = -1.0f; verts[4].y = -1.0f; verts[4].u = u.x; verts[4].v = u.y;
		verts[5].x =  1.0f; verts[5].y = -1.0f; verts[5].u = v.x; verts[5].v = u.y;

		// Scale the quad about the origin by (sx,sy), then translate it by (x,y).
		for (int i = 0; i < 6; ++i) {
			verts[i].x = verts[i].x * sx + x;
			verts[i].y = verts[i].y * sy + y;
		}
	};

	// We're going to blit onto dst.
	cf_apply_canvas(dst);

	// Create a quad where positions come from dst, and UV's come from src.
	float w = v1.x - u1.x;
	float h = v1.y - u1.y;
	float x = (u1.x + v1.x) - 1.0f;
	float y = (u1.y + v1.y) - 1.0f;
	Vertex verts[6];
	fill_quad(x, y, w, h, u0, v0, verts);
	cf_mesh_append_vertex_data(app->blit_mesh, verts, 6);

	// Read pixels from src.
	cf_material_set_texture_fs(app->blit_material, "u_image", cf_canvas_get_target(src));

	// Blit onto dst.
	cf_apply_mesh(app->blit_mesh);
	cf_apply_shader(app->blit_shader, app->blit_material);
	cf_draw_elements();
}

CF_Mesh cf_make_mesh(CF_UsageType usage_type, int vertex_buffer_size, int index_buffer_size, int instance_buffer_size)
{
	CF_MeshInternal* mesh = (CF_MeshInternal*)CF_CALLOC(sizeof(CF_MeshInternal));
	mesh->need_vertex_sync = true;
	mesh->need_index_sync = true;
	mesh->need_instance_sync = true;
	mesh->vertices.size = vertex_buffer_size;
	mesh->indices.size = index_buffer_size;
	mesh->instances.size = instance_buffer_size;
	mesh->indices.stride = sizeof(uint32_t);
	//mesh->usage = s_wrap(usage_type);
	CF_Mesh result = { (uint64_t)mesh };
	return result;
}

void cf_destroy_mesh(CF_Mesh mesh_handle)
{
	CF_MeshInternal* mesh = (CF_MeshInternal*)mesh_handle.id;
	//if (mesh->vertices.handle.id) sg_destroy_buffer(mesh->vertices.handle);
	//if (mesh->indices.handle.id) sg_destroy_buffer(mesh->indices.handle);
	//if (mesh->instances.handle.id) sg_destroy_buffer(mesh->instances.handle);
	CF_FREE(mesh);
}

void cf_mesh_set_attributes(CF_Mesh mesh_handle, const CF_VertexAttribute* attributes, int attribute_count, int vertex_stride, int instance_stride)
{
	CF_MeshInternal* mesh = (CF_MeshInternal*)mesh_handle.id;
	//attribute_count = min(attribute_count, CF_MESH_MAX_VERTEX_ATTRIBUTES);
	mesh->attribute_count = attribute_count;
	mesh->vertices.stride = vertex_stride;
	mesh->instances.stride = instance_stride;
	for (int i = 0; i < attribute_count; ++i) {
		//mesh->attributes[i] = attributes[i];
		//mesh->attributes[i].name = sintern(attributes[i].name);
	}
}

static void s_sync_vertex_buffer(CF_MeshInternal* mesh, void* data, int size)
{
	mesh->need_vertex_sync = false;
	//if (mesh->vertices.handle.id) sg_destroy_buffer(mesh->vertices.handle);
	//sg_buffer_desc desc = { };
	//desc.size = mesh->vertices.size;
	//desc.type = SG_BUFFERTYPE_VERTEXBUFFER;
	//desc.usage = mesh->usage;
	//desc.data.ptr = data;
	//desc.data.size = size;
	//mesh->vertices.handle = sg_make_buffer(desc);
	//mesh->vertices.offset = 0;
}

void cf_mesh_update_vertex_data(CF_Mesh mesh_handle, void* data, int count)
{
	CF_MeshInternal* mesh = (CF_MeshInternal*)mesh_handle.id;
	int size = count * mesh->vertices.stride;
	if (size > mesh->vertices.size) {
		mesh->vertices.size = size;
		mesh->need_vertex_sync = true;
	}
	CF_ASSERT(mesh->attribute_count);
	if (mesh->need_vertex_sync) {
		//s_sync_vertex_buffer(mesh, mesh->usage == SG_USAGE_IMMUTABLE ? data : NULL, size);
	} else {
		//sg_range range = { data, (size_t)size };
		//sg_update_buffer(mesh->vertices.handle, range);
	}
	mesh->vertices.element_count = count;
}

int cf_mesh_append_vertex_data(CF_Mesh mesh_handle, void* data, int append_count)
{
	CF_MeshInternal* mesh = (CF_MeshInternal*)mesh_handle.id;
	int size = append_count * mesh->vertices.stride;
	CF_ASSERT(mesh->attribute_count);
	CF_ASSERT(mesh->vertices.size >= size);
	if (mesh->need_vertex_sync) {
		s_sync_vertex_buffer(mesh, NULL, mesh->vertices.size);
	}
	//sg_range range = { data, (size_t)size };
	//if (!sg_query_buffer_will_overflow(mesh->vertices.handle, size)) {
	//	int offset = sg_append_buffer(mesh->vertices.handle, range);
	//	mesh->vertices.offset = offset;
	//	mesh->vertices.element_count = append_count;
	//	mesh->vertices.was_appended = true;
	//	return offset;
	//} else {
	//	return 0;
	//}
	return 0;
}

bool cf_mesh_will_overflow_vertex_data(CF_Mesh mesh_handle, int append_count)
{
	CF_MeshInternal* mesh = (CF_MeshInternal*)mesh_handle.id;
	//return sg_query_buffer_will_overflow(mesh->vertices.handle, append_count * mesh->vertices.stride);
	return false;
}

static void s_sync_instance_buffer(CF_MeshInternal* mesh, void* data, int size)
{
	mesh->need_instance_sync = false;
	//if (mesh->instances.handle.id) sg_destroy_buffer(mesh->instances.handle);
	//sg_buffer_desc desc = { };
	//desc.size = mesh->instances.size;
	//desc.type = SG_BUFFERTYPE_VERTEXBUFFER;
	//desc.usage = mesh->usage;
	//desc.data.ptr = data;
	//desc.data.size = size;
	//mesh->instances.handle = sg_make_buffer(desc);
	//mesh->instances.offset = 0;
}

void cf_mesh_update_instance_data(CF_Mesh mesh_handle, void* data, int count)
{
	CF_MeshInternal* mesh = (CF_MeshInternal*)mesh_handle.id;
	int size = count * mesh->instances.stride;
	if (size > mesh->instances.size) {
		mesh->instances.size = size;
		mesh->need_instance_sync = true;
	}
	CF_ASSERT(mesh->attribute_count);
	if (mesh->need_instance_sync) {
		//s_sync_instance_buffer(mesh, mesh->usage == SG_USAGE_IMMUTABLE ? data : NULL, size);
	} else {
		//sg_range range = { data, (size_t)size };
		//sg_update_buffer(mesh->instances.handle, range);
	}
	mesh->instances.element_count = count;
}

int cf_mesh_append_instance_data(CF_Mesh mesh_handle, void* data, int append_count)
{
	CF_MeshInternal* mesh = (CF_MeshInternal*)mesh_handle.id;
	int size = append_count * mesh->instances.stride;
	CF_ASSERT(mesh->attribute_count);
	CF_ASSERT(mesh->instances.size >= size);
	if (mesh->need_instance_sync) {
		s_sync_instance_buffer(mesh, NULL, mesh->instances.size);
	}
	//sg_range range = { data, (size_t)size };
	//if (!sg_query_buffer_will_overflow(mesh->instances.handle, size)) {
	//	int offset = sg_append_buffer(mesh->instances.handle, range);
	//	mesh->instances.offset = offset;
	//	mesh->instances.element_count = append_count;
	//	mesh->instances.was_appended = true;
	//	return offset;
	//} else {
	//	return 0;
	//}
	return 0;
}

bool cf_mesh_will_overflow_instance_data(CF_Mesh mesh_handle, int append_count)
{
	CF_MeshInternal* mesh = (CF_MeshInternal*)mesh_handle.id;
	//return sg_query_buffer_will_overflow(mesh->instances.handle, append_count * mesh->instances.stride);
	return 0;
}

static void s_sync_index_buffer(CF_MeshInternal* mesh, uint32_t* indices, int size)
{
	mesh->need_index_sync = false;
	//if (mesh->indices.handle.id) sg_destroy_buffer(mesh->indices.handle);
	//sg_buffer_desc desc = { };
	//desc.size = mesh->indices.size;
	//desc.type = SG_BUFFERTYPE_INDEXBUFFER;
	//desc.usage = mesh->usage;
	//desc.data.ptr = indices;
	//desc.data.size = size;
	//mesh->indices.handle = sg_make_buffer(desc);
	//mesh->indices.offset = 0;
}

void cf_mesh_update_index_data(CF_Mesh mesh_handle, uint32_t* indices, int count)
{
	CF_MeshInternal* mesh = (CF_MeshInternal*)mesh_handle.id;
	int size = count * sizeof(uint32_t);
	if (size > mesh->indices.size) {
		mesh->indices.size = size;
		mesh->need_index_sync = true;
	}
	CF_ASSERT(mesh->attribute_count);
	if (mesh->need_index_sync) {
		//s_sync_index_buffer(mesh, mesh->usage == SG_USAGE_IMMUTABLE ? indices : NULL, size);
	} else {
		//sg_range range = { indices, (size_t)size };
		//sg_update_buffer(mesh->indices.handle, range);
	}
	mesh->indices.element_count = count;
}

int cf_mesh_append_index_data(CF_Mesh mesh_handle, uint32_t* indices, int append_count)
{
	CF_MeshInternal* mesh = (CF_MeshInternal*)mesh_handle.id;
	int size = append_count * sizeof(uint32_t);
	CF_ASSERT(mesh->indices.size >= size);
	if (mesh->need_index_sync) {
		s_sync_index_buffer(mesh, NULL, mesh->indices.size);
	}
	//sg_range range = { indices, (size_t)size };
	//if (!sg_query_buffer_will_overflow(mesh->indices.handle, size)) {
	//	int offset = sg_append_buffer(mesh->indices.handle, range);
	//	mesh->indices.offset = offset;
	//	mesh->indices.element_count = append_count;
	//	mesh->indices.was_appended = true;
	//	return offset;
	//} else {
	//	return 0;
	//}
	return 0;
}

bool cf_mesh_will_overflow_index_data(CF_Mesh mesh_handle, int append_count)
{
	CF_MeshInternal* mesh = (CF_MeshInternal*)mesh_handle.id;
	//return sg_query_buffer_will_overflow(mesh->indices.handle, append_count * sizeof(uint32_t));
	return 0;
}

CF_RenderState cf_render_state_defaults()
{
	CF_RenderState state;
	state.blend.enabled = false;
	state.cull_mode = CF_CULL_MODE_NONE;
	state.blend.pixel_format = CF_PIXELFORMAT_DEFAULT;
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

struct CF_Uniform
{
	const char* name;
	CF_UniformType type;
	int array_length;
	void* data;
	int size;
};

struct CF_MaterialTex
{
	const char* name;
	CF_Texture handle;
};

struct CF_MaterialState
{
	Array<CF_Uniform> uniforms;
	Array<CF_MaterialTex> textures;
};

struct CF_MaterialInternal
{
	CF_RenderState state;
	CF_MaterialState vs;
	CF_MaterialState fs;
	CF_Arena uniform_arena;
	CF_Arena block_arena;
};

CF_Material cf_make_material()
{
	CF_MaterialInternal* material = CF_NEW(CF_MaterialInternal);
	cf_arena_init(&material->uniform_arena, 4, 1024);
	cf_arena_init(&material->block_arena, 4, 1024);
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
	material->state = render_state;
}

static void s_material_set_texture(CF_MaterialState* state, const char* name, CF_Texture texture)
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
	}
}

void cf_material_set_texture_vs(CF_Material material_handle, const char* name, CF_Texture texture)
{
	CF_MaterialInternal* material = (CF_MaterialInternal*)material_handle.id;
	name = sintern(name);
	s_material_set_texture(&material->vs, name, texture);
}

void cf_material_set_texture_fs(CF_Material material_handle, const char* name, CF_Texture texture)
{
	CF_MaterialInternal* material = (CF_MaterialInternal*)material_handle.id;
	name = sintern(name);
	s_material_set_texture(&material->fs, name, texture);
}

void cf_material_clear_textures(CF_Material material_handle)
{
	CF_MaterialInternal* material = (CF_MaterialInternal*)material_handle.id;
	material->vs.textures.clear();
	material->fs.textures.clear();
}

static void s_material_set_uniform(CF_Arena* arena, CF_MaterialState* state, const char* name, void* data, CF_UniformType type, int array_length)
{
	if (array_length <= 0) array_length = 1;
	CF_Uniform* uniform = NULL;
	for (int i = 0; i < state->uniforms.count(); ++i) {
		CF_Uniform* u = state->uniforms + i;
		if (u->name == name) {
			uniform = u;
			break;
		}
	}
	int size = s_uniform_size(type) * array_length;
	if (!uniform) {
		uniform = &state->uniforms.add();
		uniform->name = name;
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
	s_material_set_uniform(&material->uniform_arena, &material->vs, name, data, type, array_length);
}

void cf_material_set_uniform_fs(CF_Material material_handle, const char* name, void* data, CF_UniformType type, int array_length)
{
	CF_MaterialInternal* material = (CF_MaterialInternal*)material_handle.id;
	name = sintern(name);
	s_material_set_uniform(&material->uniform_arena, &material->fs, name, data, type, array_length);
}

void cf_material_clear_uniforms(CF_Material material_handle)
{
	CF_MaterialInternal* material = (CF_MaterialInternal*)material_handle.id;
	arena_reset(&material->uniform_arena);
	material->vs.uniforms.clear();
	material->fs.uniforms.clear();
}

static void s_end_pass()
{
	if (s_canvas) {
		SDL_GpuSubmit(s_canvas->cmd);
		CF_MeshInternal* mesh = s_canvas->mesh;
		if (mesh) {
			if (mesh->vertices.was_appended) {
				mesh->vertices.was_appended = false;
				mesh->vertices.element_count = 0;
			}
			if (mesh->indices.was_appended) {
				mesh->indices.was_appended = false;
				mesh->indices.element_count = 0;
			}
			if (mesh->instances.was_appended) {
				mesh->instances.was_appended = false;
				mesh->instances.element_count = 0;
			}
		}
	}
}

void cf_clear_screen(float red, float green, float blue, float alpha)
{
	SDL_GpuCommandBuffer* cmd = SDL_GpuAcquireCommandBuffer(app->device);
	Uint32 w, h;
	SDL_GpuTexture* swapchain_tex = SDL_GpuAcquireSwapchainTexture(cmd, app->window, &w, &h);
	if (swapchain_tex != NULL) {
		SDL_GpuColorAttachmentInfo info = { 0 };
		info.textureSlice.texture = swapchain_tex;
		info.clearColor = { red, green, blue , alpha };
		info.loadOp = SDL_GPU_LOADOP_CLEAR;
		info.storeOp = SDL_GPU_STOREOP_STORE;

		SDL_GpuRenderPass* pass = SDL_GpuBeginRenderPass(cmd, &info, 1, NULL);
		SDL_GpuEndRenderPass(pass);
	}
	SDL_GpuSubmit(cmd);
}

void cf_clear_depth_stencil(float depth, uint32_t stencil)
{
	SDL_GpuCommandBuffer* cmd = SDL_GpuAcquireCommandBuffer(app->device);
	Uint32 w, h;
	SDL_GpuTexture* swapchain_tex = SDL_GpuAcquireSwapchainTexture(cmd, app->window, &w, &h);
	if (swapchain_tex != NULL) {
		SDL_GpuDepthStencilAttachmentInfo info = { 0 };
		info.textureSlice.texture = swapchain_tex;
		info.depthStencilClearValue.depth = depth;
		info.depthStencilClearValue.stencil = stencil;
		info.loadOp = SDL_GPU_LOADOP_CLEAR;
		info.storeOp = SDL_GPU_STOREOP_STORE;
		info.stencilLoadOp = SDL_GPU_LOADOP_CLEAR;
		info.stencilStoreOp = SDL_GPU_STOREOP_STORE;

		SDL_GpuRenderPass* pass = SDL_GpuBeginRenderPass(cmd, NULL, 0, &info);
		SDL_GpuEndRenderPass(pass);
	}
	SDL_GpuSubmit(cmd);
}

void cf_apply_canvas(CF_Canvas canvas_handle)
{
	//CF_CanvasInternal* canvas = (CF_CanvasInternal*)pass_handle.id;
	//s_end_pass();
	//s_canvas = canvas;
	//s_canvas_clear_settings(canvas);
	//if (clear) {
	//	//canvas->action.colors[0].load_action = SG_LOADACTION_CLEAR;
	//	//canvas->action.depth.load_action = SG_LOADACTION_CLEAR;
	//	//canvas->action.stencil.load_action = SG_LOADACTION_CLEAR;
	//} else {
	//	//canvas->action.colors[0].load_action = SG_LOADACTION_LOAD;
	//	//canvas->action.depth.load_action = SG_LOADACTION_LOAD;
	//	//canvas->action.stencil.load_action = SG_LOADACTION_LOAD;
	//}
	//if (canvas->pass_is_default) {
	//	//sg_begin_default_pass(&canvas->action, app->w, app->h);
	//} else {
	//	//sg_begin_pass(canvas->pass, &canvas->action);
	//}
}

void cf_apply_viewport(int x, int y, int width, int height)
{
	//sg_apply_viewport(x, y, width, height, false);
}

void cf_apply_scissor(int x, int y, int width, int height)
{
	//sg_apply_scissor_rect(x, y, width, height, false);
}

void cf_apply_mesh(CF_Mesh mesh_handle)
{
	//CF_ASSERT(s_canvas);
	//CF_MeshInternal* mesh = (CF_MeshInternal*)mesh_handle.id;
	//s_canvas->mesh = mesh;
}

static void s_copy_uniforms(SDL_GpuCommandBuffer* cmd, CF_Arena* arena, CF_ShaderInternal* shd, CF_MaterialState* mstate, bool vs)
{
	// Create any required uniform blocks for all uniforms matching between which uniforms
	// the material has and the shader needs.
	int block_size = vs ? shd->vs_block_size : shd->fs_block_size;
	if (!block_size) return;
	void* block = cf_arena_alloc(arena, block_size);
	for (int i = 0; i < mstate->uniforms.count(); ++i) {
		CF_Uniform uniform = mstate->uniforms[i];
		int idx = vs ? shd->vs_index(uniform.name) : shd->fs_index(uniform.name);
		if (idx >= 0) {
			int offset = vs ? shd->vs_uniform_block_members[idx].offset : shd->fs_uniform_block_members[idx].offset;
			void* dst = (void*)(((uintptr_t)block) + offset);
			CF_MEMCPY(dst, uniform.data, uniform.size);
		}
	}

	// Send uniform data to the GPU.
	if (vs) {
		SDL_GpuPushVertexUniformData(cmd, 0, block, (uint32_t)block_size);
	} else {
		SDL_GpuPushFragmentUniformData(cmd, 0, block, (uint32_t)block_size);
	}

	cf_arena_reset(arena);
}

static SDL_GpuVertexElementFormat s_wrap(CF_VertexFormat format)
{
	switch (format) {
	case CF_VERTEX_FORMAT_UINT: return SDL_GPU_VERTEXELEMENTFORMAT_UINT;
	case CF_VERTEX_FORMAT_FLOAT: return SDL_GPU_VERTEXELEMENTFORMAT_FLOAT;
	case CF_VERTEX_FORMAT_FLOAT2: return SDL_GPU_VERTEXELEMENTFORMAT_VECTOR2;
	case CF_VERTEX_FORMAT_FLOAT3: return SDL_GPU_VERTEXELEMENTFORMAT_VECTOR3;
	case CF_VERTEX_FORMAT_FLOAT4: return SDL_GPU_VERTEXELEMENTFORMAT_VECTOR4;
	case CF_VERTEX_FORMAT_UBYTE4N: return SDL_GPU_VERTEXELEMENTFORMAT_BYTE4;
	case CF_VERTEX_FORMAT_SHORT2: return SDL_GPU_VERTEXELEMENTFORMAT_SHORT2;
	case CF_VERTEX_FORMAT_SHORT4: return SDL_GPU_VERTEXELEMENTFORMAT_SHORT4;
	case CF_VERTEX_FORMAT_SHORT2N: return SDL_GPU_VERTEXELEMENTFORMAT_NORMALIZEDSHORT2;
	case CF_VERTEX_FORMAT_SHORT4N: return SDL_GPU_VERTEXELEMENTFORMAT_NORMALIZEDSHORT4;
	case CF_VERTEX_FORMAT_HALFVECTOR2: return SDL_GPU_VERTEXELEMENTFORMAT_HALFVECTOR2;
	case CF_VERTEX_FORMAT_HALFVECTOR4: return SDL_GPU_VERTEXELEMENTFORMAT_HALFVECTOR4;
	}
}

static SDL_GpuCompareOp s_wrap(CF_CompareFunction compare_function)
{
	switch (compare_function)
	{
	case CF_COMPARE_FUNCTION_ALWAYS:                return SDL_GPU_COMPAREOP_ALWAYS;
	case CF_COMPARE_FUNCTION_NEVER:                 return SDL_GPU_COMPAREOP_NEVER;
	case CF_COMPARE_FUNCTION_LESS_THAN:             return SDL_GPU_COMPAREOP_LESS;
	case CF_COMPARE_FUNCTION_EQUAL:                 return SDL_GPU_COMPAREOP_EQUAL;
	case CF_COMPARE_FUNCTION_NOT_EQUAL:             return SDL_GPU_COMPAREOP_NOT_EQUAL;
	case CF_COMPARE_FUNCTION_LESS_THAN_OR_EQUAL:    return SDL_GPU_COMPAREOP_LESS_OR_EQUAL;
	case CF_COMPARE_FUNCTION_GREATER_THAN:          return SDL_GPU_COMPAREOP_GREATER;
	case CF_COMPARE_FUNCTION_GREATER_THAN_OR_EQUAL: return SDL_GPU_COMPAREOP_GREATER_OR_EQUAL;
	default:                                        return SDL_GPU_COMPAREOP_ALWAYS;
	}
}

static SDL_GpuStencilOp s_wrap(CF_StencilOp stencil_op)
{
	switch (stencil_op)
	{
	case CF_STENCIL_OP_KEEP:            return SDL_GPU_STENCILOP_KEEP;
	case CF_STENCIL_OP_ZERO:            return SDL_GPU_STENCILOP_ZERO;
	case CF_STENCIL_OP_REPLACE:         return SDL_GPU_STENCILOP_REPLACE;
	case CF_STENCIL_OP_INCREMENT_CLAMP: return SDL_GPU_STENCILOP_INCREMENT_AND_CLAMP;
	case CF_STENCIL_OP_DECREMENT_CLAMP: return SDL_GPU_STENCILOP_DECREMENT_AND_CLAMP;
	case CF_STENCIL_OP_INVERT:          return SDL_GPU_STENCILOP_INVERT;
	case CF_STENCIL_OP_INCREMENT_WRAP:  return SDL_GPU_STENCILOP_INCREMENT_AND_WRAP;
	case CF_STENCIL_OP_DECREMENT_WRAP:  return SDL_GPU_STENCILOP_DECREMENT_AND_WRAP;
	default:                            return SDL_GPU_STENCILOP_KEEP;
	}
}

static SDL_GpuBlendOp s_wrap(CF_BlendOp blend_op)
{
	switch (blend_op)
	{
	case CF_BLEND_OP_ADD:              return SDL_GPU_BLENDOP_ADD;
	case CF_BLEND_OP_SUBTRACT:         return SDL_GPU_BLENDOP_SUBTRACT;
	case CF_BLEND_OP_REVERSE_SUBTRACT: return SDL_GPU_BLENDOP_REVERSE_SUBTRACT;
	case CF_BLEND_OP_MIN:              return SDL_GPU_BLENDOP_MIN;
	case CF_BLEND_OP_MAX:              return SDL_GPU_BLENDOP_MAX;
	default:                           return SDL_GPU_BLENDOP_ADD;
	}
}

void cf_apply_shader(CF_Shader shader_handle, CF_Material material_handle)
{
	CF_ASSERT(s_canvas);
	CF_MeshInternal* mesh = s_canvas->mesh;
	CF_MaterialInternal* material = (CF_MaterialInternal*)material_handle.id;
	CF_ShaderInternal* shader = (CF_ShaderInternal*)shader_handle.id;
	CF_RenderState* state = &material->state;
	bool has_instance_data = mesh->instances.size > 0;

	SDL_GpuColorAttachmentDescription color_info;
	CF_MEMSET(&color_info, 0, sizeof(color_info));
	color_info.format = SDL_GpuGetSwapchainTextureFormat(app->device, app->window);
	color_info.blendState.blendEnable = state->blend.enabled;
	color_info.blendState.alphaBlendOp = s_wrap(state->blend.alpha_op);
	color_info.blendState.colorBlendOp = s_wrap(state->blend.rgb_op);
	color_info.blendState.srcColorBlendFactor = SDL_GPU_BLENDFACTOR_ONE;
	color_info.blendState.srcAlphaBlendFactor = SDL_GPU_BLENDFACTOR_ONE;
	color_info.blendState.dstColorBlendFactor = SDL_GPU_BLENDFACTOR_ZERO;
	color_info.blendState.dstAlphaBlendFactor = SDL_GPU_BLENDFACTOR_ZERO;
	int mask_r = (int)state->blend.write_R_enabled << 0;
	int mask_g = (int)state->blend.write_G_enabled << 1;
	int mask_b = (int)state->blend.write_B_enabled << 2;
	int mask_a = (int)state->blend.write_A_enabled << 3;
	color_info.blendState.colorWriteMask = (uint32_t)(mask_r | mask_g | mask_b | mask_a);

	SDL_GpuGraphicsPipelineCreateInfo pip_info;
	CF_MEMSET(&pip_info, 0, sizeof(pip_info));
	pip_info.attachmentInfo.colorAttachmentCount = 1;
	pip_info.attachmentInfo.colorAttachmentDescriptions = &color_info;
	pip_info.multisampleState.sampleMask = 0xFFFF;
	pip_info.primitiveType = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
	pip_info.vertexShader = shader->vs;
	pip_info.fragmentShader = shader->fs;
	pip_info.attachmentInfo.hasDepthStencilAttachment = state->depth_write_enabled;
	pip_info.attachmentInfo.depthStencilFormat = SDL_GPU_TEXTUREFORMAT_D24_UNORM_S8_UINT;

	SDL_GpuVertexAttribute* attributes = SDL_stack_alloc(SDL_GpuVertexAttribute, mesh->attribute_count);
	int attribute_count = 0;
	for (int i = 0; i < mesh->attribute_count; ++i) {
		int idx = shader->get_input_index(mesh->attributes[i].name);
		if (idx >= 0) {
			SDL_GpuVertexAttribute* attr = attributes + attribute_count++;
			attr->binding = 0;
			attr->location = shader->input_locations[idx];
			attr->format = s_wrap(mesh->attributes[i].format);
			attr->offset = mesh->attributes[i].offset;
		}
	}
	pip_info.vertexInputState.vertexAttributeCount = mesh->attribute_count;
	pip_info.vertexInputState.vertexAttributes = attributes;
	SDL_GpuVertexBinding vertex_bindings[2];
	vertex_bindings[0].binding = 0;
	vertex_bindings[0].stride = mesh->vertices.stride;
	vertex_bindings[0].inputRate = SDL_GPU_VERTEXINPUTRATE_VERTEX;
	vertex_bindings[0].stepRate = 0;
	pip_info.vertexInputState.vertexBindings = vertex_bindings;
	if (has_instance_data) {
		vertex_bindings[1].binding = 1;
		vertex_bindings[1].stride = mesh->instances.stride;
		vertex_bindings[1].inputRate = SDL_GPU_VERTEXINPUTRATE_INSTANCE;
		vertex_bindings[1].stepRate = 0;
		pip_info.vertexInputState.vertexBindingCount = 2;
	} else {
		pip_info.vertexInputState.vertexBindingCount = 1;
	}

	pip_info.primitiveType = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
	pip_info.rasterizerState.fillMode = SDL_GPU_FILLMODE_FILL;
	pip_info.rasterizerState.cullMode = SDL_GPU_CULLMODE_NONE;
	pip_info.rasterizerState.frontFace = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE;
	pip_info.rasterizerState.depthBiasEnable = false;
	pip_info.rasterizerState.depthBiasConstantFactor = 0;
	pip_info.rasterizerState.depthBiasClamp = 0;
	pip_info.rasterizerState.depthBiasSlopeFactor = 0;
	pip_info.multisampleState.sampleCount = SDL_GPU_SAMPLECOUNT_1;
	pip_info.multisampleState.sampleMask = 0;
	pip_info.depthStencilState = { 0 };

	pip_info.depthStencilState.depthTestEnable = state->depth_write_enabled;
	pip_info.depthStencilState.depthWriteEnable = state->depth_write_enabled;
	pip_info.depthStencilState.compareOp = s_wrap(state->depth_compare);
	pip_info.depthStencilState.stencilTestEnable = state->stencil.enabled;
	pip_info.depthStencilState.backStencilState.failOp = s_wrap(state->stencil.back.fail_op);
	pip_info.depthStencilState.backStencilState.passOp = s_wrap(state->stencil.back.pass_op);
	pip_info.depthStencilState.backStencilState.depthFailOp = s_wrap(state->stencil.back.depth_fail_op);
	pip_info.depthStencilState.backStencilState.compareOp = s_wrap(state->stencil.back.compare);
	pip_info.depthStencilState.frontStencilState.failOp = s_wrap(state->stencil.front.fail_op);
	pip_info.depthStencilState.frontStencilState.passOp = s_wrap(state->stencil.front.pass_op);
	pip_info.depthStencilState.frontStencilState.depthFailOp = s_wrap(state->stencil.front.depth_fail_op);
	pip_info.depthStencilState.frontStencilState.compareOp = s_wrap(state->stencil.front.compare);
	pip_info.depthStencilState.compareMask = state->stencil.read_mask;
	pip_info.depthStencilState.writeMask = state->stencil.write_mask;
	pip_info.depthStencilState.reference = state->stencil.reference;

	SDL_GpuGraphicsPipeline* pip = SDL_GpuCreateGraphicsPipeline(app->device, &pip_info);
	CF_ASSERT(pip);
	SDL_GpuCommandBuffer* cmd = SDL_GpuAcquireCommandBuffer(app->device);
	s_canvas->pip = pip;
	s_canvas->cmd = cmd;

	SDL_GpuColorAttachmentInfo pass_color_info = { 0 };
	pass_color_info.textureSlice.texture = s_canvas->texture;
	pass_color_info.clearColor = { 0.0f, 0.0f, 0.0f, 1.0f };
	pass_color_info.loadOp = SDL_GPU_LOADOP_LOAD;
	pass_color_info.storeOp = SDL_GPU_STOREOP_STORE;
	SDL_GpuRenderPass* pass = SDL_GpuBeginRenderPass(cmd, &pass_color_info, 1, NULL);
	SDL_GpuBindGraphicsPipeline(pass, pip);
	SDL_GpuBufferBinding bind;
	bind.buffer = NULL; // @TODO.
	bind.offset = 0; // @TODO.
	SDL_GpuBindVertexBuffers(pass, 0, &bind, 1);
	// @TODO
	//SDL_GpuBindIndexBuffer

	// Copy over uniform data.
	s_copy_uniforms(cmd, &material->block_arena, shader, &material->vs, true);
	s_copy_uniforms(cmd, &material->block_arena, shader, &material->fs, false);
}

void cf_draw_elements()
{
	//CF_MeshInternal* mesh = s_canvas->mesh;
	//sg_draw(0, mesh->vertices.element_count, mesh->instances.element_count + 1); // TODO - +1??
	//sg_destroy_pipeline(s_canvas->pip);
	app->draw_call_count++;
}

void cf_unapply_canvas()
{
	s_end_pass();
	s_canvas = NULL;
}

void cf_commit()
{
	s_end_pass();
	//sg_commit();
	//sg_commit();
}

void cf_destroy_graphics()
{
	if (s_default_canvas) {
		cf_destroy_canvas({ (uint64_t)s_default_canvas });
		s_default_canvas = NULL;
	}
}

#include <SPIRV-Reflect/spirv_reflect.c>
