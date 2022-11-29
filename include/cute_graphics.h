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

#ifndef CUTE_GRAPHICS_H
#define CUTE_GRAPHICS_H

#include "cute_defines.h"
#include "cute_result.h"
#include "cute_color.h"
#include "cute_c_runtime.h"

#include "sokol/sokol_gfx.h"

//--------------------------------------------------------------------------------------------------
// C API

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

/**
 * This header wraps low-level 3D rendering APIs. You're probably looking for other headers in Cute
 * Framework, not this one. This header is for implementing your own custom rendering stuff, and is
 * intended only for advanced users.
 * 
 * If you want to draw sprites, see: cute_sprite.h
 * If you want to draw lines or shapes, see: cute_batch.h
 * If you want to draw text, see: cute_font.h + cute_batch.h
 * 
 * Quick list of unsupported features to keep the initial API really tiny and simple. These can be
 * potentially added later, but are not slated for v1.00 of CF's initial release. Most of these
 * features make more sense for the 3D use-case as opposed to 2D.
 * 
 *     - Mipmaps
 *     - MSAA
 *     - Blend color constant
 *     - Multiple render targets (aka color/texture attachments)
 *     - Depth bias tunables
 *     - uint16_t indices (only uint32_t supported)
 *     - Cube map
 *     - 3D textures
 *     - Texture arrays
 *     - Sampler types signed/unsigned int (only float supported)
 *     - Other primitive types besides triangles
 *     - UV wrap border colors
 *     - Face winding order (defaults to CCW only)
 *     - Anisotropy tunable
 *     - Min/max LOD tunable
 * 
 * The basic flow of rendering a frame looks something like this:
 * 
 *     for each pass {
 *         cf_begin_pass(pass);
 *         for each mesh {
 *             cf_mesh_update_vertex_data(mesh, ...);
 *             cf_apply_mesh(mesh);
 *             for each material {
 *                 cf_material_set_uniform_vs(material, ...);
 *                 cf_material_set_uniform_fs(material, ...);
 *                 for each shader {
 *                     cf_apply_shader(shader, material);
 *                     cf_draw_elements(...);
 *                 }
 *             }
 *         }
 *         cf_end_pass();
 *     }
 *     cf_commit();
 * 
 * Where each pass writes to a render target texture.
 */

#define CF_GRAPHICS_IS_VALID(X) (X.id != 0)

typedef struct CF_Texture { uint64_t id; } CF_Texture;
typedef struct CF_Pass { uint64_t id; } CF_Pass;
typedef struct CF_Mesh { uint64_t id; } CF_Mesh;
typedef struct CF_Material { uint64_t id; } CF_Material;
typedef struct CF_Shader { uint64_t id; } CF_Shader;

//--------------------------------------------------------------------------------------------------

typedef struct CF_Matrix4x4
{
	float elements[16];
} CF_Matrix4x4;

CUTE_API CF_Matrix4x4 CUTE_CALL cf_matrix_identity();
CUTE_API CF_Matrix4x4 CUTE_CALL cf_matrix_ortho_2d(float w, float h, float x, float y);

//--------------------------------------------------------------------------------------------------
// Device queries.

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

CUTE_API CF_BackendType CUTE_CALL cf_query_backend();

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

// Call this function to see if a particular pixel format can be used for a particular
// blending operation. Not all platforms support all combos, and some platforms don't support
// particular pixel formats at all.
CUTE_API bool CUTE_CALL cf_query_pixel_format(CF_PixelFormat format, CF_PixelFormatOp op);

#define CF_DEVICE_FEATURE_DEFS \
	CF_ENUM(DEVICE_FEATURE_INSTANCING,       0) \
	CF_ENUM(DEVICE_FEATURE_MSAA,             1) \
	CF_ENUM(DEVICE_FEATURE_TEXTURE_CLAMP,    2) \

typedef enum CF_DeviceFeature
{
	#define CF_ENUM(K, V) CF_##K = V,
	CF_DEVICE_FEATURE_DEFS
	#undef CF_ENUM
} CF_DeviceFeature;

// Query to see if the device can support a particular feature.
CUTE_API bool CUTE_CALL cf_query_device_feature(CF_DeviceFeature feature);

#define CF_RESOURCE_LIMIT_DEFS \
	CF_ENUM(RESOURCE_LIMIT_TEXTURE_DIMENSION,       0) \
	CF_ENUM(RESOURCE_LIMIT_VERTEX_ATTRIBUTE_MAX,    1) \

typedef enum CF_ResourceLimit
{
	#define CF_ENUM(K, V) CF_##K = V,
	CF_RESOURCE_LIMIT_DEFS
	#undef CF_ENUM
} CF_ResourceLimit;

// One notable limit is on GLES2 the maximum number of vertex attributes has
// been reportedly lower than other device types.
CUTE_API int CUTE_CALL cf_query_resource_limit(CF_ResourceLimit resource_limit);

//--------------------------------------------------------------------------------------------------
// Texture.

#define CF_USAGE_TYPE_DEFS \
	CF_ENUM(USAGE_TYPE_IMMUTABLE, 0) /* Can not be changed once created. */ \
	CF_ENUM(USAGE_TYPE_DYNAMIC,   1) /* Can be changed occasionally, but not once per frame. */ \
	CF_ENUM(USAGE_TYPE_STREAM,    2) /* Intended to be altered each frame, e.g. streaming data. */ \

typedef enum CF_UsageType
{
	#define CF_ENUM(K, V) CF_##K = V,
	CF_USAGE_TYPE_DEFS
	#undef CF_ENUM
} CF_UsageType;

#define CF_FILTER_DEFS \
	CF_ENUM(FILTER_NEAREST, 0) \
	CF_ENUM(FILTER_LINEAR,  1) \

typedef enum CF_Filter
{
	#define CF_ENUM(K, V) CF_##K = V,
	CF_FILTER_DEFS
	#undef CF_ENUM
} CF_Filter;

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

typedef struct CF_TextureParams
{
	CF_PixelFormat pixel_format;
	CF_UsageType usage;
	CF_Filter filter;
	CF_WrapMode wrap_u;
	CF_WrapMode wrap_v;
	int width;
	int height;
	bool render_target;    // If true you can render to this texture via `CF_Pass`.
	int initial_data_size; // Must be non-zero for immutable textures.
	void* initial_data;    // Must be non-NULL for immutable textures.
} CF_TextureParams;

CUTE_API CF_TextureParams CUTE_CALL cf_texture_defaults();
CUTE_API CF_Texture CUTE_CALL cf_make_texture(CF_TextureParams texture_params);
CUTE_API void CUTE_CALL cf_destroy_texture(CF_Texture texture);
CUTE_API void CUTE_CALL cf_update_texture(CF_Texture texture, void* data, int size);

//--------------------------------------------------------------------------------------------------
// Shader.

/**
 * Here's the rundown you need to know in order to use custom shaders in Cute Framework (CF). This
 * is only necessary if you want custom rendering.
 * 
 * For now shaders are difficult to use. This is an industry-wide problem. We have many different
 * shading languages and many different devices to deal with, but very little work has gone into
 * making high-quality and easy to use shader solutions. Most shader cross-compilers are way too
 * complex and riddled with giant dependencies, making them a poor for for CF.
 * 
 * The best option (besides writing our own cross-compiler) is to use sokol_gfx.h, a very well written
 * thin wrapper around low-level 3D APIs. It supports a variety of backends:
 * 
 *  - Metal
 *  - OpenGL Core 3.3
 *  - OpenGL ES2
 *  - OpenGL ES3
 *  - D3D11
 *  - WebGPU
 * 
 * This lets CF run basically anywhere, including phones and web browsers. In the future SDL (Simple
 * Direct Media Library) will implement a GPU API that exposes a shader compiler. But until then we're
 * stuck using an offline compiler solution. It's still a pretty good solution though! It just means
 * a little extra work to generate shaders.
 * 
 * Cute Framework comes with compatible binaries Windows, Linux and MacOS to compile shaders onto
 * all supported platforms using the tool sokol-shdc. They are found in the `tools` folder.
 * https://github.com/floooh/sokol-tools/blob/master/docs/sokol-shdc.md
 * 
 * Just make sure to compile with the `--reflection` parameter. Once done `your_shader.h` will be
 * generated. Include it you can build a `CF_SokolShader`. It becomes a one-liner to create the shader
 * at runtime.
 * 
 *     #include "my_shader.h"
 *     CF_Shader my_shd = CF_MAKE_SOKOL_SHADER(my_shader);
 */

#define CF_MAKE_SOKOL_SHADER(prefix) \
	cf_make_shader({ \
		prefix##_shader_desc, \
		prefix##_attr_slot, \
		prefix##_image_slot, \
		prefix##_uniformblock_slot, \
		prefix##_uniformblock_size, \
		prefix##_uniform_offset, \
		prefix##_uniform_desc, \
	})

typedef struct CF_SokolShader
{
	const sg_shader_desc* (*get_desc_fn)(sg_backend backend);
	int (*get_attr_slot)(const char* attr_name);
	int (*get_image_slot)(sg_shader_stage stage, const char* img_name);
	int (*get_uniformblock_slot)(sg_shader_stage stage, const char* ub_name);
	size_t (*get_uniformblock_size)(sg_shader_stage stage, const char* ub_name);
	int (*get_uniform_offset)(sg_shader_stage stage, const char* ub_name, const char* u_name);
	sg_shader_uniform_desc (*get_uniform_desc)(sg_shader_stage stage, const char* ub_name, const char* u_name);
} CF_SokolShader;

CUTE_API CF_Shader CUTE_CALL cf_make_shader(CF_SokolShader sokol_shader);
CUTE_API void CUTE_CALL cf_destroy_shader(CF_Shader shader);

//--------------------------------------------------------------------------------------------------
// Render Passes.

/**
 * A rendering pass contains a target (and optional depth/stencil buffer) along with some operations
 * on what to do with the target. A render target means a texture the GPU can write to.
 */

#define CF_PASS_INIT_OP_DEFS \
	CF_ENUM(PASS_INIT_OP_CLEAR,    /* Clear to contents of the target. */ 0) \
	CF_ENUM(PASS_INIT_OP_LOAD,     /* Load the previous contents of the target. */ 1) \
	CF_ENUM(PASS_INIT_OP_DONTCARE, /* Don't specify either. */ 2) \

typedef enum CF_PassInitOp
{
	#define CF_ENUM(K, V) CF_##K = V,
	CF_PASS_INIT_OP_DEFS
	#undef CF_ENUM
} CF_PassInitOp;

typedef struct CF_PassParams
{
	const char* name;
	CF_PassInitOp color_op;
	CF_Color color_value;
	CF_PassInitOp depth_op;
	float depth_value;
	CF_PassInitOp stencil_op;
	uint8_t stencil_value;
	CF_Texture target;
	CF_Texture depth_stencil;
} CF_PassParams;

CUTE_API CF_PassParams CUTE_CALL cf_pass_defaults();
CUTE_API CF_Pass CUTE_CALL cf_make_pass(CF_PassParams pass_params);
CUTE_API void CUTE_CALL cf_destroy_pass(CF_Pass pass);

//--------------------------------------------------------------------------------------------------
// Mesh.

/**
 * A mesh is a container of triangles, along with optional indices and instance data. After a mesh
 * is created the layout of the vertices in memory must be described. We use an array of
 * `CF_VertexAttribute` to define how the GPU will interpret the vertices we send it.
 * 
 * `CF_VertexAttribute` are also used to specify instance data by setting `step_type` to
 * `CF_ATTRIBUTE_STEP_PER_INSTANCE` instead of the default `ATTRIBUTE_STEP_PER_VERTEX`.
 * 
 * Data for meshes can be immutable, dynamic, or streamed, just like textures. Immutable meshes are
 * perfect for terrain or building rendering, anything static in the world. Dynamic meshes can be
 * occasionally updated, but are still more like an immutable mesh in terms of performance. Streamed
 * meshes can be updated each frame, perfect for streaming data to the GPU.
 */

#define CF_VERTEX_FORMAT_DEFS  \
	CF_ENUM(VERTEX_FORMAT_INVALID,  0 ) \
	CF_ENUM(VERTEX_FORMAT_FLOAT,    1 ) \
	CF_ENUM(VERTEX_FORMAT_FLOAT2,   2 ) \
	CF_ENUM(VERTEX_FORMAT_FLOAT3,   3 ) \
	CF_ENUM(VERTEX_FORMAT_FLOAT4,   4 ) \
	CF_ENUM(VERTEX_FORMAT_BYTE4N,   5 ) \
	CF_ENUM(VERTEX_FORMAT_UBYTE4N,  6 ) \
	CF_ENUM(VERTEX_FORMAT_SHORT2N,  7 ) \
	CF_ENUM(VERTEX_FORMAT_USHORT2N, 8 ) \
	CF_ENUM(VERTEX_FORMAT_SHORT4N,  9 ) \
	CF_ENUM(VERTEX_FORMAT_USHORT4N, 10) \

typedef enum CF_VertexFormat
{
	#define CF_ENUM(K, V) CF_##K = V,
	CF_VERTEX_FORMAT_DEFS
	#undef CF_ENUM
} CF_VertexFormat;

#define CF_ATTRIBUTE_STEP_DEFS  \
	CF_ENUM(ATTRIBUTE_STEP_PER_VERTEX,   0 ) \
	CF_ENUM(ATTRIBUTE_STEP_PER_INSTANCE, 1 ) \

typedef enum CF_AttributeStep
{
	#define CF_ENUM(K, V) CF_##K = V,
	CF_ATTRIBUTE_STEP_DEFS
	#undef CF_ENUM
} CF_AttributeStep;

typedef struct CF_VertexAttribute
{
	const char* name;
	CF_VertexFormat format;
	int offset;
	CF_AttributeStep step_type;
} CF_VertexAttribute;

CUTE_API CF_Mesh CUTE_CALL cf_make_mesh(CF_UsageType usage_type, int vertex_buffer_size, int index_buffer_size, int instance_buffer_size);
CUTE_API void CUTE_CALL cf_destroy_mesh(CF_Mesh mesh);

/**
 * Informs CF and the GPU what the memory layout of your vertices and instance data looks like.
 * You must call this before uploading any data to the GPU. The max number of attributes is 16.
 * Any more attributes beyond 16 will be ignored.
 * 
 * The limit of 16 vertex attributes is less on GLES2, see `cf_query_resource_limit`.
 */
CUTE_API void CUTE_CALL cf_mesh_set_attributes(CF_Mesh mesh, const CF_VertexAttribute* attributes, int attribute_count, int vertex_stride, int instance_stride);

/**
 * The mesh must have been created with `CF_USAGE_TYPE_DYNAMIC` or `CF_USAGE_TYPE_STREAM` in order
 * to call this function more than once. For `CF_USAGE_TYPE_IMMUTABLE` this function can only be
 * called once. For dynamic/stream cases you can only call this function once per frame.
 */
CUTE_API void CUTE_CALL cf_mesh_update_vertex_data(CF_Mesh mesh, void* data, int count);

/**
 * The mesh must have been created with `CF_USAGE_TYPE_DYNAMIC` or `CF_USAGE_TYPE_STREAM` in order
 * to call this function more than once. This function can be called multiple times per frame. The
 * intended use-case is to stream bits of data to the GPU and issue a `cf_draw_elements` call. The
 * only elements that will be drawn are the elements from the last call to `cf_mesh_append_index_data`,
 * all previously appended data will remain untouched.
 */
CUTE_API int CUTE_CALL cf_mesh_append_vertex_data(CF_Mesh mesh, void* data, int count);

/**
 * Use this when streaming data to the GPU to make sure the internal streaming buffers are not overrun.
 * You specified this size when creating the mesh. Use this function to understand if you're sending
 * too much to the GPU all at once. You might need to send less data or increase the size of your mesh's
 * internal buffers.
 */
CUTE_API bool CUTE_CALL cf_mesh_will_overflow_vertex_data(CF_Mesh mesh, int append_count);

/**
 * Use this when streaming data to the GPU to make sure the internal streaming buffers are not overrun.
 * You specified this size when creating the mesh. Use this function to understand if you're sending
 * too much to the GPU all at once. You might need to send less data or increase the size of your mesh's
 * internal buffers.
 */
CUTE_API void CUTE_CALL cf_mesh_update_instance_data(CF_Mesh mesh, void* data, int count);

/**
 * The mesh must have been created with `CF_USAGE_TYPE_DYNAMIC` or `CF_USAGE_TYPE_STREAM` in order
 * to call this function more than once. This function can be called multiple times per frame. The
 * intended use-case is to stream bits of data to the GPU and issue a `cf_draw_elements` call. The
 * only elements that will be drawn are the elements from the last call to `cf_mesh_append_index_data`,
 * all previously appended data will remain untouched.
 */
CUTE_API int CUTE_CALL cf_mesh_append_instance_data(CF_Mesh mesh, void* data, int count);

/**
 * Use this when streaming data to the GPU to make sure the internal streaming buffers are not overrun.
 * You specified this size when creating the mesh. Use this function to understand if you're sending
 * too much to the GPU all at once. You might need to send less data or increase the size of your mesh's
 * internal buffers.
 */
CUTE_API bool CUTE_CALL cf_mesh_will_overflow_instance_data(CF_Mesh mesh, int append_count);

/**
 * Use this when streaming data to the GPU to make sure the internal streaming buffers are not overrun.
 * You specified this size when creating the mesh. Use this function to understand if you're sending
 * too much to the GPU all at once. You might need to send less data or increase the size of your mesh's
 * internal buffers.
 */
CUTE_API void CUTE_CALL cf_mesh_update_index_data(CF_Mesh mesh, uint32_t* indices, int count);

/**
 * The mesh must have been created with `CF_USAGE_TYPE_DYNAMIC` or `CF_USAGE_TYPE_STREAM` in order
 * to call this function more than once. This function can be called multiple times per frame. The
 * intended use-case is to stream bits of data to the GPU and issue a `cf_draw_elements` call. The
 * only elements that will be drawn are the elements from the last call to `cf_mesh_append_index_data`,
 * all previously appended data will remain untouched.
 */
CUTE_API int CUTE_CALL cf_mesh_append_index_data(CF_Mesh mesh, uint32_t* indices, int count);

/**
 * Use this when streaming data to the GPU to make sure the internal streaming buffers are not overrun.
 * You specified this size when creating the mesh. Use this function to understand if you're sending
 * too much to the GPU all at once. You might need to send less data or increase the size of your mesh's
 * internal buffers.
 */
CUTE_API bool CUTE_CALL cf_mesh_will_overflow_index_data(CF_Mesh mesh, int append_count);

//--------------------------------------------------------------------------------------------------
// Render state.

/**
 * The `CF_RenderState` is a big collection of various rendering settings, such as culling mode,
 * blending operations, depth and stencil settings, etc. Altering these on a material always means
 * increasing your draw call count. It's best to try and set these once and leave them alone, though
 * this is not always possible.
 */

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

typedef struct CF_StencilFunction
{
	CF_CompareFunction compare;
	CF_StencilOp fail_op;
	CF_StencilOp depth_fail_op;
	CF_StencilOp pass_op;
} CF_StencilFunction;

typedef struct CF_StencilParams
{
	bool enabled;
	uint8_t read_mask;
	uint8_t write_mask;
	uint8_t reference;
	CF_StencilFunction front;
	CF_StencilFunction back;
} CF_StencilParams;

typedef struct CF_BlendState
{
	bool enabled;
	CF_PixelFormat pixel_format;
	bool write_R_enabled;
	bool write_G_enabled;
	bool write_B_enabled;
	bool write_A_enabled;
	CF_BlendOp rgb_op;
	CF_BlendFactor rgb_src_blend_factor;
	CF_BlendFactor rgb_dst_blend_factor;
	CF_BlendOp alpha_op;
	CF_BlendFactor alpha_src_blend_factor;
	CF_BlendFactor alpha_dst_blend_factor;
} CF_BlendState;

typedef struct CF_RenderState
{
	CF_CullMode cull_mode;
	CF_BlendState blend;
	CF_CompareFunction depth_compare;
	bool depth_write_enabled;
	CF_StencilParams stencil;
} CF_RenderState;

CUTE_API CF_RenderState CUTE_CALL cf_render_state_defaults();

//--------------------------------------------------------------------------------------------------
// Material.

/**
 * Materials store inputs to shaders. They hold uniforms and textures. Uniforms are like global
 * variables inside of a shader stage (either the vertex or fragment shaders). For efficiency, all
 * uniforms are packed into a "uniform buffer", a contiguous chunk of memory on the GPU. We must
 * specify which uniform buffer each uniform belongs to.
 * 
 * A material can hold a large number of inputs, though there are hard-limits on how many inputs
 * an individual shader can accept, especially to keep shaders as cross-platform compatible as
 * possible.
 * 
 * When using sokol-shdc it will naturally enforce these limits for you, such as:
 * 
 * - Max number of uniform buffers for each shader stage (4)
 * - Max number of uniforms in a uniform buffer (16)
 * - Max number of vertex attributes (16) (less on GLES2, see `cf_query_resource_limit`)
 * - Max number of textures for each shader stag (12)
 */

#define CF_UNIFORM_TYPE_DEFS \
	CF_ENUM(UNIFORM_TYPE_FLOAT,  0) \
	CF_ENUM(UNIFORM_TYPE_FLOAT2, 1) \
	CF_ENUM(UNIFORM_TYPE_FLOAT4, 2) \
	CF_ENUM(UNIFORM_TYPE_INT,    3) \
	CF_ENUM(UNIFORM_TYPE_INT2,   4) \
	CF_ENUM(UNIFORM_TYPE_INT4,   5) \
	CF_ENUM(UNIFORM_TYPE_MAT4,   6) \

typedef enum CF_UniformType
{
	#define CF_ENUM(K, V) CF_##K = V,
	CF_UNIFORM_TYPE_DEFS
	#undef CF_ENUM
} CF_UniformType;

CUTE_API CF_Material CUTE_CALL cf_make_material();
CUTE_API void CUTE_CALL cf_destroy_material(CF_Material material);
CUTE_API void CUTE_CALL cf_material_set_render_state(CF_Material material, CF_RenderState render_state);
CUTE_API void CUTE_CALL cf_material_set_texture_vs(CF_Material material, const char* name, CF_Texture texture);
CUTE_API void CUTE_CALL cf_material_set_texture_fs(CF_Material material, const char* name, CF_Texture texture);
CUTE_API void CUTE_CALL cf_material_set_uniform_vs(CF_Material material, const char* block_name, const char* name, void* data, CF_UniformType type, int array_length);
CUTE_API void CUTE_CALL cf_material_set_uniform_fs(CF_Material material, const char* block_name, const char* name, void* data, CF_UniformType type, int array_length);

//--------------------------------------------------------------------------------------------------
// Rendering Functions.

CUTE_API void CUTE_CALL cf_begin_pass(CF_Pass pass);
CUTE_API void CUTE_CALL cf_apply_viewport(float x, float y, float width, float height);
CUTE_API void CUTE_CALL cf_apply_scissor(float x, float y, float width, float height);
CUTE_API void CUTE_CALL cf_apply_mesh(CF_Mesh mesh);

/**
 * All textures and uniforms available in the material will be automatically matched up and sent
 * to the shader, for whichever inputs the shader accepts. Any missing inputs will be cleared to 0.
 */
CUTE_API void CUTE_CALL cf_apply_shader(CF_Shader shader, CF_Material material);
CUTE_API void CUTE_CALL cf_draw_elements();
CUTE_API void CUTE_CALL cf_end_pass();

#ifdef __cplusplus
}
#endif // __cplusplus

//--------------------------------------------------------------------------------------------------
// C++ API

#ifdef CUTE_CPP

namespace cute
{

using Texture  = CF_Texture;
using Pass = CF_Pass;
using Mesh = CF_Mesh;
using Material = CF_Material;
using Shader = CF_Shader;
using Matrix4x4 = CF_Matrix4x4;
using TextureParams = CF_TextureParams;
using SokolShader = CF_SokolShader;
using PassParams = CF_PassParams;
using VertexAttribute = CF_VertexAttribute;
using StencilFunction = CF_StencilFunction;
using StencilParams = CF_StencilParams;
using BlendState = CF_BlendState;
using RenderState = CF_RenderState;

enum BackendType : int
{
	#define CF_ENUM(K, V) K = V,
	CF_BACKEND_TYPE_DEFS
	#undef CF_ENUM
};

enum DeviceFeature : int
{
	#define CF_ENUM(K, V) K = V,
	CF_DEVICE_FEATURE_DEFS
	#undef CF_ENUM
};

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

enum ResourceLimit : int
{
	#define CF_ENUM(K, V) K = V,
	CF_RESOURCE_LIMIT_DEFS
	#undef CF_ENUM
};

enum UsageType : int
{
	#define CF_ENUM(K, V) K = V,
	CF_USAGE_TYPE_DEFS
	#undef CF_ENUM
};

enum Filter : int
{
	#define CF_ENUM(K, V) K = V,
	CF_FILTER_DEFS
	#undef CF_ENUM
};

enum WrapMode : int
{
	#define CF_ENUM(K, V) K = V,
	CF_WRAP_MODE_DEFS
	#undef CF_ENUM
};

enum PassInitOp : int
{
	#define CF_ENUM(K, V) K = V,
	CF_PASS_INIT_OP_DEFS
	#undef CF_ENUM
};

enum VertexFormat : int
{
	#define CF_ENUM(K, V) K = V,
	CF_VERTEX_FORMAT_DEFS
	#undef CF_ENUM
};

enum AttributeStep : int
{
	#define CF_ENUM(K, V) K = V,
	CF_ATTRIBUTE_STEP_DEFS
	#undef CF_ENUM
};

enum CullMode : int
{
	#define CF_ENUM(K, V) K = V,
	CF_CULL_MODE_DEFS
	#undef CF_ENUM
};

enum CompareFunction : int
{
	#define CF_ENUM(K, V) K = V,
	CF_COMPARE_FUNCTION_DEFS
	#undef CF_ENUM
};

enum StencilOp : int
{
	#define CF_ENUM(K, V) K = V,
	CF_STENCIL_OP_DEFS
	#undef CF_ENUM
};

enum BlendOp : int
{
	#define CF_ENUM(K, V) K = V,
	CF_BLEND_OP_DEFS
	#undef CF_ENUM
};

enum BlendFactor : int
{
	#define CF_ENUM(K, V) K = V,
	CF_BLEND_FACTOR_DEFS
	#undef CF_ENUM
};

enum UniformType : int
{
	#define CF_ENUM(K, V) K = V,
	CF_UNIFORM_TYPE_DEFS
	#undef CF_ENUM
};

CUTE_INLINE BackendType CUTE_CALL query_backend() { return (BackendType)cf_query_backend(); }
CUTE_INLINE bool query_pixel_format(PixelFormat format, PixelFormatOp op) { cf_query_pixel_format((CF_PixelFormat)format, (CF_PixelFormatOp)op); }
CUTE_INLINE bool query_device_feature(DeviceFeature feature) { cf_query_device_feature((CF_DeviceFeature)feature); }
CUTE_INLINE int query_resource_limit(ResourceLimit resource_limit) { cf_query_resource_limit((CF_ResourceLimit)resource_limit); }
CUTE_INLINE TextureParams texture_defaults() { return cf_texture_defaults(); }
CUTE_INLINE Texture make_texture(TextureParams texture_params) { return cf_make_texture(texture_params); }
CUTE_INLINE void destroy_texture(Texture texture) { cf_destroy_texture(texture); }
CUTE_INLINE void update_texture(Texture texture, void* data, int size) { cf_update_texture(texture, data, size); }
CUTE_INLINE Shader make_shader(SokolShader sokol_shader) { return cf_make_shader(sokol_shader); }
CUTE_INLINE void destroy_shader(Shader shader) { cf_destroy_shader(shader); }
CUTE_INLINE PassParams pass_defaults() { return cf_pass_defaults(); }
CUTE_INLINE Pass make_pass(PassParams pass_params) { return cf_make_pass(pass_params); }
CUTE_INLINE void destroy_pass(Pass pass) { cf_destroy_pass(pass); }
CUTE_INLINE Mesh make_mesh(UsageType usage_type, int vertex_buffer_size, int index_buffer_size, int instance_buffer_size) { cf_make_mesh((CF_UsageType)usage_type, vertex_buffer_size, index_buffer_size, instance_buffer_size); }
CUTE_INLINE void destroy_mesh(Mesh mesh) { cf_destroy_mesh(mesh); }
CUTE_INLINE void mesh_set_attributes(Mesh mesh, const VertexAttribute* attributes, int attribute_count, int vertex_stride, int instance_stride) { cf_mesh_set_attributes(mesh, attributes, attribute_count, vertex_stride, instance_stride); }
CUTE_INLINE void mesh_update_vertex_data(Mesh mesh, void* data, int count) { cf_mesh_update_vertex_data(mesh, data, count); }
CUTE_INLINE int mesh_append_vertex_data(Mesh mesh, void* data, int append_count) { return cf_mesh_append_vertex_data(mesh, data, append_count); }
CUTE_INLINE bool mesh_will_overflow_vertex_data(Mesh mesh, int append_count) { return cf_mesh_will_overflow_vertex_data(mesh, append_count); }
CUTE_INLINE void mesh_update_instance_data(Mesh mesh, void* data, int count) { cf_mesh_update_instance_data(mesh, data, count); }
CUTE_INLINE int mesh_append_instance_data(Mesh mesh, void* data, int append_count) { return cf_mesh_append_instance_data(mesh, data, append_count); }
CUTE_INLINE bool mesh_will_overflow_instance_data(Mesh mesh, int append_count) { return cf_mesh_will_overflow_instance_data(mesh, append_count); }
CUTE_INLINE void mesh_update_index_data(Mesh mesh, uint32_t* indices, int count) { cf_mesh_update_index_data(mesh, indices, count); }
CUTE_INLINE int mesh_append_index_data(Mesh mesh, uint32_t* indices, int append_count) { return cf_mesh_append_index_data(mesh, indices, append_count); }
CUTE_INLINE bool mesh_will_overflow_index_data(Mesh mesh, int append_count) { return cf_mesh_will_overflow_index_data(mesh, append_count); }
CUTE_INLINE RenderState render_state_defaults() { return cf_render_state_defaults(); }
CUTE_INLINE Material make_material() { return cf_make_material(); }
CUTE_INLINE void destroy_material(Material material) { cf_destroy_material(material); }
CUTE_INLINE void material_set_render_state(Material material, RenderState render_state) { cf_material_set_render_state(material, render_state); }
CUTE_INLINE void material_set_texture_vs(Material material, const char* name, Texture texture) { cf_material_set_texture_vs(material, name, texture); }
CUTE_INLINE void material_set_texture_fs(Material material, const char* name, Texture texture) { cf_material_set_texture_fs(material, name, texture); }
CUTE_INLINE void material_set_uniform_vs(Material material, const char* block_name, const char* name, void* data, UniformType type, int array_length) { cf_material_set_uniform_vs(material, block_name, name, data, (CF_UniformType)type, array_length); }
CUTE_INLINE void material_set_uniform_fs(Material material, const char* block_name, const char* name, void* data, UniformType type, int array_length) { cf_material_set_uniform_fs(material, block_name, name, data, (CF_UniformType)type, array_length); }
CUTE_INLINE void begin_pass(Pass pass) { cf_begin_pass(pass); }
CUTE_INLINE void apply_viewport(float x, float y, float w, float h) { cf_apply_viewport(x, y, w, h); }
CUTE_INLINE void apply_scissor(float x, float y, float w, float h) { cf_apply_scissor(x, y, w, h); }
CUTE_INLINE void apply_mesh(Mesh mesh) { cf_apply_mesh(mesh); }
CUTE_INLINE void apply_shader(Shader shader, Material material) { cf_apply_shader(shader, material); }
CUTE_INLINE void draw_elements() { cf_draw_elements(); }
CUTE_INLINE void end_pass() { cf_end_pass(); }

}

#endif // CUTE_CPP

#endif // CUTE_GRAPHICS_H
