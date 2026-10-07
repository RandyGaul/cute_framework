/*
	Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

#ifndef CF_SHADER_BYTECODE_H
#define CF_SHADER_BYTECODE_H

#include "cute_defines.h"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

// Shared structures between cute-shader and cute framework.
// There should be little to no external includes since cute-shader is not depending on cute framework.

/**
 * @enum     CF_PixelFormat
 * @category graphics
 * @brief    The various supported pixel formats for GPU.
 * @remarks  Pixel format support varies depending on driver, hardware, and usage flags.
 *           The `PIXEL_FORMAT_R8G8B8A8_UNORM` represents a safe default format.
 * @related  CF_PixelFormat cf_pixel_format_to_string CF_PixelFormatOp
 */
#define CF_PIXEL_FORMAT_DEFS \
	/* @entry Invalid pixel format. */                                                         \
	CF_ENUM(PIXEL_FORMAT_INVALID,                -1)                                           \
	/* @entry 8-bit alpha channel, 8 bits total, unsigned normalized. */                       \
	CF_ENUM(PIXEL_FORMAT_A8_UNORM,                0)                                           \
	/* @entry 8-bit red channel, 8 bits total, unsigned normalized. */                         \
	CF_ENUM(PIXEL_FORMAT_R8_UNORM,                1)                                           \
	/* @entry 8-bit red/green channels, 16 bits total, unsigned normalized. */                 \
	CF_ENUM(PIXEL_FORMAT_R8G8_UNORM,              2)                                           \
	/* @entry 8-bit red/green/blue/alpha channels, 32 bits total, unsigned normalized. */      \
	CF_ENUM(PIXEL_FORMAT_R8G8B8A8_UNORM,          3)                                           \
	/* @entry 16-bit red channel, 16 bits total, unsigned normalized. */                       \
	CF_ENUM(PIXEL_FORMAT_R16_UNORM,               4)                                           \
	/* @entry 16-bit red/green channels, 32 bits total, unsigned normalized. */                \
	CF_ENUM(PIXEL_FORMAT_R16G16_UNORM,            5)                                           \
	/* @entry 16-bit red/green/blue/alpha channels, 64 bits total, unsigned normalized. */     \
	CF_ENUM(PIXEL_FORMAT_R16G16B16A16_UNORM,      6)                                           \
	/* @entry 10-bit red/green/blue channels, 2-bit alpha channel, 32 bits total, unsigned normalized. */\
	CF_ENUM(PIXEL_FORMAT_R10G10B10A2_UNORM,       7)                                           \
	/* @entry 5-bit blue, 6-bit green, 5-bit red channels, 16 bits total, unsigned normalized. */\
	CF_ENUM(PIXEL_FORMAT_B5G6R5_UNORM,            8)                                           \
	/* @entry 5-bit blue/green/red channels, 1-bit alpha channel, 16 bits total, unsigned normalized. */\
	CF_ENUM(PIXEL_FORMAT_B5G5R5A1_UNORM,          9)                                           \
	/* @entry 4-bit blue/green/red/alpha channels, 16 bits total, unsigned normalized. */      \
	CF_ENUM(PIXEL_FORMAT_B4G4R4A4_UNORM,         10)                                           \
	/* @entry 8-bit blue/green/red/alpha channels, 32 bits total, unsigned normalized. */      \
	CF_ENUM(PIXEL_FORMAT_B8G8R8A8_UNORM,         11)                                           \
	/* @entry BC1 compressed format, unsigned normalized. */                                   \
	CF_ENUM(PIXEL_FORMAT_BC1_RGBA_UNORM,         12)                                           \
	/* @entry BC2 compressed format, unsigned normalized. */                                   \
	CF_ENUM(PIXEL_FORMAT_BC2_RGBA_UNORM,         13)                                           \
	/* @entry BC3 compressed format, unsigned normalized. */                                   \
	CF_ENUM(PIXEL_FORMAT_BC3_RGBA_UNORM,         14)                                           \
	/* @entry BC4 compressed format, unsigned normalized. */                                   \
	CF_ENUM(PIXEL_FORMAT_BC4_R_UNORM,            15)                                           \
	/* @entry BC5 compressed format, unsigned normalized. */                                   \
	CF_ENUM(PIXEL_FORMAT_BC5_RG_UNORM,           16)                                           \
	/* @entry BC7 compressed format, unsigned normalized. */                                   \
	CF_ENUM(PIXEL_FORMAT_BC7_RGBA_UNORM,         17)                                           \
	/* @entry BC6H compressed format, signed float. */                                         \
	CF_ENUM(PIXEL_FORMAT_BC6H_RGB_FLOAT,         18)                                           \
	/* @entry BC6H compressed format, unsigned float. */                                       \
	CF_ENUM(PIXEL_FORMAT_BC6H_RGB_UFLOAT,        19)                                           \
	/* @entry 8-bit red channel, 8 bits total, signed normalized. */                           \
	CF_ENUM(PIXEL_FORMAT_R8_SNORM,               20)                                           \
	/* @entry 8-bit red/green channels, 16 bits total, signed normalized. */                   \
	CF_ENUM(PIXEL_FORMAT_R8G8_SNORM,             21)                                           \
	/* @entry 8-bit red/green/blue/alpha channels, 32 bits total, signed normalized. */        \
	CF_ENUM(PIXEL_FORMAT_R8G8B8A8_SNORM,         22)                                           \
	/* @entry 16-bit red channel, 16 bits total, signed normalized. */                         \
	CF_ENUM(PIXEL_FORMAT_R16_SNORM,              23)                                           \
	/* @entry 16-bit red/green channels, 32 bits total, signed normalized. */                  \
	CF_ENUM(PIXEL_FORMAT_R16G16_SNORM,           24)                                           \
	/* @entry 16-bit red/green/blue/alpha channels, 64 bits total, signed normalized. */       \
	CF_ENUM(PIXEL_FORMAT_R16G16B16A16_SNORM,     25)                                           \
	/* @entry 16-bit red channel, 16 bits total, float. */                                     \
	CF_ENUM(PIXEL_FORMAT_R16_FLOAT,              26)                                           \
	/* @entry 16-bit red/green channels, 32 bits total, float. */                              \
	CF_ENUM(PIXEL_FORMAT_R16G16_FLOAT,           27)                                           \
	/* @entry 16-bit red/green/blue/alpha channels, 64 bits total, float. */                   \
	CF_ENUM(PIXEL_FORMAT_R16G16B16A16_FLOAT,     28)                                           \
	/* @entry 32-bit red channel, 32 bits total, float. */                                     \
	CF_ENUM(PIXEL_FORMAT_R32_FLOAT,              29)                                           \
	/* @entry 32-bit red/green channels, 64 bits total, float. */                              \
	CF_ENUM(PIXEL_FORMAT_R32G32_FLOAT,           30)                                           \
	/* @entry 32-bit red/green/blue/alpha channels, 128 bits total, float. */                  \
	CF_ENUM(PIXEL_FORMAT_R32G32B32A32_FLOAT,     31)                                           \
	/* @entry 11-bit red/green channels, 10-bit blue channel, 32 bits total, unsigned float. */\
	CF_ENUM(PIXEL_FORMAT_R11G11B10_UFLOAT,       32)                                           \
	/* @entry 8-bit red channel, 8 bits total, unsigned integer. */                            \
	CF_ENUM(PIXEL_FORMAT_R8_UINT,                33)                                           \
	/* @entry 8-bit red/green channels, 16 bits total, unsigned integer. */                    \
	CF_ENUM(PIXEL_FORMAT_R8G8_UINT,              34)                                           \
	/* @entry 8-bit red/green/blue/alpha channels, 32 bits total, unsigned integer. */         \
	CF_ENUM(PIXEL_FORMAT_R8G8B8A8_UINT,          35)                                           \
	/* @entry 16-bit red-only channel, unsigned integer. */                                    \
	CF_ENUM(PIXEL_FORMAT_R16_UINT,               36)                                           \
	/* @entry 16-bit red/green channels, 32 bits total, unsigned integer. */                   \
	CF_ENUM(PIXEL_FORMAT_R16G16_UINT,            37)                                           \
	/* @entry 16-bit red/green/blue/alpha channels, 64 bits total, unsigned integer. */        \
	CF_ENUM(PIXEL_FORMAT_R16G16B16A16_UINT,      38)                                           \
	/* @entry 8-bit red channel, 8 bits total, signed integer. */                              \
	CF_ENUM(PIXEL_FORMAT_R8_INT,                 39)                                           \
	/* @entry 8-bit red/green channels, 16 bits total, signed integer. */                      \
	CF_ENUM(PIXEL_FORMAT_R8G8_INT,               40)                                           \
	/* @entry 8-bit red/green/blue/alpha channels, 32 bits total, signed integer. */           \
	CF_ENUM(PIXEL_FORMAT_R8G8B8A8_INT,           41)                                           \
	/* @entry 16-bit red channel, 16 bits total, signed integer. */                            \
	CF_ENUM(PIXEL_FORMAT_R16_INT,                42)                                           \
	/* @entry 16-bit red/green channels, 32 bits total, signed integer. */                     \
	CF_ENUM(PIXEL_FORMAT_R16G16_INT,             43)                                           \
	/* @entry 16-bit red/green/blue/alpha channels, 64 bits total, signed integer. */          \
	CF_ENUM(PIXEL_FORMAT_R16G16B16A16_INT,       44)                                           \
	/* @entry 8-bit red/green/blue/alpha channels, 32 bits total, unsigned normalized, sRGB. */\
	CF_ENUM(PIXEL_FORMAT_R8G8B8A8_UNORM_SRGB,    45)                                           \
	/* @entry 8-bit blue/green/red/alpha channels, 32 bits total, unsigned normalized, sRGB. */\
	CF_ENUM(PIXEL_FORMAT_B8G8R8A8_UNORM_SRGB,    46)                                           \
	/* @entry BC1 compressed format, unsigned normalized, sRGB. */                             \
	CF_ENUM(PIXEL_FORMAT_BC1_RGBA_UNORM_SRGB,    47)                                           \
	/* @entry BC2 compressed format, unsigned normalized, sRGB. */                             \
	CF_ENUM(PIXEL_FORMAT_BC2_RGBA_UNORM_SRGB,    48)                                           \
	/* @entry BC3 compressed format, unsigned normalized, sRGB. */                             \
	CF_ENUM(PIXEL_FORMAT_BC3_RGBA_UNORM_SRGB,    49)                                           \
	/* @entry BC7 compressed format, unsigned normalized, sRGB. */                             \
	CF_ENUM(PIXEL_FORMAT_BC7_RGBA_UNORM_SRGB,    50)                                           \
	/* @entry 16-bit depth, 16 bits total, unsigned normalized. */                             \
	CF_ENUM(PIXEL_FORMAT_D16_UNORM,              51)                                           \
	/* @entry 24-bit depth, 24 bits total, unsigned normalized. */                             \
	CF_ENUM(PIXEL_FORMAT_D24_UNORM,              52)                                           \
	/* @entry 32-bit depth, 32 bits total, float. */                                           \
	CF_ENUM(PIXEL_FORMAT_D32_FLOAT,              53)                                           \
	/* @entry 24-bit depth, 8-bit stencil, 32 bits total, unsigned normalized depth, unsigned integer stencil. */\
	CF_ENUM(PIXEL_FORMAT_D24_UNORM_S8_UINT,      54)                                           \
	/* @entry 32-bit depth, 8-bit stencil, 40 bits total, float depth, unsigned integer stencil. */\
	CF_ENUM(PIXEL_FORMAT_D32_FLOAT_S8_UINT,      55)
	/* @end */

typedef enum CF_PixelFormat
{
	#define CF_ENUM(K, V) CF_##K = V,
	CF_PIXEL_FORMAT_DEFS
	#undef CF_ENUM
} CF_PixelFormat;

/**
 * @function cf_pixel_format_to_string
 * @category graphics
 * @brief    Returns a `CF_PixelFormat` converted to a C string.
 * @related  CF_PixelFormat cf_pixel_format_to_string CF_PixelFormatOp
 */
CF_INLINE const char* cf_pixel_format_to_string(CF_PixelFormat format) {
	switch (format) {
	#define CF_ENUM(K, V) case CF_##K: return CF_STRINGIZE(CF_##K);
	CF_PIXEL_FORMAT_DEFS
	#undef CF_ENUM
	default: return NULL;
	}
}

/**
 * @enum     CF_TextureType
 * @category graphics
 * @brief    The shape of a texture: 2D, cube map, 3D, or 2D array.
 * @remarks  Matches the sampler type in the shader: `sampler2D`, `samplerCube`, `sampler3D`, or
 *           `sampler2DArray`. See `CF_TextureParams` and `cf_texture_update_layer`.
 * @related  CF_TextureType cf_texture_type_to_string CF_TextureParams cf_make_texture cf_texture_update_layer
 */
#define CF_TEXTURE_TYPE_DEFS \
	/* @entry An ordinary 2D texture (the default). */                                          \
	CF_ENUM(TEXTURE_TYPE_2D,       0)                                                           \
	/* @entry A cube map: six square 2D faces, sampled by direction with `samplerCube`. */      \
	CF_ENUM(TEXTURE_TYPE_CUBE,     1)                                                           \
	/* @entry A 3D (volume) texture, sampled with `sampler3D`. */                               \
	CF_ENUM(TEXTURE_TYPE_3D,       2)                                                           \
	/* @entry An array of 2D layers, sampled with `sampler2DArray`. */                          \
	CF_ENUM(TEXTURE_TYPE_2D_ARRAY, 3)                                                           \
	/* @end */

typedef enum CF_TextureType
{
	#define CF_ENUM(K, V) CF_##K = V,
	CF_TEXTURE_TYPE_DEFS
	#undef CF_ENUM
} CF_TextureType;

/**
 * @function cf_texture_type_to_string
 * @category graphics
 * @brief    Returns a `CF_TextureType` value as a string.
 * @related  CF_TextureType
 */
CF_INLINE const char* cf_texture_type_to_string(CF_TextureType type) {
	switch (type) {
	#define CF_ENUM(K, V) case CF_##K: return CF_STRINGIZE(CF_##K);
	CF_TEXTURE_TYPE_DEFS
	#undef CF_ENUM
	default: return NULL;
	}
}

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
 * @struct   CF_ShaderResourceInfo
 * @category graphics
 * @brief    A storage image or storage buffer a compute shader declares.
 * @related  CF_ShaderInfo CF_ShaderBytecode
 */
typedef struct CF_ShaderResourceInfo
{
	/* @member Name of the resource as declared. */
	const char* name;
	/* @member Descriptor set (0 readonly, 1 read-write). */
	int set;
	/* @member Binding within the set. */
	int binding;
	/* @member True for a readonly resource. */
	bool readonly;
} CF_ShaderResourceInfo;
// @end

/**
 * @enum     CF_ShaderWriteKind
 * @category graphics
 * @brief    What a compute shader write site writes into, for the GLES backend's compute emulation.
 * @related  CF_ShaderWriteSite
 */
#define CF_SHADER_WRITE_KIND_DEFS \
	/* @entry An `imageStore` into a storage image. */ \
	CF_ENUM(SHADER_WRITE_KIND_IMAGE,  0) \
	/* @entry An assignment into an element (or a member of one) of a storage buffer. */ \
	CF_ENUM(SHADER_WRITE_KIND_BUFFER, 1) \
	/* @end */

typedef enum CF_ShaderWriteKind
{
	#define CF_ENUM(K, V) CF_##K = V,
	CF_SHADER_WRITE_KIND_DEFS
	#undef CF_ENUM
} CF_ShaderWriteKind;

/**
 * @struct   CF_ShaderWriteSite
 * @category graphics
 * @brief    One write in a compute shader, as the GLES backend emulates it.
 * @remarks  GLES3 has no compute. Its backend runs a compute shader as a fragment pass that captures each
 *           write site's value and destination, then draws one point per invocation into the destination.
 *           Sites are numbered in source order; the transpiled shader selects the active one by uniform.
 * @related  CF_ShaderInfo CF_ShaderWriteKind
 */
typedef struct CF_ShaderWriteSite
{
	/* @member Image or buffer. */
	CF_ShaderWriteKind kind;
	/* @member The destination's name as declared. */
	const char* name;
	/* @member The destination's descriptor set. */
	int set;
	/* @member The destination's binding. */
	int binding;
	/* @member 32-bit words written: 1, 2 or 4 (always 4 for images). */
	int words;
} CF_ShaderWriteSite;
// @end

/**
 * @enum     CF_ShaderWgslBindingKind
 * @category graphics
 * @brief    What one group/binding pair of a shader's WGSL source holds, for the WebGPU backend.
 * @remarks  CF descriptor set N is WGSL group N. Inside a resource group, sampled texture i (in binding order) is
 *           binding 2i and its sampler binding 2i+1; storage textures follow, then storage buffers, then the load
 *           sides of split storage images (see `CF_ShaderWgslSplit`). Inside a uniform group, block slot u is
 *           binding u.
 * @related  CF_ShaderWgslBinding CF_ShaderWgslSplit CF_ShaderInfo
 */
#define CF_SHADER_WGSL_BINDING_KIND_DEFS \
	/* @entry A sampled texture: `texture_2d<f32>`, `texture_cube<f32>`, `texture_depth_2d` and so on. */ \
	CF_ENUM(SHADER_WGSL_BINDING_KIND_SAMPLED_TEXTURE,    0) \
	/* @entry The sampler paired with a sampled texture: `sampler` or `sampler_comparison`. */ \
	CF_ENUM(SHADER_WGSL_BINDING_KIND_SAMPLER,            1) \
	/* @entry A storage texture: `texture_storage_2d<format, access>`. */ \
	CF_ENUM(SHADER_WGSL_BINDING_KIND_STORAGE_TEXTURE,    2) \
	/* @entry A storage buffer: `var<storage, read>` or `var<storage, read_write>`. */ \
	CF_ENUM(SHADER_WGSL_BINDING_KIND_STORAGE_BUFFER,     3) \
	/* @entry A uniform block: `var<uniform>`, laid out to match its std140 bytes. */ \
	CF_ENUM(SHADER_WGSL_BINDING_KIND_UNIFORM_BUFFER,     4) \
	/* @entry The `texture_2d` load side of a split storage image, see `CF_ShaderWgslSplit`. */ \
	CF_ENUM(SHADER_WGSL_BINDING_KIND_SPLIT_LOAD_TEXTURE, 5) \
	/* @end */

typedef enum CF_ShaderWgslBindingKind
{
	#define CF_ENUM(K, V) CF_##K = V,
	CF_SHADER_WGSL_BINDING_KIND_DEFS
	#undef CF_ENUM
} CF_ShaderWgslBindingKind;

/**
 * @function cf_shader_wgsl_binding_kind_to_string
 * @category graphics
 * @brief    Returns a `CF_ShaderWgslBindingKind` converted to a C string.
 * @related  CF_ShaderWgslBindingKind
 */
static inline const char* cf_shader_wgsl_binding_kind_to_string(CF_ShaderWgslBindingKind kind)
{
	switch (kind) {
	#define CF_ENUM(K, V) case CF_##K: return "CF_" #K;
	CF_SHADER_WGSL_BINDING_KIND_DEFS
	#undef CF_ENUM
	}
	return NULL;
}

/**
 * @enum     CF_ShaderWgslSampleType
 * @category graphics
 * @brief    The sample type of a sampled texture in a shader's WGSL source.
 * @related  CF_ShaderWgslBinding
 */
#define CF_SHADER_WGSL_SAMPLE_TYPE_DEFS \
	/* @entry `texture_2d<f32>` and the like, bound to a filterable texture. */ \
	CF_ENUM(SHADER_WGSL_SAMPLE_TYPE_FLOAT,              0) \
	/* @entry `f32` textures that cannot be filtered, such as 32-bit float formats without float32-filterable. */ \
	CF_ENUM(SHADER_WGSL_SAMPLE_TYPE_UNFILTERABLE_FLOAT, 1) \
	/* @entry `texture_depth_2d` and the like. */ \
	CF_ENUM(SHADER_WGSL_SAMPLE_TYPE_DEPTH,              2) \
	/* @entry `texture_2d<i32>` and the like. */ \
	CF_ENUM(SHADER_WGSL_SAMPLE_TYPE_SINT,               3) \
	/* @entry `texture_2d<u32>` and the like. */ \
	CF_ENUM(SHADER_WGSL_SAMPLE_TYPE_UINT,               4) \
	/* @end */

typedef enum CF_ShaderWgslSampleType
{
	#define CF_ENUM(K, V) CF_##K = V,
	CF_SHADER_WGSL_SAMPLE_TYPE_DEFS
	#undef CF_ENUM
} CF_ShaderWgslSampleType;

/**
 * @function cf_shader_wgsl_sample_type_to_string
 * @category graphics
 * @brief    Returns a `CF_ShaderWgslSampleType` converted to a C string.
 * @related  CF_ShaderWgslSampleType
 */
static inline const char* cf_shader_wgsl_sample_type_to_string(CF_ShaderWgslSampleType type)
{
	switch (type) {
	#define CF_ENUM(K, V) case CF_##K: return "CF_" #K;
	CF_SHADER_WGSL_SAMPLE_TYPE_DEFS
	#undef CF_ENUM
	}
	return NULL;
}

/**
 * @enum     CF_ShaderWgslAccess
 * @category graphics
 * @brief    The access mode of a storage texture or storage buffer in a shader's WGSL source.
 * @related  CF_ShaderWgslBinding
 */
#define CF_SHADER_WGSL_ACCESS_DEFS \
	/* @entry `read`. */ \
	CF_ENUM(SHADER_WGSL_ACCESS_READ,       0) \
	/* @entry `write`. Storage textures only. */ \
	CF_ENUM(SHADER_WGSL_ACCESS_WRITE,      1) \
	/* @entry `read_write`. */ \
	CF_ENUM(SHADER_WGSL_ACCESS_READ_WRITE, 2) \
	/* @end */

typedef enum CF_ShaderWgslAccess
{
	#define CF_ENUM(K, V) CF_##K = V,
	CF_SHADER_WGSL_ACCESS_DEFS
	#undef CF_ENUM
} CF_ShaderWgslAccess;

/**
 * @function cf_shader_wgsl_access_to_string
 * @category graphics
 * @brief    Returns a `CF_ShaderWgslAccess` converted to a C string.
 * @related  CF_ShaderWgslAccess
 */
static inline const char* cf_shader_wgsl_access_to_string(CF_ShaderWgslAccess access)
{
	switch (access) {
	#define CF_ENUM(K, V) case CF_##K: return "CF_" #K;
	CF_SHADER_WGSL_ACCESS_DEFS
	#undef CF_ENUM
	}
	return NULL;
}

/**
 * @struct   CF_ShaderWgslBinding
 * @category graphics
 * @brief    One group/binding pair declared by a shader's WGSL source, with everything a bind group layout entry needs.
 * @remarks  Members that do not apply to the binding's `kind` are zero.
 * @related  CF_ShaderWgslBindingKind CF_ShaderWgslSampleType CF_ShaderWgslAccess CF_ShaderWgslSplit CF_ShaderInfo
 */
typedef struct CF_ShaderWgslBinding
{
	/* @member The sampler, image or block this binding serves, as declared in the shader. */
	const char* name;
	/* @member What the binding holds. */
	CF_ShaderWgslBindingKind kind;
	/* @member The CF descriptor set, equal to the WGSL group. */
	int set;
	/* @member The resource's binding within its CF set, as declared in the shader. */
	int slot;
	/* @member The WGSL binding. */
	int binding;
	/* @member Sampled, split-load and storage textures: the texture's shape. */
	CF_TextureType dimension;
	/* @member Sampled and split-load textures: the sample type. */
	CF_ShaderWgslSampleType sample_type;
	/* @member Sampled textures: true for `texture_multisampled_2d` and `texture_depth_multisampled_2d`. */
	bool multisampled;
	/* @member Storage and split-load textures: the image's texel format, or `CF_PIXEL_FORMAT_INVALID` for a format `CF_PixelFormat` lacks (`r32ui`). */
	CF_PixelFormat storage_format;
	/* @member Storage textures and storage buffers: the access mode. */
	CF_ShaderWgslAccess storage_access;
	/* @member Samplers: true for `sampler_comparison`. */
	bool comparison;
} CF_ShaderWgslBinding;
// @end

/**
 * @struct   CF_ShaderWgslSplit
 * @category graphics
 * @brief    A storage image a shader both loads and stores, split in two for WebGPU.
 * @remarks  WebGPU only allows read-write storage access for r32 formats, and forbids binding one texture as both a
 *           writable storage texture and a sampled texture in one dispatch. A shader that loads and stores an image of
 *           any other format stores through a write-only storage texture at `store_binding`, and loads through a
 *           `texture_2d` at `load_binding`. Bind a copy of the image, taken before the dispatch, as the load side.
 * @related  CF_ShaderWgslBinding CF_ShaderInfo
 */
typedef struct CF_ShaderWgslSplit
{
	/* @member The image's name as declared. */
	const char* name;
	/* @member The CF descriptor set, equal to the WGSL group. */
	int set;
	/* @member The WGSL binding of the write-only storage texture. */
	int store_binding;
	/* @member The WGSL binding of the `texture_2d` load side. */
	int load_binding;
} CF_ShaderWgslSplit;
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
	/* @member Number of readonly storage textures. */
	int num_storage_textures;
	/* @member Number of readonly storage buffers. */
	int num_storage_buffers;
	/* @member Number of readwrite storage textures. */
	int num_readwrite_storage_textures;
	/* @member Number of readwrite storage buffers. */
	int num_readwrite_storage_buffers;

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

	/* @member Compute workgroup size (zero for non-compute shaders). Metal pipelines dispatch with this. */
	int local_size[3];

	/* @member Number of storage images (compute), readonly and read-write. */
	int num_storage_image_infos;
	/* @member Each storage image: name, set, binding, readonly. The GLES backend binds them by name. */
	CF_ShaderResourceInfo* storage_image_infos;

	/* @member Number of storage buffers (compute), readonly and read-write. */
	int num_storage_buffer_infos;
	/* @member Each storage buffer: name, set, binding, readonly. */
	CF_ShaderResourceInfo* storage_buffer_infos;

	/* @member Number of write sites in a compute shader's GLSL ES output (zero elsewhere). */
	int num_write_sites;
	/* @member The GLSL ES compute emulation's write sites, see `CF_ShaderWriteSite`. */
	CF_ShaderWriteSite* write_sites;

	/* @member Number of bindings the WGSL source declares (zero without WGSL). */
	int num_wgsl_bindings;
	/* @member Every group/binding pair of the WGSL source, see `CF_ShaderWgslBinding`. */
	CF_ShaderWgslBinding* wgsl_bindings;

	/* @member Number of split storage images in the WGSL source. */
	int num_wgsl_splits;
	/* @member Storage images split into a store side and a load side, see `CF_ShaderWgslSplit`. */
	CF_ShaderWgslSplit* wgsl_splits;
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
	/* @member The transpiled HLSL SM 5.1 source for D3D12 (compiled to DXBC by the system FXC at runtime). */
	const char* hlsl_src;
	/* @member Size of the HLSL source. */
	size_t hlsl_src_size;
	/* @member The transpiled MSL source for Metal (compiled by the OS at runtime; entry point "main0"). */
	const char* msl_src;
	/* @member Size of the MSL source. */
	size_t msl_src_size;
	/* @member The transpiled WGSL source for WebGPU (entry point "main"), or NULL when not compiled for WebGPU. */
	const char* wgsl_src;
	/* @member Size of the WGSL source. */
	size_t wgsl_src_size;
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
