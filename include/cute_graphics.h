/*
	Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

#ifndef CF_GRAPHICS_H
#define CF_GRAPHICS_H

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
 * If you want to draw sprites, see: cute_draw.h
 * If you want to draw lines or shapes, see: cute_draw.h
 * If you want to draw text, see: cute_draw.h
 * 
 * Quick list of unsupported features to keep the initial API really tiny and simple. These can be
 * potentially added later, but are not slated for v1.00 of CF's initial release. Most of these
 * features make more sense for the 3D use-case as opposed to 2D. The ones marked with stars are
 * currently considered higher priority for adding in the future.
 * 
 *     - Mipmaps *
 *     - MSAA
 *     - Blend color constant
 *     - Multiple render targets (aka color/texture attachments) *
 *     - Depth bias tunables
 *     - uint16_t indices (only uint32_t supported) *
 *     - Cube map
 *     - 3D textures
 *     - Texture arrays *
 *     - Sampler types signed/unsigned int (only float supported)
 *     - Other primitive types besides triangles
 *     - UV wrap border colors
 *     - Face winding order (defaults to CCW only)
 *     - Anisotropy tunable
 *     - Min/max LOD tunable
 *     - No direct access to the underlying device *
 * 
 * The basic flow of rendering a frame looks something like this:
 * 
 *     for each canvas {
 *         cf_apply_canvas(canvas);
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
 *     }
 */

#define CF_GRAPHICS_IS_VALID(X) (X.id != 0)

/**
 * @struct   CF_Texture
 * @category graphics
 * @brief    An opaque handle representing a texture.
 * @remarks  A texture is a buffer of data sent to the GPU for random access. Usually textures are used to store image data.
 * @related  CF_Texture CF_Canvas CF_Material CF_Shader CF_TextureParams cf_texture_defaults cf_make_texture cf_destroy_texture cf_update_texture cf_material_set_texture_vs cf_material_set_texture_fs
 */
typedef struct CF_Texture { uint64_t id; } CF_Texture;
// @end

/**
 * @struct   CF_Canvas
 * @category graphics
 * @brief    An opaque handle representing a canvas.
 * @remarks  TODO
 * @related  CF_Texture CF_Canvas CF_Material CF_Shader CF_CanvasParams cf_canvas_defaults cf_make_canvas cf_destroy_canvas cf_apply_canvas
 */
typedef struct CF_Canvas { uint64_t id; } CF_Canvas;
// @end

/**
 * @struct   CF_Mesh
 * @category graphics
 * @brief    An opaque handle representing a mesh.
 * @remarks  A mesh is a container of triangles, along with optional indices and instance data. After a mesh
 *           is created the layout of the vertices in memory must be described. We use an array of
 *           `CF_VertexAttribute` to define how the GPU will interpret the vertices we send it.
 *           
 *           `CF_VertexAttribute` are also used to specify instance data by setting `step_type` to
 *           `CF_ATTRIBUTE_STEP_PER_INSTANCE` instead of the default `CF_ATTRIBUTE_STEP_PER_VERTEX`.
 *           
 *           Data for meshes can be immutable, dynamic, or streamed, just like textures. Immutable meshes are
 *           perfect for terrain or building rendering, anything static in the world. Dynamic meshes can be
 *           occasionally updated, but are still more like an immutable mesh in terms of performance. Streamed
 *           meshes can be updated each frame, perfect for streaming data to the GPU.
 * @related  CF_Texture CF_Canvas CF_Material CF_Shader cf_make_mesh cf_destroy_mesh cf_mesh_set_attributes cf_mesh_update_vertex_data cf_mesh_update_instance_data cf_mesh_update_index_data cf_apply_mesh
 */
typedef struct CF_Mesh { uint64_t id; } CF_Mesh;
// @end

/**
 * @struct   CF_Material
 * @category graphics
 * @brief    An opaque handle representing a material.
 * @remarks  Materials store inputs to shaders. They hold uniforms and textures. Uniforms are like global
 *           variables inside of a shader stage (either the vertex or fragment shaders). For efficiency, all
 *           uniforms are packed into a "uniform buffer", a contiguous chunk of memory on the GPU. We must
 *           specify which uniform buffer each uniform belongs to.
 *           
 *           A material can hold a large number of inputs, though there are hard-limits on how many inputs
 *           an individual shader can accept, especially to keep shaders as cross-platform compatible as
 *           possible.
 *           
 *           When using sokol-shdc (see `CF_MAKE_SOKOL_SHADER`) it will naturally enforce these limits for you, such as:
 *           
 *           - Max number of uniform buffers for each shader stage (4)
 *           - Max number of uniforms in a uniform buffer (16)
 *           - Max number of vertex attributes (16)
 *           - Max number of textures for each shader stag (12)
 * @related  CF_Texture CF_Canvas CF_Material CF_Shader cf_make_material cf_destroy_material cf_material_set_render_state cf_material_set_texture_vs cf_material_set_texture_fs cf_material_set_uniform_vs cf_material_set_uniform_fs cf_apply_shader
 */
typedef struct CF_Material { uint64_t id; } CF_Material;
// @end

/**
 * @struct   CF_Shader
 * @category graphics
 * @brief    An opaque handle representing a shader.
 * @remarks  A shader is a small program that runs on the GPU. They come in the form of vertex and fragment shaders. See `CF_MAKE_SOKOL_SHADER` for an overview.
 * @related  CF_Texture CF_Canvas CF_Material CF_Shader CF_SokolShader CF_MAKE_SOKOL_SHADER cf_make_shader cf_destroy_shader cf_apply_shader
 */
typedef struct CF_Shader { uint64_t id; } CF_Shader;
// @end

//--------------------------------------------------------------------------------------------------
// Device queries.

/**
 * @enum     CF_BackendType
 * @category graphics
 * @brief    The various supported graphics backends.
 * @related  CF_BackendType cf_backend_type_to_string cf_query_backend
 */
#define CF_BACKEND_TYPE_DEFS \
	/* @entry OpenGL 3.3 Core Profile. */    \
	CF_ENUM(BACKEND_TYPE_GLCORE33,        0) \
	/* @entry OpenGL ES 3.0. */              \
	CF_ENUM(BACKEND_TYPE_GLES3,           1) \
	/* @entry DirectX 11. */                 \
	CF_ENUM(BACKEND_TYPE_D3D11,           2) \
	/* @entry Metal for iOS. */              \
	CF_ENUM(BACKEND_TYPE_METAL_IOS,       3) \
	/* @entry Metal for MacOS. */            \
	CF_ENUM(BACKEND_TYPE_METAL_MACOS,     4) \
	/* @entry Metal for debug simulator (XCode). */ \
	CF_ENUM(BACKEND_TYPE_METAL_SIMULATOR, 5) \
	/* @entry WebGPU (for browsers). */      \
	CF_ENUM(BACKEND_TYPE_WGPU,            6) \
	/* @end */

typedef enum CF_BackendType
{
	#define CF_ENUM(K, V) CF_##K = V,
	CF_BACKEND_TYPE_DEFS
	#undef CF_ENUM
} CF_BackendType;

/**
 * @function cf_backend_type_to_string
 * @category graphics
 * @brief    Returns a `CF_BackendType` converted to a string.
 * @related  CF_BackendType cf_backend_type_to_string cf_query_backend
 */
CF_INLINE const char* cf_backend_type_to_string(CF_BackendType type) {
	switch (type) {
	#define CF_ENUM(K, V) case CF_##K: return CF_STRINGIZE(CF_##K);
	CF_BACKEND_TYPE_DEFS
	#undef CF_ENUM
	default: return NULL;
	}
}

/**
 * @function cf_query_backend
 * @category graphics
 * @brief    Returns which `CF_BackendType` is currently active.
 * @related  CF_BackendType cf_backend_type_to_string cf_query_backend
 */
CF_API CF_BackendType CF_CALL cf_query_backend();

/**
 * @enum     CF_PixelFormat
 * @category graphics
 * @brief    The various supported pixel formats.
 * @remarks  Not all types are supported on each backend. Be sure to check with `cf_query_pixel_format` if a particular pixel format
 *           is available for your use-case. If unsure, just use `CF_PIXELFORMAT_DEFAULT` for red-green-blue-alpha, 8 bits per component (32 bits total),
 *           which get mapped to 4 floats on the GPU from [0,1].
 *           
 *           This table helps understand how to read the pixel format notation.
 *           
 *           Notation | Description
 *           --- | ---
 *           RGBA | R stands for red, G stands for green, B stands for blue, A stands for alpha.
 *           XX | A number, describes the number of bits used by the pixel format. For example `CF_PIXELFORMAT_R8` uses 8 bits, while `PIXELFORMAT_RG16F` uses 16 bits.
 *           N | A normalized number from [0,1]. All the bits of the format will be treated as an integer. For example, `CF_PIXELFORMAT_R8` starts as an 8-bit value on the CPU, but is mapped from [0,255] to [0,1] as a float on the GPU.
 *           UI | The GPU will interpret the bits of a pixel as an unsigned integer.
 *           SI | The GPU will interpret the bits of a pixel as a signed integer.
 *           F | The GPU will interpret the bits of a pixel as a floating point value.
 *           N | Stands for "normalized". The pixel's bits are mapped from [0,2^##] to floating point [0,1] on the GPU.
 *           SN | Signed-normalized. The same as N, but mapped to floating point [-1,1] on the GPU.
 *           
 *           The other formats such as BC (Block Compression for DirectX), PVRTC (for iOS), and ETC2 (OpenGL ES and ARM devices) are common compression formats.
 *           These can potentially save a lot of memory on the GPU, but are only good for certain games (mainly 3D games), and are only available on certain backends.
 * @related  CF_PixelFormat cf_pixel_format_to_string CF_PixelFormatOp cf_query_pixel_format
 */
#define CF_PIXELFORMAT_DEFS \
	/* @entry The default pixel format.  */                                                            \
	CF_ENUM(PIXELFORMAT_DEFAULT,         0 )                                                           \
	/* @entry 8-bit red-only channel.    */                                                            \
	CF_ENUM(PIXELFORMAT_R8,              1 )                                                           \
	/* @entry 8-bit red-only channel, in signed normalized form. */                                    \
	CF_ENUM(PIXELFORMAT_R8SN,            2 )                                                           \
	/* @entry 8-bit red-only channel, in unsigned integer form. */                                     \
	CF_ENUM(PIXELFORMAT_R8UI,            3 )                                                           \
	/* @entry 8-bit red-only channel, in signed integer form. */                                       \
	CF_ENUM(PIXELFORMAT_R8SI,            4 )                                                           \
	/* @entry 16-bit red-only channel.    */                                                           \
	CF_ENUM(PIXELFORMAT_R16,             5 )                                                           \
	/* @entry 16-bit red-only channel, in signed normalized form. */                                   \
	CF_ENUM(PIXELFORMAT_R16SN,           6 )                                                           \
	/* @entry 16-bit red-only channel, in unsigned integer form. */                                    \
	CF_ENUM(PIXELFORMAT_R16UI,           7 )                                                           \
	/* @entry 16-bit red-only channel, in signed integer form. */                                      \
	CF_ENUM(PIXELFORMAT_R16SI,           8 )                                                           \
	/* @entry 16-bit red-only channel, in floating point form. */                                      \
	CF_ENUM(PIXELFORMAT_R16F,            9 )                                                           \
	/* @entry 8-bit red/green channels, 16 bits total. */                                              \
	CF_ENUM(PIXELFORMAT_RG8,             10)                                                           \
	/* @entry 8-bit red/green channels, in signed normalized form, 16 bits total. */                   \
	CF_ENUM(PIXELFORMAT_RG8SN,           11)                                                           \
	/* @entry 8-bit red/green channels, in unsigned integer form, 16 bits total. */                    \
	CF_ENUM(PIXELFORMAT_RG8UI,           12)                                                           \
	/* @entry 8-bit red/green channels, in signed integer form, 16 bits total. */                      \
	CF_ENUM(PIXELFORMAT_RG8SI,           13)                                                           \
	/* @entry 32-bit red-only channel, in unsigned integer form. */                                    \
	CF_ENUM(PIXELFORMAT_R32UI,           14)                                                           \
	/* @entry 32-bit red-only channel, in signed integer form. */                                      \
	CF_ENUM(PIXELFORMAT_R32SI,           15)                                                           \
	/* @entry 32-bit red-only channel, in floating point form. */                                      \
	CF_ENUM(PIXELFORMAT_R32F,            16)                                                           \
	/* @entry 16-bit red-green channels, 32 bits total. */                                             \
	CF_ENUM(PIXELFORMAT_RG16,            17)                                                           \
	/* @entry 16-bit red-green channels, in signed normalized form, 32 bits total. */                  \
	CF_ENUM(PIXELFORMAT_RG16SN,          18)                                                           \
	/* @entry 16-bit red-green channels, in unsigned integer form, 32 bits total. */                   \
	CF_ENUM(PIXELFORMAT_RG16UI,          19)                                                           \
	/* @entry 16-bit red-green channels, in signed integer form, 32 bits total. */                     \
	CF_ENUM(PIXELFORMAT_RG16SI,          20)                                                           \
	/* @entry 16-bit red-green channels, in floating point form, 32 bits total. */                     \
	CF_ENUM(PIXELFORMAT_RG16F,           21)                                                           \
	/* @entry 8-bit red-green-blue-alpha channels, 32 bits total. */                                   \
	CF_ENUM(PIXELFORMAT_RGBA8,           22)                                                           \
	/* @entry TODO. */                                                                                 \
	CF_ENUM(PIXELFORMAT_SRGB8A8,         23)                                                           \
	/* @entry 8-bit red-green-blue-alpha channels, in signed normalized form, 32 bits total. */        \
	CF_ENUM(PIXELFORMAT_RGBA8SN,         24)                                                           \
	/* @entry 8-bit red-green-blue-alpha channels, in unsigned integer form, 32 bits total. */         \
	CF_ENUM(PIXELFORMAT_RGBA8UI,         25)                                                           \
	/* @entry 8-bit red-green-blue-alpha channels, in signed integer form, 32 bits total. */           \
	CF_ENUM(PIXELFORMAT_RGBA8SI,         26)                                                           \
	/* @entry 8-bit blue-green-red-alpha channels, 32 bits total. */                                   \
	CF_ENUM(PIXELFORMAT_BGRA8,           27)                                                           \
	/* @entry 10-bit red-green-blue channels, 2-bit alpha channel, 32 bits total. */                   \
	CF_ENUM(PIXELFORMAT_RGB10A2,         28)                                                           \
	/* @entry 11-bit red-green channels, 10-bit blue channel, in floating point form, 32 bits total. */\
	CF_ENUM(PIXELFORMAT_RG11B10F,        29)                                                           \
	/* @entry 32-bit red-green channels, in unsigned integer form, 64 bits total. */                   \
	CF_ENUM(PIXELFORMAT_RG32UI,          30)                                                           \
	/* @entry 32-bit red-green channels, in signed integer form, 64 bits total. */                     \
	CF_ENUM(PIXELFORMAT_RG32SI,          31)                                                           \
	/* @entry 32-bit red-green channels, in floating point form, 64 bits total. */                     \
	CF_ENUM(PIXELFORMAT_RG32F,           32)                                                           \
	/* @entry 16-bit red-green-blue-alpha channels, 64 bits total. */                                  \
	CF_ENUM(PIXELFORMAT_RGBA16,          33)                                                           \
	/* @entry 16-bit red-green-blue-alpha channels, in signed normalized form, 64 bits total. */       \
	CF_ENUM(PIXELFORMAT_RGBA16SN,        34)                                                           \
	/* @entry 16-bit red-green-blue-alpha channels, in unsigned integer form, 64 bits total. */        \
	CF_ENUM(PIXELFORMAT_RGBA16UI,        35)                                                           \
	/* @entry 16-bit red-green-blue-alpha channels, in signed integer form, 64 bits total. */          \
	CF_ENUM(PIXELFORMAT_RGBA16SI,        36)                                                           \
	/* @entry 16-bit red-green-blue-alpha channels, in floating point form, 64 bits total. */          \
	CF_ENUM(PIXELFORMAT_RGBA16F,         37)                                                           \
	/* @entry 32-bit red-green-blue-alpha channels, in unsigned integer form, 128 bits total. */       \
	CF_ENUM(PIXELFORMAT_RGBA32UI,        38)                                                           \
	/* @entry 32-bit red-green-blue-alpha channels, in signed integer form, 128 bits total. */         \
	CF_ENUM(PIXELFORMAT_RGBA32SI,        39)                                                           \
	/* @entry 32-bit red-green-blue-alpha channels, in floating point form, 128 bits total. */         \
	CF_ENUM(PIXELFORMAT_RGBA32F,         40)                                                           \
	/* @entry 32-bit red-green-blue-alpha channels, in floating point form, 128 bits total. */         \
	CF_ENUM(PIXELFORMAT_DEPTH,           41)                                                           \
	/* @entry 24-bit depth channel, 8-bit stencil channel. */                                          \
	CF_ENUM(PIXELFORMAT_DEPTH_STENCIL,   42)                                                           \
	/* @entry Block Compression 1. */                                                                  \
	CF_ENUM(PIXELFORMAT_BC1_RGBA,        43)                                                           \
	/* @entry Block Compression 2. */                                                                  \
	CF_ENUM(PIXELFORMAT_BC2_RGBA,        44)                                                           \
	/* @entry Block Compression 3. */                                                                  \
	CF_ENUM(PIXELFORMAT_BC3_RGBA,        45)                                                           \
	/* @entry Block Compression 4. */                                                                  \
	CF_ENUM(PIXELFORMAT_BC4_R,           46)                                                           \
	/* @entry Block Compression 4 (signed normalized). */                                              \
	CF_ENUM(PIXELFORMAT_BC4_RSN,         47)                                                           \
	/* @entry Block Compression 5. */                                                                  \
	CF_ENUM(PIXELFORMAT_BC5_RG,          48)                                                           \
	/* @entry Block Compression 5 (signed normalized). */                                              \
	CF_ENUM(PIXELFORMAT_BC5_RGSN,        49)                                                           \
	/* @entry Block Compression 6. */                                                                  \
	CF_ENUM(PIXELFORMAT_BC6H_RGBF,       50)                                                           \
	/* @entry Block Compression 6 (unsigned). */                                                       \
	CF_ENUM(PIXELFORMAT_BC6H_RGBUF,      51)                                                           \
	/* @entry Block Compression 7. */                                                                  \
	CF_ENUM(PIXELFORMAT_BC7_RGBA,        52)                                                           \
	/* @entry PVRTC compression 7. */                                                                  \
	CF_ENUM(PIXELFORMAT_PVRTC_RGB_2BPP,  53)                                                           \
	/* @entry PVRTC red-green-blue 2-bits-per-pixel. */                                                \
	CF_ENUM(PIXELFORMAT_PVRTC_RGB_4BPP,  54)                                                           \
	/* @entry PVRTC red-green-blue 4-bits-per-pixel. */                                                \
	CF_ENUM(PIXELFORMAT_PVRTC_RGBA_2BPP, 55)                                                           \
	/* @entry PVRTC red-green-blue-alpha 2-bits-per-pixel. */                                          \
	CF_ENUM(PIXELFORMAT_PVRTC_RGBA_4BPP, 56)                                                           \
	/* @entry PVRTC red-green-blue-alpha 4-bits-per-pixel. */                                          \
	CF_ENUM(PIXELFORMAT_ETC2_RGB8,       57)                                                           \
	/* @entry ETC2 red-green-blue 8-bit channels. */                                                   \
	CF_ENUM(PIXELFORMAT_ETC2_RGB8A1,     58)                                                           \
	/* @entry ETC2 red-green-blue 8-bit channels, 1-bit alpha channel. */                              \
	CF_ENUM(PIXELFORMAT_ETC2_RGBA8,      59)                                                           \
	/* @entry ETC2 red-green-blue-alpha 8-bit channels. */                                             \
	CF_ENUM(PIXELFORMAT_ETC2_RG11,       60)                                                           \
	/* @entry ETC2 red-green 11-bit channels. */                                                       \
	CF_ENUM(PIXELFORMAT_ETC2_RG11SN,     61)                                                           \
	/* @entry TODO. */                                                                                 \
	CF_ENUM(PIXELFORMAT_RGB9E5,          62)                                                           \
	/* @entry ETC2 red-green 11-bit channels, in signed-normalized form. */                            \
	CF_ENUM(PIXELFORMAT_COUNT,           63)                                                           \
	/* @end */

typedef enum CF_PixelFormat
{
	#define CF_ENUM(K, V) CF_##K = V,
	CF_PIXELFORMAT_DEFS
	#undef CF_ENUM
} CF_PixelFormat;

/**
 * @function cf_pixel_format_to_string
 * @category graphics
 * @brief    Returns a `CF_PixelFormat` converted to a C string.
 * @related  CF_PixelFormat cf_pixel_format_to_string CF_PixelFormatOp cf_query_pixel_format
 */
CF_INLINE const char* cf_pixel_format_to_string(CF_PixelFormat format) {
	switch (format) {
	#define CF_ENUM(K, V) case CF_##K: return CF_STRINGIZE(CF_##K);
	CF_PIXELFORMAT_DEFS
	#undef CF_ENUM
	default: return NULL;
	}
}

/**
 * @enum     CF_PixelFormatOp
 * @category graphics
 * @brief    The various supported operations a pixel format can perform.
 * @remarks  Not all types are supported on each backend. Be sure to check with `cf_query_pixel_format` if a particular pixel format
 *           is available for your use-case.
 * @related  CF_PixelFormat cf_pixel_format_op_to_string CF_PixelFormatOp cf_query_pixel_format
 */
#define CF_PIXELFORMAT_OP_DEFS \
	/* @entry Nearest-neighbor filtering. Good for pixel art. */          \
	CF_ENUM(PIXELFORMAT_OP_NEAREST_FILTER,  0)                            \
	/* @entry Bilinear filtering, good general purpose option. */         \
	CF_ENUM(PIXELFORMAT_OP_BILINEAR_FILTER, 1)                            \
	/* @entry Used to render to a texture. */                             \
	CF_ENUM(PIXELFORMAT_OP_RENDER_TARGET,   2)                            \
	/* @entry Performs hardware-accelerated alpha-blending. */            \
	CF_ENUM(PIXELFORMAT_OP_ALPHA_BLENDING,  3)                            \
	/* @entry Performs hardware-accelerated multi-sample antialiasing. */ \
	CF_ENUM(PIXELFORMAT_OP_MSAA,            4)                            \
	/* @entry Performs hardware-accelerated depth-culling. */             \
	CF_ENUM(PIXELFORMAT_OP_DEPTH,           5)                            \
	/* @end */

typedef enum CF_PixelFormatOp
{
	#define CF_ENUM(K, V) CF_##K = V,
	CF_PIXELFORMAT_OP_DEFS
	#undef CF_ENUM
} CF_PixelFormatOp;

/**
 * @function cf_pixel_format_op_to_string
 * @category graphics
 * @brief    Returns a `CF_PixelFormatOp` converted to a C string.
 * @related  CF_PixelFormat cf_pixel_format_op_to_string CF_PixelFormatOp cf_query_pixel_format
 */
CF_INLINE const char* cf_pixel_format_op_to_string(CF_PixelFormatOp op) {
	switch (op) {
	#define CF_ENUM(K, V) case CF_##K: return CF_STRINGIZE(CF_##K);
	CF_PIXELFORMAT_OP_DEFS
	#undef CF_ENUM
	default: return NULL;
	}
}

/**
 * @function cf_query_pixel_format
 * @category graphics
 * @brief    Returns true if a particular `CF_PixelFormat` is compatible with a certain `CF_PixelFormatOp`.
 * @remarks  Not all backends support each combination of format and operation, some backends don't support particular pixel formats at all.
 *           Be sure to query the device with this function to make sure your use-case is supported.
 * @related  CF_PixelFormat cf_pixel_format_op_to_string CF_PixelFormatOp cf_query_pixel_format
 */
CF_API bool CF_CALL cf_query_pixel_format(CF_PixelFormat format, CF_PixelFormatOp op);

/**
 * @enum     CF_DeviceFeature
 * @category graphics
 * @brief    Some various device features that may or may not be supported on various backends.
 * @remarks  Check to see if a particular feature is available on your backend with `cf_query_device_feature`.
 * @related  CF_DeviceFeature cf_device_feature_to_string cf_query_device_feature
 */
#define CF_DEVICE_FEATURE_DEFS \
	/* @entry Texture clamp addressing style, e.g. `CF_WRAP_MODE_CLAMP_TO_EDGE` or `CF_WRAP_MODE_CLAMP_TO_BORDER` . */ \
	CF_ENUM(DEVICE_FEATURE_TEXTURE_CLAMP,    0)                                                                        \
	/* @end */

typedef enum CF_DeviceFeature
{
	#define CF_ENUM(K, V) CF_##K = V,
	CF_DEVICE_FEATURE_DEFS
	#undef CF_ENUM
} CF_DeviceFeature;

/**
 * @function cf_device_feature_to_string
 * @category graphics
 * @brief    Returns a `CF_DeviceFeature` converted to a C string.
 * @related  CF_DeviceFeature cf_device_feature_to_string cf_query_device_feature
 */
CF_INLINE const char* cf_device_feature_to_string(CF_DeviceFeature feature) {
	switch (feature) {
	#define CF_ENUM(K, V) case CF_##K: return CF_STRINGIZE(CF_##K);
	CF_DEVICE_FEATURE_DEFS
	#undef CF_ENUM
	default: return NULL;
	}
}

/**
 * @function cf_query_device_feature
 * @category graphics
 * @brief    Query to see if the device can support a particular feature.
 * @related  CF_PixelFormat cf_pixel_format_op_to_string CF_PixelFormatOp cf_query_pixel_format
 */
CF_API bool CF_CALL cf_query_device_feature(CF_DeviceFeature feature);

/**
 * @enum     CF_ResourceLimit
 * @category graphics
 * @brief    Some backends have limits on specific GPU resources.
 * @related  CF_ResourceLimit cf_resource_limit_to_string cf_query_resource_limit
 */
#define CF_RESOURCE_LIMIT_DEFS \
	/* @entry Limit on the number of dimensions a texture can have, e.g. 2 or 3. */ \
	CF_ENUM(RESOURCE_LIMIT_TEXTURE_DIMENSION,       0)                              \
	/* @entry Limit on the number of vertex attributes. */                          \
	CF_ENUM(RESOURCE_LIMIT_VERTEX_ATTRIBUTE_MAX,    1)                              \
	/* @end */

typedef enum CF_ResourceLimit
{
	#define CF_ENUM(K, V) CF_##K = V,
	CF_RESOURCE_LIMIT_DEFS
	#undef CF_ENUM
} CF_ResourceLimit;

/**
 * @function cf_resource_limit_to_string
 * @category graphics
 * @brief    Returns a `CF_ResourceLimit` converted to a C string.
 * @related  CF_ResourceLimit cf_resource_limit_to_string cf_query_resource_limit
 */
CF_INLINE const char* cf_resource_limit_to_string(CF_ResourceLimit limit) {
	switch (limit) {
	#define CF_ENUM(K, V) case CF_##K: return CF_STRINGIZE(CF_##K);
	CF_RESOURCE_LIMIT_DEFS
	#undef CF_ENUM
	default: return NULL;
	}
}

/**
 * @function cf_query_resource_limit
 * @category graphics
 * @brief    Query the device for resource limits.
 * @related  CF_ResourceLimit cf_resource_limit_to_string cf_query_resource_limit
 */
CF_API int CF_CALL cf_query_resource_limit(CF_ResourceLimit limit);

//--------------------------------------------------------------------------------------------------
// Texture.

/**
 * @enum     CF_UsageType
 * @category graphics
 * @brief    The access pattern for data sent to the GPU.
 * @related  CF_UsageType cf_usage_type_to_string CF_TextureParams cf_make_mesh
 */
#define CF_USAGE_TYPE_DEFS \
	/* @entry Can not be changed once created. */                        \
	CF_ENUM(USAGE_TYPE_IMMUTABLE, 0)                                     \
	/* @entry Can be changed occasionally, but not once per frame. */    \
	CF_ENUM(USAGE_TYPE_DYNAMIC,   1)                                     \
	/* @entry Intended to be altered each frame, e.g. streaming data. */ \
	CF_ENUM(USAGE_TYPE_STREAM,    2)                                     \
	/* @end */

typedef enum CF_UsageType
{
	#define CF_ENUM(K, V) CF_##K = V,
	CF_USAGE_TYPE_DEFS
	#undef CF_ENUM
} CF_UsageType;

/**
 * @function cf_usage_type_to_string
 * @category graphics
 * @brief    Returns a `CF_UsageType` converted to a C string.
 * @related  CF_UsageType cf_usage_type_to_string CF_TextureParams cf_make_mesh
 */
CF_INLINE const char* cf_usage_type_to_string(CF_UsageType type) {
	switch (type) {
	#define CF_ENUM(K, V) case CF_##K: return CF_STRINGIZE(CF_##K);
	CF_USAGE_TYPE_DEFS
	#undef CF_ENUM
	default: return NULL;
	}
}

/**
 * @enum     CF_Filter
 * @category graphics
 * @brief    Filtering options for how to access `CF_Texture` data on the GPU.
 * @related  CF_Filter cf_filter_to_string CF_TextureParams
 */
#define CF_FILTER_DEFS \
	/* @entry Nearest-neighbor filtering. Good for pixel art. */             \
	CF_ENUM(FILTER_NEAREST, 0)                                               \
	/* @entry Linear (bilinear) filtering. A good general purpose option. */ \
	CF_ENUM(FILTER_LINEAR,  1)                                               \
	/* @end */

typedef enum CF_Filter
{
	#define CF_ENUM(K, V) CF_##K = V,
	CF_FILTER_DEFS
	#undef CF_ENUM
} CF_Filter;

/**
 * @function cf_filter_to_string
 * @category graphics
 * @brief    Returns a `CF_Filter` converted to a C string.
 * @related  CF_Filter cf_filter_to_string CF_TextureParams
 */
CF_INLINE const char* cf_filter_to_string(CF_Filter filter) {
	switch (filter) {
	#define CF_ENUM(K, V) case CF_##K: return CF_STRINGIZE(CF_##K);
	CF_FILTER_DEFS
	#undef CF_ENUM
	default: return NULL;
	}
}

/**
 * @enum     CF_WrapMode
 * @category graphics
 * @brief    Wrap modes to define behavior when addressing a texture beyond the [0,1] range.
 * @related  CF_WrapMode cf_wrap_mode_string CF_TextureParams
 */
#define CF_WRAP_MODE_DEFS \
	/* @entry The default is `CF_WRAP_MODE_REPEAT`. */                         \
	CF_ENUM(WRAP_MODE_DEFAULT,         0)                                      \
	/* @entry Repeats the image. */                                            \
	CF_ENUM(WRAP_MODE_REPEAT,          1)                                      \
	/* @entry Clamps a UV coordinate to the nearest edge pixel. */             \
	CF_ENUM(WRAP_MODE_CLAMP_TO_EDGE,   2)                                      \
	/* @entry Clamps a UV coordinate to the border color. */                   \
	CF_ENUM(WRAP_MODE_CLAMP_TO_BORDER, 3)                                      \
	/* @entry The same as `CF_WRAP_MODE_REPEAT` but mirrors back and forth. */ \
	CF_ENUM(WRAP_MODE_MIRRORED_REPEAT, 4)                                      \
	/* @end */

typedef enum CF_WrapMode
{
	#define CF_ENUM(K, V) CF_##K = V,
	CF_WRAP_MODE_DEFS
	#undef CF_ENUM
} CF_WrapMode;

/**
 * @function cf_wrap_mode_string
 * @category graphics
 * @brief    Returns a `CF_WrapMode` converted to a C string.
 * @related  CF_WrapMode cf_wrap_mode_string CF_TextureParams
 */
CF_INLINE const char* cf_wrap_mode_string(CF_WrapMode mode) {
	switch (mode) {
	#define CF_ENUM(K, V) case CF_##K: return CF_STRINGIZE(CF_##K);
	CF_WRAP_MODE_DEFS
	#undef CF_ENUM
	default: return NULL;
	}
}

/**
 * @struct   CF_TextureParams
 * @category graphics
 * @brief    A collection of parameters to create a `CF_Texture` with `cf_make_texture`.
 * @remarks  You may get a set of good default values by calling `cf_texture_defaults`.
 * @related  CF_TextureParams cf_texture_defaults CF_Texture cf_make_texture cf_destroy_texture cf_update_texture
 */
typedef struct CF_TextureParams
{
	/* @member The pixel format for this texture's data. See `CF_PixelFormat`. */
	CF_PixelFormat pixel_format;

	/* @member The memory access pattern for this texture on the GPU. See `CF_UsageType`. */
	CF_UsageType usage;

	/* @member The filtering operation to use when fetching data out of the texture, on the GPU. See `CF_Filter`. */
	CF_Filter filter;

	/* @member The texture wrapping behavior when addressing beyond [0,1] for the u-coordinate. See `CF_WrapMode`. */
	CF_WrapMode wrap_u;

	/* @member The texture wrapping behavior when addressing beyond [0,1] for the v-coordinate. See `CF_WrapMode`. */
	CF_WrapMode wrap_v;

	/* @member Number of elements (usually pixels) along the width of the texture. */
	int width;

	/* @member Number of elements (usually pixels) along the height of the texture. */
	int height;

	/* @member If true you can render to this texture via `CF_Canvas`. */
	bool render_target;

	/* @member The size of the `initial_data` member. Must be non-zero for immutable textures. */
	int initial_data_size;

	/* @member The intial byte data to fill the texture in with. Must not be `NULL` for immutable textures. See `CF_USAGE_TYPE_IMMUTABLE`. */
	void* initial_data;
} CF_TextureParams;
// @end

/**
 * @function cf_texture_defaults
 * @category graphics
 * @brief    Returns a good set of default values for `CF_TextureParams` to call `cf_make_texture`.
 * @related  CF_TextureParams CF_Texture cf_make_texture
 */
CF_API CF_TextureParams CF_CALL cf_texture_defaults(int w, int h);

/**
 * @function cf_make_texture
 * @category graphics
 * @brief    Returns a new `CF_Texture`.
 * @param    texture_params  The texture parameters as a `CF_TextureParams`.
 * @return   Free it up with `cf_destroy_texture` when done.
 * @related  CF_TextureParams CF_Texture cf_make_texture cf_destroy_texture cf_update_texture
 */
CF_API CF_Texture CF_CALL cf_make_texture(CF_TextureParams texture_params);

/**
 * @function cf_destroy_texture
 * @category graphics
 * @brief    Destroys a `CF_Texture` created by `cf_make_texture`.
 * @param    texture   The texture.
 * @related  CF_TextureParams CF_Texture cf_make_texture cf_destroy_texture cf_update_texture
 */
CF_API void CF_CALL cf_destroy_texture(CF_Texture texture);

/**
 * @function cf_update_texture
 * @category graphics
 * @brief    Updates the contents of a `CF_Texture`.
 * @param    texture    The texture.
 * @param    data       The data to upload to the texture.
 * @param    size       The size in bytes of `data`.
 * @remarks  The texture must not have been created with `CF_USAGE_TYPE_IMMUTABLE`.
 * @related  CF_TextureParams CF_Texture cf_make_texture cf_destroy_texture cf_update_texture
 */
CF_API void CF_CALL cf_update_texture(CF_Texture texture, void* data, int size);

//--------------------------------------------------------------------------------------------------
// Shader.

/**
 * @function CF_MAKE_SOKOL_SHADER
 * @category graphics
 * @brief    Creates a shader from a shader compiled by sokol-shdc.
 * @param    prefix     The name of your sokol-shdc compiled shader. See remarks for details.
 * @remarks  There's an industry-wide problem where cross-platform shaders are difficult to setup. We have many
 *           different shading languages and many different devices to deal with, but very little work has gone
 *           into making high-quality and easy to use shader solutions. Most shader cross-compilers are way too
 *           complex and riddled with giant dependencies, making them a poor fit for CF's style.
 *           
 *           The best option (besides writing our own cross-compiler) is to use sokol_gfx.h, a very well written
 *           thin wrapper around low-level 3D APIs. It supports a variety of backends:
 *           
 *            - Metal
 *            - OpenGL Core 3.3
 *            - OpenGL ES2
 *            - OpenGL ES3
 *            - D3D11
 *            - WebGPU
 *           
 *           This lets CF run basically anywhere, including phones and web browsers. In the future SDL (Simple
 *           Direct Media Library) will implement a GPU API that exposes a shader compiler. But until then we're
 *           stuck using an offline compiler solution. It's still a pretty good solution though! It just means
 *           a little extra work to generate shaders.
 *           
 *           Cute Framework comes with compatible binaries Windows, Linux and MacOS to compile shaders onto
 *           all supported platforms using the tool [sokol-shdc](https://github.com/floooh/sokol-tools/blob/master/docs/sokol-shdc.md). They are found in the `tools` folder.
 *           The basic idea is to write your shader _one time_ in GLSL, then sokol-shdc will cross-compiler the shader
 *           into a header file that's compatible with all supported backends.
 *           
 *           Just make sure to call the sokol-shdc compiler with the `--reflection` parameter. Once done, `my_shader.h`
 *           is ready to go! Include `my_shader.h` and get a `CF_SokolShader` with a single call to `cf_make_shader`.
 *           
 *           ```cpp
 *           #include "my_shader.h"
 *           CF_Shader my_shd = CF_MAKE_SOKOL_SHADER(my_shader);
 *           ```
 * @related  CF_MAKE_SOKOL_SHADER CF_SokolShader CF_Shader cf_make_shader cf_destroy_shader cf_apply_shader CF_Material
 */
#ifdef __cplusplus
#define CF_MAKE_SOKOL_SHADER(prefix) \
	cf_make_shader({ \
		prefix##_shader_desc, \
		prefix##_attr_slot, \
		prefix##_image_slot, \
		prefix##_uniformblock_slot, \
		prefix##_uniformblock_size, \
		prefix##_uniform_offset, \
		prefix##_uniform_desc \
	})
#else
#define CF_MAKE_SOKOL_SHADER(prefix) \
	cf_make_shader((CF_SokolShader){ \
		prefix##_shader_desc, \
		prefix##_attr_slot, \
		prefix##_image_slot, \
		prefix##_uniformblock_slot, \
		prefix##_uniformblock_size, \
		prefix##_uniform_offset, \
		prefix##_uniform_desc \
	})
#endif

/**
 * @struct   CF_SokolShader
 * @category graphics
 * @brief    A virtual table for a sokol-shdc compiled shader.
 * @remarks  See `CF_MAKE_SOKOL_SHADER` for an overview.
 * @related  CF_MAKE_SOKOL_SHADER CF_SokolShader CF_Shader cf_make_shader cf_destroy_shader cf_apply_shader CF_Material
 */
typedef struct CF_SokolShader
{
	/* This is setup automagically by `CF_MAKE_SOKOL_SHADER`. */
	const sg_shader_desc* (*get_desc_fn)(sg_backend backend);

	/* This is setup automagically by `CF_MAKE_SOKOL_SHADER`. */
	int (*get_attr_slot)(const char* attr_name);

	/* This is setup automagically by `CF_MAKE_SOKOL_SHADER`. */
	int (*get_image_slot)(sg_shader_stage stage, const char* img_name);

	/* This is setup automagically by `CF_MAKE_SOKOL_SHADER`. */
	int (*get_uniformblock_slot)(sg_shader_stage stage, const char* ub_name);

	/* This is setup automagically by `CF_MAKE_SOKOL_SHADER`. */
	size_t (*get_uniformblock_size)(sg_shader_stage stage, const char* ub_name);

	/* This is setup automagically by `CF_MAKE_SOKOL_SHADER`. */
	int (*get_uniform_offset)(sg_shader_stage stage, const char* ub_name, const char* u_name);

	/* This is setup automagically by `CF_MAKE_SOKOL_SHADER`. */
	sg_shader_uniform_desc (*get_uniform_desc)(sg_shader_stage stage, const char* ub_name, const char* u_name);
} CF_SokolShader;
// @end

/**
 * @function cf_make_shader
 * @category graphics
 * @brief    Creates a shader from a shader compiled by sokol-shdc.
 * @param    sokol_shader  A compiled shader.
 * @remarks  You should instead call `CF_MAKE_SOKOL_SHADER` unless you really know what you're doing.
 * @related  CF_MAKE_SOKOL_SHADER CF_SokolShader CF_Shader cf_make_shader cf_destroy_shader cf_apply_shader CF_Material
 */
CF_API CF_Shader CF_CALL cf_make_shader(CF_SokolShader sokol_shader);

/**
 * @function cf_destroy_shader
 * @category graphics
 * @brief    Frees up a `CF_Shader` created by `cf_make_shader`.
 * @param    shader     A shader.
 * @related  CF_MAKE_SOKOL_SHADER CF_SokolShader CF_Shader cf_make_shader cf_destroy_shader cf_apply_shader CF_Material
 */
CF_API void CF_CALL cf_destroy_shader(CF_Shader shader);

//--------------------------------------------------------------------------------------------------
// Render Canvases.

/**
 * @struct   CF_CanvasParams
 * @category graphics
 * @brief    A texture the GPU can draw upon (with an optional depth/stencil texture).
 * @remarks  The clear color settings are used when `cf_apply_canvas` is called. You can change the clear color
 *           by calling `cf_clear_color`. Usually you will not need to create a canvas at all, as it's an advanced feature for
 *           users who want to draw to an off-screen buffer. Use cases can include rendering reflections, advanced lighting
 *           techniques, or other kinds of multi-pass effects.
 * @related  CF_CanvasParams cf_canvas_defaults cf_make_canvas cf_destroy_canvas cf_apply_canvas cf_clear_color
 */
typedef struct CF_CanvasParams
{
	/* @member The name of the canvas, for debug purposes. */
	const char* name;

	/* @member The texture used to store pixel information when rendering to the canvas. See `CF_TextureParams`. */
	CF_TextureParams target;

	/* @member The texture used to store depth and stencil information when rendering to the canvas. See `CF_TextureParams`. */
	CF_TextureParams depth_stencil_target;
} CF_CanvasParams;
// @end

/**
 * @function cf_canvas_defaults
 * @category graphics
 * @brief    Returns a good set of default values for a `CF_CanvasParams` to call `cf_make_canvas`.
 * @related  CF_CanvasParams cf_canvas_defaults cf_make_canvas cf_destroy_canvas cf_apply_canvas cf_clear_color
 */
CF_API CF_CanvasParams CF_CALL cf_canvas_defaults(int w, int h);

/**
 * @function cf_make_canvas
 * @category graphics
 * @brief    Returns a new `CF_Canvas` for offscreen rendering.
 * @related  CF_CanvasParams cf_canvas_defaults cf_make_canvas cf_destroy_canvas cf_apply_canvas cf_clear_color
 */
CF_API CF_Canvas CF_CALL cf_make_canvas(CF_CanvasParams canvas_params);

/**
 * @function cf_destroy_canvas
 * @category graphics
 * @brief    Frees up a `CF_Canvas` created by `cf_make_canvas`.
 * @related  CF_CanvasParams cf_canvas_defaults cf_make_canvas cf_destroy_canvas cf_apply_canvas cf_clear_color
 */
CF_API void CF_CALL cf_destroy_canvas(CF_Canvas canvas);

/**
 * @function cf_canvas_get_target
 * @category graphics
 * @brief    Returns the `target` texture the canvas renders upon.
 * @related  CF_CanvasParams cf_canvas_defaults cf_make_canvas cf_destroy_canvas cf_apply_canvas cf_clear_color
 */
CF_API CF_Texture CF_CALL cf_canvas_get_target(CF_Canvas canvas);

/**
 * @function cf_canvas_get_depth_stencil_target
 * @category graphics
 * @brief    Returns the `depth_stencil_target` texture the canvas renders upon.
 * @related  CF_CanvasParams cf_canvas_defaults cf_make_canvas cf_destroy_canvas cf_apply_canvas cf_clear_color
 */
CF_API CF_Texture CF_CALL cf_canvas_get_depth_stencil_target(CF_Canvas canvas);

/**
 * @function cf_canvas_get_backend_target_handle
 * @category graphics
 * @brief    Returns a backend-specific handle to the underlying pixel texture of a canvas.
 * @related  CF_CanvasParams cf_canvas_defaults cf_make_canvas cf_canvas_get_backend_target_handle cf_canvas_get_backend_depth_stencil_handle
 */
CF_API uint64_t CF_CALL cf_canvas_get_backend_target_handle(CF_Canvas canvas);

/**
 * @function cf_canvas_get_backend_depth_stencil_handle
 * @category graphics
 * @brief    Returns a backend-specific handle to the underlying depth-stencil texture of a canvas.
 * @related  CF_CanvasParams cf_canvas_defaults cf_make_canvas cf_canvas_get_backend_target_handle cf_canvas_get_backend_depth_stencil_handle
 */
CF_API uint64_t CF_CALL cf_canvas_get_backend_depth_stencil_handle(CF_Canvas canvas);

/**
 * @function cf_canvas_blit
 * @category graphics
 * @brief    Blits one canvas onto another.
 * @param    src           The source texture to copy pixels from.
 * @param    u0            The normalized coordinate of the top-left of the source rect.
 * @param    v0            The normalized coordinate of the bottom-right of the source rect.
 * @param    src           The canvas where pixels are copied from.
 * @param    u1            The normalized coordinate of the top-left of the destination rect.
 * @param    v1            The normalized coordinate of the bottom-right of the destination rect.
 * @param    dst           The destination canvas where pixels are copied to.
 * @remarks  The texture formats of the underlying canvas's must be PIXELFORMAT_DEFAULT. Each u/v coordinate
 *           is normalized, meaning a number from 0 to 1. This lets the function operate on canvas's of any
 *           size. To convert a coordinate to a normalized coordinate, simply divide the x/y of your coordinate
 *           by the width/height of the canvas.
 * @related  CF_Canvas
 */
CF_API void CF_CALL cf_canvas_blit(CF_Canvas src, CF_V2 u0, CF_V2 v0, CF_Canvas dst, CF_V2 u1, CF_V2 v1);

//--------------------------------------------------------------------------------------------------
// Mesh.

/**
 * @enum     CF_VertexFormat
 * @category graphics
 * @brief    The possible formats for vertex attributes (inputs to vertex shader, coming from `CF_Mesh`).
 * @remarks  To help understand the notation see `CF_PixelFormat`.
 * @related  CF_VertexFormat cf_vertex_format_string CF_VertexAttribute cf_mesh_set_attributes
 */
#define CF_VERTEX_FORMAT_DEFS  \
	/* @entry Invalid. */                                           \
	CF_ENUM(VERTEX_FORMAT_INVALID,  0 )                             \
	/* @entry A single 32-bit float. */                             \
	CF_ENUM(VERTEX_FORMAT_FLOAT,    1 )                             \
	/* @entry Two 32-bit floats. */                                 \
	CF_ENUM(VERTEX_FORMAT_FLOAT2,   2 )                             \
	/* @entry Three 32-bit floats. */                               \
	CF_ENUM(VERTEX_FORMAT_FLOAT3,   3 )                             \
	/* @entry Four 32-bit floats. */                                \
	CF_ENUM(VERTEX_FORMAT_FLOAT4,   4 )                             \
	/* @entry Four 8-bit signed bytes, in normalized form. */       \
	CF_ENUM(VERTEX_FORMAT_BYTE4N,   5 )                             \
	/* @entry Four 8-bit unsigned bytes, in normalized form. */     \
	CF_ENUM(VERTEX_FORMAT_UBYTE4N,  6 )                             \
	/* @entry Two 16-bit signed bytes, in normalized form. */       \
	CF_ENUM(VERTEX_FORMAT_SHORT2N,  7 )                             \
	/* @entry Two 16-bit unsigned bytes, in normalized form. */     \
	CF_ENUM(VERTEX_FORMAT_USHORT2N, 8 )                             \
	/* @entry Four 16-bit signed bytes, in normalized form. */      \
	CF_ENUM(VERTEX_FORMAT_SHORT4N,  9 )                             \
	/* @entry Four 16-bit unsigned bytes, in normalized form. */    \
	CF_ENUM(VERTEX_FORMAT_USHORT4N, 10)                             \
	/* @end */

typedef enum CF_VertexFormat
{
	#define CF_ENUM(K, V) CF_##K = V,
	CF_VERTEX_FORMAT_DEFS
	#undef CF_ENUM
} CF_VertexFormat;

/**
 * @function cf_vertex_format_string
 * @category graphics
 * @brief    Frees up a `CF_Canvas` created by `cf_make_canvas`.
 * @related  CF_VertexFormat cf_vertex_format_string CF_VertexAttribute cf_mesh_set_attributes
 */
CF_INLINE const char* cf_vertex_format_string(CF_VertexFormat format) {
	switch (format) {
	#define CF_ENUM(K, V) case CF_##K: return CF_STRINGIZE(CF_##K);
	CF_VERTEX_FORMAT_DEFS
	#undef CF_ENUM
	default: return NULL;
	}
}

/**
 * @enum     CF_AttributeStep
 * @category graphics
 * @brief    Describes how attribute data is interpreted between each invocation of a vertex shader.
 * @related  CF_AttributeStep cf_attribute_step_string CF_VertexAttribute cf_mesh_set_attributes
 */
#define CF_ATTRIBUTE_STEP_DEFS  \
	/* @entry Take a step forward in the vertex buffer (the `stride`) once per vertex. */   \
	CF_ENUM(ATTRIBUTE_STEP_PER_VERTEX,   0 )                                                \
	/* @entry Take a step forward in the vertex buffer (the `stride`) once per instance. */ \
	CF_ENUM(ATTRIBUTE_STEP_PER_INSTANCE, 1 )                                                \
	/* @end */

typedef enum CF_AttributeStep
{
	#define CF_ENUM(K, V) CF_##K = V,
	CF_ATTRIBUTE_STEP_DEFS
	#undef CF_ENUM
} CF_AttributeStep;

/**
 * @function cf_attribute_step_string
 * @category graphics
 * @brief    Returns a `CF_AttributeStep` converted to a C string.
 * @related  CF_AttributeStep cf_attribute_step_string CF_VertexAttribute cf_mesh_set_attributes
 */
CF_INLINE const char* cf_attribute_step_string(CF_AttributeStep step) {
	switch (step) {
	#define CF_ENUM(K, V) case CF_##K: return CF_STRINGIZE(CF_##K);
	CF_ATTRIBUTE_STEP_DEFS
	#undef CF_ENUM
	default: return NULL;
	}
}

/**
 * @struct   CF_VertexAttribute
 * @category graphics
 * @brief    Describes the memory layout of vertex attributes.
 * @remarks  An attribute is a component of a vertex, usually one, two, three or four floats/integers. A vertex is an input
 *           to a vertex shader. A `CF_Mesh` is a collection of vertices and attribute layout definitions.
 * @related  CF_VertexAttribute cf_mesh_set_attributes
 */
typedef struct CF_VertexAttribute
{
	/* @member The name of the vertex attribute as it appears in the shader. */
	const char* name;

	/* @member The layout in memory of one vertex. See `CF_VertexFormat`. */
	CF_VertexFormat format;

	/* @member The offset in memory from the beginning of a vertex to this attribute. */
	int offset;

	/* @member The step behavior to distinguish between vertex-stepping and instance-stepping. See `CF_AttributeStep`. */
	CF_AttributeStep step_type;
} CF_VertexAttribute;
// @end

/**
 * @function cf_make_mesh
 * @category graphics
 * @brief    Returns a `CF_Mesh`.
 * @param    usage_type            Distinguish between per-vertex or per-attribute stepping. See `CF_UsageType`.
 * @param    vertex_buffer_size    The size of the mesh's vertex buffer.
 * @param    index_buffer_size     The size of the mesh's index buffer. Set to 0 if you're not using indices.
 * @param    instance_buffer_size  The size of the mesh's instance buffer. Set to 0 if you're not using instancing.
 * @related  CF_Mesh cf_make_mesh cf_destroy_mesh cf_mesh_set_attributes cf_mesh_update_vertex_data cf_mesh_update_instance_data cf_mesh_update_index_data
 */
CF_API CF_Mesh CF_CALL cf_make_mesh(CF_UsageType usage_type, int vertex_buffer_size, int index_buffer_size, int instance_buffer_size);

/**
 * @function cf_destroy_mesh
 * @category graphics
 * @brief    Frees up a `CF_Mesh` previously created with `cf_make_mesh`.
 * @param    mesh       The mesh.
 * @related  CF_Mesh cf_make_mesh cf_destroy_mesh cf_mesh_set_attributes cf_mesh_update_vertex_data cf_mesh_update_instance_data cf_mesh_update_index_data
 */
CF_API void CF_CALL cf_destroy_mesh(CF_Mesh mesh);

/**
 * @function cf_mesh_set_attributes
 * @category graphics
 * @brief    Informs CF and the GPU what the memory layout of your vertices and instance data looks like.
 * @param    mesh             The mesh.
 * @param    attributes       Vertex attributes to define the memory layout of the mesh vertices.
 * @param    attribute_count  Number of attributes in `attributes`.
 * @param    vertex_stride    Number of bytes between each vertex.
 * @param    instance_stride  Number of bytes between each instance.
 * @remarks  You must call this before uploading any data to the GPU. The max number of attributes is 16. Any more attributes beyond 16 will be ignored.
 * @related  CF_Mesh cf_make_mesh cf_destroy_mesh cf_mesh_set_attributes cf_mesh_update_vertex_data cf_mesh_update_instance_data cf_mesh_update_index_data
 */
CF_API void CF_CALL cf_mesh_set_attributes(CF_Mesh mesh, const CF_VertexAttribute* attributes, int attribute_count, int vertex_stride, int instance_stride);

/**
 * @function cf_mesh_update_vertex_data
 * @category graphics
 * @brief    Overwrites the vertex data of a mesh.
 * @param    mesh       The mesh.
 * @param    data       A pointer to vertex data.
 * @param    count      Number of bytes in `data`.
 * @return   Returns the number of bytes written.
 * @remarks  The mesh must have been created with `CF_USAGE_TYPE_DYNAMIC` or `CF_USAGE_TYPE_STREAM` in order to call this function more
 *           than once. For `CF_USAGE_TYPE_IMMUTABLE` this function can only be called once. For dynamic/stream cases you can only call
 *           this function once per frame.
 * @related  CF_Mesh cf_make_mesh cf_destroy_mesh cf_mesh_set_attributes cf_mesh_update_vertex_data cf_mesh_update_instance_data cf_mesh_update_index_data
 */
CF_API void CF_CALL cf_mesh_update_vertex_data(CF_Mesh mesh, void* data, int count);

/**
 * @function cf_mesh_append_vertex_data
 * @category graphics
 * @brief    Appends vertex data onto the end of the mesh's internal vertex buffer.
 * @param    mesh       The mesh.
 * @param    data       A pointer to vertex data.
 * @param    count      Number of bytes in `data`.
 * @return   Returns the number of bytes appended.
 * @remarks  The mesh must have been created with `CF_USAGE_TYPE_DYNAMIC` or `CF_USAGE_TYPE_STREAM` in order
 *           to call this function more than once. This function can be called multiple times per frame. The
 *           intended use-case is to stream bits of data to the GPU and issue a `cf_draw_elements` call. The
 *           only elements that will be drawn are the elements from the last call to `cf_mesh_append_index_data`,
 *           all previously appended data will remain untouched.
 * @related  CF_Mesh cf_make_mesh cf_destroy_mesh cf_mesh_set_attributes cf_mesh_update_vertex_data cf_mesh_update_instance_data cf_mesh_update_index_data
 */
CF_API int CF_CALL cf_mesh_append_vertex_data(CF_Mesh mesh, void* data, int count);

/**
 * @function cf_mesh_will_overflow_vertex_data
 * @category graphics
 * @brief    Returns true if a number of bytes to append would overflow the internal vertex buffer.
 * @param    mesh          The mesh.
 * @param    append_count  A number of bytes to append.
 * @remarks  Use this when streaming data to the GPU to make sure the internal streaming buffers are not overrun.
 *           You specified this size when creating the mesh. Use this function to understand if you're sending
 *           too much to the GPU all at once. You might need to send less data or increase the size of your mesh's
 *           internal buffers.
 * @related  CF_Mesh cf_make_mesh cf_destroy_mesh cf_mesh_set_attributes cf_mesh_update_vertex_data cf_mesh_update_instance_data cf_mesh_update_index_data
 */
CF_API bool CF_CALL cf_mesh_will_overflow_vertex_data(CF_Mesh mesh, int append_count);

/**
 * @function cf_mesh_update_instance_data
 * @category graphics
 * @brief    Overwrites the instance data of a mesh.
 * @param    mesh       The mesh.
 * @param    data       A pointer to instance data.
 * @param    count      Number of bytes in `data`.
 * @remarks  The mesh must have been created with `CF_USAGE_TYPE_DYNAMIC` or `CF_USAGE_TYPE_STREAM` in order to call this function more
 *           than once. For `CF_USAGE_TYPE_IMMUTABLE` this function can only be called once. For dynamic/stream cases you can only call
 *           this function once per frame.
 * @related  CF_Mesh cf_make_mesh cf_destroy_mesh cf_mesh_set_attributes cf_mesh_update_vertex_data cf_mesh_update_instance_data cf_mesh_update_index_data
 */
CF_API void CF_CALL cf_mesh_update_instance_data(CF_Mesh mesh, void* data, int count);

/**
 * @function cf_mesh_append_instance_data
 * @category graphics
 * @brief    Appends instance data onto the end of the mesh's internal instance buffer.
 * @param    mesh       The mesh.
 * @param    data       A pointer to instance data.
 * @param    count      Number of bytes in `data`.
 * @return   Returns the number of bytes appended.
 * @remarks  The mesh must have been created with `CF_USAGE_TYPE_DYNAMIC` or `CF_USAGE_TYPE_STREAM` in order
 *           to call this function more than once. This function can be called multiple times per frame. The
 *           intended use-case is to stream bits of data to the GPU and issue a `cf_draw_elements` call. The
 *           only elements that will be drawn are the elements from the last call to `cf_mesh_append_index_data`,
 *           all previously appended data will remain untouched.
 * @related  CF_Mesh cf_make_mesh cf_destroy_mesh cf_mesh_set_attributes cf_mesh_update_vertex_data cf_mesh_update_instance_data cf_mesh_update_index_data
 */
CF_API int CF_CALL cf_mesh_append_instance_data(CF_Mesh mesh, void* data, int count);

/**
 * @function cf_mesh_will_overflow_instance_data
 * @category graphics
 * @brief    Returns true if a number of bytes to append would overflow the internal vertex buffer.
 * @param    mesh          The mesh.
 * @param    append_count  A number of bytes to append.
 * @remarks  Use this when streaming data to the GPU to make sure the internal streaming buffers are not overrun.
 *           You specified this size when creating the mesh. Use this function to understand if you're sending
 *           too much to the GPU all at once. You might need to send less data or increase the size of your mesh's
 *           internal buffers.
 * @related  CF_Mesh cf_make_mesh cf_destroy_mesh cf_mesh_set_attributes cf_mesh_update_vertex_data cf_mesh_update_instance_data cf_mesh_update_index_data
 */
CF_API bool CF_CALL cf_mesh_will_overflow_instance_data(CF_Mesh mesh, int append_count);

/**
 * @function cf_mesh_update_index_data
 * @category graphics
 * @brief    Overwrites the index data of a mesh.
 * @param    mesh       The mesh.
 * @param    indices    A pointer to index data.
 * @param    count      Number of bytes in `data`.
 * @remarks  The mesh must have been created with `CF_USAGE_TYPE_DYNAMIC` or `CF_USAGE_TYPE_STREAM` in order to call this function more
 *           than once. For `CF_USAGE_TYPE_IMMUTABLE` this function can only be called once. For dynamic/stream cases you can only call
 *           this function once per frame.
 * @related  CF_Mesh cf_make_mesh cf_destroy_mesh cf_mesh_set_attributes cf_mesh_update_vertex_data cf_mesh_update_instance_data cf_mesh_update_index_data
 */
CF_API void CF_CALL cf_mesh_update_index_data(CF_Mesh mesh, uint32_t* indices, int count);

/**
 * @function cf_mesh_append_index_data
 * @category graphics
 * @brief    Appends index data onto the end of the mesh's internal index buffer.
 * @param    mesh       The mesh.
 * @param    indices    A pointer to index data.
 * @param    count      Number of bytes in `data`.
 * @return   Returns the number of bytes appended.
 * @remarks  The mesh must have been created with `CF_USAGE_TYPE_DYNAMIC` or `CF_USAGE_TYPE_STREAM` in order
 *           to call this function more than once. This function can be called multiple times per frame. The
 *           intended use-case is to stream bits of data to the GPU and issue a `cf_draw_elements` call. The
 *           only elements that will be drawn are the elements from the last call to `cf_mesh_append_index_data`,
 *           all previously appended data will remain untouched.
 * @related  CF_Mesh cf_make_mesh cf_destroy_mesh cf_mesh_set_attributes cf_mesh_update_vertex_data cf_mesh_update_instance_data cf_mesh_update_index_data
 */
CF_API int CF_CALL cf_mesh_append_index_data(CF_Mesh mesh, uint32_t* indices, int count);

/**
 * @function cf_mesh_will_overflow_index_data
 * @category graphics
 * @brief    Returns true if a number of bytes to append would overflow the internal index buffer.
 * @param    mesh          The mesh.
 * @param    append_count  A number of bytes to append.
 * @remarks  Use this when streaming data to the GPU to make sure the internal streaming buffers are not overrun.
 *           You specified this size when creating the mesh. Use this function to understand if you're sending
 *           too much to the GPU all at once. You might need to send less data or increase the size of your mesh's
 *           internal buffers.
 * @related  CF_Mesh cf_make_mesh cf_destroy_mesh cf_mesh_set_attributes cf_mesh_update_vertex_data cf_mesh_update_instance_data cf_mesh_update_index_data
 */
CF_API bool CF_CALL cf_mesh_will_overflow_index_data(CF_Mesh mesh, int append_count);

//--------------------------------------------------------------------------------------------------
// Render state.

/**
 * @enum     CF_CullMode
 * @category graphics
 * @brief    Settings to control if triangles are culled in clockwise order, counter-clockwise order, or not at all.
 * @related  CF_CullMode cf_cull_mode_string CF_RenderState
 */
#define CF_CULL_MODE_DEFS \
	/* @entry No culling at all. */                        \
	CF_ENUM(CULL_MODE_NONE,  0)                            \
	/* @entry Cull triangles ordered clockwise. */         \
	CF_ENUM(CULL_MODE_FRONT, 1)                            \
	/* @entry Cull triangles ordered counter-clockwise. */ \
	CF_ENUM(CULL_MODE_BACK,  2)                            \
	/* @end */

typedef enum CF_CullMode
{
	#define CF_ENUM(K, V) CF_##K = V,
	CF_CULL_MODE_DEFS
	#undef CF_ENUM
} CF_CullMode;

/**
 * @function cf_cull_mode_string
 * @category graphics
 * @brief    Returns a `CF_CullMode` converted to a C string.
 * @related  CF_CullMode cf_cull_mode_string CF_RenderState
 */
CF_INLINE const char* cf_cull_mode_string(CF_CullMode mode) {
	switch (mode) {
	#define CF_ENUM(K, V) case CF_##K: return CF_STRINGIZE(CF_##K);
	CF_CULL_MODE_DEFS
	#undef CF_ENUM
	default: return NULL;
	}
}

/**
 * @enum     CF_CompareFunction
 * @category graphics
 * @brief    Compare operations available for depth/stencil.
 * @related  CF_CompareFunction cf_compare_function_string CF_StencilOp CF_StencilFunction
 */
#define CF_COMPARE_FUNCTION_DEFS \
	/* @entry Always perform the operation. */         \
	CF_ENUM(COMPARE_FUNCTION_ALWAYS,                0) \
	/* @entry Never perform the operation. */          \
	CF_ENUM(COMPARE_FUNCTION_NEVER,                 1) \
	/* @entry < */                                     \
	CF_ENUM(COMPARE_FUNCTION_LESS_THAN,             2) \
	/* @entry == */                                    \
	CF_ENUM(COMPARE_FUNCTION_EQUAL,                 3) \
	/* @entry != */                                    \
	CF_ENUM(COMPARE_FUNCTION_NOT_EQUAL,             4) \
	/* @entry <= */                                    \
	CF_ENUM(COMPARE_FUNCTION_LESS_THAN_OR_EQUAL,    5) \
	/* @entry > */                                     \
	CF_ENUM(COMPARE_FUNCTION_GREATER_THAN,          6) \
	/* @entry >= */                                    \
	CF_ENUM(COMPARE_FUNCTION_GREATER_THAN_OR_EQUAL, 7) \
	/* @end */

typedef enum CF_CompareFunction
{
	#define CF_ENUM(K, V) CF_##K = V,
	CF_COMPARE_FUNCTION_DEFS
	#undef CF_ENUM
} CF_CompareFunction;

/**
 * @function cf_compare_function_string
 * @category graphics
 * @brief    Returns a `CF_CompareFunction` converted to a C string.
 * @related  CF_CompareFunction cf_compare_function_string CF_StencilOp CF_StencilFunction
 */
CF_INLINE const char* cf_compare_function_string(CF_CompareFunction compare) {
	switch (compare) {
	#define CF_ENUM(K, V) case CF_##K: return CF_STRINGIZE(CF_##K);
	CF_COMPARE_FUNCTION_DEFS
	#undef CF_ENUM
	default: return NULL;
	}
}

/**
 * @enum     CF_StencilOp
 * @category graphics
 * @brief    Stencil operations. These can happen when passing/failing a stencil test.
 * @related  CF_StencilOp cf_stencil_op_string CF_StencilFunction
 */
#define CF_STENCIL_OP_DEFS \
	/* @entry Keep. */                     \
	CF_ENUM(STENCIL_OP_KEEP,            0) \
	/* @entry Zero. */                     \
	CF_ENUM(STENCIL_OP_ZERO,            1) \
	/* @entry Replace. */                  \
	CF_ENUM(STENCIL_OP_REPLACE,         2) \
	/* @entry Increment clamp. */          \
	CF_ENUM(STENCIL_OP_INCREMENT_CLAMP, 3) \
	/* @entry Decrement clamp. */          \
	CF_ENUM(STENCIL_OP_DECREMENT_CLAMP, 4) \
	/* @entry Invert. */                   \
	CF_ENUM(STENCIL_OP_INVERT,          5) \
	/* @entry Increment wrap. */           \
	CF_ENUM(STENCIL_OP_INCREMENT_WRAP,  6) \
	/* @entry Decrement wrap. */           \
	CF_ENUM(STENCIL_OP_DECREMENT_WRAP,  7) \
	/* @end */

typedef enum CF_StencilOp
{
	#define CF_ENUM(K, V) CF_##K = V,
	CF_STENCIL_OP_DEFS
	#undef CF_ENUM
} CF_StencilOp;

/**
 * @function cf_stencil_op_string
 * @category graphics
 * @brief    Returns a `CF_StencilOp` converted to a C string.
 * @related  CF_StencilOp cf_stencil_op_string CF_StencilFunction
 */
CF_INLINE const char* cf_stencil_op_string(CF_StencilOp op) {
	switch (op) {
	#define CF_ENUM(K, V) case CF_##K: return CF_STRINGIZE(CF_##K);
	CF_STENCIL_OP_DEFS
	#undef CF_ENUM
	default: return NULL;
	}
}

/**
 * @enum     CF_BlendOp
 * @category graphics
 * @brief    Blend operations between two color components.
 * @remarks  See `CF_BlendState` for an overview.
 * @related  CF_BlendOp cf_blend_op_string CF_BlendState
 */
#define CF_BLEND_OP_DEFS \
	/* @entry Add. */                     \
	CF_ENUM(BLEND_OP_ADD,              0) \
	/* @entry Subtract, A - B. */         \
	CF_ENUM(BLEND_OP_SUBTRACT,         1) \
	/* @entry Reverse subtract, B - A. */ \
	CF_ENUM(BLEND_OP_REVERSE_SUBTRACT, 2) \
	/* @end */

typedef enum CF_BlendOp
{
	#define CF_ENUM(K, V) CF_##K = V,
	CF_BLEND_OP_DEFS
	#undef CF_ENUM
} CF_BlendOp;

/**
 * @function cf_blend_op_string
 * @category graphics
 * @brief    Returns a `CF_BlendOp` converted to a C string.
 * @related  CF_StencilOp cf_stencil_op_string CF_StencilFunction
 */
CF_INLINE const char* cf_blend_op_string(CF_BlendOp op) {
	switch (op) {
	#define CF_ENUM(K, V) case CF_##K: return CF_STRINGIZE(CF_##K);
	CF_BLEND_OP_DEFS
	#undef CF_ENUM
	default: return NULL;
	}
}

/**
 * @enum     CF_BlendFactor
 * @category graphics
 * @brief    Blend factors to compose a blend equation.
 * @remarks  See `CF_BlendState` for an overview.
 * @related  CF_BlendFactor cf_blend_factor_string CF_BlendState
 */
#define CF_BLEND_FACTOR_DEFS \
	/* @entry 0 */                                 \
	CF_ENUM(BLENDFACTOR_ZERO,                  0 ) \
	/* @entry 1 */                                 \
	CF_ENUM(BLENDFACTOR_ONE,                   1 ) \
	/* @entry S.color */                           \
	CF_ENUM(BLENDFACTOR_SRC_COLOR,             2 ) \
	/* @entry (1 - S.rgb) */                       \
	CF_ENUM(BLENDFACTOR_ONE_MINUS_SRC_COLOR,   3 ) \
	/* @entry S.alpha */                           \
	CF_ENUM(BLENDFACTOR_SRC_ALPHA,             4 ) \
	/* @entry (1 - S.alpha) */                     \
	CF_ENUM(BLENDFACTOR_ONE_MINUS_SRC_ALPHA,   5 ) \
	/* @entry D.rgb */                             \
	CF_ENUM(BLENDFACTOR_DST_COLOR,             6 ) \
	/* @entry (1 - D.rgb) */                       \
	CF_ENUM(BLENDFACTOR_ONE_MINUS_DST_COLOR,   7 ) \
	/* @entry D.alpha */                           \
	CF_ENUM(BLENDFACTOR_DST_ALPHA,             8 ) \
	/* @entry (1 - D.alpha) */                     \
	CF_ENUM(BLENDFACTOR_ONE_MINUS_DST_ALPHA,   9 ) \
	/* @entry min(S.alpha, 1 - D.alpha) */         \
	CF_ENUM(BLENDFACTOR_SRC_ALPHA_SATURATED,   10) \
	/* @entry C (constant color not currently supported) */ \
	CF_ENUM(BLENDFACTOR_BLEND_COLOR,           11) \
	/* @entry 1 - C.rgb (constant color not currently supported) */ \
	CF_ENUM(BLENDFACTOR_ONE_MINUS_BLEND_COLOR, 12) \
	/* @entry C.alpha (constant color not currently supported) */ \
	CF_ENUM(BLENDFACTOR_BLEND_ALPHA,           13) \
	/* @entry (1 - C.alpha) (constant color not currently supported) */ \
	CF_ENUM(BLENDFACTOR_ONE_MINUS_BLEND_ALPHA, 14) \
	/* @end */

typedef enum CF_BlendFactor
{
	#define CF_ENUM(K, V) CF_##K = V,
	CF_BLEND_FACTOR_DEFS
	#undef CF_ENUM
} CF_BlendFactor;

/**
 * @function cf_blend_factor_string
 * @category graphics
 * @brief    Returns a `CF_BlendFactor` converted to a C string.
 * @related  CF_BlendFactor cf_blend_factor_string CF_BlendState
 */
CF_INLINE const char* cf_blend_factor_string(CF_BlendFactor factor) {
	switch (factor) {
	#define CF_ENUM(K, V) case CF_##K: return CF_STRINGIZE(CF_##K);
	CF_BLEND_FACTOR_DEFS
	#undef CF_ENUM
	default: return NULL;
	}
}

/**
 * @struct   CF_StencilFunction
 * @category graphics
 * @brief    Defines functions for stencil rendering.
 * @remarks  The stencil buffer stores references values used for rendering with comparisons. Only comparisons that end up
 *           logically true pass the stencil test and end up getting drawn. For an overview of stencil testing [learnopengl.com
 *           has an excellent article](https://learnopengl.com/Advanced-OpenGL/Stencil-testing) on the topic.
 * @related  CF_StencilFunction CF_StencilParams CF_RenderState cf_material_set_render_state
 */
typedef struct CF_StencilFunction
{
	/* @member Comparison type for the stencil test. See `CF_CompareFunction`. */
	CF_CompareFunction compare;

	/* @member An operation to perform upon failing a stencil test. See `CF_StencilOp`. */
	CF_StencilOp fail_op;

	/* @member An operation to perform upon failing a depth test. See `CF_StencilOp`. */
	CF_StencilOp depth_fail_op;

	/* @member An operation to perform upon passing a stencil test. See `CF_StencilOp`. */
	CF_StencilOp pass_op;
} CF_StencilFunction;
// @end

/**
 * @struct   CF_StencilParams
 * @category graphics
 * @brief    Settings for the stencil buffer.
 * @remarks  For an overview of stencil testing [learnopengl.com has an excellent article](https://learnopengl.com/Advanced-OpenGL/Stencil-testing) on the topic.
 * @related  CF_StencilFunction CF_StencilParams CF_RenderState cf_material_set_render_state
 */
typedef struct CF_StencilParams
{
	/* @member The stencil buffer will not be used unless this is true. */
	bool enabled;

	/* @member Used to control which bits get read from the stencil buffer. */
	uint8_t read_mask;

	/* @member Used to control which bits get written to the stencil buffer. */
	uint8_t write_mask;

	/* @member After reading from the stencil buffer, the `reference` value is used in a comparison to perform a stencil operation. See `CF_StencilFunction` and `CF_StencilOp`. */
	uint8_t reference;

	/* @member The stencil function to use for front-facing triangles (counter-clockwise). */
	CF_StencilFunction front;

	/* @member The stencil function to use for back-facing triangles (clockwise). */
	CF_StencilFunction back;
} CF_StencilParams;
// @end

/**
 * @struct   CF_BlendState
 * @category graphics
 * @brief    Defines how colors are mixed together when the GPU is drawing individual pixels.
 * @remarks  There are many ways to blend two colors together to create all kinds of different effects. Defining how we draw one thing atop
 *           another is called "compositing", or "image compositing". The actual operation of mixing two colors together to form a
 *           a pixel is called blending. We describe blend equations as little math equations.
 *           
 *           We can say the pixel is called P, while the input colors are S and D (source and destination). Modern GPUs provide some
 *           common _operators_ for defining blend functions: add, subtract, reverse subtract, min and max (see `CF_BlendOp`). Cute Framework only
 *           uses add, subtract, and reverse subtract, as min/max are not very cross-platform compatible. On each side of an _operator_ are the
 *           _factors_ (see `CF_BlendFactor`).
 *           
 *           Here is the add operator `CF_BLEND_OP_ADD`:
 *           
 *           P = S + D
 *           
 *           Recap: P is the pixel to write, S is the source factor, while D is the destination factor. Usually the D (destination factor) is
 *           the old pixel value, while S (source factor) is a new image getting draw over old pixel contents. Therefor, P is the final color
 *           after compositing a new image on top of an old image.
 *           
 *           Blend factors (see `CF_BlendFactor`) are components of a color, including the alpha component. The most common setup for your
 *           blend state is to use [premultiplied alpha](https://blog.demofox.org/2015/06/19/what-is-pre-multiplied-alpha-and-why-does-it-matter/) when loading your images.
 *           Cute Framework internally loads up images in premultiplied alpha format for you. Cute Framework's default blend state is to use
 *           _additive blending_. The blend function looks like this:
 *           
 *           P = 1 x S + D x (1 - S.alpha)
 *           
 *           Or re-written with Cute Framework enums:
 *           
 *           P = S x CF_BLENDFACTOR_ONE CF_BLEND_OP_ADD D x CF_BLENDFACTOR_ONE_MINUS_SRC_ALPHA
 *           
 *           Which can be setup with this kind of code:
 *           
 *           ```cpp
 *           draw->material = cf_make_material();
 *           CF_RenderState state = cf_render_state_defaults();
 *           state.blend.enabled = true;
 *           state.blend.rgb_src_blend_factor = CF_BLENDFACTOR_ONE;
 *           state.blend.rgb_dst_blend_factor = CF_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
 *           state.blend.rgb_op = CF_BLEND_OP_ADD;
 *           state.blend.alpha_src_blend_factor = CF_BLENDFACTOR_ONE;
 *           state.blend.alpha_dst_blend_factor = CF_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
 *           state.blend.alpha_op = CF_BLEND_OP_ADD;
 *           draw->render_states.add(state);
 *           cf_material_set_render_state(draw->material, state);
 *           ```
 *           
 *           You can of course define your own blend state in any way you like to perform all kinds of compositing effects. However, dynamically changing the
 *           blend state will result in more draw calls to the GPU, which negatively affects performance. A major win for premultiplied alpha and
 *           additive blending is that some common rendering needs can all be batched together within one draw call, without requiring a change to
 *           the blend state. One example is rendering additively or with transparency in the same draw call. There are some other benefits that come from
 *           this style as well. Here are some nice links on the topic.
 *           - https://developer.nvidia.com/content/alpha-blending-pre-or-not-pre
 *           - https://shawnhargreaves.com/blog/premultiplied-alpha.html
 * @related  CF_BlendFactor CF_BlendOp CF_RenderState cf_material_set_render_state
 */
typedef struct CF_BlendState
{
	/* @member True enables modifying the default blend state. */
	bool enabled;

	/* @member The pixel format to perform blend operations. This should match the target texture you're rendering to. See `CF_PixelFormat`. */
	CF_PixelFormat pixel_format;

	/* @member Set to true to blend the red component. */
	bool write_R_enabled;

	/* @member Set to true to blend the green component. */
	bool write_G_enabled;

	/* @member Set to true to blend the blue component. */
	bool write_B_enabled;

	/* @member Set to true to blend the alpha component. */
	bool write_A_enabled;

	/* @member The blend operator to use for the color components. See `CF_BlendOp`. */
	CF_BlendOp rgb_op;

	/* @member The factor to use for S in the blend equation for the color components. See remarks for details. */
	CF_BlendFactor rgb_src_blend_factor;

	/* @member The factor to use for D in the blend equation for the color components. See remarks for details. */
	CF_BlendFactor rgb_dst_blend_factor;

	/* @member The blend operator to use for the alpha component. See `CF_BlendOp`. */
	CF_BlendOp alpha_op;

	/* @member The factor to use for S in the blend equation for the alpha component. See remarks for details. */
	CF_BlendFactor alpha_src_blend_factor;

	/* @member The factor to use for D in the blend equation for the alpha component. See remarks for details. */
	CF_BlendFactor alpha_dst_blend_factor;
} CF_BlendState;
// @end

/**
 * @struct   CF_RenderState
 * @category graphics
 * @brief    A bag of rendering related settings.
 * @remarks  The `CF_RenderState` is a big collection of various rendering settings, such as culling mode,
 *           blending operations, depth and stencil settings, etc. Altering these on a material always means
 *           increasing your draw call count. It's best to try and set these once and leave them alone, though
 *           this is not always possible.
 * @related  CF_BlendState CF_CullMode CF_StencilParams cf_material_set_render_state
 */
typedef struct CF_RenderState
{
	/* @member Controls whether or not to cull triangles based on their winding order. See `CF_CullMode`. */
	CF_CullMode cull_mode;

	/* @member Controls how the GPU blends pixels together during compositing. See `CF_BlendState`. */
	CF_BlendState blend;

	/* @member Defines how to perform depth-testing. See `CF_CompareFunction`. */
	CF_CompareFunction depth_compare;

	/* @member Must be true to enable depth-testing and use of the depth buffer. */
	bool depth_write_enabled;

	/* @member Sets up how to perform (if at all) stencil testing. See `CF_StencilParams`. */
	CF_StencilParams stencil;
} CF_RenderState;
// @end

/**
 * @function cf_render_state_defaults
 * @category graphics
 * @brief    Returns a good set of default parameters for a `CF_RenderState`.
 * @related  CF_RenderState cf_render_state_defaults cf_material_set_render_state
 */
CF_API CF_RenderState CF_CALL cf_render_state_defaults();

//--------------------------------------------------------------------------------------------------
// Material.

/**
 * @enum     CF_UniformType
 * @category graphics
 * @brief    The available types of uniforms.
 * @remarks  A uniform is like a global variable for a shader. We set uniforms by using a `CF_Material`.
 * @related  CF_UniformType cf_uniform_type_string CF_Material cf_make_material
 */
#define CF_UNIFORM_TYPE_DEFS \
	/* @entry In a shader: `uniform float` */  \
	CF_ENUM(UNIFORM_TYPE_FLOAT,  0)            \
	/* @entry In a shader: `uniform vec2` */   \
	CF_ENUM(UNIFORM_TYPE_FLOAT2, 1)            \
	/* @entry In a shader: `uniform vec4` */   \
	CF_ENUM(UNIFORM_TYPE_FLOAT4, 2)            \
	/* @entry In a shader: `uniform int` */    \
	CF_ENUM(UNIFORM_TYPE_INT,    3)            \
	/* @entry In a shader: `uniform int[2]` */ \
	CF_ENUM(UNIFORM_TYPE_INT2,   4)            \
	/* @entry In a shader: `uniform int[4]` */ \
	CF_ENUM(UNIFORM_TYPE_INT4,   5)            \
	/* @entry In a shader: `uniform mat4` */   \
	CF_ENUM(UNIFORM_TYPE_MAT4,   6)            \
	/* @end */

typedef enum CF_UniformType
{
	#define CF_ENUM(K, V) CF_##K = V,
	CF_UNIFORM_TYPE_DEFS
	#undef CF_ENUM
} CF_UniformType;

/**
 * @function cf_uniform_type_string
 * @category graphics
 * @brief    Returns a `CF_UniformType` converted to a C string.
 * @related  CF_UniformType cf_uniform_type_string CF_Material cf_make_material
 */
CF_INLINE const char* cf_uniform_type_string(CF_UniformType type) {
	switch (type) {
	#define CF_ENUM(K, V) case CF_##K: return CF_STRINGIZE(CF_##K);
	CF_UNIFORM_TYPE_DEFS
	#undef CF_ENUM
	default: return NULL;
	}
}

/**
 * @function cf_make_material
 * @category graphics
 * @brief    Creates a new material.
 * @remarks  A material holds render state (see `CF_RenderState`), texture inputs (see `CF_Texture`), as well as shader inputs called
 *           uniforms (see `CF_UniformType`). For an overview see `CF_Material`.
 * @related  CF_UniformType CF_Material cf_make_material cf_destroy_material cf_material_set_render_state cf_material_set_texture_vs cf_material_set_texture_fs cf_material_set_uniform_vs cf_material_set_uniform_fs
 */
CF_API CF_Material CF_CALL cf_make_material();

/**
 * @function cf_destroy_material
 * @category graphics
 * @brief    Frees up a material created by `cf_make_material`.
 * @related  CF_UniformType CF_Material cf_make_material cf_destroy_material cf_material_set_render_state cf_material_set_texture_vs cf_material_set_texture_fs cf_material_set_uniform_vs cf_material_set_uniform_fs
 */
CF_API void CF_CALL cf_destroy_material(CF_Material material);

/**
 * @function cf_material_set_render_state
 * @category graphics
 * @brief    Sets the render state for a material.
 * @param    material      The material.
 * @param    render_state  The new render state to set on `material`.
 * @remarks  See `CF_RenderState` for an overview.
 * @related  CF_UniformType CF_Material cf_make_material cf_destroy_material cf_material_set_render_state cf_material_set_texture_vs cf_material_set_texture_fs cf_material_set_uniform_vs cf_material_set_uniform_fs
 */
CF_API void CF_CALL cf_material_set_render_state(CF_Material material, CF_RenderState render_state);

/**
 * @function cf_material_set_texture_vs
 * @category graphics
 * @brief    Sets up a texture, used for inputs to vertex shaders.
 * @param    material      The material.
 * @param    name          The name of the texture, for referring to within a vertex shader.
 * @param    texture       Data (usually an image) for a shader to access.
 * @remarks  See `CF_Texture` and `CF_TextureParams` for an overview.
 * @related  CF_UniformType CF_Material cf_make_material cf_destroy_material cf_material_set_render_state cf_material_set_texture_vs cf_material_set_texture_fs cf_material_set_uniform_vs cf_material_set_uniform_fs
 */
CF_API void CF_CALL cf_material_set_texture_vs(CF_Material material, const char* name, CF_Texture texture);

/**
 * @function cf_material_set_texture_fs
 * @category graphics
 * @brief    Sets up a texture, used for inputs to fragment shaders.
 * @param    material      The material.
 * @param    name          The name of the texture, for referring to within a fragment shader.
 * @param    texture       Data (usually an image) for a shader to access.
 * @remarks  See `CF_Texture` and `CF_TextureParams` for an overview.
 * @related  CF_UniformType CF_Material cf_make_material cf_destroy_material cf_material_set_render_state cf_material_set_texture_vs cf_material_set_texture_fs cf_material_set_uniform_vs cf_material_set_uniform_fs
 */
CF_API void CF_CALL cf_material_set_texture_fs(CF_Material material, const char* name, CF_Texture texture);

/**
 * @function cf_material_clear_textures
 * @category graphics
 * @brief    Clears all textures previously set by `cf_material_set_texture_vs` or `cf_material_set_texture_fs`.
 * @param    material      The material.
 * @remarks  See `CF_Texture` and `CF_TextureParams` for an overview.
 * @related  CF_UniformType CF_Material cf_make_material cf_destroy_material cf_material_set_render_state cf_material_set_texture_vs cf_material_set_texture_fs cf_material_set_uniform_vs cf_material_set_uniform_fs
 */
CF_API void CF_CALL cf_material_clear_textures(CF_Material material);

/**
 * @function cf_material_set_uniform_vs
 * @category graphics
 * @brief    Sets up a uniform value, used for inputs to vertex shaders.
 * @param    material      The material.
 * @param    block_name    The block name acts like namespace, and groups together uniforms in a single contiguous chunk of memory. You should place
 *                         uniforms that are related to each other, and accessed at the same time, into the same block.
 * @param    name          The name of the uniform as it appears in the shader.
 * @param    data          The value of the uniform.
 * @param    type          The type of the uniform. See `CF_UniformType`.
 * @param    array_length  The number of elements in the uniform array. Usually this is just `1`, as in, not an array but just one variable.
 * @remarks  Uniforms set here do not need to exist in the shader. It's completely acceptable (and encouraged) to setup many uniforms in a material.
 *           Once the material is applied via `cf_apply_shader`, all shader input uniforms (e.g. `uniform vec4 u_my_color`) are dynamically matched up
 *           with uniform values stored in the `CF_Material`. Any uniforms in the material that don't match up will simply be ignored, and cleared to 0
 *           in the shader.
 *           
 *           `CF_Material`'s design supports using one material with various shaders, or using various materials with one shader. Since uniforms are
 *           grouped up into uniform blocks the performance overhead is usually quite minimal for setting a variety of uniform and shader combinations.
 * @related  CF_UniformType CF_Material cf_make_material cf_destroy_material cf_material_set_render_state cf_material_set_texture_vs cf_material_set_texture_fs cf_material_set_uniform_vs cf_material_set_uniform_fs
 */
CF_API void CF_CALL cf_material_set_uniform_vs(CF_Material material, const char* block_name, const char* name, void* data, CF_UniformType type, int array_length);

/**
 * @function cf_material_set_uniform_fs
 * @category graphics
 * @brief    Sets up a uniform value, used for inputs to fragment shaders.
 * @param    material      The material.
 * @param    block_name    The block name acts like namespace, and groups together uniforms in a single contiguous chunk of memory. You should place
 *                         uniforms that are related to each other, and accessed at the same time, into the same block.
 * @param    name          The name of the uniform as it appears in the shader.
 * @param    data          The value of the uniform.
 * @param    type          The type of the uniform. See `CF_UniformType`.
 * @param    array_length  The number of elements in the uniform array. Usually this is just `1`, as in, not an array but just one variable.
 * @remarks  Uniforms set here do not need to exist in the shader. It's completely acceptable (and encouraged) to setup many uniforms in a material.
 *           Once the material is applied via `cf_apply_shader`, all shader input uniforms (e.g. `uniform vec4 u_my_color`) are dynamically matched up
 *           with uniform values stored in the `CF_Material`. Any uniforms in the material that don't match up will simply be ignored, and cleared to 0
 *           in the shader.
 *           
 *           `CF_Material`'s design supports using one material with various shaders, or using various materials with one shader. Since uniforms are
 *           grouped up into uniform blocks the performance overhead is usually quite minimal for setting a variety of uniform and shader combinations.
 * @related  CF_UniformType CF_Material cf_make_material cf_destroy_material cf_material_set_render_state cf_material_set_texture_vs cf_material_set_texture_fs cf_material_set_uniform_vs cf_material_set_uniform_fs
 */
CF_API void CF_CALL cf_material_set_uniform_fs(CF_Material material, const char* block_name, const char* name, void* data, CF_UniformType type, int array_length);

/**
 * @function cf_material_clear_uniforms
 * @category graphics
 * @brief    Clears any uniforms previously set by `cf_material_set_uniform_vs` or `cf_material_set_uniform_fs`.
 * @param    material      The material.
 * @related  CF_UniformType CF_Material cf_make_material cf_destroy_material cf_material_set_render_state cf_material_set_texture_vs cf_material_set_texture_fs cf_material_set_uniform_vs cf_material_set_uniform_fs
 */
CF_API void CF_CALL cf_material_clear_uniforms(CF_Material material);

//--------------------------------------------------------------------------------------------------
// Rendering Functions.

/**
 * @function cf_clear_color
 * @category graphics
 * @brief    Sets the color used when clearing a canvas.
 * @remarks  This will get used when `cf_apply_canvas` or when `cf_app_draw_onto_screen` is called.
 * @related  cf_clear_color cf_clear_color2 cf_clear_depth_stencil cf_apply_canvas cf_app_draw_onto_screen
 */
CF_API void CF_CALL cf_clear_color(float red, float green, float blue, float alpha);

/**
 * @function cf_clear_color2
 * @category graphics
 * @brief    Sets the color used when clearing a canvas.
 * @remarks  This will get used when `cf_apply_canvas` or when `cf_app_draw_onto_screen` is called.
 * @related  cf_clear_color cf_clear_color2 cf_clear_depth_stencil cf_apply_canvas cf_app_draw_onto_screen
 */
CF_API void CF_CALL cf_clear_color2(CF_Color color);

/**
 * @function cf_clear_depth_stencil
 * @category graphics
 * @brief    Sets the depth/stencil values used when clearing a canvas, if depth/stencil are enabled (see `CF_RenderState`).
 * @remarks  This will get used when `cf_apply_canvas` or when `cf_app_draw_onto_screen` is called.
 * @related  cf_clear_color cf_clear_color2 cf_clear_depth_stencil cf_apply_canvas cf_app_draw_onto_screen
 */
CF_API void CF_CALL cf_clear_depth_stencil(float depth, float stencil);

/**
 * @function cf_apply_canvas
 * @category graphics
 * @brief    Sets up which canvas to draw to.
 * @param    canvas     The canvas to draw to.
 * @param    clear      Clears the screen to `cf_clear_color` if true.
 * @related  CF_Canvas cf_clear_color cf_apply_viewport cf_apply_scissor
 */
CF_API void CF_CALL cf_apply_canvas(CF_Canvas canvas, bool clear);

/**
 * @function cf_apply_viewport
 * @category graphics
 * @brief    Sets up a viewport to render within.
 * @param    x          Center of the viewport on the x-axis.
 * @param    y          Center of the viewport on the y-axis.
 * @param    width      Width of the viewport in pixels.
 * @param    height     Height of the viewport in pixels.
 * @remarks  The viewport is a window on the screen to render within. The canvas will be stretched to fit onto the viewport.
 * @related  cf_apply_canvas cf_apply_viewport cf_apply_scissor
 */
CF_API void CF_CALL cf_apply_viewport(int x, int y, int width, int height);

/**
 * @function cf_apply_scissor
 * @category graphics
 * @brief    Sets up a scissor box to clip rendering within.
 * @param    x          Center of the scissor box on the x-axis.
 * @param    y          Center of the scissor box on the y-axis.
 * @param    width      Width of the scissor box in pixels.
 * @param    height     Height of the scissor box in pixels.
 * @remarks  The scissor box is a window on the screen that rendering will be clipped within. Any rendering that occurs outside the
 *           scissor box will simply be ignored, rendering nothing and leaving the previous pixel contents untouched.
 * @related  cf_apply_canvas cf_apply_viewport cf_apply_scissor
 */
CF_API void CF_CALL cf_apply_scissor(int x, int y, int width, int height);

/**
 * @function cf_apply_mesh
 * @category graphics
 * @brief    Uses a specific mesh for rendering.
 * @remarks  The mesh contains vertex data, defining the geometry to be rendered. The mesh vertices are sent to the GPU as inputs to
 *           the vertex shader. See `CF_Mesh` for an overview.
 * @related  CF_Mesh cf_create_mesh cf_apply_shader cf_draw_elements
 */
CF_API void CF_CALL cf_apply_mesh(CF_Mesh mesh);

/**
 * @function cf_apply_shader
 * @category graphics
 * @brief    Uses a specific shader + material combo for rendering.
 * @remarks  The `CF_Shader` defines how to render a mesh's geometry, set by `cf_apply_mesh`. The `CF_Mesh` holds input geometry to the
 *           vertex shader. A `CF_Material` defines uniform and texture inputs to the shader.
 * @related  CF_Mesh cf_create_mesh cf_apply_shader cf_draw_elements
 */
CF_API void CF_CALL cf_apply_shader(CF_Shader shader, CF_Material material);

/**
 * @function cf_draw_elements
 * @category graphics
 * @brief    Draws all elements within the last applied mesh.
 * @remarks  If the mesh is a static mesh with usage `CF_USAGE_TYPE_IMMUTABLE` the number of elements drawn will always be consistent with the mesh's
 *           initial data. For `USAGE_TYPE_DYNAMIC` and `CF_USAGE_TYPE_STREAM` the number of elements will always match the previous call to
 *           `cf_mesh_update_***` or `cf_mesh_append_***`.
 * @related  CF_Mesh cf_create_mesh cf_apply_shader cf_apply_canvas
 */
CF_API void CF_CALL cf_draw_elements();

/**
 * @function cf_unapply_canvas
 * @category graphics
 * @brief    An optional function to end the current rendering pass.
 * @remarks  This is only useful when a particular canvas needs to be destroyed, though it may be currently applied. For example, if the screen is
 *           resized and you want to resize some of your canvases as well.
 * @related  CF_Canvas cf_apply_canvas
 */
CF_API void CF_CALL cf_unapply_canvas();

#ifdef __cplusplus
}
#endif // __cplusplus

//--------------------------------------------------------------------------------------------------
// C++ API

#ifdef CF_CPP

namespace Cute
{

using Texture  = CF_Texture;
using Canvas = CF_Canvas;
using Mesh = CF_Mesh;
using Material = CF_Material;
using Shader = CF_Shader;
using TextureParams = CF_TextureParams;
using SokolShader = CF_SokolShader;
using CanvasParams = CF_CanvasParams;
using VertexAttribute = CF_VertexAttribute;
using StencilFunction = CF_StencilFunction;
using StencilParams = CF_StencilParams;
using BlendState = CF_BlendState;
using RenderState = CF_RenderState;
using BackendType = CF_BackendType;

using BackendType = CF_BackendType;
#define CF_ENUM(K, V) CF_INLINE constexpr CF_BackendType K = CF_##K;
CF_BACKEND_TYPE_DEFS
#undef CF_ENUM

CF_INLINE constexpr const char* to_string(CF_BackendType type) { switch(type) {
	#define CF_ENUM(K, V) case CF_##K: return #K;
	CF_BACKEND_TYPE_DEFS
	#undef CF_ENUM
	default: return NULL;
	}
}

using DeviceFeature = CF_DeviceFeature;
#define CF_ENUM(K, V) CF_INLINE constexpr DeviceFeature K = CF_##K;
CF_DEVICE_FEATURE_DEFS
#undef CF_ENUM

CF_INLINE constexpr const char* to_string(DeviceFeature type) { switch(type) {
	#define CF_ENUM(K, V) case CF_##K: return #K;
	CF_DEVICE_FEATURE_DEFS
	#undef CF_ENUM
	default: return NULL;
	}
}

using PixelFormat = CF_PixelFormat;
#define CF_ENUM(K, V) CF_INLINE constexpr PixelFormat K = CF_##K;
CF_PIXELFORMAT_DEFS
#undef CF_ENUM

CF_INLINE constexpr const char* to_string(PixelFormat type) { switch(type) {
	#define CF_ENUM(K, V) case CF_##K: return #K;
	CF_PIXELFORMAT_DEFS
	#undef CF_ENUM
	default: return NULL;
	}
}

using PixelFormatOp = CF_PixelFormatOp;
#define CF_ENUM(K, V) CF_INLINE constexpr PixelFormatOp K = CF_##K;
CF_PIXELFORMAT_OP_DEFS
#undef CF_ENUM

CF_INLINE constexpr const char* to_string(PixelFormatOp type) { switch(type) {
	#define CF_ENUM(K, V) case CF_##K: return #K;
	CF_PIXELFORMAT_OP_DEFS
	#undef CF_ENUM
	default: return NULL;
	}
}

using ResourceLimit = CF_ResourceLimit;
#define CF_ENUM(K, V) CF_INLINE constexpr ResourceLimit K = CF_##K;
CF_RESOURCE_LIMIT_DEFS
#undef CF_ENUM

CF_INLINE constexpr const char* to_string(ResourceLimit type) { switch(type) {
	#define CF_ENUM(K, V) case CF_##K: return #K;
	CF_RESOURCE_LIMIT_DEFS
	#undef CF_ENUM
	default: return NULL;
	}
}

using UsageType = CF_UsageType;
#define CF_ENUM(K, V) CF_INLINE constexpr UsageType K = CF_##K;
CF_USAGE_TYPE_DEFS
#undef CF_ENUM

CF_INLINE constexpr const char* to_string(UsageType type) { switch(type) {
	#define CF_ENUM(K, V) case CF_##K: return #K;
	CF_USAGE_TYPE_DEFS
	#undef CF_ENUM
	default: return NULL;
	}
}

using Filter = CF_Filter;
#define CF_ENUM(K, V) CF_INLINE constexpr Filter K = CF_##K;
CF_FILTER_DEFS
#undef CF_ENUM

CF_INLINE constexpr const char* to_string(Filter type) { switch(type) {
	#define CF_ENUM(K, V) case CF_##K: return #K;
	CF_FILTER_DEFS
	#undef CF_ENUM
	default: return NULL;
	}
}

using WrapMode = CF_WrapMode;
#define CF_ENUM(K, V) CF_INLINE constexpr WrapMode K = CF_##K;
CF_WRAP_MODE_DEFS
#undef CF_ENUM

CF_INLINE constexpr const char* to_string(WrapMode type) { switch(type) {
	#define CF_ENUM(K, V) case CF_##K: return #K;
	CF_WRAP_MODE_DEFS
	#undef CF_ENUM
	default: return NULL;
	}
}

using VertexFormat = CF_VertexFormat;
#define CF_ENUM(K, V) CF_INLINE constexpr VertexFormat K = CF_##K;
CF_VERTEX_FORMAT_DEFS
#undef CF_ENUM

CF_INLINE constexpr const char* to_string(VertexFormat type) { switch(type) {
	#define CF_ENUM(K, V) case CF_##K: return #K;
	CF_VERTEX_FORMAT_DEFS
	#undef CF_ENUM
	default: return NULL;
	}
}

using AttributeStep = CF_AttributeStep;
#define CF_ENUM(K, V) CF_INLINE constexpr AttributeStep K = CF_##K;
CF_ATTRIBUTE_STEP_DEFS
#undef CF_ENUM

CF_INLINE constexpr const char* to_string(AttributeStep type) { switch(type) {
	#define CF_ENUM(K, V) case CF_##K: return #K;
	CF_ATTRIBUTE_STEP_DEFS
	#undef CF_ENUM
	default: return NULL;
	}
}

using CullMode = CF_CullMode;
#define CF_ENUM(K, V) CF_INLINE constexpr CullMode K = CF_##K;
CF_CULL_MODE_DEFS
#undef CF_ENUM

CF_INLINE constexpr const char* to_string(CullMode type) { switch(type) {
	#define CF_ENUM(K, V) case CF_##K: return #K;
	CF_CULL_MODE_DEFS
	#undef CF_ENUM
	default: return NULL;
	}
}

using CompareFunction = CF_CompareFunction;
#define CF_ENUM(K, V) CF_INLINE constexpr CompareFunction K = CF_##K;
CF_COMPARE_FUNCTION_DEFS
#undef CF_ENUM

CF_INLINE constexpr const char* to_string(CompareFunction type) { switch(type) {
	#define CF_ENUM(K, V) case CF_##K: return #K;
	CF_COMPARE_FUNCTION_DEFS
	#undef CF_ENUM
	default: return NULL;
	}
}

using StencilOp = CF_StencilOp;
#define CF_ENUM(K, V) CF_INLINE constexpr StencilOp K = CF_##K;
CF_STENCIL_OP_DEFS
#undef CF_ENUM

CF_INLINE constexpr const char* to_string(StencilOp type) { switch(type) {
	#define CF_ENUM(K, V) case CF_##K: return #K;
	CF_STENCIL_OP_DEFS
	#undef CF_ENUM
	default: return NULL;
	}
}

using BlendOp = CF_BlendOp;
#define CF_ENUM(K, V) CF_INLINE constexpr BlendOp K = CF_##K;
CF_BLEND_OP_DEFS
#undef CF_ENUM

CF_INLINE constexpr const char* to_string(BlendOp type) { switch(type) {
	#define CF_ENUM(K, V) case CF_##K: return #K;
	CF_BLEND_OP_DEFS
	#undef CF_ENUM
	default: return NULL;
	}
}

using BlendFactor = CF_BlendFactor;
#define CF_ENUM(K, V) CF_INLINE constexpr BlendFactor K = CF_##K;
CF_BLEND_FACTOR_DEFS
#undef CF_ENUM

CF_INLINE constexpr const char* to_string(BlendFactor type) { switch(type) {
	#define CF_ENUM(K, V) case CF_##K: return #K;
	CF_BLEND_FACTOR_DEFS
	#undef CF_ENUM
	default: return NULL;
	}
}

using UniformType = CF_UniformType;
#define CF_ENUM(K, V) CF_INLINE constexpr UniformType K = CF_##K;
CF_UNIFORM_TYPE_DEFS
#undef CF_ENUM

CF_INLINE constexpr const char* to_string(UniformType type) { switch(type) {
	#define CF_ENUM(K, V) case CF_##K: return #K;
	CF_UNIFORM_TYPE_DEFS
	#undef CF_ENUM
	default: return NULL;
	}
}

CF_INLINE BackendType query_backend() { return cf_query_backend(); }
CF_INLINE bool query_pixel_format(PixelFormat format, PixelFormatOp op) { return cf_query_pixel_format(format, op); }
CF_INLINE bool query_device_feature(DeviceFeature feature) { return cf_query_device_feature(feature); }
CF_INLINE int query_resource_limit(ResourceLimit resource_limit) { return cf_query_resource_limit(resource_limit); }
CF_INLINE TextureParams texture_defaults(int w, int h) { return cf_texture_defaults(w, h); }
CF_INLINE Texture make_texture(TextureParams texture_params) { return cf_make_texture(texture_params); }
CF_INLINE void destroy_texture(Texture texture) { cf_destroy_texture(texture); }
CF_INLINE void update_texture(Texture texture, void* data, int size) { cf_update_texture(texture, data, size); }
CF_INLINE Shader make_shader(SokolShader sokol_shader) { return cf_make_shader(sokol_shader); }
CF_INLINE void destroy_shader(Shader shader) { cf_destroy_shader(shader); }
CF_INLINE CanvasParams canvas_defaults(int w, int h) { return cf_canvas_defaults(w, h); }
CF_INLINE Canvas make_canvas(CanvasParams pass_params) { return cf_make_canvas(pass_params); }
CF_INLINE void destroy_canvas(Canvas canvas) { cf_destroy_canvas(canvas); }
CF_INLINE Texture canvas_get_target(Canvas canvas) { return cf_canvas_get_target(canvas); }
CF_INLINE Texture canvas_get_depth_stencil_target(Canvas canvas) { return cf_canvas_get_depth_stencil_target(canvas); }
CF_INLINE uint64_t canvas_get_backend_target_handle(Canvas canvas) { return cf_canvas_get_backend_target_handle(canvas); }
CF_INLINE uint64_t canvas_get_backend_depth_stencil_handle(Canvas canvas) { return cf_canvas_get_backend_depth_stencil_handle(canvas); }
CF_INLINE void canvas_blit(Canvas src, v2 u0, v2 v0, Canvas dst, v2 u1, v2 v1) { cf_canvas_blit(src, u0, v0, dst, u1, v1); }
CF_INLINE Mesh make_mesh(UsageType usage_type, int vertex_buffer_size, int index_buffer_size, int instance_buffer_size) { return cf_make_mesh(usage_type, vertex_buffer_size, index_buffer_size, instance_buffer_size); }
CF_INLINE void destroy_mesh(Mesh mesh) { cf_destroy_mesh(mesh); }
CF_INLINE void mesh_set_attributes(Mesh mesh, const VertexAttribute* attributes, int attribute_count, int vertex_stride, int instance_stride) { cf_mesh_set_attributes(mesh, attributes, attribute_count, vertex_stride, instance_stride); }
CF_INLINE void mesh_update_vertex_data(Mesh mesh, void* data, int count) { cf_mesh_update_vertex_data(mesh, data, count); }
CF_INLINE int mesh_append_vertex_data(Mesh mesh, void* data, int append_count) { return cf_mesh_append_vertex_data(mesh, data, append_count); }
CF_INLINE bool mesh_will_overflow_vertex_data(Mesh mesh, int append_count) { return cf_mesh_will_overflow_vertex_data(mesh, append_count); }
CF_INLINE void mesh_update_instance_data(Mesh mesh, void* data, int count) { cf_mesh_update_instance_data(mesh, data, count); }
CF_INLINE int mesh_append_instance_data(Mesh mesh, void* data, int append_count) { return cf_mesh_append_instance_data(mesh, data, append_count); }
CF_INLINE bool mesh_will_overflow_instance_data(Mesh mesh, int append_count) { return cf_mesh_will_overflow_instance_data(mesh, append_count); }
CF_INLINE void mesh_update_index_data(Mesh mesh, uint32_t* indices, int count) { cf_mesh_update_index_data(mesh, indices, count); }
CF_INLINE int mesh_append_index_data(Mesh mesh, uint32_t* indices, int append_count) { return cf_mesh_append_index_data(mesh, indices, append_count); }
CF_INLINE bool mesh_will_overflow_index_data(Mesh mesh, int append_count) { return cf_mesh_will_overflow_index_data(mesh, append_count); }
CF_INLINE RenderState render_state_defaults() { return cf_render_state_defaults(); }
CF_INLINE Material make_material() { return cf_make_material(); }
CF_INLINE void destroy_material(Material material) { cf_destroy_material(material); }
CF_INLINE void material_set_render_state(Material material, RenderState render_state) { cf_material_set_render_state(material, render_state); }
CF_INLINE void material_set_texture_vs(Material material, const char* name, Texture texture) { cf_material_set_texture_vs(material, name, texture); }
CF_INLINE void material_set_texture_fs(Material material, const char* name, Texture texture) { cf_material_set_texture_fs(material, name, texture); }
CF_INLINE void material_clear_textures(Material material) { cf_material_clear_textures(material); }
CF_INLINE void material_set_uniform_vs(Material material, const char* block_name, const char* name, void* data, UniformType type, int array_length) { cf_material_set_uniform_vs(material, block_name, name, data, type, array_length); }
CF_INLINE void material_set_uniform_fs(Material material, const char* block_name, const char* name, void* data, UniformType type, int array_length) { cf_material_set_uniform_fs(material, block_name, name, data, type, array_length); }
CF_INLINE void material_clear_uniforms(Material material) { cf_material_clear_uniforms(material); }
CF_INLINE void apply_canvas(Canvas canvas, bool clear = true) { cf_apply_canvas(canvas, clear); }
CF_INLINE void apply_viewport(int x, int y, int w, int h) { cf_apply_viewport(x, y, w, h); }
CF_INLINE void apply_scissor(int x, int y, int w, int h) { cf_apply_scissor(x, y, w, h); }
CF_INLINE void apply_mesh(Mesh mesh) { cf_apply_mesh(mesh); }
CF_INLINE void apply_shader(Shader shader, Material material) { cf_apply_shader(shader, material); }
CF_INLINE void draw_elements() { cf_draw_elements(); }
CF_INLINE void unapply_canvas() { cf_unapply_canvas(); }
CF_INLINE void clear_color(float r, float g, float b, float a) { cf_clear_color(r, g, b, a); }
CF_INLINE void clear_color(Color color) { cf_clear_color2(color); }

}

#endif // CF_CPP

#endif // CF_GRAPHICS_H
