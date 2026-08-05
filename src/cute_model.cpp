/*
	Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

#include <cute_alloc.h>
#include <cute_file_system.h>
#include <cute_image.h>
#include <cute_graphics.h>
#include <cute_model.h>

// CKIT_IMPLEMENTATION lives in cute_ckit.cpp; this TU only borrows the macros. String
// handling below stays on ckit's s-api so every allocation in this file is ckit-side.
#include <cute/ckit.h>
#define CUTE_MODEL_IMPLEMENTATION
#include <cute/cute_model.h>

// cm_error() covers loader failures; this covers the one failure that happens before the
// loader ever runs (the VFS read).
static const char* s_model_error;

static void* s_model_read_fn(const char* path, int* size_out, void* udata)
{
	// `path` is a .gltf's external URI, percent-decoded, relative to the model's directory.
	const char* dir = (const char*)udata;
	char* full = sfmake("%s/%s", dir, path);
	size_t size = 0;
	void* data = cf_fs_read_entire_file_to_memory(full, &size);
	sfree(full);
	*size_out = (int)size;
	return data;
}

static void s_model_free_fn(void* data, void* udata)
{
	CF_UNUSED(udata);
	cf_free(data);
}

CM_Model* cf_make_model(const char* virtual_path)
{
	s_model_error = NULL;
	size_t size = 0;
	void* data = cf_fs_read_entire_file_to_memory(virtual_path, &size);
	if (!data) {
		s_model_error = "Unable to read the model file from the virtual file system.";
		return NULL;
	}

	// The model's directory, so external references resolve next to the file.
	int dir_len = 0;
	for (int i = 0; virtual_path[i]; ++i) {
		if (virtual_path[i] == '/') dir_len = i;
	}
	char* dir = sfmake("%.*s", dir_len, virtual_path);

	CM_LoadParams params = { };
	params.read_fn = s_model_read_fn;
	params.free_fn = s_model_free_fn;
	params.udata = dir;
	CM_Model* model = cm_load_ex(data, (int)size, params);

	sfree(dir);
	cf_free(data);
	return model;
}

void cf_destroy_model(CM_Model* model)
{
	if (model) cm_free(model);
}

const char* cf_model_error(void)
{
	return s_model_error ? s_model_error : cm_error();
}

CF_Texture cf_make_texture_from_model_image(const CM_Model* model, int image_index)
{
	if (model && image_index >= 0 && image_index < model->image_count) {
		CM_Image* image = model->images + image_index;
		const uint8_t* bytes = image->data;
		// Sniff PNG vs JPEG by magic number; glTF exporters commonly embed either.
		bool jpg = image->size > 2 && bytes[0] == 0xFF && bytes[1] == 0xD8;
		CF_Image img;
		CF_Result decoded = jpg
			? cf_image_load_jpg_from_memory(image->data, image->size, &img)
			: cf_image_load_png_from_memory(image->data, image->size, &img);
		if (!cf_is_error(decoded)) {
			CF_TextureParams params = cf_texture_defaults(img.w, img.h);
			params.filter = CF_FILTER_LINEAR;
			params.allocate_mipmaps = true;
			// cf_generate_mipmaps downsamples with GPU blits, which need render-target usage.
			params.usage |= CF_TEXTURE_USAGE_COLOR_TARGET_BIT;
			CF_Texture tex = cf_make_texture(params);
			cf_texture_update(tex, img.pix, img.w * img.h * (int)sizeof(CF_Pixel));
			cf_generate_mipmaps(tex);
			cf_image_free(&img);
			return tex;
		}
	}

	// Unused slot (-1), bad index, or undecodable bytes: 1x1 white, always drawable.
	CF_Pixel white = cf_make_pixel_rgb(255, 255, 255);
	CF_Texture tex = cf_make_texture(cf_texture_defaults(1, 1));
	cf_texture_update(tex, &white, (int)sizeof(white));
	return tex;
}
