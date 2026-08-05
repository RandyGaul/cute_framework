/*
	Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

#ifndef CF_MODEL_H
#define CF_MODEL_H

#include "cute_defines.h"
#include "cute_graphics.h"

//--------------------------------------------------------------------------------------------------
// C API
//
// Virtual-file-system glue for cute_model.h, CF's glTF 2.0 / GLB loader. The loader itself lives
// in libraries/cute/cute_model.h and does no file IO of its own; these functions wire it to CF's
// VFS (external .bin buffers and image files referenced by a .gltf resolve relative to the model's
// virtual directory) and to CF's image decoders and texture creation.
//
// The model's data and animation runtime are the CM_* API: meshes and vertex streams, the node
// hierarchy, skins, animation clip sampling (cm_animate/cm_blend/cm_skin_palette), interleaving
// via cm_interleave, and so on. To use them, include the loader's header in the translation units
// that touch model data:
//
//     #include <cute/ckit.h>
//     #include <cute/cute_model.h>
//
// Do NOT define CUTE_MODEL_IMPLEMENTATION -- CF's library already compiles the implementation for
// you (just like CKIT_IMPLEMENTATION). See the model3d sample for the complete flow from
// cf_make_model to a skinned, animated, instanced draw.

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

typedef struct CM_Model CM_Model;

/**
 * @function cf_make_model
 * @category model
 * @brief    Loads a GLB or .gltf model from the virtual file system.
 * @param    virtual_path  A virtual path to the model file (see the Virtual File System topic).
 * @return   Returns the loaded model, or `NULL` on failure -- call `cf_model_error` for the reason.
 * @remarks  External references inside a .gltf (a `.bin` buffer, image files) resolve through the
 *           VFS relative to the model's directory, so a model mounted with its sidecar files simply
 *           works. The returned `CM_Model` owns all of its arrays; free the whole thing with
 *           `cf_destroy_model`. The model's data and animation API (`cm_animate`,
 *           `cm_skin_palette`, `cm_interleave`, ...) lives in `libraries/cute/cute_model.h` -- see
 *           the includes note at the top of this header.
 * @related  cf_destroy_model cf_model_error cf_make_texture_from_model_image
 */
CF_API CM_Model* CF_CALL cf_make_model(const char* virtual_path);

/**
 * @function cf_destroy_model
 * @category model
 * @brief    Frees a model and everything it owns.
 * @param    model  The model from `cf_make_model`. `NULL` is allowed.
 * @related  cf_make_model cf_model_error
 */
CF_API void CF_CALL cf_destroy_model(CM_Model* model);

/**
 * @function cf_model_error
 * @category model
 * @brief    Returns a human-readable reason for the last `cf_make_model` failure.
 * @remarks  Static storage, valid until the next load. Only meaningful after `cf_make_model`
 *           returns `NULL`.
 * @related  cf_make_model
 */
CF_API const char* CF_CALL cf_model_error(void);

/**
 * @function cf_make_texture_from_model_image
 * @category model
 * @brief    Decodes one of a model's embedded images into a ready-to-sample `CF_Texture`.
 * @param    model        The model from `cf_make_model`.
 * @param    image_index  An index into the model's images, e.g. a `CM_TextureRef`'s `image` member.
 * @return   Returns a linear-filtered texture with a full mip chain generated.
 * @remarks  Embedded glTF images are PNG or JPEG bytes; both decode here. An `image_index` of -1
 *           (the loader's "slot unused" sentinel), an out-of-range index, or an undecodable image
 *           all return a 1x1 white texture, so feeding a material slot straight through always
 *           yields something drawable:
 *
 *           ```c
 *           CF_Texture albedo = cf_make_texture_from_model_image(model, model->materials[0].base_color_texture.image);
 *           ```
 *
 *           Destroy the result with `cf_destroy_texture`. For block-compressed shipping textures
 *           see `cf_make_texture_from_dds` and the Shipping 3D topic.
 * @related  cf_make_model CF_Texture cf_make_texture_from_dds cf_destroy_texture
 */
CF_API CF_Texture CF_CALL cf_make_texture_from_model_image(const CM_Model* model, int image_index);

#ifdef __cplusplus
}
#endif // __cplusplus

//--------------------------------------------------------------------------------------------------
// C++ API

#ifdef CF_CPP

namespace Cute
{

CF_INLINE CM_Model* make_model(const char* virtual_path) { return cf_make_model(virtual_path); }
CF_INLINE void destroy_model(CM_Model* model) { cf_destroy_model(model); }
CF_INLINE const char* model_error() { return cf_model_error(); }
CF_INLINE CF_Texture make_texture_from_model_image(const CM_Model* model, int image_index) { return cf_make_texture_from_model_image(model, image_index); }

}

#endif // CF_CPP

#endif // CF_MODEL_H
