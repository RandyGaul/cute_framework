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
 * @struct   CF_ShaderWgslBinding
 * @category graphics
 * @brief    One group/binding pair declared by a shader's WGSL source.
 * @remarks  The binding's full type (texture dimension, sample type, storage format and access) is spelled out in the
 *           WGSL declaration itself.
 * @related  CF_ShaderWgslBindingKind CF_ShaderWgslSplit CF_ShaderInfo
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
	/* @member Shader reflection info. */
	CF_ShaderInfo shader_info;
	/* @member The transpiled WGSL source for WebGPU (entry point "main"), or NULL when not compiled for WebGPU. */
	const char* wgsl_src;
	/* @member Size of the WGSL source. */
	size_t wgsl_src_size;
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
