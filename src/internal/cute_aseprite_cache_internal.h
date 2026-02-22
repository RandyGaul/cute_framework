/*
	Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

#ifndef CF_ASEPRITE_CACHE_INTERNAL_H
#define CF_ASEPRITE_CACHE_INTERNAL_H

#include <cute_defines.h>
#include <cute_sprite.h>
#include <cute_custom_sprite.h>
#include <cute_math.h>

#include <cute/cute_aseprite.h>

#define CF_SPRITE_ID_INVALID (~0ULL)

CF_Result cf_aseprite_cache_load(const char* aseprite_path, CF_Sprite* sprite_out);
CF_Result cf_aseprite_cache_load_from_memory(const char* unique_name, const void* data, int sz, CF_Sprite* sprite_out);
void cf_aseprite_cache_unload(const char* aseprite_path);
CF_Result cf_aseprite_cache_load_ase(const char* aseprite_path, ase_t** ase);

CF_API void cf_make_aseprite_cache();
CF_API void cf_destroy_aseprite_cache();
void cf_aseprite_cache_get_pixels(uint64_t image_id, void* buffer, int bytes_to_fill);

// Per-aseprite asset. Indexed by asset_id in the flat array.
struct CF_SpriteAsset
{
	const char* path;
	ase_t* ase;
	int w, h;
	CF_MAP(CF_Animation*) animations;
	dyna CF_SpriteSlice* slices;
	dyna CF_V2* pivots;
	dyna CF_Aabb* center_patches;
	dyna uint64_t* frame_ids;
	int blend_count;                  // Total blends (1 = default only).
	dyna uint64_t** blend_frame_ids;  // Array of frame_id arrays per blend.
	dyna void** owned_pixel_buffers;  // Pixel buffers from re-blending (index 1+).
};

CF_SpriteAsset* cf_sprite_get_asset(uint64_t asset_id);

int cf_aseprite_cache_add_blend(const char* path, uint64_t layer_mask);
uint64_t cf_aseprite_layer_mask(ase_t* ase, const char** layer_names, int count);

// Register an externally-owned animation table as a sprite asset (used by custom sprite cache).
// The caller retains ownership of the animations; the asset will not free them.
uint64_t cf_register_external_sprite_asset(const char* name, int w, int h, CF_MAP(CF_Animation*) animations);

// Reload an asset's pixel data and animations from a newly parsed ase_t.
// Reuses frame IDs where possible so other sprites sharing the asset pick up changes.
// Takes ownership of new_ase.
void cf_aseprite_cache_reload_asset(uint64_t asset_id, ase_t* new_ase);

#endif // CF_ASEPRITE_CACHE_INTERNAL_H
