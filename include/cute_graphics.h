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
 * omit since they aren't super useful for 2D.
 * 
 *     - Mipmaps
 *     - MSAA
 *     - Blend color constant
 *     - Multiple render targets (aka color/texture attachments)
 *     - Depth bias tunables
 *     - Cube map
 *     - 3D textures
 *     - Texture arrays
 *     - Other primitive types besides triangles
 *     - Anisotropy tunable
 *     - Min/max LOD tunable
 *
 * These features are to be added to CF in the future:
 * 
 *     - Indexed meshes
 *     - Instance rendering
 *     - Compute shaders
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
 *         cf_commit();
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

//--------------------------------------------------------------------------------------------------
// Device queries.

/**
 * @enum     CF_BackendType
 * @category graphics
 * @brief    The various supported graphics backends.
 * @related  CF_BackendType cf_backend_type_to_string cf_query_backend
 */
#define CF_BACKEND_TYPE_DEFS \
	/* @entry Invalid backend type (unitialized or failed to create). */           \
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
	CF_ENUM(BACKEND_TYPE_SECRET_NDA,  4)                                           \
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
 * @brief    The various supported pixel formats for GPU.
 * @remarks  Pixel format support varies depending on driver, hardware, and usage flags.
 *           The `PIXEL_FORMAT_R8G8B8A8_UNORM` represents a safe default format.
 * @related  CF_PixelFormat cf_pixel_format_to_string CF_PixelFormatOp
 */
#define CF_PIXEL_FORMAT_DEFS \
	/* @entry Invalid pixel format. */                                                         \
	CF_ENUM(PIXEL_FORMAT_INVALID,                -1)                                           \
	/* @entry 8-bit red/green/blue/alpha channels, 32 bits total, unsigned normalized. */      \
	CF_ENUM(PIXEL_FORMAT_R8G8B8A8_UNORM,          0)                                           \
	/* @entry 8-bit blue/green/red/alpha channels, 32 bits total, unsigned normalized. */      \
	CF_ENUM(PIXEL_FORMAT_B8G8R8A8_UNORM,          1)                                           \
	/* @entry 5-bit blue/green channels, 6-bit red channel, 16 bits total, unsigned normalized. */\
	CF_ENUM(PIXEL_FORMAT_B5G6R5_UNORM,            2)                                           \
	/* @entry 5-bit blue/green/red channels, 1-bit alpha channel, 16 bits total, unsigned normalized. */\
	CF_ENUM(PIXEL_FORMAT_B5G5R5A1_UNORM,          3)                                           \
	/* @entry 4-bit blue/green/red/alpha channels, 16 bits total, unsigned normalized. */      \
	CF_ENUM(PIXEL_FORMAT_B4G4R4A4_UNORM,          4)                                           \
	/* @entry 10-bit red/green/blue channels, 2-bit alpha channel, 32 bits total, unsigned normalized. */\
	CF_ENUM(PIXEL_FORMAT_R10G10B10A2_UNORM,       5)                                           \
	/* @entry 16-bit red/green channels, 32 bits total, unsigned normalized. */                \
	CF_ENUM(PIXEL_FORMAT_R16G16_UNORM,            6)                                           \
	/* @entry 16-bit red/green/blue/alpha channels, 64 bits total, unsigned normalized. */     \
	CF_ENUM(PIXEL_FORMAT_R16G16B16A16_UNORM,      7)                                           \
	/* @entry 8-bit red-only channel, unsigned normalized. */                                  \
	CF_ENUM(PIXEL_FORMAT_R8_UNORM,                8)                                           \
	/* @entry 8-bit alpha-only channel, unsigned normalized. */                                \
	CF_ENUM(PIXEL_FORMAT_A8_UNORM,                9)                                           \
	/* @entry Block Compression 1, unsigned normalized. */                                     \
	CF_ENUM(PIXEL_FORMAT_BC1_UNORM,              10)                                           \
	/* @entry Block Compression 2, unsigned normalized. */                                     \
	CF_ENUM(PIXEL_FORMAT_BC2_UNORM,              11)                                           \
	/* @entry Block Compression 3, unsigned normalized. */                                     \
	CF_ENUM(PIXEL_FORMAT_BC3_UNORM,              12)                                           \
	/* @entry Block Compression 7, unsigned normalized. */                                     \
	CF_ENUM(PIXEL_FORMAT_BC7_UNORM,              13)                                           \
	/* @entry 8-bit red/green channels, 16 bits total, signed normalized. */                   \
	CF_ENUM(PIXEL_FORMAT_R8G8_SNORM,             14)                                           \
	/* @entry 8-bit red/green/blue/alpha channels, 32 bits total, signed normalized. */        \
	CF_ENUM(PIXEL_FORMAT_R8G8B8A8_SNORM,         15)                                           \
	/* @entry 16-bit red-only channel, floating point. */                                      \
	CF_ENUM(PIXEL_FORMAT_R16_FLOAT,              16)                                           \
	/* @entry 16-bit red/green channels, 32 bits total, floating point. */                     \
	CF_ENUM(PIXEL_FORMAT_R16G16_FLOAT,           17)                                           \
	/* @entry 16-bit red/green/blue/alpha channels, 64 bits total, floating point. */          \
	CF_ENUM(PIXEL_FORMAT_R16G16B16A16_FLOAT,     18)                                           \
	/* @entry 32-bit red-only channel, floating point. */                                      \
	CF_ENUM(PIXEL_FORMAT_R32_FLOAT,              19)                                           \
	/* @entry 32-bit red/green channels, 64 bits total, floating point. */                     \
	CF_ENUM(PIXEL_FORMAT_R32G32_FLOAT,           20)                                           \
	/* @entry 32-bit red/green/blue/alpha channels, 128 bits total, floating point. */         \
	CF_ENUM(PIXEL_FORMAT_R32G32B32A32_FLOAT,     21)                                           \
	/* @entry 8-bit red-only channel, unsigned integer. */                                     \
	CF_ENUM(PIXEL_FORMAT_R8_UINT,                22)                                           \
	/* @entry 8-bit red/green channels, 16 bits total, unsigned integer. */                    \
	CF_ENUM(PIXEL_FORMAT_R8G8_UINT,              23)                                           \
	/* @entry 8-bit red/green/blue/alpha channels, 32 bits total, unsigned integer. */         \
	CF_ENUM(PIXEL_FORMAT_R8G8B8A8_UINT,          24)                                           \
	/* @entry 16-bit red-only channel, unsigned integer. */                                    \
	CF_ENUM(PIXEL_FORMAT_R16_UINT,               25)                                           \
	/* @entry 16-bit red/green channels, 32 bits total, unsigned integer. */                   \
	CF_ENUM(PIXEL_FORMAT_R16G16_UINT,            26)                                           \
	/* @entry 16-bit red/green/blue/alpha channels, 64 bits total, unsigned integer. */        \
	CF_ENUM(PIXEL_FORMAT_R16G16B16A16_UINT,      27)                                           \
	/* @entry 8-bit red/green/blue/alpha channels, 32 bits total, sRGB color space. */         \
	CF_ENUM(PIXEL_FORMAT_R8G8B8A8_UNORM_SRGB,    28)                                           \
	/* @entry 8-bit blue/green/red/alpha channels, 32 bits total, sRGB color space. */         \
	CF_ENUM(PIXEL_FORMAT_B8G8R8A8_UNORM_SRGB,    29)                                           \
	/* @entry Block Compression 3, sRGB color space. */                                        \
	CF_ENUM(PIXEL_FORMAT_BC3_UNORM_SRGB,         30)                                           \
	/* @entry Block Compression 7, sRGB color space. */                                        \
	CF_ENUM(PIXEL_FORMAT_BC7_UNORM_SRGB,         31)                                           \
	/* @entry 16-bit depth channel, unsigned normalized. */                                    \
	CF_ENUM(PIXEL_FORMAT_D16_UNORM,              32)                                           \
	/* @entry 24-bit depth channel, unsigned normalized. */                                    \
	CF_ENUM(PIXEL_FORMAT_D24_UNORM,              33)                                           \
	/* @entry 32-bit depth channel, floating point. */                                         \
	CF_ENUM(PIXEL_FORMAT_D32_FLOAT,              34)                                           \
	/* @entry 24-bit depth channel, 8-bit stencil channel, unsigned normalized. */             \
	CF_ENUM(PIXEL_FORMAT_D24_UNORM_S8_UINT,      35)                                           \
	/* @entry 32-bit depth channel, 8-bit stencil channel, floating point. */                  \
	CF_ENUM(PIXEL_FORMAT_D32_FLOAT_S8_UINT,      36)                                           \
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
	CF_ENUM(TEXTURE_USAGE_COMPUTE_STORAGE_READ_BIT,  0x00000020)                      \
	/* @entry The texture will be used as writeable storage in compute pipelines. */  \
	CF_ENUM(TEXTURE_USAGE_COMPUTE_STORAGE_WRITE_BIT, 0x00000040)                      \
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

	/* @member The texture wrapping behavior when addressing beyond [0,1] for the u-coordinate. See `CF_WrapMode`. */
	CF_WrapMode wrap_u;

	/* @member The texture wrapping behavior when addressing beyond [0,1] for the v-coordinate. See `CF_WrapMode`. */
	CF_WrapMode wrap_v;

	/* @member Number of elements (usually pixels) along the width of the texture. */
	int width;

	/* @member Number of elements (usually pixels) along the height of the texture. */
	int height;

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
 * @related  CF_TextureParams CF_Texture cf_make_texture cf_destroy_texture cf_texture_update
 */
CF_API void CF_CALL cf_texture_update(CF_Texture texture, void* data, int size);

/**
 * @function cf_texture_handle
 * @category graphics
 * @brief    Returns an SDL_GpuTexture* casted to a `uint64_t`.
 * @remarks  This is useful for e.g. rendering textures in an external system like Dear ImGui.
 * @related  CF_TextureParams CF_Texture cf_make_texture
 */
CF_API uint64_t CF_CALL cf_texture_handle(CF_Texture texture);

//--------------------------------------------------------------------------------------------------
// Shader.

/**
 * @enum     CF_ShaderStage
 * @category graphics
 * @brief    Distinction between vertex and fragment shaders.
 * @related  CF_Shader cf_shader_directory cf_make_shader
 */
#define CF_SHADER_STAGE_DEFS \
	/* @entry */ \
	CF_ENUM(SHADER_STAGE_VERTEX,   0) \
	/* @entry */ \
	CF_ENUM(SHADER_STAGE_FRAGMENT, 1) \
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
 * @param    path     A virtual path to the folder with your shaders (subfolders supported). See [Virtual File System](https://randygaul.github.io/cute_framework/#/topics/virtual_file_system).
 * @remarks  Shaders can `#include` each other as long as they exist in this directory. Changes to shaders on disk
 *           may also be watched via `cf_shader_on_changed` to support shader reloading during development.
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
 * @param    vertex_path   A virtual path to the shader. See [Virtual File System](https://randygaul.github.io/cute_framework/#/topics/virtual_file_system).
 * @remarks  The shader paths must be in the shader directory. See `cf_shader_directory`. Note the expected glsl version is 450.
 *           
 *           You must setup shader inputs (max of 32 inputs, e.g. `in` keyword) and resources sets in a specific way. Use the
             following resource sets and ordering in your shaders:
 *           
 *           For _VERTEX_ shaders:
 *            0: Sampled textures, followed by storage textures, followed by storage buffers
 *            1: Uniform buffers
 *           For _FRAGMENT_ shaders:
 *            2: Sampled textures, followed by storage textures, followed by storage buffers
 *            3: Uniform buffers
 *           
 *           Example _VERTEX shader:
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
 * @param    vertex_path   A virtual path to the shader. See [Virtual File System](https://randygaul.github.io/cute_framework/#/topics/virtual_file_system).
 * @remarks  The shader paths must be in the shader directory. See `cf_shader_directory`.
 * @related  CF_Shader cf_make_shader cf_shader_directory cf_apply_shader CF_Material
 */
CF_API CF_Shader CF_CALL cf_make_shader_from_source(const char* vertex_src, const char* fragment_src);

/**
 * @function cf_compile_shader_to_bytecode
 * @category graphics
 * @brief    Compiles a shader to SPIR-V bytecode.
 * @param    shader_src   Raw glsl, version 450, for the shader as a string.
 * @param    stage        The shaderstrage to differentiate between vertex or fragment shaders.
 * @remarks  This function is good for precompiling shaders to bytecode, which can help speed up app
 *           startup times. SPIR-V blobs can be saved straight to disk and shipped with your game. Load
 *           the bytecode blob pair (vertex + fragment shader blobs) into a `CF_Shader` via `cf_make_shader_from_bytecode`.
 * @related  CF_Shader cf_make_shader_from_bytecode cf_make_shader_from_bytecode
 */
CF_API const dyna uint8_t* CF_CALL cf_compile_shader_to_bytecode(const char* shader_src, CF_ShaderStage stage);

/**
 * @function cf_make_shader_from_bytecode
 * @category graphics
 * @brief    Creates a shader from SPIR-V bytecode.
 * @param    vertex_bytecode    A bytecode blob from `cf_compile_shader_to_bytecode` for the vertex shader.
 * @param    fragment_bytecode  A bytecode blob from `cf_compile_shader_to_bytecode` for the fragment shader.
 * @remarks  This function is good for precompiling shaders from bytecode, which can help speed up app
 *           startup times. SPIR-V blobs can be saved straight to disk and shipped with your game. Create the
 *           bytecode blob with `cf_make_shader_from_bytecode`.
 * @related  CF_Shader cf_make_shader_from_bytecode cf_make_shader_from_bytecode
 */
CF_API CF_Shader CF_CALL cf_make_shader_from_bytecode(const dyna uint8_t* vertex_bytecode, const dyna uint8_t* fragment_bytecode);

/**
 * @function cf_destroy_shader
 * @category graphics
 * @brief    Frees up a `CF_Shader` created by `cf_make_shader`.
 * @param    shader     A shader.
 * @related  CF_Shader cf_make_shader cf_destroy_shader cf_apply_shader CF_Material
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

	/* @member Defaults to false. If true enables a depth-stencil buffer attachment. */
	bool depth_stencil_enable;

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
 * @related  CF_Mesh cf_make_mesh cf_destroy_mesh cf_mesh_update_vertex_data
 */
CF_API CF_Mesh CF_CALL cf_make_mesh(int vertex_buffer_size_in_bytes, const CF_VertexAttribute* attributes, int attribute_count, int vertex_stride);

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
 * @param    count      Number of bytes in `data`.
 * @return   Returns the number of bytes written.
 * @related  CF_Mesh cf_make_mesh cf_destroy_mesh cf_mesh_update_vertex_data
 */
CF_API void CF_CALL cf_mesh_update_vertex_data(CF_Mesh mesh, void* data, int count);

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
	CF_ENUM(UNIFORM_TYPE_UNKNOWN, -1)          \
	/* @entry In a shader: `uniform float` */  \
	CF_ENUM(UNIFORM_TYPE_FLOAT,    0)          \
	/* @entry In a shader: `uniform vec2` */   \
	CF_ENUM(UNIFORM_TYPE_FLOAT2,   1)          \
	/* @entry In a shader: `uniform vec4` */   \
	CF_ENUM(UNIFORM_TYPE_FLOAT4,   2)          \
	/* @entry In a shader: `uniform int` */    \
	CF_ENUM(UNIFORM_TYPE_INT,      3)          \
	/* @entry In a shader: `uniform int[2]` */ \
	CF_ENUM(UNIFORM_TYPE_INT2,     4)          \
	/* @entry In a shader: `uniform int[4]` */ \
	CF_ENUM(UNIFORM_TYPE_INT4,     5)          \
	/* @entry In a shader: `uniform mat4` */   \
	CF_ENUM(UNIFORM_TYPE_MAT4,     6)          \
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
CF_API void CF_CALL cf_draw_elements();

/**
 * @function cf_commit
 * @category graphics
 * @brief    Submits all previous draw commands to the GPU.
 * @remarks  You must call this after calling `cf_apply_shader` to "complete" the rendering pass.
 * @related  CF_Canvas cf_apply_canvas cf_apply_mesh cf_apply_shader
 */
CF_API void CF_CALL cf_commit();

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

using PixelFormat = CF_PixelFormat;
#define CF_ENUM(K, V) CF_INLINE constexpr PixelFormat K = CF_##K;
CF_PIXEL_FORMAT_DEFS
#undef CF_ENUM

CF_INLINE constexpr const char* to_string(PixelFormat type) { switch(type) {
	#define CF_ENUM(K, V) case CF_##K: return #K;
	CF_PIXEL_FORMAT_DEFS
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
CF_BLENDFACTOR_DEFS
#undef CF_ENUM

CF_INLINE constexpr const char* to_string(BlendFactor type) { switch(type) {
	#define CF_ENUM(K, V) case CF_##K: return #K;
	CF_BLENDFACTOR_DEFS
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

using ShaderStage = CF_ShaderStage;

CF_INLINE void clear_color(float r, float b, float g, float a) { cf_clear_color(r, g, b, a); }
CF_INLINE void clear_color(CF_Color color) { cf_clear_color(color.r, color.g, color.b, color.a); }
CF_INLINE BackendType query_backend() { return cf_query_backend(); }
CF_INLINE TextureParams texture_defaults(int w, int h) { return cf_texture_defaults(w, h); }
CF_INLINE Texture make_texture(TextureParams texture_params) { return cf_make_texture(texture_params); }
CF_INLINE void destroy_texture(Texture texture) { cf_destroy_texture(texture); }
CF_INLINE void texture_update(Texture texture, void* data, int size) { cf_texture_update(texture, data, size); }
CF_INLINE Shader make_shader(const char* vertex, const char* fragment) { return cf_make_shader(vertex, fragment); }
CF_INLINE void shader_directory(const char* path) { cf_shader_directory(path); }
CF_INLINE void shader_on_changed(void (*on_changed_fn)(const char* path, void* udata), void* udata) { cf_shader_on_changed(on_changed_fn, udata); }
CF_INLINE Shader make_shader_from_source(const char* vertex_src, const char* fragment_src) { return cf_make_shader_from_source(vertex_src, fragment_src); }
CF_INLINE const dyna uint8_t* compile_shader_to_bytecode(const char* shader_src, ShaderStage stage) { return cf_compile_shader_to_bytecode(shader_src, stage); }
CF_INLINE Shader make_shader_from_bytecode(const dyna uint8_t* vertex_bytecode, const dyna uint8_t* fragment_bytecode) { return cf_make_shader_from_bytecode(vertex_bytecode, fragment_bytecode); }
CF_INLINE void destroy_shader(Shader shader) { cf_destroy_shader(shader); }
CF_INLINE CanvasParams canvas_defaults(int w, int h) { return cf_canvas_defaults(w, h); }
CF_INLINE Canvas make_canvas(CanvasParams pass_params) { return cf_make_canvas(pass_params); }
CF_INLINE void destroy_canvas(Canvas canvas) { cf_destroy_canvas(canvas); }
CF_INLINE Texture canvas_get_target(Canvas canvas) { return cf_canvas_get_target(canvas); }
CF_INLINE Texture canvas_get_depth_stencil_target(Canvas canvas) { return cf_canvas_get_depth_stencil_target(canvas); }
CF_INLINE void canvas_blit(Canvas src, v2 u0, v2 v0, Canvas dst, v2 u1, v2 v1) { cf_canvas_blit(src, u0, v0, dst, u1, v1); }
CF_INLINE Mesh make_mesh(int vertex_buffer_size_in_bytes, const VertexAttribute* attributes, int attribute_count, int vertex_stride) { return cf_make_mesh(vertex_buffer_size_in_bytes, attributes, attribute_count, vertex_stride); }
CF_INLINE void destroy_mesh(Mesh mesh) { cf_destroy_mesh(mesh); }
CF_INLINE void mesh_update_vertex_data(Mesh mesh, void* data, int count) { cf_mesh_update_vertex_data(mesh, data, count); }
//CF_INLINE void mesh_update_index_data(Mesh mesh, uint32_t* indices, int count) {(mesh, indices, count); }
CF_INLINE RenderState render_state_defaults() { return cf_render_state_defaults(); }
CF_INLINE Material make_material() { return cf_make_material(); }
CF_INLINE void destroy_material(Material material) { cf_destroy_material(material); }
CF_INLINE void material_set_render_state(Material material, RenderState render_state) { cf_material_set_render_state(material, render_state); }
CF_INLINE void material_set_texture_vs(Material material, const char* name, Texture texture) { cf_material_set_texture_vs(material, name, texture); }
CF_INLINE void material_set_texture_fs(Material material, const char* name, Texture texture) { cf_material_set_texture_fs(material, name, texture); }
CF_INLINE void material_clear_textures(Material material) { cf_material_clear_textures(material); }
CF_INLINE void material_set_uniform_vs(Material material, const char* name, void* data, UniformType type, int array_length) { cf_material_set_uniform_vs(material, name, data, type, array_length); }
CF_INLINE void material_set_uniform_fs(Material material, const char* name, void* data, UniformType type, int array_length) { cf_material_set_uniform_fs(material, name, data, type, array_length); }
CF_INLINE void material_clear_uniforms(Material material) { cf_material_clear_uniforms(material); }
CF_INLINE void apply_canvas(Canvas canvas, bool clear = false) { cf_apply_canvas(canvas, false); }
CF_INLINE void apply_viewport(int x, int y, int w, int h) { cf_apply_viewport(x, y, w, h); }
CF_INLINE void apply_scissor(int x, int y, int w, int h) { cf_apply_scissor(x, y, w, h); }
CF_INLINE void apply_mesh(Mesh mesh) { cf_apply_mesh(mesh); }
CF_INLINE void apply_shader(Shader shader, Material material) { cf_apply_shader(shader, material); }
CF_INLINE void draw_elements() { cf_draw_elements(); }
CF_INLINE void commit() { cf_commit(); }

}

#endif // CF_CPP

#endif // CF_GRAPHICS_H
