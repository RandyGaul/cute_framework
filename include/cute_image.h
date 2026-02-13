/*
	Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

#ifndef CF_IMAGE_H
#define CF_IMAGE_H

#include "cute_defines.h"
#include "cute_result.h"
#include "cute_graphics.h"

//--------------------------------------------------------------------------------------------------
// C API

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

/**
 * @struct   CF_Image
 * @category image
 * @brief    An array of pixels.
 * @remarks  You probably do not need this. In Cute Framework loading images manually is not often
 *           necessary, as most games can use CF's [Draw API](https://randygaul.github.io/cute_framework/topics/drawing) to get sprites onto the screen.
 *           However, a good use case is, for example, if you want to implement some custom shader and feed it a texture.
 * @related  CF_Image CF_ImageIndexed cf_image_load_png cf_image_save_png cf_image_save_png_to_memory cf_image_premultiply
 */
typedef struct CF_Image
{
	/* @member Width of the image in pixels. */
	int w;

	/* @member Height of the image in pixels. */
	int h;

	/* @member An array of pixels. */
	CF_Pixel* pix;
} CF_Image;
// @end

/**
 * @struct   CF_ImageIndexed
 * @category image
 * @brief    An array of pixels, where each pixel is an index into a palette.
 * @remarks  You probably do not need this. In Cute Framework loading images manually is not often
 *           necessary, as most games can use CF's [Draw API](https://randygaul.github.io/cute_framework/topics/drawing) to get sprites onto the screen.
 *           However, a good use case is, for example, if you want to implement some custom shader and feed it a texture.
 * @related  CF_Image CF_ImageIndexed cf_image_load_png
 */
typedef struct CF_ImageIndexed
{
	/* @member Width of the image in pixels. */
	int w;

	/* @member Height of the image in pixels. */
	int h;

	/* @member An array of pixels. */
	uint8_t* pix;

	/* @member The number of elements in the `palette`. */
	uint8_t palette_len;

	/* @member The color palette for this image. */
	CF_Pixel palette[256];
} CF_ImageIndexed;
// @end

// -------------------------------------------------------------------------------------------------
// PNG loading.

/**
 * @function cf_image_load_png
 * @category image
 * @brief    Loads a png image.
 * @param    virtual_path  A virtual path to the image file. See [Virtual File System](https://randygaul.github.io/cute_framework/topics/virtual_file_system).
 * @param    img           Out parameter for the image.
 * @return   Check the `CF_Result` for errors.
 * @related  CF_Image cf_image_load_png cf_image_free cf_image_load_png_from_memory cf_image_load_png_wh cf_image_load_png_indexed cf_image_save_png cf_image_save_png_to_memory cf_image_premultiply
 */
CF_API CF_Result CF_CALL cf_image_load_png(const char* virtual_path, CF_Image* img);

/**
 * @function cf_image_load_png_from_memory
 * @category image
 * @brief    Loads a png image from memory.
 * @param    data          Pointer to the png file in memory.
 * @param    size          The number of bytes in the `data` pointer.
 * @param    img           Out parameter for the image.
 * @return   Check the `CF_Result` for errors.
 * @related  CF_Image cf_image_load_png cf_image_load_png_from_memory cf_image_load_png_wh cf_image_load_png_indexed cf_image_save_png cf_image_save_png_to_memory cf_image_premultiply
 */
CF_API CF_Result CF_CALL cf_image_load_png_from_memory(const void* data, int size, CF_Image* img);

/**
 * @function cf_image_load_png_wh
 * @category image
 * @brief    Loads just the width/height out of a png image, without processing the pixels.
 * @param    data          Pointer to the png file in memory.
 * @param    size          The number of bytes in the `data` pointer.
 * @param    w             Out parameter for the width of the image.
 * @param    h             Out parameter for the height of the image.
 * @return   Check the `CF_Result` for errors.
 * @related  CF_Image cf_image_load_png cf_image_load_png_from_memory cf_image_load_png_wh cf_image_load_png_indexed cf_image_save_png cf_image_save_png_to_memory cf_image_premultiply
 */
CF_API CF_Result CF_CALL cf_image_load_png_wh(const void* data, int size, int* w, int* h);

/**
 * @function cf_image_free
 * @category image
 * @brief    Frees a png image.
 * @param    img           The image to free.
 * @return   Check the `CF_Result` for errors.
 * @related  CF_Image cf_image_load_png cf_image_free cf_image_load_png_from_memory cf_image_load_png_wh cf_image_load_png_indexed cf_image_save_png cf_image_save_png_to_memory cf_image_premultiply
 */
CF_API void CF_CALL cf_image_free(CF_Image* img);

/**
 * @function cf_image_load_png_indexed
 * @category image
 * @brief    Loads a png image in paletted form.
 * @param    virtual_path  A virtual path to the image file. See [Virtual File System](https://randygaul.github.io/cute_framework/topics/virtual_file_system).
 * @param    img           Out parameter for the image.
 * @return   Check the `CF_Result` for errors.
 * @related  CF_ImageIndexed cf_image_load_png_indexed cf_image_load_png_from_memory_indexed cf_image_free_indexed cf_image_depalette
 */
CF_API CF_Result CF_CALL cf_image_load_png_indexed(const char* virtual_path, CF_ImageIndexed* img);

/**
 * @function cf_image_load_png_from_memory_indexed
 * @category image
 * @brief    Loads a png image in paletted form.
 * @param    data          Pointer to the image in memory.
 * @param    size          The number of bytes in the `data` pointer.
 * @param    img           Out parameter for the image.
 * @return   Check the `CF_Result` for errors.
 * @related  CF_ImageIndexed cf_image_load_png_indexed cf_image_load_png_from_memory_indexed cf_image_free_indexed cf_image_depalette
 */
CF_API CF_Result CF_CALL cf_image_load_png_from_memory_indexed(const void* data, int size, CF_ImageIndexed* img);

/**
 * @function cf_image_free_indexed
 * @category image
 * @brief    Frees an indexed png image.
 * @param    img           The image to free.
 * @return   Check the `CF_Result` for errors.
 * @related  CF_ImageIndexed cf_image_load_png_indexed cf_image_load_png_from_memory_indexed cf_image_free_indexed cf_image_depalette
 */
CF_API void CF_CALL cf_image_free_indexed(CF_ImageIndexed* img);

// -------------------------------------------------------------------------------------------------
// PNG saving.

/**
 * @function cf_image_save_png
 * @category image
 * @brief    Saves an image to a PNG file on disk.
 * @param    path          The file path to write the PNG to. Does *not* use the virtual file system.
 * @param    img           The image to save.
 * @return   Check the `CF_Result` for errors.
 * @related  CF_Image cf_image_load_png cf_image_free cf_image_save_png cf_image_save_png_to_memory
 */
CF_API CF_Result CF_CALL cf_image_save_png(const char* path, CF_Image* img);

/**
 * @function cf_image_save_png_to_memory
 * @category image
 * @brief    Saves an image to a PNG in memory.
 * @param    img           The image to save.
 * @param    out_data      Out parameter for the PNG data. Caller must free with `cf_free`.
 * @param    out_size      Out parameter for the size of the PNG data in bytes.
 * @return   Check the `CF_Result` for errors.
 * @related  CF_Image cf_image_load_png cf_image_free cf_image_save_png cf_image_save_png_to_memory
 */
CF_API CF_Result CF_CALL cf_image_save_png_to_memory(CF_Image* img, void** out_data, int* out_size);

// -------------------------------------------------------------------------------------------------
// Image operations.

/**
 * @function cf_image_depalette
 * @category image
 * @brief    Converts a paletted image to a typical image with an array of pixels.
 * @param    img           The image to depalette.
 * @return   Returns an image without a palette.
 * @related  CF_Image CF_ImageIndexed cf_image_load_png_indexed
 */
CF_API CF_Image CF_CALL cf_image_depalette(CF_ImageIndexed* img);

/**
 * @function cf_image_premultiply
 * @category image
 * @brief    Premultiplies the alpha component of each pixel with the RGB color components.
 * @param    img           The image to premultiply.
 * @remarks  Premultiplying images is a common way to deal with colors, especially when blending.
 *           Here is a good resource for learning about [premultiplied alpha](https://iquilezles.org/articles/premultipliedalpha/).
 * @related  CF_Image
 */
CF_API void CF_CALL cf_image_premultiply(CF_Image* img);

/**
 * @function cf_image_flip_horizontal
 * @category image
 * @brief    Flips the image on the y-axis.
 * @related  CF_Image
 */
CF_API void CF_CALL cf_image_flip_horizontal(CF_Image* img);

/**
 * @function cf_debug_dump_greyscale_pixels
 * @category image
 * @brief    Saves out greyscale image to a png file on disk.
 * @remarks  Does *not* use the virtual file system. Instead it uses plain old platform-dependent path notation. This
 *           function is not really optimized whatsoever and outputs poorly compressed file sizes.
 * @related  CF_Image cf_debug_dump_greyscale_pixels cf_debug_dump_pixels
 */
CF_API void CF_CALL cf_debug_dump_greyscale_pixels(const char* path, uint8_t* pixels, int w, int h);

/**
 * @function cf_debug_dump_pixels
 * @category image
 * @brief    Saves out an image to a png file on disk.
 * @remarks  Does *not* use the virtual file system. Instead it uses plain old platform-dependent path notation. This
 *           function is not really optimized whatsoever and outputs poorly compressed file sizes.
 * @related  CF_Image cf_debug_dump_greyscale_pixels cf_debug_dump_pixels
 */
CF_API void CF_CALL cf_debug_dump_pixels(const char* path, CF_Pixel* pixels, int w, int h);

#ifdef __cplusplus
}
#endif // __cplusplus

//--------------------------------------------------------------------------------------------------
// C++ API

#ifdef CF_CPP

namespace Cute
{

// -------------------------------------------------------------------------------------------------
// PNG loading.

CF_INLINE CF_Result image_load_png(const char* virtual_path, CF_Image* img) { return cf_image_load_png(virtual_path, img); }
CF_INLINE CF_Result image_load_png_mem(const void* data, int size, CF_Image* img) { return cf_image_load_png_from_memory(data, size, img); }
CF_INLINE CF_Result image_load_png_wh(const void* data, int size, int* w, int* h) { return cf_image_load_png_wh(data, size, w, h); }
CF_INLINE void image_free(CF_Image* img) { cf_image_free(img); }

CF_INLINE CF_Result image_load_png_indexed(const char* virtual_path, CF_ImageIndexed* img) { return cf_image_load_png_indexed(virtual_path, img); }
CF_INLINE CF_Result image_load_png_mem_indexed(const void* data, int size, CF_ImageIndexed* img) { return cf_image_load_png_from_memory_indexed(data, size, img); }
CF_INLINE void image_free(CF_ImageIndexed* img) { cf_image_free_indexed(img); }

// -------------------------------------------------------------------------------------------------
// PNG saving.

CF_INLINE CF_Result image_save_png(const char* path, CF_Image* img) { return cf_image_save_png(path, img); }
CF_INLINE CF_Result image_save_png_to_memory(CF_Image* img, void** out_data, int* out_size) { return cf_image_save_png_to_memory(img, out_data, out_size); }

// -------------------------------------------------------------------------------------------------
// Image operations.

CF_INLINE CF_Image image_depalette(CF_ImageIndexed* img) { return cf_image_depalette(img); }
CF_INLINE void image_premultiply(CF_Image* img) { cf_image_premultiply(img); }
CF_INLINE void image_flip_horizontal(CF_Image* img) { cf_image_flip_horizontal(img); }
CF_INLINE void debug_dump_greyscale_pixels(const char* path, uint8_t* pixels, int w, int h) { cf_debug_dump_greyscale_pixels(path, pixels, w, h); }
CF_INLINE void debug_dump_pixels(const char* path, CF_Pixel* pixels, int w, int h) { cf_debug_dump_pixels(path, pixels, w, h); }

}

#endif // CF_CPP

#endif // CF_IMAGE_H
