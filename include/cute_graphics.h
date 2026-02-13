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
#include "cute_shader_bytecode.h"

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
 * If you want to draw sprites, lines/shapes, or text, see: cute_draw.h
 *
 * Quick list of unsupported features. CF's focus is on the 2D use case, so most of these features are
 * omitted since they aren't super useful for 2D.
 *
 *     - Multiple render targets (aka color/texture attachments)
 *     - Cube map
 *     - 3D textures
 *     - Texture arrays
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

/**
 * @struct   CF_Texture
 * @category graphics
 * @brief    An opaque handle representing a texture.
 * @remarks  A texture is a buffer of data sent to the GPU for random access. Usually textures are used to store image data.
 * @related  CF_Texture CF_Canvas CF_Material CF_Shader CF_TextureParams cf_texture_defaults cf_make_texture cf_destroy_texture cf_texture_update cf_material_set_texture_vs cf_material_set_texture_fs
 */
typedef struct CF_Texture { uint64_t id; } CF_Texture;
// @end

/**
 * @struct   CF_Canvas
 * @category graphics
 * @brief    An opaque handle representing a canvas.
 * @related  CF_Texture CF_Canvas CF_Material CF_Shader CF_CanvasParams cf_canvas_defaults cf_make_canvas cf_destroy_canvas cf_apply_canvas
 */
typedef struct CF_Canvas { uint64_t id; } CF_Canvas;
// @end

/**
 * @struct   CF_Readback
 * @category graphics
 * @brief    An opaque handle representing an async GPU readback operation.
 * @example > Rendering to a canvas with cf_render_to, then reading back pixel data.
 *     // Create a canvas.
 *     CF_Canvas canvas = cf_make_canvas(cf_canvas_defaults(256, 256));
 *
 *     // Queue up draw calls and flush them to the canvas.
 *     cf_draw_sprite(&my_sprite);
 *     cf_render_to(canvas, true);
 *
 *     // Initiate async readback (ends the active render pass automatically).
 *     // Save this `CF_Readback` for later! This is an *async* object.
 *     CF_Readback readback = cf_canvas_readback(canvas);
 *
 *     // ...Elsewhere, and some # of frames later...
 *     // Poll each frame until the GPU finishes the download.
 *     if (cf_readback_ready(readback)) {
 *         int size = cf_readback_size(readback);
 *         void* pixels = cf_alloc(size);
 *         cf_readback_data(readback, pixels, size);
 *         // pixels now holds RGBA8 data (256 * 256 * 4 bytes).
 *         // ... use the pixel data ...
 *         cf_free(pixels);
 *         cf_destroy_readback(readback);
 *     }
 * @remarks  A readback initiates an async GPU-to-CPU copy of pixel data from a canvas.
 *           Poll with `cf_readback_ready` and retrieve data with `cf_readback_data`.
 *           The pixel format matches the canvas target format (typically RGBA8, 4 bytes per pixel).
 *           On web/Emscripten builds, readback is unsupported and returns a zero handle.
 * @related  CF_Canvas cf_canvas_readback cf_readback_ready cf_readback_data cf_readback_size cf_destroy_readback
 */
typedef struct CF_Readback { uint64_t id; } CF_Readback;
// @end

/**
 * @struct   CF_Mesh
 * @category graphics
 * @brief    An opaque handle representing a mesh.
 * @remarks  A mesh is a container of triangles, along with optional indices. After a mesh
 *           is created the layout of the vertices in memory must be described. We use an array of
 *           `CF_VertexAttribute` to define how the GPU will interpret the vertices we send it.
 * @related  CF_Texture CF_Canvas CF_Material CF_Shader cf_make_mesh cf_destroy_mesh cf_mesh_update_vertex_data cf_apply_mesh
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
 * @related  CF_Texture CF_Canvas CF_Material CF_Shader cf_make_material cf_destroy_material cf_material_set_render_state cf_material_set_texture_vs cf_material_set_texture_fs cf_material_set_uniform_vs cf_material_set_uniform_fs cf_apply_shader
 */
typedef struct CF_Material { uint64_t id; } CF_Material;
// @end

/**
 * @struct   CF_Shader
 * @category graphics
 * @brief    An opaque handle representing a shader.
 * @remarks  A shader is a small program that runs on the GPU. They come in the form of vertex and fragment shaders.
 * @related  CF_Texture CF_Canvas CF_Material CF_Shader cf_make_shader cf_destroy_shader cf_apply_shader
 */
typedef struct CF_Shader { uint64_t id; } CF_Shader;
// @end

/**
 * @struct   CF_ComputeShader
 * @category graphics
 * @brief    An opaque handle representing a compute shader.
 * @remarks  A compute shader is a program that runs on the GPU outside the graphics pipeline.
 *           Compute shaders are only available on SDL_GPU backends (not GLES3).
 * @related  CF_ComputeShader cf_make_compute_shader cf_destroy_compute_shader cf_dispatch_compute
 */
typedef struct CF_ComputeShader { uint64_t id; } CF_ComputeShader;
// @end

/**
 * @struct   CF_StorageBuffer
 * @category graphics
 * @brief    An opaque handle representing a GPU storage buffer.
 * @remarks  Storage buffers are GPU-accessible buffers used with compute and graphics shaders.
 *           They are only available on SDL_GPU backends (not GLES3).
 * @related  CF_StorageBuffer CF_StorageBufferParams cf_make_storage_buffer cf_destroy_storage_buffer cf_update_storage_buffer
 */
typedef struct CF_StorageBuffer { uint64_t id; } CF_StorageBuffer;
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
	/* @entry Invalid backend type (uninitialized or failed to create). */           \
	CF_ENUM(BACKEND_TYPE_INVALID, -1)                                              \
	/* @entry Vulkan backend. */                                                   \
	CF_ENUM(BACKEND_TYPE_VULKAN, 0)                                                \
	/* @entry DirectX 11 backend (legacy support). */                              \
	CF_ENUM(BACKEND_TYPE_D3D11,  1)                                                \
	/* @entry DirectX 12 backend. */                                               \
	CF_ENUM(BACKEND_TYPE_D3D12,  2)                                                \
	/* @entry Metal backend. */                                                    \
	CF_ENUM(BACKEND_TYPE_METAL,  3)                                                \
	/* @entry A "secret" backend for platforms under non-disclosure agreement. */  \
	CF_ENUM(BACKEND_TYPE_PRIVATE,  4)                                              \
	/* @entry OpenGL ES 3 backend. */                                              \
	CF_ENUM(BACKEND_TYPE_GLES3,  5)                                                \
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
CF_API CF_BackendType CF_CALL cf_query_backend(void);

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
 * @enum     CF_PixelFormatOp
 * @category graphics
 * @brief    The various supported operations a pixel format can perform.
 * @remarks  Not all types are supported on each backend. Be sure to check with `cf_query_pixel_format` if a particular pixel format
 *           is available for your use-case.
 * @related  CF_PixelFormat cf_pixel_format_op_to_string CF_PixelFormatOp
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
 * @related  CF_PixelFormat cf_pixel_format_op_to_string CF_PixelFormatOp
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
 * @brief    Queries whether a pixel format supports a specific operation on the current backend.
 * @param    format  The `CF_PixelFormat` to query.
 * @param    op      The operation to test, see `CF_PixelFormatOp`.
 * @return   True if the operation is supported for the format, otherwise false.
 * @related  CF_PixelFormat CF_PixelFormatOp cf_texture_supports_format
 */
CF_API bool CF_CALL cf_query_pixel_format(CF_PixelFormat format, CF_PixelFormatOp op);

//--------------------------------------------------------------------------------------------------
// Texture.

/**
 * @enum     CF_TextureUsageFlagBits
 * @category graphics
 * @brief    Bitmask flags that indicate the intended usage of a texture.
 * @remarks  These flags define how a texture will be utilized in graphics and compute pipelines.
 *           Multiple flags can be combined using a bitwise OR operation.
 * @related  CF_TextureUsageFlagBits CF_TextureUsageFlags
 */
#define CF_TEXTURE_USAGE_DEFS \
	/* @entry The texture will be used as a sampler in shaders. */                    \
	CF_ENUM(TEXTURE_USAGE_SAMPLER_BIT,               0x00000001)                      \
	/* @entry The texture will be used as a color render target. */                   \
	CF_ENUM(TEXTURE_USAGE_COLOR_TARGET_BIT,          0x00000002)                      \
	/* @entry The texture will be used as a depth or stencil render target. */        \
	CF_ENUM(TEXTURE_USAGE_DEPTH_STENCIL_TARGET_BIT,  0x00000004)                      \
	/* @entry The texture will be used as read-only storage in graphics pipelines. */ \
	CF_ENUM(TEXTURE_USAGE_GRAPHICS_STORAGE_READ_BIT, 0x00000008)                      \
	/* @entry The texture will be used as read-only storage in compute pipelines. */  \
	CF_ENUM(TEXTURE_USAGE_COMPUTE_STORAGE_READ_BIT,  0x00000010)                      \
	/* @entry The texture will be used as writeable storage in compute pipelines. */  \
	CF_ENUM(TEXTURE_USAGE_COMPUTE_STORAGE_WRITE_BIT, 0x00000020)                      \
	/* @end */

typedef uint32_t CF_TextureUsageFlags;

typedef enum CF_TextureUsageBits
{
	#define CF_ENUM(K, V) CF_##K = V,
	CF_TEXTURE_USAGE_DEFS
	#undef CF_ENUM
} CF_TextureUsageBits;

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
 * @enum     CF_MipFilter
 * @category graphics
 * @brief    Describes how the GPU samples between mipmap levels.
 * @related  CF_MipFilter cf_mip_filter_to_string CF_TextureParams
 */
#define CF_MIP_FILTER_DEFS                                                  \
	/* @entry Samples the nearest mip level without blending. */            \
	CF_ENUM(MIP_FILTER_NEAREST, 0)                                          \
	/* @entry Linearly blends between mip levels for smooth transitions. */ \
	CF_ENUM(MIP_FILTER_LINEAR, 1)                                           \
	/* @end */

typedef enum CF_MipFilter
{
	#define CF_ENUM(K, V) CF_##K = V,
	CF_MIP_FILTER_DEFS
	#undef CF_ENUM
} CF_MipFilter;

/**
 * @function cf_mip_filter_to_string
 * @category graphics
 * @brief    Returns a `CF_MipFilter` converted to a C string.
 * @related  CF_MipFilter cf_mip_filter_to_string CF_TextureParams
 */
CF_INLINE const char* cf_mip_filter_to_string(CF_MipFilter filter) {
	switch (filter) {
	#define CF_ENUM(K, V) case CF_##K: return CF_STRINGIZE(CF_##K);
	CF_MIP_FILTER_DEFS
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
	/* @entry Repeats the image. */                                            \
	CF_ENUM(WRAP_MODE_REPEAT,          0)                                      \
	/* @entry Clamps a UV coordinate to the nearest edge pixel. */             \
	CF_ENUM(WRAP_MODE_CLAMP_TO_EDGE,   1)                                      \
	/* @entry The same as `CF_WRAP_MODE_REPEAT` but mirrors back and forth. */ \
	CF_ENUM(WRAP_MODE_MIRRORED_REPEAT, 2)                                      \
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
 * @related  CF_TextureParams cf_texture_defaults CF_Texture cf_make_texture cf_destroy_texture cf_texture_update
 */
typedef struct CF_TextureParams
{
	/* @member The pixel format for this texture's data. See `CF_PixelFormat`. */
	CF_PixelFormat pixel_format;

	/* @member The memory access pattern for this texture on the GPU. See `CF_TextureUsageBits`. */
	CF_TextureUsageFlags usage;

	/* @member The filtering operation to use when fetching data out of the texture, on the GPU. See `CF_Filter`. */
	CF_Filter filter;

	/* @member The texture wrapping behavior when addressing beyond [0,1) for the u-coordinate. See `CF_WrapMode`. */
	CF_WrapMode wrap_u;

	/* @member The texture wrapping behavior when addressing beyond [0,1) for the v-coordinate. See `CF_WrapMode`. */
	CF_WrapMode wrap_v;

	/* @member The filtering operation to use when fetching data out of a mipmap, on the GPU. See `CF_MipFilter`. */
	CF_MipFilter mip_filter;

	/* @member Number of elements (usually pixels) along the width of the texture. */
	int width;

	/* @member Number of elements (usually pixels) along the height of the texture. */
	int height;

	/* @member 0 = auto compute from dimensions if `allocate_mipmaps` is true, else specify an explicit number. */
	int mip_count;

	/* @member Defaulted to false, true to allocate a full mipmap chain for the texture. */
	bool allocate_mipmaps;

	/* @member Mipmap level bias; positive = blurrier, negative = sharper. */
	float mip_lod_bias;

	/* @member Maximum anisotropy level; 1.0 disables anisotropic filtering. */
	float max_anisotropy;

	/* @member Set this to true if you plan to update the texture contents each frame. */
	bool stream;
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
 * @related  CF_TextureParams CF_Texture cf_make_texture cf_destroy_texture cf_texture_update
 */
CF_API CF_Texture CF_CALL cf_make_texture(CF_TextureParams texture_params);

/**
 * @function cf_destroy_texture
 * @category graphics
 * @brief    Destroys a `CF_Texture` created by `cf_make_texture`.
 * @param    texture   The texture.
 * @related  CF_TextureParams CF_Texture cf_make_texture cf_destroy_texture cf_texture_update
 */
CF_API void CF_CALL cf_destroy_texture(CF_Texture texture);

/**
 * @function cf_texture_update
 * @category graphics
 * @brief    Updates the contents of a `CF_Texture`.
 * @param    texture    The texture.
 * @param    data       The data to upload to the texture.
 * @param    size       The size in bytes of `data`.
 * @remarks  If you plan to frequently update the texture once per frame, it's recommended to set `stream` to
 *           true in the creation params `CF_TextureParams`.
 * @related  CF_TextureParams CF_Texture cf_make_texture cf_destroy_texture cf_texture_update cf_texture_update_mip cf_generate_mipmaps
 */
CF_API void CF_CALL cf_texture_update(CF_Texture texture, void* data, int size);

/**
 * @function cf_texture_update_mip
 * @category graphics
 * @brief    Updates the contents of a specific mip level of a `CF_Texture`.
 * @param    texture    The texture to update.
 * @param    data       Pointer to the raw pixel data to upload.
 * @param    size       Size in bytes of `data`.
 * @param    mip_level  The mipmap level to update (0 = base level).
 * @remarks  If you update the texture frequently (e.g., once per frame), it's recommended to set `stream = true`
 *           when creating the texture using `CF_TextureParams`.
 * @related  CF_TextureParams CF_Texture cf_make_texture cf_destroy_texture cf_texture_update cf_texture_update_mip cf_generate_mipmaps
 */
CF_API void CF_CALL cf_texture_update_mip(CF_Texture texture, void* data, int size, int mip_level);

/**
 * @function cf_generate_mipmaps
 * @category graphics
 * @brief    Generates all remaining mip levels from the base level of the texture.
 * @param    texture    The texture to generate mipmaps for.
 * @remarks  This is useful when the base level has been updated manually (e.g., for dynamic or render target textures)
 *           and you want to downsample to fill in the full mip chain.
 * @related  CF_TextureParams CF_Texture cf_make_texture cf_destroy_texture cf_texture_update cf_texture_update_mip cf_generate_mipmaps
 */
CF_API void CF_CALL cf_generate_mipmaps(CF_Texture texture);

/**
 * @function cf_texture_handle
 * @category graphics
 * @brief    Returns an SDL_GPUTexture* casted to a `uint64_t`.
 * @remarks  This is useful for e.g. rendering textures in an external system like Dear ImGui.
 * @related  CF_TextureParams CF_Texture cf_make_texture
 */
CF_API uint64_t CF_CALL cf_texture_handle(CF_Texture texture);

/**
 * @function cf_texture_binding_handle
 * @category graphics
 * @brief    Returns an SDL_GPUTextureSamplerBinding* casted to a `uint64_t`.
 * @remarks  This is useful for e.g. rendering textures in an external system like Dear ImGui.
 * @related  CF_TextureParams CF_Texture cf_make_texture
 */
CF_API uint64_t CF_CALL cf_texture_binding_handle(CF_Texture texture);

//--------------------------------------------------------------------------------------------------
// Shader.

/**
 * @enum     CF_ShaderStage
 * @category graphics
 * @brief    Distinction between vertex and fragment shaders.
 * @related  CF_Shader cf_shader_directory cf_make_shader
 */
#define CF_SHADER_STAGE_DEFS \
	/* @entry */                      \
	CF_ENUM(SHADER_STAGE_VERTEX,   0) \
	/* @entry */                      \
	CF_ENUM(SHADER_STAGE_FRAGMENT, 1) \
	/* @entry */                      \
	CF_ENUM(SHADER_STAGE_COMPUTE,  2) \
	/* @end */

typedef enum CF_ShaderStage
{
	#define CF_ENUM(K, V) CF_##K = V,
	CF_SHADER_STAGE_DEFS
	#undef CF_ENUM
} CF_ShaderStage;

/**
 * @function cf_shader_directory
 * @category graphics
 * @brief    Sets up the app's shader directory.
 * @param    path     A virtual path to the folder with your shaders (subfolders supported). See [Virtual File System](https://randygaul.github.io/cute_framework/topics/virtual_file_system).
 * @remarks  Shaders can `#include` each other as long as they exist in this directory. Changes to shaders on disk
 *           may also be watched via `cf_shader_on_changed` to support shader reloading during development. If you call `cf_shader_directory` with
 *           the path `"/assets/shaders"`, you should then supply paths to `cf_make_shader` relative to the shader directory, and
 *           simply pass in paths such as `"/shader.vert`" or `"shader.frag"`. This also applies to `#include` between shaders.
 * @related  CF_Shader cf_make_shader cf_destroy_shader cf_apply_shader CF_Material
 */
CF_API void CF_CALL cf_shader_directory(const char* path);

/**
 * @function cf_shader_on_changed
 * @category graphics
 * @brief    Reports when a shader within the shader directory (see `cf_shader_directory`) changes on-disk.
 * @param    on_changed_fn   The reporting callback.
 * @param    udata           An optional `void*` passed back to you whenever `on_changed_fn` is called.
 * @remarks  This is an optional function intended to help facilitate runtime shader reloading during development.
 *           Callbacks are issued when `cf_app_update` is called.
 * @related  CF_Shader cf_make_shader cf_destroy_shader cf_apply_shader CF_Material
 */
CF_API void CF_CALL cf_shader_on_changed(void (*on_changed_fn)(const char* path, void* udata), void* udata);

/**
 * @function cf_make_shader
 * @category graphics
 * @brief    Creates a shader from glsl source code.
 * @param    vertex_path   A virtual path to the shader. See [Virtual File System](https://randygaul.github.io/cute_framework/topics/virtual_file_system).
 * @remarks  The shader paths must be in the shader directory. See `cf_shader_directory`. For example, if you call `cf_shader_directory` with
 *           the path `"/assets/shaders"`, you should then supply paths to `cf_make_shader` relative to the shader directory, and
 *           simply pass in paths such as `"/shader.vert`" or `"shader.frag"`. This also applies to `#include` between shaders.
 *
 *           Note the expected glsl version is 450.
 *
 *           You must setup shader inputs (max of 32 inputs, e.g. `in` keyword) and resources sets in a specific way. Use the
             following resource sets and ordering in your shaders:
 *
 *           For _VERTEX_ shaders:
 *           ```
 *               0: Sampled textures, followed by storage textures, followed by storage buffers
 *               1: Uniform buffers
 *           ```
 *
 *           For _FRAGMENT_ shaders:
 *           ```
 *               2: Sampled textures, followed by storage textures, followed by storage buffers
 *               3: Uniform buffers
 *           ```
 *
 *           Example _VERTEX_ shader:
 *           ```glsl
 *           layout (set = 0, binding = 0) uniform sampler2D u_image;
 *
 *           layout (set = 1, binding = 0) uniform uniform_block {
 *               vec2 u_texture_size;
 *           };
 *           ```
 *
 *           Example _FRAGMENT_ shader:
 *           ```glsl
 *           layout (set = 2, binding = 0) uniform sampler2D u_image;
 *
 *           layout (set = 3, binding = 0) uniform uniform_block {
 *               vec2 u_texture_size;
 *           };
 *           ```
 *
 *           For uniforms you only have one uniform block available, and it *must* be named `uniform_block`. However, if your
 *           shader is make from the draw api (`cf_make_draw_shader`) uniform blocks must be named user_uniforms.
 *
 *           Shaders that sit in the shader directory may be `#include`'d into another shader. Though, it doesn't work
 *           quite exactly like a C/C++ include, it's very similar -- each shader may be included into another
 *           shader *only once*. If you try to include a file multiple times (such as circular dependencies,
 *           or if two files try to include the same file) subsequent includes will be ignored.
 * @related  CF_Shader cf_make_shader cf_shader_directory cf_apply_shader CF_Material
 */
CF_API CF_Shader CF_CALL cf_make_shader(const char* vertex_path, const char* fragment_path);

/**
 * @function cf_make_shader_from_source
 * @category graphics
 * @brief    Creates a shader from strings containing glsl source code.
 * @param    vertex_src    The vertex shader source as C-string.
 * @param    fragment_src  The fragment shader source as C-string.
 * @related  CF_Shader cf_make_shader cf_shader_directory cf_apply_shader CF_Material
 */
CF_API CF_Shader CF_CALL cf_make_shader_from_source(const char* vertex_src, const char* fragment_src);

/**
 * @function cf_compile_shader_to_bytecode
 * @category graphics
 * @brief    Compiles a shader to SPIR-V bytecode.
 * @param    shader_src   Raw glsl, version 450, for the shader as a string.
 * @param    stage        The shader stage to differentiate between vertex or fragment shaders.
 * @remarks  This function is good for precompiling shaders to bytecode, which can help speed up app
 *           startup times. SPIR-V blobs can be saved straight to disk and shipped with your game. Load
 *           the bytecode blob pair (vertex + fragment shader blobs) into a `CF_Shader` via `cf_make_shader_from_bytecode`.
 *           The value returned from this function should be passed to `cf_free_shader_bytecode` when it is no longer needed.
 * @related  CF_Shader CF_ShaderBytecode cf_make_shader_from_bytecode cf_free_shader_bytecode
 */
CF_API CF_ShaderBytecode CF_CALL cf_compile_shader_to_bytecode(const char* shader_src, CF_ShaderStage stage);

/**
 * @function cf_free_shader_bytecode
 * @category graphics
 * @brief    Free a bytecode blob previously returned from `cf_compile_shader_to_bytecode`.
 * @param    bytecode   The bytecode blob to free.
 * @remarks  This function must only be called on the bytecode blob returned from `cf_compile_shader_to_bytecode`.
 *           It cannot be called on the bytecode blob generated as a header from the `cute-shaderc` compiler.
 */
CF_API void CF_CALL cf_free_shader_bytecode(CF_ShaderBytecode bytecode);

/**
 * @function cf_make_shader_from_bytecode
 * @category graphics
 * @brief    Creates a shader from SPIR-V bytecode.
 * @param    vertex_bytecode    A bytecode blob from `cf_compile_shader_to_bytecode` or the cute-shaderc compiler for the vertex shader.
 * @param    fragment_bytecode  A bytecode blob from `cf_compile_shader_to_bytecode` or the cute-shaderc compiler for the fragment shader.
 * @remarks  This function is good for precompiling shaders from bytecode, which can help speed up app
 *           startup times. SPIR-V blobs can be saved straight to disk and shipped with your game. Create the
 *           bytecode blob with `cf_make_shader_from_bytecode`.
 * @related  CF_Shader cf_make_shader_from_bytecode cf_make_shader_from_bytecode
 */
CF_API CF_Shader CF_CALL cf_make_shader_from_bytecode(CF_ShaderBytecode vertex_bytecode, CF_ShaderBytecode fragment_bytecode);

/**
 * @function cf_destroy_shader
 * @category graphics
 * @brief    Frees up a `CF_Shader` created by `cf_make_shader`.
 * @param    shader     A shader.
 * @related  CF_Shader cf_make_shader cf_destroy_shader cf_apply_shader CF_Material
 */
CF_API void CF_CALL cf_destroy_shader(CF_Shader shader);

//--------------------------------------------------------------------------------------------------
// Compute Shaders.
//
// Compute shaders run on the GPU outside the graphics pipeline. They are only available on
// SDL_GPU backends (Vulkan, D3D12, Metal). GLES3 stubs return null handles / no-op.
//
// Uniforms and sampled textures are supplied via a CF_Material's compute stage (cs).
// Storage buffers and storage textures are bound directly via the dispatch struct.

/**
 * @function cf_make_compute_shader
 * @category graphics
 * @brief    Creates a compute shader from a file in the shader directory.
 * @param    path   A virtual path to the compute shader source, relative to the shader directory.
 * @remarks  Compute shaders must use GLSL 450 and follow the SDL_GPU resource set layout convention.
 *           Resources must be assigned to specific descriptor sets depending on their type:
 *
 *           For _COMPUTE_ shaders:
 *           ```
 *               Set 0: Sampled textures, followed by readonly storage textures, followed by readonly storage buffers
 *               Set 1: Read-write storage textures, followed by read-write storage buffers
 *               Set 2: Uniform buffers
 *           ```
 *
 *           Example _COMPUTE_ shader:
 *           ```glsl
 *           layout (set = 0, binding = 0) uniform sampler2D u_input;
 *
 *           layout (set = 1, binding = 0, rgba8) uniform writeonly image2D u_output;
 *
 *           layout (set = 2, binding = 0) uniform uniform_block {
 *               float u_time;
 *           };
 *
 *           layout (local_size_x = 16, local_size_y = 16, local_size_z = 1) in;
 *           ```
 *
 *           Important notes:
 *           - Sampled textures (set 0) are bound via `cf_material_set_texture_cs`.
 *           - Uniforms (set 2) are bound via `cf_material_set_uniform_cs`.
 *           - Read-write storage textures (set 1) are bound via `CF_ComputeDispatch::rw_textures`.
 *           - Readonly storage textures (set 0, after samplers) are bound via `CF_ComputeDispatch::ro_textures`.
 *           - Do NOT use `return` to exit threads before a `barrier()` call. All threads in a workgroup
 *             must reach `barrier()` uniformly, or the pipeline will fail to compile on some backends.
 *             Instead, guard `imageStore` and other side-effects behind a bounds check.
 * @related  CF_ComputeShader cf_make_compute_shader cf_make_compute_shader_from_source cf_make_compute_shader_from_bytecode cf_destroy_compute_shader cf_dispatch_compute
 */
CF_API CF_ComputeShader CF_CALL cf_make_compute_shader(const char* path);

/**
 * @function cf_make_compute_shader_from_source
 * @category graphics
 * @brief    Creates a compute shader from a GLSL 450 source string.
 * @param    src    The compute shader source as a C-string.
 * @remarks  The source must follow the compute shader resource set layout described in `cf_make_compute_shader`.
 * @related  CF_ComputeShader cf_make_compute_shader cf_make_compute_shader_from_source cf_make_compute_shader_from_bytecode cf_destroy_compute_shader cf_dispatch_compute
 */
CF_API CF_ComputeShader CF_CALL cf_make_compute_shader_from_source(const char* src);

/**
 * @function cf_make_compute_shader_from_bytecode
 * @category graphics
 * @brief    Creates a compute shader from SPIR-V bytecode.
 * @param    bytecode   A bytecode blob from `cf_compile_shader_to_bytecode` with `CF_SHADER_STAGE_COMPUTE`.
 * @related  CF_ComputeShader cf_make_compute_shader cf_make_compute_shader_from_source cf_make_compute_shader_from_bytecode cf_destroy_compute_shader cf_dispatch_compute
 */
CF_API CF_ComputeShader CF_CALL cf_make_compute_shader_from_bytecode(CF_ShaderBytecode bytecode);

/**
 * @function cf_destroy_compute_shader
 * @category graphics
 * @brief    Frees up a compute shader.
 * @param    shader   The compute shader to destroy.
 * @related  CF_ComputeShader cf_make_compute_shader cf_destroy_compute_shader
 */
CF_API void CF_CALL cf_destroy_compute_shader(CF_ComputeShader shader);

//--------------------------------------------------------------------------------------------------
// Storage Buffers.

/**
 * @struct   CF_StorageBufferParams
 * @category graphics
 * @brief    Parameters for creating a storage buffer.
 * @related  CF_StorageBuffer cf_storage_buffer_defaults cf_make_storage_buffer
 */
typedef struct CF_StorageBufferParams
{
	/* @member Size in bytes. */
	int size;

	/* @member GPU can read in compute stage (default true). */
	bool compute_readable;

	/* @member GPU can write in compute stage (default false). */
	bool compute_writable;

	/* @member GPU can read in graphics vertex/fragment stage (default false). */
	bool graphics_readable;
} CF_StorageBufferParams;
// @end

/**
 * @function cf_storage_buffer_defaults
 * @category graphics
 * @brief    Returns sensible defaults for `CF_StorageBufferParams`.
 * @param    size   The size in bytes of the storage buffer.
 * @related  CF_StorageBuffer CF_StorageBufferParams cf_make_storage_buffer
 */
CF_INLINE CF_StorageBufferParams cf_storage_buffer_defaults(int size) {
	CF_StorageBufferParams params;
	params.size = size;
	params.compute_readable = true;
	params.compute_writable = false;
	params.graphics_readable = false;
	return params;
}

/**
 * @function cf_make_storage_buffer
 * @category graphics
 * @brief    Creates a GPU storage buffer.
 * @param    params   Parameters for the storage buffer, see `CF_StorageBufferParams`.
 * @related  CF_StorageBuffer CF_StorageBufferParams cf_storage_buffer_defaults cf_destroy_storage_buffer cf_update_storage_buffer
 */
CF_API CF_StorageBuffer CF_CALL cf_make_storage_buffer(CF_StorageBufferParams params);

/**
 * @function cf_update_storage_buffer
 * @category graphics
 * @brief    Uploads CPU data into a storage buffer.
 * @param    buffer   The storage buffer to update.
 * @param    data     Pointer to the data to upload.
 * @param    size     Size in bytes of the data to upload.
 * @related  CF_StorageBuffer cf_make_storage_buffer cf_destroy_storage_buffer
 */
CF_API void CF_CALL cf_update_storage_buffer(CF_StorageBuffer buffer, const void* data, int size);

/**
 * @function cf_destroy_storage_buffer
 * @category graphics
 * @brief    Frees up a storage buffer.
 * @param    buffer   The storage buffer to destroy.
 * @related  CF_StorageBuffer cf_make_storage_buffer
 */
CF_API void CF_CALL cf_destroy_storage_buffer(CF_StorageBuffer buffer);

//--------------------------------------------------------------------------------------------------
// Compute Dispatch.

/**
 * @struct   CF_ComputeDispatch
 * @category graphics
 * @brief    Parameters for dispatching a compute shader.
 * @remarks  Read-write resources are bound at compute pass creation time (SDL_GPU requirement).
 *           Read-only storage resources are bound after the pipeline bind.
 *           Sampled textures and uniforms come from the material (name-matched).
 * @related  CF_ComputeShader CF_ComputeDispatch cf_compute_dispatch_defaults cf_dispatch_compute
 */
typedef struct CF_ComputeDispatch
{
	/* @member Read-write storage buffers (bound at pass creation). */
	CF_StorageBuffer* rw_buffers;
	/* @member Number of read-write storage buffers. */
	int rw_buffer_count;
	/* @member Read-write storage textures (bound at pass creation). */
	CF_Texture* rw_textures;
	/* @member Number of read-write storage textures. */
	int rw_texture_count;

	/* @member Read-only storage buffers (bound after pipeline bind). */
	CF_StorageBuffer* ro_buffers;
	/* @member Number of read-only storage buffers. */
	int ro_buffer_count;
	/* @member Read-only storage textures (bound after pipeline bind). */
	CF_Texture* ro_textures;
	/* @member Number of read-only storage textures. */
	int ro_texture_count;

	/* @member Workgroup count X. */
	int group_count_x;
	/* @member Workgroup count Y. */
	int group_count_y;
	/* @member Workgroup count Z. */
	int group_count_z;
} CF_ComputeDispatch;
// @end

/**
 * @function cf_compute_dispatch_defaults
 * @category graphics
 * @brief    Returns sensible defaults for `CF_ComputeDispatch`.
 * @param    gx   Workgroup count X.
 * @param    gy   Workgroup count Y.
 * @param    gz   Workgroup count Z.
 * @related  CF_ComputeDispatch cf_dispatch_compute
 */
CF_INLINE CF_ComputeDispatch cf_compute_dispatch_defaults(int gx, int gy, int gz) {
	CF_ComputeDispatch d;
	CF_MEMSET(&d, 0, sizeof(d));
	d.group_count_x = gx;
	d.group_count_y = gy;
	d.group_count_z = gz;
	return d;
}

/**
 * @function cf_dispatch_compute
 * @category graphics
 * @brief    Dispatches a compute shader.
 * @param    shader     The compute shader to dispatch.
 * @param    material   Holds uniforms and sampled textures for the compute stage (set via `cf_material_set_uniform_cs` / `cf_material_set_texture_cs`).
 * @param    dispatch   Storage resources and workgroup counts.
 * @related  CF_ComputeShader CF_ComputeDispatch CF_Material cf_material_set_uniform_cs cf_material_set_texture_cs
 */
CF_API void CF_CALL cf_dispatch_compute(CF_ComputeShader shader, CF_Material material, CF_ComputeDispatch dispatch);

//--------------------------------------------------------------------------------------------------
// Render Canvases.

/**
 * @enum     CF_SampleCount
 * @category graphics
 * @brief    Multisample count used for MSAA render targets.
 * @remarks  Turning this on will attempt to use hardware to blur everything you render.
 *           You may not sample from canvas textures with sample counts greater than 1.
 * @related  CF_SampleCount cf_sample_count_string CF_TextureParams
 */
#define CF_SAMPLE_COUNT_DEFS \
	/* @entry No multisampling. */                          \
	CF_ENUM(SAMPLE_COUNT_1, 0)                              \
	/* @entry Multisample anti-aliasing with 2x samples. */ \
	CF_ENUM(SAMPLE_COUNT_2, 1)                              \
	/* @entry Multisample anti-aliasing with 4x samples. */ \
	CF_ENUM(SAMPLE_COUNT_4, 2)                              \
	/* @entry Multisample anti-aliasing with 8x samples. */ \
	CF_ENUM(SAMPLE_COUNT_8, 3)                              \
	/* @end */

typedef enum CF_SampleCount
{
	#define CF_ENUM(K, V) CF_##K = V,
	CF_SAMPLE_COUNT_DEFS
	#undef CF_ENUM
} CF_SampleCount;

/**
 * @function cf_samplecount_string
 * @category graphics
 * @brief    Returns a `CF_SampleCount` as a string.
 * @related  CF_SampleCount CF_TextureParams
 */
CF_INLINE const char* cf_samplecount_string(CF_SampleCount count) {
	switch (count) {
	#define CF_ENUM(K, V) case CF_##K: return CF_STRINGIZE(CF_##K);
	CF_SAMPLE_COUNT_DEFS
	#undef CF_ENUM
	default: return NULL;
	}
}

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

	/* @member Defaults to false. If true enables a depth-stencil buffer attachment. */
	bool depth_stencil_enable;

	/* @member The texture used to store depth and stencil information when rendering to the canvas. See `CF_TextureParams`. */
	CF_TextureParams depth_stencil_target;

	/* @member MSAA sample count; must be 1, 2, 4, or 8 (see `CF_SampleCount`). Defaults to 1 (no MSAA). */
	CF_SampleCount sample_count;
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
 * @remarks  If you turn on MSAA you may not sample from this texture.
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
 * @function cf_clear_canvas
 * @category graphics
 * @brief    Clears the color and depth-stencil targets of the given canvas.
 * @param    canvas  The canvas to clear.
 * @remarks  This clears the canvas to its configured clear color and clears depth to 1.0 (furthest depth).
 *           If the canvas has no depth-stencil target, only the color target will be cleared.
 * @related  cf_make_canvas cf_clear_color cf_clear_depth_stencil
 */
CF_API void CF_CALL cf_clear_canvas(CF_Canvas canvas);

//--------------------------------------------------------------------------------------------------
// Readback.

/**
 * @function cf_canvas_readback
 * @category graphics
 * @brief    Initiates an async GPU-to-CPU copy of pixel data from a canvas.
 * @param    canvas  The canvas to read back pixel data from.
 * @return   Returns a `CF_Readback` handle. Returns a zero handle on failure or if unsupported (e.g. web/Emscripten).
 * @remarks  Ends any active render pass silently. Each readback uses its own command buffer and fence.
 *           For screen readback, use `cf_canvas_readback(cf_app_get_canvas())`.
 * @related  CF_Readback cf_readback_ready cf_readback_data cf_readback_size cf_destroy_readback
 */
CF_API CF_Readback CF_CALL cf_canvas_readback(CF_Canvas canvas);

/**
 * @function cf_readback_ready
 * @category graphics
 * @brief    Polls whether the async readback has completed.
 * @param    readback  The readback handle to poll.
 * @return   Returns true once the GPU has finished the download. Caches the result after first true.
 * @related  CF_Readback cf_canvas_readback cf_readback_data cf_readback_size cf_destroy_readback
 */
CF_API bool CF_CALL cf_readback_ready(CF_Readback readback);

/**
 * @function cf_readback_data
 * @category graphics
 * @brief    Copies readback pixel data into the provided buffer.
 * @param    readback  The readback handle.
 * @param    data      Pointer to the destination buffer.
 * @param    size      Size of the destination buffer in bytes.
 * @return   Returns the number of bytes copied, or 0 if the readback is not yet ready. Copies `min(size, total)` bytes.
 * @remarks  Pixel format matches the canvas target format (typically RGBA8, 4 bytes per pixel).
 * @related  CF_Readback cf_canvas_readback cf_readback_ready cf_readback_size cf_destroy_readback
 */
CF_API int CF_CALL cf_readback_data(CF_Readback readback, void* data, int size);

/**
 * @function cf_readback_size
 * @category graphics
 * @brief    Returns the total size in bytes of the readback data.
 * @param    readback  The readback handle.
 * @return   Returns `w * h * bytes_per_pixel` for the readback, or 0 if the handle is invalid.
 * @related  CF_Readback cf_canvas_readback cf_readback_ready cf_readback_data cf_destroy_readback
 */
CF_API int CF_CALL cf_readback_size(CF_Readback readback);

/**
 * @function cf_destroy_readback
 * @category graphics
 * @brief    Destroys a readback handle and releases all associated resources.
 * @param    readback  The readback handle to destroy.
 * @remarks  If the readback is still in-flight, this will block until the GPU completes.
 * @related  CF_Readback cf_canvas_readback cf_readback_ready cf_readback_data cf_readback_size
 */
CF_API void CF_CALL cf_destroy_readback(CF_Readback readback);

//--------------------------------------------------------------------------------------------------
// Mesh.

/**
 * @enum     CF_VertexFormat
 * @category graphics
 * @brief    The various supported vertex formats.
 * @remarks  Vertex formats define the type and size of vertex attributes in a vertex buffer.
 * @related  CF_VertexFormat cf_vertex_format_to_string CF_VertexFormatOp cf_query_vertex_format
 */
#define CF_VERTEX_FORMAT_DEFS \
	/* @entry 32-bit signed integer. */                     \
	CF_ENUM(VERTEX_FORMAT_INT,              0)              \
	/* @entry Two 32-bit signed integers. */                \
	CF_ENUM(VERTEX_FORMAT_INT2,             1)              \
	/* @entry Three 32-bit signed integers. */              \
	CF_ENUM(VERTEX_FORMAT_INT3,             2)              \
	/* @entry Four 32-bit signed integers. */               \
	CF_ENUM(VERTEX_FORMAT_INT4,             3)              \
	/* @entry 32-bit unsigned integer. */                   \
	CF_ENUM(VERTEX_FORMAT_UINT,             4)              \
	/* @entry Two 32-bit unsigned integers. */              \
	CF_ENUM(VERTEX_FORMAT_UINT2,            5)              \
	/* @entry Three 32-bit unsigned integers. */            \
	CF_ENUM(VERTEX_FORMAT_UINT3,            6)              \
	/* @entry Four 32-bit unsigned integers. */             \
	CF_ENUM(VERTEX_FORMAT_UINT4,            7)              \
	/* @entry 32-bit floating point number. */              \
	CF_ENUM(VERTEX_FORMAT_FLOAT,            8)              \
	/* @entry Two 32-bit floating point numbers. */         \
	CF_ENUM(VERTEX_FORMAT_FLOAT2,           9)              \
	/* @entry Three 32-bit floating point numbers. */       \
	CF_ENUM(VERTEX_FORMAT_FLOAT3,           10)             \
	/* @entry Four 32-bit floating point numbers. */        \
	CF_ENUM(VERTEX_FORMAT_FLOAT4,           11)             \
	/* @entry Two 8-bit signed integers. */                 \
	CF_ENUM(VERTEX_FORMAT_BYTE2,            12)             \
	/* @entry Four 8-bit signed integers. */                \
	CF_ENUM(VERTEX_FORMAT_BYTE4,            13)             \
	/* @entry Two 8-bit unsigned integers. */               \
	CF_ENUM(VERTEX_FORMAT_UBYTE2,           14)             \
	/* @entry Four 8-bit unsigned integers. */              \
	CF_ENUM(VERTEX_FORMAT_UBYTE4,           15)             \
	/* @entry Two 8-bit signed normalized integers. */      \
	CF_ENUM(VERTEX_FORMAT_BYTE2_NORM,       16)             \
	/* @entry Four 8-bit signed normalized integers. */     \
	CF_ENUM(VERTEX_FORMAT_BYTE4_NORM,       17)             \
	/* @entry Two 8-bit unsigned normalized integers. */    \
	CF_ENUM(VERTEX_FORMAT_UBYTE2_NORM,      18)             \
	/* @entry Four 8-bit unsigned normalized integers. */   \
	CF_ENUM(VERTEX_FORMAT_UBYTE4_NORM,      19)             \
	/* @entry Two 16-bit signed integers. */                \
	CF_ENUM(VERTEX_FORMAT_SHORT2,           20)             \
	/* @entry Four 16-bit signed integers. */               \
	CF_ENUM(VERTEX_FORMAT_SHORT4,           21)             \
	/* @entry Two 16-bit unsigned integers. */              \
	CF_ENUM(VERTEX_FORMAT_USHORT2,          22)             \
	/* @entry Four 16-bit unsigned integers. */             \
	CF_ENUM(VERTEX_FORMAT_USHORT4,          23)             \
	/* @entry Two 16-bit signed normalized integers. */     \
	CF_ENUM(VERTEX_FORMAT_SHORT2_NORM,      24)             \
	/* @entry Four 16-bit signed normalized integers. */    \
	CF_ENUM(VERTEX_FORMAT_SHORT4_NORM,      25)             \
	/* @entry Two 16-bit unsigned normalized integers. */   \
	CF_ENUM(VERTEX_FORMAT_USHORT2_NORM,     26)             \
	/* @entry Four 16-bit unsigned normalized integers. */  \
	CF_ENUM(VERTEX_FORMAT_USHORT4_NORM,     27)             \
	/* @entry Two 16-bit floating point numbers. */         \
	CF_ENUM(VERTEX_FORMAT_HALF2,            28)             \
	/* @entry Four 16-bit floating point numbers. */        \
	CF_ENUM(VERTEX_FORMAT_HALF4,            29)             \
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

	/* @member Set to true if you want this attribute bound to the mesh's instance buffer, instead of the vertex buffer. */
	bool per_instance;
} CF_VertexAttribute;
// @end

// Max number of vertex attributes allowed on a mesh.
#define CF_MESH_MAX_VERTEX_ATTRIBUTES (32)

/**
 * @function cf_make_mesh
 * @category graphics
 * @brief    Returns a `CF_Mesh`.
 * @param    vertex_buffer_size_in_bytes  The size of the mesh's vertex buffer.
 * @param    attributes                   Vertex attributes to define the memory layout of the mesh vertices.
 * @param    attribute_count              Number of attributes in `attributes`.
 * @param    vertex_stride                Number of bytes between each vertex.
 * @remarks  The max number of attributes is `CF_MESH_MAX_VERTEX_ATTRIBUTES` (32). Any more attributes beyond 32 will be ignored.
 * @related  CF_Mesh cf_make_mesh cf_destroy_mesh cf_mesh_update_vertex_data cf_mesh_set_index_buffer cf_mesh_set_instance_buffer
 */
CF_API CF_Mesh CF_CALL cf_make_mesh(int vertex_buffer_size_in_bytes, const CF_VertexAttribute* attributes, int attribute_count, int vertex_stride);

/**
 * @function cf_mesh_set_index_buffer
 * @category graphics
 * @brief    Sets up an index buffer on the mesh, for indexed style rendering.
 * @param    mesh                         The mesh.
 * @param    index_buffer_size_in_bytes   The size of the mesh's index buffer.
 * @param    index_bit_count              The number of bits to use for indices, must be either 16 or 32.
 * @related  CF_Mesh cf_make_mesh cf_mesh_update_index_data
 */
CF_API void CF_CALL cf_mesh_set_index_buffer(CF_Mesh mesh, int index_buffer_size_in_bytes, int index_bit_count);

/**
 * @function cf_mesh_set_instance_buffer
 * @category graphics
 * @brief    Sets up an instance buffer on the mesh, for instanced style rendering.
 * @param    mesh                         The mesh.
 * @param    instance_buffer_size_in_bytes  The size of the mesh's index buffer.
 * @param    instance_stride                The number of bytes for each instance data.
 * @related  CF_Mesh cf_make_mesh cf_mesh_update_instance_data
 */
CF_API void CF_CALL cf_mesh_set_instance_buffer(CF_Mesh mesh, int instance_buffer_size_in_bytes, int instance_stride);

/**
 * @function cf_destroy_mesh
 * @category graphics
 * @brief    Frees up a `CF_Mesh` previously created with `cf_make_mesh`.
 * @param    mesh       The mesh.
 * @related  CF_Mesh cf_make_mesh cf_destroy_mesh cf_mesh_update_vertex_data
 */
CF_API void CF_CALL cf_destroy_mesh(CF_Mesh mesh);

/**
 * @function cf_mesh_update_vertex_data
 * @category graphics
 * @brief    Overwrites the vertex data of a mesh.
 * @param    mesh       The mesh.
 * @param    data       A pointer to vertex data.
 * @param    count      Number of vertices in `data`.
 * @related  CF_Mesh cf_make_mesh cf_destroy_mesh cf_mesh_update_vertex_data
 */
CF_API void CF_CALL cf_mesh_update_vertex_data(CF_Mesh mesh, void* data, int count);

/**
 * @function cf_mesh_update_index_data
 * @category graphics
 * @brief    Overwrites the index data of a mesh.
 * @param    mesh       The mesh.
 * @param    data       A pointer to index data.
 * @param    count      Number of indices in `data`.
 * @related  CF_Mesh cf_make_mesh cf_destroy_mesh cf_mesh_set_index_buffer
 */
CF_API void CF_CALL cf_mesh_update_index_data(CF_Mesh mesh, void* data, int count);

/**
 * @function cf_mesh_update_instance_data
 * @category graphics
 * @brief    Overwrites the instance data of a mesh.
 * @param    mesh       The mesh.
 * @param    data       A pointer to instance data.
 * @param    count      Number of instances in `data`.
 * @related  CF_Mesh cf_make_mesh cf_destroy_mesh cf_mesh_set_instance_buffer
 */
CF_API void CF_CALL cf_mesh_update_instance_data(CF_Mesh mesh, void* data, int count);

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
	/* @entry Minimum value. */           \
	CF_ENUM(BLEND_OP_MIN,              3) \
	/* @entry Maximum value. */           \
	CF_ENUM(BLEND_OP_MAX,              4) \
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
#define CF_BLENDFACTOR_DEFS \
	/* @entry 0 */                                    \
	CF_ENUM(BLENDFACTOR_ZERO,                      0) \
	/* @entry 1 */                                    \
	CF_ENUM(BLENDFACTOR_ONE,                       1) \
	/* @entry S.color */                              \
	CF_ENUM(BLENDFACTOR_SRC_COLOR,                 2) \
	/* @entry (1 - S.rgb) */                          \
	CF_ENUM(BLENDFACTOR_ONE_MINUS_SRC_COLOR,       3) \
	/* @entry D.rgb */                                \
	CF_ENUM(BLENDFACTOR_DST_COLOR,                 4) \
	/* @entry (1 - D.rgb) */                          \
	CF_ENUM(BLENDFACTOR_ONE_MINUS_DST_COLOR,       5) \
	/* @entry S.alpha */                              \
	CF_ENUM(BLENDFACTOR_SRC_ALPHA,                 6) \
	/* @entry (1 - S.alpha) */                        \
	CF_ENUM(BLENDFACTOR_ONE_MINUS_SRC_ALPHA,       7) \
	/* @entry D.alpha */                              \
	CF_ENUM(BLENDFACTOR_DST_ALPHA,                 8) \
	/* @entry (1 - D.alpha) */                        \
	CF_ENUM(BLENDFACTOR_ONE_MINUS_DST_ALPHA,       9) \
	/* @entry C */                                    \
	CF_ENUM(BLENDFACTOR_CONSTANT_COLOR,           10) \
	/* @entry (1 - C.rgb) */                          \
	CF_ENUM(BLENDFACTOR_ONE_MINUS_CONSTANT_COLOR, 11) \
	/* @entry min(S.alpha, 1 - D.alpha) */            \
	CF_ENUM(BLENDFACTOR_SRC_ALPHA_SATURATE,       12) \
	/* @end */

typedef enum CF_BlendFactor
{
	#define CF_ENUM(K, V) CF_##K = V,
	CF_BLENDFACTOR_DEFS
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
	CF_BLENDFACTOR_DEFS
	#undef CF_ENUM
	default: return NULL;
	}
}
/**
 * @enum     CF_PrimitiveType
 * @category graphics
 * @brief    Primitive topology used for rendering geometry.
 * @remarks  Controls how vertex input is interpreted as geometry.
 * @related  CF_PrimitiveType cf_primitive_type_string
 */
#define CF_PRIMITIVE_TYPE_DEFS \
	/* @entry A series of separate triangles. */       \
	CF_ENUM(PRIMITIVE_TYPE_TRIANGLELIST,    0)         \
	/* @entry A series of connected triangles. */      \
	CF_ENUM(PRIMITIVE_TYPE_TRIANGLESTRIP,   1)         \
	/* @entry A series of separate lines. */           \
	CF_ENUM(PRIMITIVE_TYPE_LINELIST,        2)         \
	/* @entry A series of connected lines. */          \
	CF_ENUM(PRIMITIVE_TYPE_LINESTRIP,       3)         \
	/* @end */

typedef enum CF_PrimitiveType
{
	#define CF_ENUM(K, V) CF_##K = V,
	CF_PRIMITIVE_TYPE_DEFS
	#undef CF_ENUM
} CF_PrimitiveType;

/**
 * @function cf_primitive_type_string
 * @category graphics
 * @brief    Returns a `CF_PrimitiveType` converted to a C string.
 * @related  CF_PrimitiveType
 */
CF_INLINE const char* cf_primitive_type_string(CF_PrimitiveType type) {
	switch (type) {
	#define CF_ENUM(K, V) case CF_##K: return CF_STRINGIZE(CF_##K);
	CF_PRIMITIVE_TYPE_DEFS
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
 *           the old pixel value, while S (source factor) is a new image getting draw over old pixel contents. Therefore, P is the final color
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
 *           CF_RenderState state = cf_render_state_defaults();
 *           state.blend.enabled = true;
 *           state.blend.rgb_src_blend_factor = CF_BLENDFACTOR_ONE;
 *           state.blend.rgb_dst_blend_factor = CF_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
 *           state.blend.rgb_op = CF_BLEND_OP_ADD;
 *           state.blend.alpha_src_blend_factor = CF_BLENDFACTOR_ONE;
 *           state.blend.alpha_dst_blend_factor = CF_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
 *           state.blend.alpha_op = CF_BLEND_OP_ADD;
 *           cf_material_set_render_state(my_material, state);
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
	/* @member The type of primitive to draw, as in triangles or lines (triangle list by default). See `CF_PrimitiveType`. */
	CF_PrimitiveType primitive_type;

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

	/* @member A scalar factor controlling the depth value added to each fragment. */
	float depth_bias_constant_factor;

	/* @member The maximum depth bias of a fragment. */
	float depth_bias_clamp;

	/* @member A scalar factor applied to a fragment's slope in depth calculations. */
	float depth_bias_slope_factor;

	/* @member True to bias fragment depth values. */
	bool enable_depth_bias;

	/* @member True to enable depth clip, false to enable depth clamp. */
	bool enable_depth_clip;
} CF_RenderState;
// @end

/**
 * @function cf_render_state_defaults
 * @category graphics
 * @brief    Returns a good set of default parameters for a `CF_RenderState`.
 * @related  CF_RenderState cf_render_state_defaults cf_material_set_render_state
 */
CF_API CF_RenderState CF_CALL cf_render_state_defaults(void);

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
	CF_ENUM(UNIFORM_TYPE_UNKNOWN, -1)          \
	/* @entry In a shader: `uniform float` */  \
	CF_ENUM(UNIFORM_TYPE_FLOAT,    0)          \
	/* @entry In a shader: `uniform vec2` */   \
	CF_ENUM(UNIFORM_TYPE_FLOAT2,   1)          \
	/* @entry In a shader: `uniform vec3` */   \
	CF_ENUM(UNIFORM_TYPE_FLOAT3,   2)          \
	/* @entry In a shader: `uniform vec4` */   \
	CF_ENUM(UNIFORM_TYPE_FLOAT4,   3)          \
	/* @entry In a shader: `uniform int` */    \
	CF_ENUM(UNIFORM_TYPE_INT,      4)          \
	/* @entry In a shader: `uniform int[2]` */ \
	CF_ENUM(UNIFORM_TYPE_INT2,     5)          \
	/* @entry In a shader: `uniform int[4]` */ \
	CF_ENUM(UNIFORM_TYPE_INT4,     6)          \
	/* @entry In a shader: `uniform mat4` */   \
	CF_ENUM(UNIFORM_TYPE_MAT4,     7)          \
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
CF_API CF_Material CF_CALL cf_make_material(void);

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
CF_API void CF_CALL cf_material_set_uniform_vs(CF_Material material, const char* name, void* data, CF_UniformType type, int array_length);

/**
 * @function cf_material_set_uniform_fs
 * @category graphics
 * @brief    Sets up a uniform value, used for inputs to fragment shaders.
 * @param    material      The material.
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
CF_API void CF_CALL cf_material_set_uniform_fs(CF_Material material, const char* name, void* data, CF_UniformType type, int array_length);

/**
 * @function cf_material_set_texture_cs
 * @category graphics
 * @brief    Sets up a sampled texture for the compute shader stage.
 * @param    material      The material.
 * @param    name          The name of the texture as it appears in the compute shader.
 * @param    texture       Data (usually an image) for a shader to access.
 * @related  CF_Material cf_material_set_uniform_cs cf_dispatch_compute
 */
CF_API void CF_CALL cf_material_set_texture_cs(CF_Material material, const char* name, CF_Texture texture);

/**
 * @function cf_material_set_uniform_cs
 * @category graphics
 * @brief    Sets up a uniform value for the compute shader stage.
 * @param    material      The material.
 * @param    name          The name of the uniform as it appears in the compute shader.
 * @param    data          The value of the uniform.
 * @param    type          The type of the uniform. See `CF_UniformType`.
 * @param    array_length  The number of elements in the uniform array.
 * @remarks  Same name-matching behavior as `cf_material_set_uniform_vs`/`cf_material_set_uniform_fs`.
 * @related  CF_Material cf_material_set_texture_cs cf_dispatch_compute
 */
CF_API void CF_CALL cf_material_set_uniform_cs(CF_Material material, const char* name, void* data, CF_UniformType type, int array_length);

/**
 * @function cf_material_clear_uniforms
 * @category graphics
 * @brief    Clears any uniforms previously set by `cf_material_set_uniform_vs`, `cf_material_set_uniform_fs`, or `cf_material_set_uniform_cs`.
 * @param    material      The material.
 * @related  CF_UniformType CF_Material cf_make_material cf_destroy_material cf_material_set_render_state cf_material_set_texture_vs cf_material_set_texture_fs cf_material_set_uniform_vs cf_material_set_uniform_fs cf_material_set_uniform_cs
 */
CF_API void CF_CALL cf_material_clear_uniforms(CF_Material material);

//--------------------------------------------------------------------------------------------------
// Rendering Functions.

/**
 * @function cf_clear_color
 * @category graphics
 * @brief    Sets the color the app will use to clear the screen/canvases.
 * @related  cf_clear_screen cf_clear_depth_stencil
 */
CF_API void CF_CALL cf_clear_color(float red, float green, float blue, float alpha);

/**
 * @function cf_clear_depth_stencil
 * @category graphics
 * @brief    Sets the depth/stencil values used when clearing a canvas, if depth/stencil are enabled (see `CF_RenderState`).
 * @remarks  This will get used when `cf_apply_canvas` or when `cf_app_draw_onto_screen` is called and `clear` parameter is true.
 * @related  cf_clear_screen cf_clear_depth_stencil
 */
CF_API void CF_CALL cf_clear_depth_stencil(float depth, uint32_t stencil);

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
 * @param    w          Width of the viewport in pixels.
 * @param    h          Height of the viewport in pixels.
 * @remarks  The viewport is a window on the screen to render within. The canvas will be stretched to fit onto the viewport. You must only call this
 *           after calling `cf_apply_shader`.
 * @related  cf_apply_canvas cf_apply_viewport cf_apply_scissor
 */
CF_API void CF_CALL cf_apply_viewport(int x, int y, int w, int h);

/**
 * @function cf_apply_scissor
 * @category graphics
 * @brief    Sets up a scissor box to clip rendering within.
 * @param    x          Center of the scissor box on the x-axis.
 * @param    y          Center of the scissor box on the y-axis.
 * @param    w          Width of the scissor box in pixels.
 * @param    h          Height of the scissor box in pixels.
 * @remarks  The scissor box is a window on the screen that rendering will be clipped within. Any rendering that occurs outside the
 *           scissor box will simply be ignored, rendering nothing and leaving the previous pixel contents untouched. You must only call this
 *           after calling `cf_apply_shader`.
 * @related  cf_apply_canvas cf_apply_viewport cf_apply_scissor
 */
CF_API void CF_CALL cf_apply_scissor(int x, int y, int w, int h);

/**
 * @function cf_apply_stencil_reference
 * @category graphics
 * @brief    Sets the stencil reference value.
 * @param    reference      The value to set the stencil reference to.
 */
CF_API void CF_CALL cf_apply_stencil_reference(int reference);

/**
 * @function cf_apply_blend_constants
 * @category graphics
 * @brief    Sets the stencil reference value.
 * @param    r      The red blend constant.
 * @param    g      The green blend constant.
 * @param    b      The blue blend constant.
 * @param    a      The alpha blend constant.
 */
CF_API void CF_CALL cf_apply_blend_constants(float r, float g, float b, float a);

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
 * @related  CF_Mesh cf_create_mesh cf_apply_shader cf_apply_canvas
 */
CF_API void CF_CALL cf_draw_elements(void);

#ifdef __cplusplus
}
#endif // __cplusplus

//--------------------------------------------------------------------------------------------------
// C++ API

#ifdef CF_CPP

namespace Cute
{

CF_INLINE void clear_color(float r, float b, float g, float a) { cf_clear_color(r, g, b, a); }
CF_INLINE void clear_color(CF_Color color) { cf_clear_color(color.r, color.g, color.b, color.a); }
CF_INLINE CF_BackendType query_backend() { return cf_query_backend(); }
CF_INLINE CF_TextureParams texture_defaults(int w, int h) { return cf_texture_defaults(w, h); }
CF_INLINE CF_Texture make_texture(CF_TextureParams texture_params) { return cf_make_texture(texture_params); }
CF_INLINE void destroy_texture(CF_Texture texture) { cf_destroy_texture(texture); }
CF_INLINE void texture_update(CF_Texture texture, void* data, int size) { cf_texture_update(texture, data, size); }
CF_INLINE CF_Shader make_shader(const char* vertex, const char* fragment) { return cf_make_shader(vertex, fragment); }
CF_INLINE void shader_directory(const char* path) { cf_shader_directory(path); }
CF_INLINE void shader_on_changed(void (*on_changed_fn)(const char* path, void* udata), void* udata) { cf_shader_on_changed(on_changed_fn, udata); }
CF_INLINE CF_Shader make_shader_from_source(const char* vertex_src, const char* fragment_src) { return cf_make_shader_from_source(vertex_src, fragment_src); }
CF_INLINE CF_ShaderBytecode compile_shader_to_bytecode(const char* shader_src, CF_ShaderStage stage) { return cf_compile_shader_to_bytecode(shader_src, stage); }
CF_INLINE void free_shader_bytecode(CF_ShaderBytecode bytecode) { return cf_free_shader_bytecode(bytecode); }
CF_INLINE CF_Shader make_shader_from_bytecode(CF_ShaderBytecode vertex_bytecode, CF_ShaderBytecode fragment_bytecode) { return cf_make_shader_from_bytecode(vertex_bytecode, fragment_bytecode); }
CF_INLINE void destroy_shader(CF_Shader shader) { cf_destroy_shader(shader); }
CF_INLINE CF_CanvasParams canvas_defaults(int w, int h) { return cf_canvas_defaults(w, h); }
CF_INLINE CF_Canvas make_canvas(CF_CanvasParams pass_params) { return cf_make_canvas(pass_params); }
CF_INLINE void destroy_canvas(CF_Canvas canvas) { cf_destroy_canvas(canvas); }
CF_INLINE CF_Texture canvas_get_target(CF_Canvas canvas) { return cf_canvas_get_target(canvas); }
CF_INLINE CF_Texture canvas_get_depth_stencil_target(CF_Canvas canvas) { return cf_canvas_get_depth_stencil_target(canvas); }
CF_INLINE void clear_canvas(CF_Canvas canvas) { cf_clear_canvas(canvas); }
CF_INLINE CF_Readback canvas_readback(CF_Canvas canvas) { return cf_canvas_readback(canvas); }
CF_INLINE bool readback_ready(CF_Readback readback) { return cf_readback_ready(readback); }
CF_INLINE int readback_data(CF_Readback readback, void* data, int size) { return cf_readback_data(readback, data, size); }
CF_INLINE int readback_size(CF_Readback readback) { return cf_readback_size(readback); }
CF_INLINE void destroy_readback(CF_Readback readback) { cf_destroy_readback(readback); }
CF_INLINE CF_Mesh make_mesh(int vertex_buffer_size_in_bytes, const CF_VertexAttribute* attributes, int attribute_count, int vertex_stride) { return cf_make_mesh(vertex_buffer_size_in_bytes, attributes, attribute_count, vertex_stride); }
CF_INLINE void destroy_mesh(CF_Mesh mesh) { cf_destroy_mesh(mesh); }
CF_INLINE void mesh_update_vertex_data(CF_Mesh mesh, void* data, int count) { cf_mesh_update_vertex_data(mesh, data, count); }
CF_INLINE void mesh_update_index_data(CF_Mesh mesh, void* data, int count) { cf_mesh_update_index_data(mesh, data, count); }
CF_INLINE void mesh_update_instance_data(CF_Mesh mesh, void* data, int count) { cf_mesh_update_instance_data(mesh, data, count); }
CF_INLINE CF_RenderState render_state_defaults() { return cf_render_state_defaults(); }
CF_INLINE CF_Material make_material() { return cf_make_material(); }
CF_INLINE void destroy_material(CF_Material material) { cf_destroy_material(material); }
CF_INLINE void material_set_render_state(CF_Material material, CF_RenderState render_state) { cf_material_set_render_state(material, render_state); }
CF_INLINE void material_set_texture_vs(CF_Material material, const char* name, CF_Texture texture) { cf_material_set_texture_vs(material, name, texture); }
CF_INLINE void material_set_texture_fs(CF_Material material, const char* name, CF_Texture texture) { cf_material_set_texture_fs(material, name, texture); }
CF_INLINE void material_clear_textures(CF_Material material) { cf_material_clear_textures(material); }
CF_INLINE void material_set_uniform_vs(CF_Material material, const char* name, void* data, CF_UniformType type, int array_length) { cf_material_set_uniform_vs(material, name, data, type, array_length); }
CF_INLINE void material_set_uniform_fs(CF_Material material, const char* name, void* data, CF_UniformType type, int array_length) { cf_material_set_uniform_fs(material, name, data, type, array_length); }
CF_INLINE void material_set_texture_cs(CF_Material material, const char* name, CF_Texture texture) { cf_material_set_texture_cs(material, name, texture); }
CF_INLINE void material_set_uniform_cs(CF_Material material, const char* name, void* data, CF_UniformType type, int array_length) { cf_material_set_uniform_cs(material, name, data, type, array_length); }
CF_INLINE void material_clear_uniforms(CF_Material material) { cf_material_clear_uniforms(material); }
CF_INLINE void apply_canvas(CF_Canvas canvas, bool clear = false) { cf_apply_canvas(canvas, clear); }
CF_INLINE void apply_viewport(int x, int y, int w, int h) { cf_apply_viewport(x, y, w, h); }
CF_INLINE void apply_scissor(int x, int y, int w, int h) { cf_apply_scissor(x, y, w, h); }
CF_INLINE void apply_mesh(CF_Mesh mesh) { cf_apply_mesh(mesh); }
CF_INLINE void apply_shader(CF_Shader shader, CF_Material material) { cf_apply_shader(shader, material); }
CF_INLINE void draw_elements() { cf_draw_elements(); }
CF_INLINE bool query_pixel_format(CF_PixelFormat format, CF_PixelFormatOp op) { return cf_query_pixel_format(format, op); }
CF_INLINE void texture_update_mip(CF_Texture texture, void* data, int size, int mip_level) { cf_texture_update_mip(texture, data, size, mip_level); }
CF_INLINE void generate_mipmaps(CF_Texture texture) { cf_generate_mipmaps(texture); }
CF_INLINE uint64_t texture_handle(CF_Texture texture) { return cf_texture_handle(texture); }
CF_INLINE uint64_t texture_binding_handle(CF_Texture texture) { return cf_texture_binding_handle(texture); }
CF_INLINE void mesh_set_index_buffer(CF_Mesh mesh, int index_buffer_size_in_bytes, int index_bit_count) { cf_mesh_set_index_buffer(mesh, index_buffer_size_in_bytes, index_bit_count); }
CF_INLINE void mesh_set_instance_buffer(CF_Mesh mesh, int instance_buffer_size_in_bytes, int instance_stride) { cf_mesh_set_instance_buffer(mesh, instance_buffer_size_in_bytes, instance_stride); }
CF_INLINE void clear_depth_stencil(float depth, uint32_t stencil) { cf_clear_depth_stencil(depth, stencil); }
CF_INLINE void apply_stencil_reference(int reference) { cf_apply_stencil_reference(reference); }
CF_INLINE void apply_blend_constants(float r, float g, float b, float a) { cf_apply_blend_constants(r, g, b, a); }
CF_INLINE CF_ComputeShader make_compute_shader(const char* path) { return cf_make_compute_shader(path); }
CF_INLINE CF_ComputeShader make_compute_shader_from_source(const char* src) { return cf_make_compute_shader_from_source(src); }
CF_INLINE CF_ComputeShader make_compute_shader_from_bytecode(CF_ShaderBytecode bytecode) { return cf_make_compute_shader_from_bytecode(bytecode); }
CF_INLINE void destroy_compute_shader(CF_ComputeShader shader) { cf_destroy_compute_shader(shader); }
CF_INLINE CF_StorageBufferParams storage_buffer_defaults(int size) { return cf_storage_buffer_defaults(size); }
CF_INLINE CF_StorageBuffer make_storage_buffer(CF_StorageBufferParams params) { return cf_make_storage_buffer(params); }
CF_INLINE void update_storage_buffer(CF_StorageBuffer buffer, const void* data, int size) { cf_update_storage_buffer(buffer, data, size); }
CF_INLINE void destroy_storage_buffer(CF_StorageBuffer buffer) { cf_destroy_storage_buffer(buffer); }
CF_INLINE CF_ComputeDispatch compute_dispatch_defaults(int gx, int gy, int gz) { return cf_compute_dispatch_defaults(gx, gy, gz); }
CF_INLINE void dispatch_compute(CF_ComputeShader shader, CF_Material material, CF_ComputeDispatch dispatch) { cf_dispatch_compute(shader, material, dispatch); }

}

CF_INLINE bool operator==(const CF_RenderState& a, const CF_RenderState& b) { return !CF_MEMCMP(&a, &b, sizeof(a)); }
CF_INLINE bool operator==(CF_Shader a, CF_Shader b) { return a.id == b.id; }

#endif // CF_CPP

#endif // CF_GRAPHICS_H
