/*
	Cute Framework
	Copyright (C) 2019 Randy Gaul https://randygaul.net

	This software is provided 'as-is', without any express or implied
	warranty.  In no event will the authors be held liable for any damages
	arising from the use of this software.

	Permission is granted to anyone to use this software for any purpose,
	including commercial applications, and to alter it and redistribute it
	freely, subject to the following restrictions:

	1. The origin of this software must not be misrepresented; you must not
	   claim that you wrote the original software. If you use this software
	   in a product, an acknowledgment in the product documentation would be
	   appreciated but is not required.
	2. Altered source versions must be plainly marked as such, and must not be
	   misrepresented as being the original software.
	3. This notice may not be removed or altered from any source distribution.
*/

#ifndef CUTE_GFX_H
#define CUTE_GFX_H

#include "cute_defines.h"
#include "cute_result.h"
#include "cute_app.h"
#include "cute_color.h"
#include "cute_c_runtime.h"

#include "sokol/sokol_gfx.h"

//--------------------------------------------------------------------------------------------------
// C API

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

/**
 * This header wraps low-level 3D rendering APIs. Since these functions are rather low-level it's
 * totally possible to implement 3D games with them, however, the focus of Cute Framework is on 2D
 * games. You're probably looking for other headers in Cute Framework, not this one.
 * 
 * If you want to draw sprites, see: cute_sprite.h
 * If you want to draw lines or shapes, see: cute_batch.h
 * If you want to draw text, see: cute_font.h and cute_batch.h
 * 
 * Here's more details for advanced graphics users:
 * The general flow is to organize your rendering into passes and pipelines, and they operate with
 * buffers, images, and shaders.
 * 
 *     Buffer   - Contains vertex (e.g. triangles), indices, or instance data.
 * 
 *     Texture  - An image on the GPU. Can also be used to store arbitrary data.
 * 
 *     Pass     - Will render to a texture. This texture could be the "framebuffer" (your screen),
 *                but could also be an off-screen intermediate texture. Each pass starts off with
 *                an init step to copy contents from a previous pass, or clear to a certain color.
 * 
 *     Shader   - A small program that runs on the GPU. It's split up into vertex/fragment shader
 *                stages. A shader also can optionally have a collection of uniforms for input. The
 *                primary inputs to shaders are buffers and textures.
 * 
 *     Uniform  - Variables that get passed into your shader. Usually these are like "globals" in
 *                your shader program to control settings or colors.
 * 
 *     Pipeline - Contains all the state needed for rendering. This includes all buffers, textures,
 *                a shader, and all other rendering state and settings required.
 * 
 * The typical structure of a frame for one game tick looks something like this:
 * 
 *     for each pass {
 *         CF_BeginPass(pass);
 *         for each pipeline {
 *             CF_ApplyViewport(...); // Optional.
 *             CF_ApplyScissor(...); // Optional.
 *             CF_ApplyPipeline(pipeline);
 *             CF_ApplyBindings(bindings);
 *             for each uniform setup {
 *                 CF_ApplyVSUniforms(...); // Optional.
 *                 CF_ApplyFSUniforms(...); // Optional.
 *                 CF_DrawElements(...); // One draw call.
 *             }
 *         }
 *         CF_EndPass();
 *     }
 *     CF_Commit();
 */
	
#define CF_TEXTURE_ATTACHMENTS_MAX (4)
#define CF_SHADER_BINDINGS_BUFFER_MAX (8)
#define CF_SHADER_BINDINGS_TEXTURE_MAX (12)
#define CF_UNIFORMS_MAX (16)
#define CF_VERTEX_ATTRIBUTES_MAX (16)
#define CF_MIPMAPS_MAX (16)

//--------------------------------------------------------------------------------------------------
// Device info and capabilities.

#define CF_BACKEND_TYPE_DEFS \
	CF_ENUM(BACKEND_TYPE_GLCORE33,        0) \
	CF_ENUM(BACKEND_TYPE_GLES2,           1) \
	CF_ENUM(BACKEND_TYPE_GLES3,           2) \
	CF_ENUM(BACKEND_TYPE_D3D11,           3) \
	CF_ENUM(BACKEND_TYPE_METAL_IOS,       4) \
	CF_ENUM(BACKEND_TYPE_METAL_MACOS,     5) \
	CF_ENUM(BACKEND_TYPE_METAL_SIMULATOR, 6) \
	CF_ENUM(BACKEND_TYPE_WGPU,            7) \

typedef enum CF_BackendType
{
	#define CF_ENUM(K, V) CF_##K = V,
	CF_BACKEND_TYPE_DEFS
	#undef CF_ENUM
} CF_BackendType;

CUTE_API CF_BackendType CUTE_CALL CF_QueryBackend();

#define CF_PIXELFORMAT_DEFS \
	CF_ENUM(PIXELFORMAT_DEFAULT,         0 ) \
	CF_ENUM(PIXELFORMAT_R8,              1 ) \
	CF_ENUM(PIXELFORMAT_R8SN,            2 ) \
	CF_ENUM(PIXELFORMAT_R8UI,            3 ) \
	CF_ENUM(PIXELFORMAT_R8SI,            4 ) \
	CF_ENUM(PIXELFORMAT_R16,             5 ) \
	CF_ENUM(PIXELFORMAT_R16SN,           6 ) \
	CF_ENUM(PIXELFORMAT_R16UI,           7 ) \
	CF_ENUM(PIXELFORMAT_R16SI,           8 ) \
	CF_ENUM(PIXELFORMAT_R16F,            9 ) \
	CF_ENUM(PIXELFORMAT_RG8,             10) \
	CF_ENUM(PIXELFORMAT_RG8SN,           11) \
	CF_ENUM(PIXELFORMAT_RG8UI,           12) \
	CF_ENUM(PIXELFORMAT_RG8SI,           13) \
	CF_ENUM(PIXELFORMAT_R32UI,           14) \
	CF_ENUM(PIXELFORMAT_R32SI,           15) \
	CF_ENUM(PIXELFORMAT_R32F,            16) \
	CF_ENUM(PIXELFORMAT_RG16,            17) \
	CF_ENUM(PIXELFORMAT_RG16SN,          18) \
	CF_ENUM(PIXELFORMAT_RG16UI,          19) \
	CF_ENUM(PIXELFORMAT_RG16SI,          20) \
	CF_ENUM(PIXELFORMAT_RG16F,           21) \
	CF_ENUM(PIXELFORMAT_RGBA8,           22) \
	CF_ENUM(PIXELFORMAT_RGBA8SN,         23) \
	CF_ENUM(PIXELFORMAT_RGBA8UI,         24) \
	CF_ENUM(PIXELFORMAT_RGBA8SI,         25) \
	CF_ENUM(PIXELFORMAT_BGRA8,           26) \
	CF_ENUM(PIXELFORMAT_RGB10A2,         27) \
	CF_ENUM(PIXELFORMAT_RG11B10F,        28) \
	CF_ENUM(PIXELFORMAT_RG32UI,          29) \
	CF_ENUM(PIXELFORMAT_RG32SI,          30) \
	CF_ENUM(PIXELFORMAT_RG32F,           31) \
	CF_ENUM(PIXELFORMAT_RGBA16,          32) \
	CF_ENUM(PIXELFORMAT_RGBA16SN,        33) \
	CF_ENUM(PIXELFORMAT_RGBA16UI,        34) \
	CF_ENUM(PIXELFORMAT_RGBA16SI,        35) \
	CF_ENUM(PIXELFORMAT_RGBA16F,         36) \
	CF_ENUM(PIXELFORMAT_RGBA32UI,        37) \
	CF_ENUM(PIXELFORMAT_RGBA32SI,        38) \
	CF_ENUM(PIXELFORMAT_RGBA32F,         39) \
	CF_ENUM(PIXELFORMAT_DEPTH,           40) \
	CF_ENUM(PIXELFORMAT_DEPTH_STENCIL,   41) \
	CF_ENUM(PIXELFORMAT_BC1_RGBA,        42) \
	CF_ENUM(PIXELFORMAT_BC2_RGBA,        43) \
	CF_ENUM(PIXELFORMAT_BC3_RGBA,        44) \
	CF_ENUM(PIXELFORMAT_BC4_R,           45) \
	CF_ENUM(PIXELFORMAT_BC4_RSN,         46) \
	CF_ENUM(PIXELFORMAT_BC5_RG,          47) \
	CF_ENUM(PIXELFORMAT_BC5_RGSN,        48) \
	CF_ENUM(PIXELFORMAT_BC6H_RGBF,       49) \
	CF_ENUM(PIXELFORMAT_BC6H_RGBUF,      50) \
	CF_ENUM(PIXELFORMAT_BC7_RGBA,        51) \
	CF_ENUM(PIXELFORMAT_PVRTC_RGB_2BPP,  52) \
	CF_ENUM(PIXELFORMAT_PVRTC_RGB_4BPP,  53) \
	CF_ENUM(PIXELFORMAT_PVRTC_RGBA_2BPP, 54) \
	CF_ENUM(PIXELFORMAT_PVRTC_RGBA_4BPP, 55) \
	CF_ENUM(PIXELFORMAT_ETC2_RGB8,       56) \
	CF_ENUM(PIXELFORMAT_ETC2_RGB8A1,     57) \
	CF_ENUM(PIXELFORMAT_ETC2_RGBA8,      58) \
	CF_ENUM(PIXELFORMAT_ETC2_RG11,       59) \
	CF_ENUM(PIXELFORMAT_ETC2_RG11SN,     60) \
	CF_ENUM(PIXELFORMAT_COUNT,           61) \

typedef enum CF_PixelFormat
{
	#define CF_ENUM(K, V) CF_##K = V,
	CF_PIXELFORMAT_DEFS
	#undef CF_ENUM
} CF_PixelFormat;

#define CF_PIXELFORMAT_OP_DEFS \
	CF_ENUM(PIXELFORMAT_OP_NEAREST_FILTER,  0) \
	CF_ENUM(PIXELFORMAT_OP_BILINEAR_FILTER, 1) \
	CF_ENUM(PIXELFORMAT_OP_RENDER_TARGET,   2) \
	CF_ENUM(PIXELFORMAT_OP_ALPHA_BLENDING,  3) \
	CF_ENUM(PIXELFORMAT_OP_MSAA,            4) \
	CF_ENUM(PIXELFORMAT_OP_DEPTH,           5) \

typedef enum CF_PixelFormatOp
{
	#define CF_ENUM(K, V) CF_##K = V,
	CF_PIXELFORMAT_OP_DEFS
	#undef CF_ENUM
} CF_PixelFormatOp;

CUTE_API bool CUTE_CALL CF_QueryPixelFormat(CF_PixelFormat format, CF_PixelFormatOp op);

#define CF_DEVICE_FEATURE_DEFS \
	CF_ENUM(DEVICE_FEATURE_INSTANCING,       0) \
	CF_ENUM(DEVICE_FEATURE_MRT,              1) \
	CF_ENUM(DEVICE_FEATURE_MRT_BLEND_STATES, 2) \
	CF_ENUM(DEVICE_FEATURE_MRT_WRITE_MASKS,  3) \
	CF_ENUM(DEVICE_FEATURE_MSAA,             4) \
	CF_ENUM(DEVICE_FEATURE_TEXTURE_ARRAY,    5) \
	CF_ENUM(DEVICE_FEATURE_TEXTURE_CLAMP,    6) \

typedef enum CF_DeviceFeature
{
	#define CF_ENUM(K, V) CF_##K = V,
	CF_DEVICE_FEATURE_DEFS
	#undef CF_ENUM
} CF_DeviceFeature;

CUTE_API bool CUTE_CALL CF_QueryDeviceFeature(CF_DeviceFeature feature);

#define CF_RESOURCE_LIMIT_DEFS \
	CF_ENUM(RESOURCE_LIMIT_TEXTURE_DIMENSION,       0) \
	CF_ENUM(RESOURCE_LIMIT_TEXTURE_ARRAY_DIMENSION, 1) \
	CF_ENUM(RESOURCE_LIMIT_TEXTURE_ARRAY_LAYER,     2) \
	CF_ENUM(RESOURCE_LIMIT_TEXTURE_CUBE_DIMENSION,  3) \
	CF_ENUM(RESOURCE_LIMIT_VERTEX_ATTRIBUTE_MAX,    4) \

typedef enum CF_ResourceLimit
{
	#define CF_ENUM(K, V) CF_##K = V,
	CF_RESOURCE_LIMIT_DEFS
	#undef CF_ENUM
} CF_ResourceLimit;

CUTE_API int CUTE_CALL CF_QueryResourceLimit(CF_ResourceLimit resource_limit);

//--------------------------------------------------------------------------------------------------
// GPU Resources.

typedef struct CF_Buffer { uint64_t id; } CF_Buffer;
typedef struct CF_Texture { uint64_t id; } CF_Texture;
typedef struct CF_Shader { uint64_t id; } CF_Shader;
typedef struct CF_Pass { uint64_t id; } CF_Pass;
typedef struct CF_Pipeline { uint64_t id; } CF_Pipeline;

#define CF_USAGE_TYPE_DEFS \
	CF_ENUM(USAGE_TYPE_IMMUTABLE, 0) \
	CF_ENUM(USAGE_TYPE_DYNAMIC,   1) \
	CF_ENUM(USAGE_TYPE_STREAM,    2) \

typedef enum CF_UsageType
{
	#define CF_ENUM(K, V) CF_##K = V,
	CF_USAGE_TYPE_DEFS
	#undef CF_ENUM
} CF_UsageType;

#define CF_BUFFER_TYPE_DEFS \
	CF_ENUM(BUFFER_TYPE_VERTEX,   0) \
	CF_ENUM(BUFFER_TYPE_INDEX,    1) \

typedef enum CF_BufferType
{
	#define CF_ENUM(K, V) CF_##K = V,
	CF_BUFFER_TYPE_DEFS
	#undef CF_ENUM
} CF_BufferType;

#define CF_INDEX_TYPE_DEFS \
	CF_ENUM(INDEX_TYPE_NONE, 0) \
	CF_ENUM(INDEX_TYPE_U16,  1) \
	CF_ENUM(INDEX_TYPE_U32,  2) \

typedef enum CF_IndexType
{
	#define CF_ENUM(K, V) CF_##K = V,
	CF_INDEX_TYPE_DEFS
	#undef CF_ENUM
} CF_IndexType;

CUTE_API CF_Buffer CUTE_CALL CF_MakeBuffer(CF_BufferType type, CF_UsageType usage, void* data, int size);
CUTE_API void CUTE_CALL CF_DestroyBuffer(CF_Buffer buffer);
CUTE_API void CUTE_CALL CF_UpdateBuffer(CF_Buffer buffer, void* data, int size);
CUTE_API int CUTE_CALL CF_AppendBuffer(CF_Buffer buffer, void* data, int size);
CUTE_API bool CUTE_CALL CF_QueryBufferWillOverflow(CF_Buffer buffer, int size);

#define CF_TEXTURE_TYPE_DEFS \
	CF_ENUM(TEXTURE_TYPE_DEFAULT,  0) \
	CF_ENUM(TEXTURE_TYPE_2D,       1) \
	CF_ENUM(TEXTURE_TYPE_2D_ARRAY, 2) \
	CF_ENUM(TEXTURE_TYPE_CUBE,     3) \

typedef enum CF_TextureType
{
	#define CF_ENUM(K, V) CF_##K = V,
	CF_TEXTURE_TYPE_DEFS
	#undef CF_ENUM
} CF_TextureType;

#define CF_CUBE_FACE_INDEX_DEFS \
	CF_ENUM(CUBE_FACE_INDEX_POS_X, 0) \
	CF_ENUM(CUBE_FACE_INDEX_NEG_X, 1) \
	CF_ENUM(CUBE_FACE_INDEX_POS_Y, 2) \
	CF_ENUM(CUBE_FACE_INDEX_NEG_Y, 3) \
	CF_ENUM(CUBE_FACE_INDEX_POS_Z, 4) \
	CF_ENUM(CUBE_FACE_INDEX_NEG_Z, 5) \
	CF_ENUM(CUBE_FACE_COUNT,       6) \

typedef enum CF_CubeFaceIndex
{
	#define CF_ENUM(K, V) CF_##K = V,
	CF_CUBE_FACE_INDEX_DEFS
	#undef CF_ENUM
} CF_CubeFaceIndex;

#define CF_PRIMITIVE_TYPE_DEFS \
	CF_ENUM(PRIMITIVE_TYPE_POINT,          0) \
	CF_ENUM(PRIMITIVE_TYPE_LINE,           1) \
	CF_ENUM(PRIMITIVE_TYPE_LINE_STRIP,     2) \
	CF_ENUM(PRIMITIVE_TYPE_TRIANGLE,       3) \
	CF_ENUM(PRIMITIVE_TYPE_TRIANGLE_STRIP, 4) \

typedef enum CF_PrimitiveType
{
	#define CF_ENUM(K, V) CF_##K = V,
	CF_PRIMITIVE_TYPE_DEFS
	#undef CF_ENUM
} CF_PrimitiveType;

#define CF_MINMAG_FILTER_DEFS \
	CF_ENUM(MINMAG_FILTER_NEAREST, 0) \
	CF_ENUM(MINMAG_FILTER_LINEAR,  1) \

typedef enum CF_MinMagFilter
{
	#define CF_ENUM(K, V) CF_##K = V,
	CF_MINMAG_FILTER_DEFS
	#undef CF_ENUM
} CF_MinMagFilter;

#define CF_MIP_FILTER_DEFS \
	CF_ENUM(MIP_FILTER_NONE,    0) \
	CF_ENUM(MIP_FILTER_NEAREST, 1) \
	CF_ENUM(MIP_FILTER_LINEAR,  2) \

typedef enum CF_MipFilter
{
	#define CF_ENUM(K, V) CF_##K = V,
	CF_MIP_FILTER_DEFS
	#undef CF_ENUM
} CF_MipFilter;

#define CF_WRAP_MODE_DEFS \
	CF_ENUM(WRAP_MODE_DEFAULT,         0) \
	CF_ENUM(WRAP_MODE_REPEAT,          1) \
	CF_ENUM(WRAP_MODE_CLAMP_TO_EDGE,   2) \
	CF_ENUM(WRAP_MODE_CLAMP_TO_BORDER, 3) \
	CF_ENUM(WRAP_MODE_MIRRORED_REPEAT, 4) \

typedef enum CF_WrapMode
{
	#define CF_ENUM(K, V) CF_##K = V,
	CF_WRAP_MODE_DEFS
	#undef CF_ENUM
} CF_WrapMode;

#define CF_BORDER_COLOR_DEFS \
	CF_ENUM(BORDER_COLOR_DEFAULT,           0) \
	CF_ENUM(BORDER_COLOR_TRANSPARENT_BLACK, 1) \
	CF_ENUM(BORDER_COLOR_OPAQUE_BLACK,      2) \
	CF_ENUM(BORDER_COLOR_OPAQUE_WHITE,      3) \

typedef enum CF_BorderColor
{
	#define CF_ENUM(K, V) CF_##K = V,
	CF_BORDER_COLOR_DEFS
	#undef CF_ENUM
} CF_BorderColor;

#define CF_VERTEX_FORMAT_DEFS  \
	CF_ENUM(VERTEX_FORMAT_INVALID,  0 ) \
	CF_ENUM(VERTEX_FORMAT_FLOAT,    1 ) \
	CF_ENUM(VERTEX_FORMAT_FLOAT2,   2 ) \
	CF_ENUM(VERTEX_FORMAT_FLOAT3,   3 ) \
	CF_ENUM(VERTEX_FORMAT_FLOAT4,   4 ) \
	CF_ENUM(VERTEX_FORMAT_BYTE4,    5 ) \
	CF_ENUM(VERTEX_FORMAT_BYTE4N,   6 ) \
	CF_ENUM(VERTEX_FORMAT_UBYTE4,   7 ) \
	CF_ENUM(VERTEX_FORMAT_UBYTE4N,  8 ) \
	CF_ENUM(VERTEX_FORMAT_SHORT2,   9 ) \
	CF_ENUM(VERTEX_FORMAT_SHORT2N,  10) \
	CF_ENUM(VERTEX_FORMAT_USHORT2N, 11) \
	CF_ENUM(VERTEX_FORMAT_SHORT4,   12) \
	CF_ENUM(VERTEX_FORMAT_SHORT4N,  13) \
	CF_ENUM(VERTEX_FORMAT_USHORT4N, 14) \

typedef enum CF_VertexFormat
{
	#define CF_ENUM(K, V) CF_##K = V,
	CF_VERTEX_FORMAT_DEFS
	#undef CF_ENUM
} CF_VertexFormat;

typedef struct CF_TextureData
{
	void* sub_image[CF_CUBE_FACE_COUNT][CF_MIPMAPS_MAX];
	int size[CF_CUBE_FACE_COUNT][CF_MIPMAPS_MAX];
} CF_TextureData;

typedef struct CF_TextureParams
{
	CF_TextureType type;
	CF_PixelFormat pixel_format;
	CF_UsageType usage;
	CF_MinMagFilter min_filter;
	CF_MinMagFilter mag_filter;
	CF_MipFilter mip_filter;
	CF_WrapMode wrap_u;
	CF_WrapMode wrap_v;
	CF_BorderColor border_color;
	int width;
	int height;
	int array_layers;
	int max_anisotropy;
	int mip_count;
	int msaa_sample_count;
	bool render_target;
	CF_TextureData initial_data;
} CF_TextureParams;

CUTE_API CF_TextureParams CUTE_CALL CF_TextureDefaults();
CUTE_API CF_Texture CUTE_CALL CF_MakeTexture(CF_TextureParams texture_params);
CUTE_API void CUTE_CALL CF_DestroyTexture(CF_Texture texture);
CUTE_API void CUTE_CALL CF_UpdateTexture(CF_Texture texture, CF_TextureData data);

//--------------------------------------------------------------------------------------------------
// Render states.

#define CF_CULL_MODE_DEFS \
	CF_ENUM(CULL_MODE_NONE,  0) \
	CF_ENUM(CULL_MODE_FRONT, 1) \
	CF_ENUM(CULL_MODE_BACK,  2) \

typedef enum CF_CullMode
{
	#define CF_ENUM(K, V) CF_##K = V,
	CF_CULL_MODE_DEFS
	#undef CF_ENUM
} CF_CullMode;

#define CF_COMPARE_FUNCTION_DEFS \
	CF_ENUM(COMPARE_FUNCTION_ALWAYS,                0) \
	CF_ENUM(COMPARE_FUNCTION_NEVER,                 1) \
	CF_ENUM(COMPARE_FUNCTION_LESS_THAN,             2) \
	CF_ENUM(COMPARE_FUNCTION_EQUAL,                 3) \
	CF_ENUM(COMPARE_FUNCTION_NOT_EQUAL,             4) \
	CF_ENUM(COMPARE_FUNCTION_LESS_THAN_OR_EQUAL,    5) \
	CF_ENUM(COMPARE_FUNCTION_GREATER_THAN,          6) \
	CF_ENUM(COMPARE_FUNCTION_GREATER_THAN_OR_EQUAL, 7) \

typedef enum CF_CompareFunction
{
	#define CF_ENUM(K, V) CF_##K = V,
	CF_COMPARE_FUNCTION_DEFS
	#undef CF_ENUM
} CF_CompareFunction;

#define CF_STENCIL_OP_DEFS \
	CF_ENUM(STENCIL_OP_KEEP,            0) \
	CF_ENUM(STENCIL_OP_ZERO,            1) \
	CF_ENUM(STENCIL_OP_REPLACE,         2) \
	CF_ENUM(STENCIL_OP_INCREMENT_CLAMP, 3) \
	CF_ENUM(STENCIL_OP_DECREMENT_CLAMP, 4) \
	CF_ENUM(STENCIL_OP_INVERT,          5) \
	CF_ENUM(STENCIL_OP_INCREMENT_WRAP,  6) \
	CF_ENUM(STENCIL_OP_DECREMENT_WRAP,  7) \

typedef enum CF_StencilOp
{
	#define CF_ENUM(K, V) CF_##K = V,
	CF_STENCIL_OP_DEFS
	#undef CF_ENUM
} CF_StencilOp;

#define CF_BLEND_OP_DEFS \
	CF_ENUM(BLEND_OP_ADD,              0) \
	CF_ENUM(BLEND_OP_SUBTRACT,         1) \
	CF_ENUM(BLEND_OP_REVERSE_SUBTRACT, 2) \

typedef enum CF_BlendOp
{
	#define CF_ENUM(K, V) CF_##K = V,
	CF_BLEND_OP_DEFS
	#undef CF_ENUM
} CF_BlendOp;

#define CF_BLEND_FACTOR_DEFS \
	CF_ENUM(BLENDFACTOR_ZERO,                  0 ) \
	CF_ENUM(BLENDFACTOR_ONE,                   1 ) \
	CF_ENUM(BLENDFACTOR_SRC_COLOR,             2 ) \
	CF_ENUM(BLENDFACTOR_ONE_MINUS_SRC_COLOR,   3 ) \
	CF_ENUM(BLENDFACTOR_SRC_ALPHA,             4 ) \
	CF_ENUM(BLENDFACTOR_ONE_MINUS_SRC_ALPHA,   5 ) \
	CF_ENUM(BLENDFACTOR_DST_COLOR,             6 ) \
	CF_ENUM(BLENDFACTOR_ONE_MINUS_DST_COLOR,   7 ) \
	CF_ENUM(BLENDFACTOR_DST_ALPHA,             8 ) \
	CF_ENUM(BLENDFACTOR_ONE_MINUS_DST_ALPHA,   9 ) \
	CF_ENUM(BLENDFACTOR_SRC_ALPHA_SATURATED,   10) \
	CF_ENUM(BLENDFACTOR_BLEND_COLOR,           11) \
	CF_ENUM(BLENDFACTOR_ONE_MINUS_BLEND_COLOR, 12) \
	CF_ENUM(BLENDFACTOR_BLEND_ALPHA,           13) \
	CF_ENUM(BLENDFACTOR_ONE_MINUS_BLEND_ALPHA, 14) \

typedef enum CF_BlendFactor
{
	#define CF_ENUM(K, V) CF_##K = V,
	CF_BLEND_FACTOR_DEFS
	#undef CF_ENUM
} CF_BlendFactor;

//--------------------------------------------------------------------------------------------------
// Shader.

#define CF_UNIFORM_TYPE_DEFS \
	CF_ENUM(CF_UNIFORM_TYPE_FLOAT,  0) \
	CF_ENUM(CF_UNIFORM_TYPE_FLOAT2, 1) \
	CF_ENUM(CF_UNIFORM_TYPE_FLOAT3, 2) \
	CF_ENUM(CF_UNIFORM_TYPE_FLOAT4, 3) \
	CF_ENUM(CF_UNIFORM_TYPE_INT,    4) \
	CF_ENUM(CF_UNIFORM_TYPE_INT2,   5) \
	CF_ENUM(CF_UNIFORM_TYPE_INT3,   6) \
	CF_ENUM(CF_UNIFORM_TYPE_INT4,   7) \
	CF_ENUM(CF_UNIFORM_TYPE_MAT4,   8) \

typedef enum CF_UniformType
{
	#define CF_ENUM(K, V) CF_##K = V,
	CF_UNIFORM_TYPE_DEFS
	#undef CF_ENUM
} CF_UniformType;

typedef struct CF_ShaderAttribute
{
	const char* glsl_vertex_attribute_name;
	const char* hlsl_semantic_name;
	int hlsl_semantic_index;
} CF_ShaderAttribute;

typedef struct CF_UniformDef
{
	const char* name;
	CF_UniformType type;
	int array_count;
} CF_UniformDef;

typedef struct CF_UniformBlockDef
{
	int size;
	CF_UniformDef uniforms[CF_UNIFORMS_MAX];
	const char* texture_names[CF_SHADER_BINDINGS_TEXTURE_MAX];
	CF_TextureType texture_types[CF_SHADER_BINDINGS_TEXTURE_MAX];
} CF_UniformBlock;

typedef struct CF_ShaderParams
{
	const char* name;
	const char* vs;
	const char* fs;
	CF_UniformBlockDef vs_uniform_block;
	CF_UniformBlockDef fs_uniform_block;
	CF_ShaderAttribute attributes[CF_VERTEX_ATTRIBUTES_MAX];
} CF_ShaderParams;

CUTE_API CF_Shader CUTE_CALL CF_MakeShader(CF_ShaderParams shader_params);
CUTE_API void CUTE_CALL CF_DestroyShader(CF_Shader shader);

//--------------------------------------------------------------------------------------------------



//--------------------------------------------------------------------------------------------------
// Render Passes.

#define CF_PASS_INIT_OP_DEFS \
	CF_ENUM(PASS_INIT_OP_CLEAR,    0) \
	CF_ENUM(PASS_INIT_OP_LOAD,     1) \
	CF_ENUM(PASS_INIT_OP_DONTCARE, 2) \

typedef enum CF_PassInitOp
{
	#define CF_ENUM(K, V) CF_##K = V,
	CF_PASS_INIT_OP_DEFS
	#undef CF_ENUM
} CF_PassInitOp;

typedef struct CF_PassTexture
{
	CF_Texture texture;
	int mip_level;
	union
	{
		int cube_face;
		int array_layer;
	} u;
};

typedef struct CF_PassParams
{
	const char* name;
	CF_PassInitOp color_ops[CF_TEXTURE_ATTACHMENTS_MAX];
	CF_Color color_values[CF_TEXTURE_ATTACHMENTS_MAX];
	CF_PassInitOp depth_op;
	float depth_value;
	CF_PassInitOp stencil_op;
	uint8_t stencil_value;
	int texture_count;
	CF_PassTexture textures[CF_TEXTURE_ATTACHMENTS_MAX];
	CF_PassTexture depth_stencil;
} CF_PassParams;

CUTE_API CF_PassParams CUTE_CALL CF_PassDefaults();
CUTE_API CF_Pass CUTE_CALL CF_MakePass(CF_PassParams pass_params);
CUTE_API void CUTE_CALL CF_DestroyPass(CF_Pass pass);

//--------------------------------------------------------------------------------------------------
// Render Pipelines.

typedef struct CF_ResourceBindings
{
	CF_Buffer vertex_buffers[CF_SHADER_BINDINGS_BUFFER_MAX];
	int vertex_buffer_offsets[CF_SHADER_BINDINGS_BUFFER_MAX];
	CF_Buffer index_buffer;
	int index_buffer_offset;
	CF_Texture vs_textures[CF_SHADER_BINDINGS_TEXTURE_MAX];
	CF_Texture fs_textures[CF_SHADER_BINDINGS_TEXTURE_MAX];
} CF_ResourceBindings;

typedef struct CF_StencilFunction
{
	CF_CompareFunction compare;
	CF_StencilOp fail_op;
	CF_StencilOp depth_fail_op;
	CF_StencilOp pass_op;
};

typedef struct CF_StencilParams
{
	bool enabled;
	uint32_t read_mask;
	uint32_t write_mask;
	uint32_t reference;
	CF_StencilFunction front;
	CF_StencilFunction back;
};

typedef struct CF_TextureAttachment
{
	CF_PixelFormat pixel_format;
	bool write_R_enabled;
	bool write_G_enabled;
	bool write_B_enabled;
	bool write_A_enabled;
	bool blending_enabled;
	CF_BlendOp rgb_op;
	CF_BlendFactor rgb_src_blend_factor;
	CF_BlendFactor rgb_dst_blend_factor;
	CF_BlendOp alpha_op;
	CF_BlendFactor alpha_src_blend_factor;
	CF_BlendFactor alpha_dst_blend_factor;
} CF_TextureAttachment;

typedef struct CF_VertexLayout
{
	CF_VertexFormat format;
	int offset;
	int stride;
	int buffer_index;
	int step_rate;
} CF_VertexAttribute;

typedef struct CF_PipelineParams
{
	const char* name;
	CF_Shader shader;
	CF_PrimitiveType primitive_type;
	CF_IndexType index_type;
	int vertex_layout_count;
	CF_VertexLayout vertex_layouts[CF_VERTEX_ATTRIBUTES_MAX];
	int texture_count;
	CF_TextureAttachment texture_slots[CF_TEXTURE_ATTACHMENTS_MAX];
	CF_CullMode cull_mode;
	CF_Color blend_color;
	CF_PixelFormat depth_format;
	CF_CompareFunction depth_compare;
	bool depth_write_enabled;
	float depth_bias;
	float depth_bias_slope_scale;
	float depth_bias_clamp;
	CF_StencilParams stencil;
};

CUTE_API CF_PipelineParams CUTE_CALL CF_PipelineDefaults();
CUTE_API CF_Pipeline CUTE_CALL CF_MakePipeline(CF_PipelineParams pipeline_params);
CUTE_API void CUTE_CALL CF_DestroyPipeline(CF_Pipeline pipeline);

//--------------------------------------------------------------------------------------------------
// Rendering Functions.

CUTE_API void CUTE_CALL CF_BeginPass(CF_Pass pass);
CUTE_API void CUTE_CALL CF_ApplyViewport(float x, float y, float width, float height);
CUTE_API void CUTE_CALL CF_ApplyScissor(float x, float y, float width, float height);
CUTE_API void CUTE_CALL CF_ApplyPipeline(CF_Pipeline pipeline);
CUTE_API void CUTE_CALL CF_ApplyBindings(CF_ResourceBindings bindings);
CUTE_API void CUTE_CALL CF_ApplyVSUniforms(int uniform_buffer_index, void* data, int size);
CUTE_API void CUTE_CALL CF_ApplyFSUniforms(int uniform_buffer_index, void* data, int size);
CUTE_API void CUTE_CALL CF_DrawElements(int base_element, int element_count, int instance_count);
CUTE_API void CUTE_CALL CF_EndPass();
CUTE_API void CUTE_CALL CF_Commit();

#ifdef __cplusplus
}
#endif // __cplusplus

//--------------------------------------------------------------------------------------------------
// C++ API

#ifdef CUTE_CPP

namespace cute
{

using Buffer = CF_Buffer;
using Texture = CF_Texture;
using Shader = CF_Shader;
using Pass = CF_Pass;
using Pipeline = CF_Pipeline;

enum BackendType : int
{
	#define CF_ENUM(K, V) K = V,
	CF_BACKEND_TYPE_DEFS
	#undef CF_ENUM
};

CUTE_INLINE BackendType QueryBackend() { return QueryBackend(); }

enum PixelFormat : int
{
	#define CF_ENUM(K, V) K = V,
	CF_PIXELFORMAT_DEFS
	#undef CF_ENUM
};

enum PixelFormatOp : int
{
	#define CF_ENUM(K, V) K = V,
	CF_PIXELFORMAT_OP_DEFS
	#undef CF_ENUM
};

CUTE_INLINE bool QueryPixelFormat(PixelFormat format, PixelFormatOp op) { return CF_QueryPixelFormat((CF_PixelFormat)format, (CF_PixelFormatOp)op); }

enum DeviceFeature : int
{
	#define CF_ENUM(K, V) K = V,
	CF_DEVICE_FEATURE_DEFS
	#undef CF_ENUM
};

CUTE_INLINE bool QueryDeviceFeature(DeviceFeature feature) { return CF_QueryDeviceFeature((CF_DeviceFeature)feature); }

enum ResourceLimit
{
	#define CF_ENUM(K, V) K = V,
	CF_RESOURCE_LIMIT_DEFS
	#undef CF_ENUM
};

CUTE_INLINE int QueryResourceLimit(ResourceLimit resource_limit) { return CF_QueryResourceLimit((CF_ResourceLimit)resource_limit); }

enum UsageType
{
	#define CF_ENUM(K, V) K = V,
	CF_USAGE_TYPE_DEFS
	#undef CF_ENUM
};

enum BufferType
{
	#define CF_ENUM(K, V) K = V,
	CF_BUFFER_TYPE_DEFS
	#undef CF_ENUM
};

enum IndexType
{
	#define CF_ENUM(K, V) K = V,
	CF_INDEX_TYPE_DEFS
	#undef CF_ENUM
};

CUTE_INLINE Buffer MakeBuffer(BufferType type, UsageType usage, void* data, int size) { return CF_MakeBuffer((CF_BufferType)type, (CF_UsageType)usage, data, size); }
CUTE_INLINE void DestroyBuffer(Buffer buffer) { CF_DestroyBuffer(buffer); }
CUTE_INLINE void UpdateBuffer(Buffer buffer, void* data, int size) { CF_UpdateBuffer(buffer, data, size); }
CUTE_INLINE int AppendBuffer(Buffer buffer, void* data, int size) { return CF_AppendBuffer(buffer, data, size); }
CUTE_INLINE bool QueryBufferWillOverflow(Buffer buffer, int size) { return CF_QueryBufferWillOverflow(buffer, size); }

enum TextureType
{
	#define CF_ENUM(K, V) K = V,
	CF_TEXTURE_TYPE_DEFS
	#undef CF_ENUM
};

enum CubeFaceIndex
{
	#define CF_ENUM(K, V) K = V,
	CF_CUBE_FACE_INDEX_DEFS
	#undef CF_ENUM
};

enum PrimitiveType
{
	#define CF_ENUM(K, V) K = V,
	CF_PRIMITIVE_TYPE_DEFS
	#undef CF_ENUM
};

enum MinMagFilter
{
	#define CF_ENUM(K, V) K = V,
	CF_MINMAG_FILTER_DEFS
	#undef CF_ENUM
};

enum MipFilter
{
	#define CF_ENUM(K, V) K = V,
	CF_MIP_FILTER_DEFS
	#undef CF_ENUM
};

enum WrapMode
{
	#define CF_ENUM(K, V) K = V,
	CF_WRAP_MODE_DEFS
	#undef CF_ENUM
};

enum BorderColor
{
	#define CF_ENUM(K, V) K = V,
	CF_BORDER_COLOR_DEFS
	#undef CF_ENUM
};

enum VertexFormat
{
	#define CF_ENUM(K, V) K = V,
	CF_VERTEX_FORMAT_DEFS
	#undef CF_ENUM
};

using TextureParams = CF_TextureParams;
using TextureData = CF_TextureData;

CUTE_INLINE TextureParams TextureDefaults() { return CF_TextureDefaults(); }
CUTE_INLINE Texture MakeTexture(TextureParams texture_params) { return CF_MakeTexture(texture_params); }
CUTE_INLINE void DestroyTexture(Texture texture) { CF_DestroyTexture(texture); }
CUTE_INLINE void UpdateTexture(Texture texture, TextureData data) { CF_UpdateTexture(texture, data); }

enum CullMode
{
	#define CF_ENUM(K, V) K = V,
	CF_CULL_MODE_DEFS
	#undef CF_ENUM
};

enum CompareFunction
{
	#define CF_ENUM(K, V) K = V,
	CF_COMPARE_FUNCTION_DEFS
	#undef CF_ENUM
};

enum StencilOp
{
	#define CF_ENUM(K, V) K = V,
	CF_STENCIL_OP_DEFS
	#undef CF_ENUM
};

enum BlendOp
{
	#define CF_ENUM(K, V) K = V,
	CF_BLEND_OP_DEFS
	#undef CF_ENUM
};

enum BlendFactor
{
	#define CF_ENUM(K, V) K = V,
	CF_BLEND_FACTOR_DEFS
	#undef CF_ENUM
};

enum UniformType
{
	#define CF_ENUM(K, V) K = V,
	CF_UNIFORM_TYPE_DEFS
	#undef CF_ENUM
};

using ShaderAttribute = CF_ShaderAttribute;
using UniformDef = CF_UniformDef;
using UniformBlockDef = CF_UniformBlockDef;
using ShaderParams = CF_ShaderParams;

CUTE_INLINE Shader MakeShader(ShaderParams shader_params) { return CF_MakeShader(shader_params); }
CUTE_INLINE void DestroyShader(Shader shader) { CF_DestroyShader(shader); }

enum PassInitOp
{
	#define CF_ENUM(K, V) K = V,
	CF_PASS_INIT_OP_DEFS
	#undef CF_ENUM
};

using PassTexture = CF_PassTexture;
using PassParams = CF_PassParams;

CUTE_INLINE PassParams PassDefaults() { return CF_PassDefaults(); }
CUTE_INLINE Pass MakePass(PassParams pass_params) { return CF_MakePass(pass_params); }
CUTE_INLINE void DestroyPass(Pass pass) { CF_DestroyPass(pass); }

using ResourceBindings = CF_ResourceBindings;
using StencilFunction = CF_StencilFunction;
using StencilParams = CF_StencilParams;
using TextureAttachment = CF_TextureAttachment;
using VertexLayout = CF_VertexLayout;
using PipelineParams = CF_PipelineParams;

CUTE_INLINE PipelineParams PipelineDefaults() { CF_PipelineDefaults(); }
CUTE_INLINE Pipeline MakePipeline(PipelineParams pipeline_params) { CF_MakePipeline(pipeline_params); }
CUTE_INLINE void DestroyPipeline(Pipeline pipeline) { CF_DestroyPipeline(pipeline); }

CUTE_INLINE void BeginPass(Pass pass) { CF_BeginPass(pass); }
CUTE_INLINE void ApplyViewport(float x, float y, float width, float height) { CF_ApplyViewport(x, y, width, height); }
CUTE_INLINE void ApplyScissor(float x, float y, float width, float height) { CF_ApplyScissor(x, y, width, height); }
CUTE_INLINE void ApplyPipeline(Pipeline pipeline) { CF_ApplyPipeline(pipeline); }
CUTE_INLINE void ApplyBindings(ResourceBindings bindings) { CF_ApplyBindings(bindings); }
CUTE_INLINE void ApplyVSUniforms(int uniform_buffer_index, void* data, int size) { CF_ApplyVSUniforms(uniform_buffer_index, data, size); }
CUTE_INLINE void ApplyFSUniforms(int uniform_buffer_index, void* data, int size) { CF_ApplyFSUniforms(uniform_buffer_index, data, size); }
CUTE_INLINE void DrawElements(int base_element, int element_count, int instance_count) { CF_DrawElements(base_element, element_count, instance_count); }
CUTE_INLINE void EndPass() { CF_EndPass(); }
CUTE_INLINE void Commit() { CF_Commit(); }

}

#endif // CUTE_CPP

#endif // CUTE_GFX_H
