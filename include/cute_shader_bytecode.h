/*
	Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

#ifndef CF_SHADER_BYTECODE_H
#define CF_SHADER_BYTECODE_H

#include <cute_defines.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

// Shared structures between cute-shader and cute framework.
// There should be little to no external includes since cute-shader is not depending on cute framework.

/**
 * @enum     CF_ShaderInfoDataType
 * @category graphics
 * @brief    Data types of shader elements.
 * @related  CF_ShaderInputInfo CF_ShaderUniformMemberInfo
 */
#define CF_SHADER_INFO_DATA_TYPE_DEFS \
	/* @entry */                          \
	CF_ENUM(SHADER_INFO_TYPE_UNKNOWN,  0) \
	/* @entry */                          \
	CF_ENUM(SHADER_INFO_TYPE_SINT,     1) \
	/* @entry */                          \
	CF_ENUM(SHADER_INFO_TYPE_UINT,     2) \
	/* @entry */                          \
	CF_ENUM(SHADER_INFO_TYPE_FLOAT,    3) \
	/* @entry */                          \
	CF_ENUM(SHADER_INFO_TYPE_SINT2,    4) \
	/* @entry */                          \
	CF_ENUM(SHADER_INFO_TYPE_UINT2,    5) \
	/* @entry */                          \
	CF_ENUM(SHADER_INFO_TYPE_FLOAT2,   6) \
	/* @entry */                          \
	CF_ENUM(SHADER_INFO_TYPE_SINT3,    7) \
	/* @entry */                          \
	CF_ENUM(SHADER_INFO_TYPE_UINT3,    8) \
	/* @entry */                          \
	CF_ENUM(SHADER_INFO_TYPE_FLOAT3,   9) \
	/* @entry */                          \
	CF_ENUM(SHADER_INFO_TYPE_SINT4,   10) \
	/* @entry */                          \
	CF_ENUM(SHADER_INFO_TYPE_UINT4,   11) \
	/* @entry */                          \
	CF_ENUM(SHADER_INFO_TYPE_FLOAT4,  12) \
	/* @entry */                          \
	CF_ENUM(SHADER_INFO_TYPE_MAT4,    13) \
	/* @end */

typedef enum CF_ShaderInfoDataType
{
	#define CF_ENUM(K, V) CF_##K = V,
	CF_SHADER_INFO_DATA_TYPE_DEFS
	#undef CF_ENUM
} CF_ShaderInfoDataType;

/**
 * @struct   CF_ShaderUniformMemberInfo
 * @category graphics
 * @brief    Information about a uniform block member.
 * @related  CF_ShaderBytecode CF_ShaderUniformInfo
 */
typedef struct CF_ShaderUniformMemberInfo
{
	/* @member Name of the member. */
	const char* name;
	/* @member Type of the member. */
	CF_ShaderInfoDataType type;
	/* @member Offset of the member. */
	int offset;
	/* @member Array length of the member. Set to 1 if it is not an array. */
	int array_length;
} CF_ShaderUniformMemberInfo;
// @end

/**
 * @struct   CF_ShaderUniformInfo
 * @category graphics
 * @brief    Information about a uniform block.
 * @remarks  The members of successive blocks are stored tightly as an array in `CF_ShaderInfo`.
 *           To access them use the following code:
 *
 *           ```c
 *           CF_ShaderInfo shader_info = bytecode.shader_info;
 *           CF_ShaderUniformMemberInfo* members = shader_info.uniform_members;
 *           for (int uniform_index = 0; uniform_index < shader_info.num_uniforms; ++uniform_index) {
 *               const CF_ShaderUniformInfo* uniform_info = &shader_info.uniforms[uniform_index]);
 *               printf("Uniform block %s has the following members:\n", uniform_info->block_name);
 *               for (int member_index = 0; member_index < uniform_info->num_members; ++member_index) {
 *                   const CF_ShaderUniformMemberInfo* member_info = &members[member_index];
 *                   printf("- %s\n", member_info->name);
 *               }
 *               // Advance the members pointer
 *               members += uniform_info->num_members;
 *           }
 *           ```
 * @related  CF_ShaderBytecode CF_ShaderUniformMemberInfo
 */
typedef struct CF_ShaderUniformInfo
{
	/* @member Name of the block. */
	const char* block_name;
	/* @member Block index. */
	int block_index;
	/* @member Block size. */
	int block_size;
	/* @member Number of members. */
	int num_members;
} CF_ShaderUniformInfo;
// @end

/**
 * @struct   CF_ShaderInputInfo
 * @category graphics
 * @brief    Information about an input of a vertex shader.
 * @related  CF_ShaderBytecode
 */
typedef struct CF_ShaderInputInfo
{
	/* @member Name of the input. */
	const char* name;
	/* @member Location of the input. */
	int location;
	/* @member Input format. */
	CF_ShaderInfoDataType format;
} CF_ShaderInputInfo;
// @end

/**
 * @struct   CF_ShaderInfo
 * @category graphics
 * @brief    Reflection info for a shader.
 * @related  CF_ShaderBytecode
 */
typedef struct CF_ShaderInfo
{
	/* @member Number of samplers. */
	int num_samplers;
	/* @member Number of storage textures. */
	int num_storage_textures;
	/* @member Number of storage buffers. */
	int num_storage_buffers;

	/* @member Number of images. */
	int num_images;
	/* @member Name of each images. */
	const char** image_names;
	/* @member Binding slot of each image. */
	int* image_binding_slots;

	/* @member Number of uniform blocks. */
	int num_uniforms;
	/* @member Information about each uniform block. */
	CF_ShaderUniformInfo* uniforms;

	/* @member Number of uniform block members. */
	int num_uniform_members;
	/* @member Members of all uniform blocks tightly packed (see `CF_ShaderUniformInfo` for more details). */
	CF_ShaderUniformMemberInfo* uniform_members;

	/* @member Number of inputs for vertex shader. */
	int num_inputs;
	/* @member Information about each vertex shader input. */
	CF_ShaderInputInfo* inputs;
} CF_ShaderInfo;
// @end

/**
 * @struct   CF_ShaderBytecode
 * @category graphics
 * @brief    A SPIR-V shader bytecode blob.
 * @remarks  This can be created either through `cf_compile_shader_to_bytecode` or the `cute-shaderc` compiler.
 * @related  CF_Shader cf_make_shader_from_bytecode cf_compile_shader_to_bytecode
 */
typedef struct CF_ShaderBytecode
{
	/* @member The SPIR-V bytecode. */
	const uint8_t* content;
	/* @member Size of the bytecode blob. */
	size_t size;
	/* @member The transpiled GLSL 300 source for GLES 3 and WebGL 2. */
	const char* glsl300_src;
	/* @member Size of the GLSL 300 source. */
	size_t glsl300_src_size;
	/* @member Shader reflection info. */
	CF_ShaderInfo shader_info;
} CF_ShaderBytecode;
// @end

static inline const char* cf_shader_info_data_type_to_string(CF_ShaderInfoDataType type)
{
	switch (type) {
	#define CF_ENUM(K, V) case CF_##K: return "CF_" #K;
	CF_SHADER_INFO_DATA_TYPE_DEFS
	#undef CF_ENUM
	}
	return NULL;
}

#ifdef __cplusplus
}
#endif // __cplusplus

#endif
