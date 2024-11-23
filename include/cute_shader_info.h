/*
	Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

#ifndef CF_SHADER_INFO_H
#define CF_SHADER_INFO_H

// Shared structures for shader reflection between cute-shader and cute framework

typedef enum CF_ShaderInfoDataType
{
	CF_SHADER_INFO_TYPE_UNKNOWN,
	CF_SHADER_INFO_TYPE_SINT,
	CF_SHADER_INFO_TYPE_UINT,
	CF_SHADER_INFO_TYPE_FLOAT,
	CF_SHADER_INFO_TYPE_SINT2,
	CF_SHADER_INFO_TYPE_UINT2,
	CF_SHADER_INFO_TYPE_FLOAT2,
	CF_SHADER_INFO_TYPE_SINT3,
	CF_SHADER_INFO_TYPE_UINT3,
	CF_SHADER_INFO_TYPE_FLOAT3,
	CF_SHADER_INFO_TYPE_SINT4,
	CF_SHADER_INFO_TYPE_UINT4,
	CF_SHADER_INFO_TYPE_FLOAT4,
	CF_SHADER_INFO_TYPE_MAT4,
} CF_ShaderInfoDataType;

typedef struct CF_ShaderUniformMemberInfo
{
	const char* name;
	CF_ShaderInfoDataType type;
	int offset;
	int array_length;
} CF_ShaderUniformMemberInfo;

typedef struct CF_ShaderUniformInfo
{
	const char* block_name;
	int block_index;
	int block_size;
	int num_members;
} CF_ShaderUniformInfo;

typedef struct CF_ShaderInputInfo {
	const char* name;
	int location;
	CF_ShaderInfoDataType format;
} CF_ShaderInputInfo;

typedef struct CF_ShaderInfo
{
	int num_samplers;
	int num_storage_textures;
	int num_storage_buffers;

	int num_images;
	const char** image_names;

	int num_uniforms;
	CF_ShaderUniformInfo* uniforms;

	int num_uniform_members;
	CF_ShaderUniformMemberInfo* uniform_members;

	int num_inputs;
	CF_ShaderInputInfo* inputs;
} CF_ShaderInfo;

static inline const char* cf_shader_info_data_type_to_string(CF_ShaderInfoDataType type) {
	switch (type) {
	case CF_SHADER_INFO_TYPE_UNKNOWN: return "CF_SHADER_INFO_TYPE_UNKNOWN";
	case CF_SHADER_INFO_TYPE_SINT:    return "CF_SHADER_INFO_TYPE_SINT";
	case CF_SHADER_INFO_TYPE_UINT:    return "CF_SHADER_INFO_TYPE_UINT";
	case CF_SHADER_INFO_TYPE_FLOAT:   return "CF_SHADER_INFO_TYPE_FLOAT";
	case CF_SHADER_INFO_TYPE_SINT2:   return "CF_SHADER_INFO_TYPE_SINT2";
	case CF_SHADER_INFO_TYPE_UINT2:   return "CF_SHADER_INFO_TYPE_UINT2";
	case CF_SHADER_INFO_TYPE_FLOAT2:  return "CF_SHADER_INFO_TYPE_FLOAT2";
	case CF_SHADER_INFO_TYPE_SINT3:   return "CF_SHADER_INFO_TYPE_SINT3";
	case CF_SHADER_INFO_TYPE_UINT3:   return "CF_SHADER_INFO_TYPE_UINT3";
	case CF_SHADER_INFO_TYPE_FLOAT3:  return "CF_SHADER_INFO_TYPE_FLOAT3";
	case CF_SHADER_INFO_TYPE_SINT4:   return "CF_SHADER_INFO_TYPE_SINT4";
	case CF_SHADER_INFO_TYPE_UINT4:   return "CF_SHADER_INFO_TYPE_UINT4";
	case CF_SHADER_INFO_TYPE_FLOAT4:  return "CF_SHADER_INFO_TYPE_FLOAT4";
	case CF_SHADER_INFO_TYPE_MAT4:    return "CF_SHADER_INFO_TYPE_MAT4";
	}
}

#endif
