/*
	Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

#include <cute_sprite.h>
#include <cute_debug_printf.h>
#include <cute_file_system.h>
#include <cute_defer.h>
#include <cute_input.h>
#include <cute_image.h>

#include <internal/cute_alloc_internal.h>
#include <internal/cute_app_internal.h>

#define CUTE_ASEPRITE_IMPLEMENTATION
#include <cute/cute_aseprite.h>

#include <internal/cute_aseprite_cache_internal.h>

using namespace Cute;

struct CF_AsepriteCache
{
	Map<void*> id_to_pixels;
	uint64_t id_gen = CF_ASEPRITE_ID_RANGE_LO;
	dyna CF_SpriteAsset* assets = NULL;
	Map<uint64_t> path_to_asset_id;
};

CF_GLOBAL static CF_AsepriteCache* g_ase_cache;

CF_SpriteAsset* cf_sprite_get_asset(uint64_t asset_id)
{
	CF_ASSERT(asset_id < (uint64_t)asize(g_ase_cache->assets));
	CF_SpriteAsset* asset = g_ase_cache->assets + asset_id;
	CF_ASSERT(asset->path);
	return asset;
}

void cf_aseprite_cache_get_pixels(uint64_t image_id, void* buffer, int bytes_to_fill)
{
	auto pixels_ptr = g_ase_cache->id_to_pixels.try_find(image_id);
	if (!pixels_ptr) {
		CF_DEBUG_PRINTF("Aseprite cache -- unable to find id %lld.\n", (long long int)image_id);
		CF_MEMSET(buffer, 0, bytes_to_fill);
	} else {
		void* pixels = *pixels_ptr;
		CF_MEMCPY(buffer, pixels, bytes_to_fill);
	}
}

CF_Image cf_sprite_get_pixels(CF_Sprite* sprite, int blend_index, const char* animation, int frame_index)
{
	CF_Image img = { 0 };
	if (!sprite || sprite->id == CF_SPRITE_ID_INVALID) return img;
	CF_SpriteAsset* asset = cf_sprite_get_asset(sprite->id);
	if (blend_index < 0 || blend_index >= asset->blend_count) return img;
	const CF_Animation* anim = map_get(asset->animations, sintern(animation));
	if (!anim) return img;
	if (frame_index < 0 || frame_index >= asize(anim->frames)) return img;
	int global_frame = frame_index + anim->frame_offset;
	uint64_t id = asset->blend_frame_ids[blend_index][global_frame];
	img.w = sprite->w;
	img.h = sprite->h;
	int bytes = img.w * img.h * (int)sizeof(CF_Pixel);
	img.pix = (CF_Pixel*)CF_ALLOC(bytes);
	cf_aseprite_cache_get_pixels(id, img.pix, bytes);
	return img;
}

void cf_make_aseprite_cache()
{
	g_ase_cache = CF_NEW(CF_AsepriteCache);
}

static void s_free_asset(CF_SpriteAsset* asset)
{
	if (asset->ase) {
		// Ase-owned asset: free animation data.
		CF_Animation** anim_vals = map_items(asset->animations);
		for (int i = 0; i < map_size(asset->animations); ++i) {
			afree(anim_vals[i]->frames);
			CF_FREE(anim_vals[i]);
		}
		map_free(asset->animations);
		cute_aseprite_free(asset->ase);
	}
	// External assets (ase == NULL) don't own their animations.
	// Free blend data (index 0 is asset->frame_ids, skip it).
	for (int i = 1; i < asize(asset->blend_frame_ids); ++i) {
		afree(asset->blend_frame_ids[i]);
	}
	afree(asset->blend_frame_ids);
	for (int i = 0; i < asize(asset->owned_pixel_buffers); ++i) {
		CF_FREE(asset->owned_pixel_buffers[i]);
	}
	afree(asset->owned_pixel_buffers);
	afree(asset->slices);
	afree(asset->pivots);
	afree(asset->center_patches);
	afree(asset->frame_ids);
}

void cf_destroy_aseprite_cache()
{
	for (int i = 0; i < asize(g_ase_cache->assets); ++i) {
		CF_SpriteAsset* asset = g_ase_cache->assets + i;
		if (!asset->path) continue;
		s_free_asset(asset);
	}
	afree(g_ase_cache->assets);
	g_ase_cache->~CF_AsepriteCache();
	CF_FREE(g_ase_cache);
}

static CF_PlayDirection s_play_direction(ase_animation_direction_t direction)
{
	switch (direction) {
	case ASE_ANIMATION_DIRECTION_FORWARDS: return CF_PLAY_DIRECTION_FORWARDS;
	case ASE_ANIMATION_DIRECTION_BACKWORDS: return CF_PLAY_DIRECTION_BACKWARDS;
	case ASE_ANIMATION_DIRECTION_PINGPONG: return CF_PLAY_DIRECTION_PINGPONG;
	}
	return CF_PLAY_DIRECTION_FORWARDS;
}

static void s_sprite(CF_SpriteAsset* asset, CF_Sprite* sprite, uint64_t asset_id)
{
	sprite->name = asset->path;
	sprite->w = asset->w;
	sprite->h = asset->h;
	sprite->id = asset_id;
	if (asset->ase->tag_count == 0) {
		cf_sprite_play(sprite, "default");
	} else {
		CF_Animation** first_anim = map_items(asset->animations);
		cf_sprite_play(sprite, (*first_anim)->name);
	}
}

static CF_Result s_aseprite_cache_load_from_memory(const char* unique_name, const void* data, int sz, CF_Sprite* sprite_out)
{
	ase_t* ase = cute_aseprite_load_from_memory(data, (int)sz, NULL);
	if (!ase) return cf_result_error("Unable to open ase file at `aseprite_path`.");

	// Allocate internal cache data structure entries.
	CF_MAP(CF_Animation*) animations = NULL;
	Array<uint64_t> ids;
	CF_V2* pivots = NULL;
	CF_Aabb* center_patches = NULL;
	afit(pivots, ase->frame_count);
	afit(center_patches, ase->frame_count);
	ids.ensure_capacity(ase->frame_count);

	for (int i = 0; i < ase->frame_count; ++i) {
		// Unique sprite id.
		uint64_t id = g_ase_cache->id_gen++;
		ids.add(id);

		// Premultiply alpha.
		ase_color_t* pix = ase->frames[i].pixels;
		for (int i = 0; i < ase->h; ++i) {
			for (int j = 0; j < ase->w; ++j) {
				float a = pix[i * ase->w + j].a / 255.0f;
				float r = pix[i * ase->w + j].r / 255.0f;
				float g = pix[i * ase->w + j].g / 255.0f;
				float b = pix[i * ase->w + j].b / 255.0f;
				r *= a;
				g *= a;
				b *= a;
				pix[i * ase->w + j].r = (uint8_t)(r * 255.0f);
				pix[i * ase->w + j].g = (uint8_t)(g * 255.0f);
				pix[i * ase->w + j].b = (uint8_t)(b * 255.0f);
			}
		}
		g_ase_cache->id_to_pixels.insert(id, ase->frames[i].pixels);

		// Fill in zero'd out pivots initially. These can get overwritten from slice data
		// if a slice has a pivot.
		apush(pivots, cf_v2(0, 0));
	}

	// Fill out the animation table from the aseprite file.
	if (ase->tag_count) {
		// Each tag represents a single animation.
		for (int i = 0; i < ase->tag_count; ++i) {
			ase_tag_t* tag = ase->tags + i;
			int from = tag->from_frame;
			int to = tag->to_frame;
			CF_Animation* animation = (CF_Animation*)CF_ALLOC(sizeof(CF_Animation));
			CF_MEMSET(animation, 0, sizeof(CF_Animation));

			animation->name = sintern(tag->name);
			animation->play_direction = s_play_direction(tag->loop_animation_direction);
			animation->frame_offset = from;
			for (int i = from; i <= to; ++i) {
				uint64_t id = ids[i];
				CF_Frame frame;
				frame.delay = ase->frames[i].duration_milliseconds / 1000.0f;
				frame.id = id;
				apush(animation->frames, frame);
			}
			map_set(animations, (uint64_t)animation->name, animation);
		}
	} else {
		// Treat the entire frame set as a single animation if there are no tags.
		CF_Animation* animation = (CF_Animation*)CF_ALLOC(sizeof(CF_Animation));
		CF_MEMSET(animation, 0, sizeof(CF_Animation));

		animation->name = sintern("default");
		animation->play_direction = CF_PLAY_DIRECTION_FORWARDS;
		for (int i = 0; i < ase->frame_count; ++i) {
			uint64_t id = ids[i];
			CF_Frame frame;
			frame.delay = ase->frames[i].duration_milliseconds / 1000.0f;
			frame.id = id;
			apush(animation->frames, frame);
		}
		map_set(animations, (uint64_t)animation->name, animation);
	}

	// Fill out slices.
	CF_SpriteSlice* slices = NULL;
	afit(slices, ase->slice_count);
	float sw = (float)ase->w;
	float sh = (float)ase->h;
	for (int i = 0; i < ase->slice_count; ++i) {
		ase_slice_t* slice = ase->slices + i;
		float x = (float)slice->origin_x - sw*0.5f;
		float y = (float)slice->origin_y;
		float w = (float)slice->w;
		float h = (float)slice->h;

		// Invert y-axis since ase saves slice as (0, 0) top-left.
		y = sh - y;
		y = y - sh*0.5f;

		// Record the slice.
		CF_Aabb bb = make_aabb(V2(x,y-h), V2(x+w,y));
		const char* slice_name = sintern(slice->name);
		apush(slices, CF_SpriteSlice {
			.frame_index = slice->frame_number,
			.name = slice_name,
			.box = bb
		});

		if (slice->has_center_as_9_slice) {
			CF_V2 min = cf_v2((float)slice->center_x, (float)slice->center_y);
			CF_V2 max = cf_v2((float)slice->center_x + slice->center_w, (float)slice->center_y + slice->center_h);
			CF_Aabb center_patch = cf_make_aabb(min, max);
			for (int frame_number = slice->frame_number; frame_number < ase->frame_count; ++frame_number) {
				center_patches[frame_number] = center_patch;
			}
		}
		if (slice->has_pivot) {
			v2 pivot = V2((float)slice->pivot_x, (float)slice->pivot_y);
			// Transform from CF's (0, 0) at center to ase's (0, 0) at top-left.
			pivot.x = pivot.x - sw * 0.5f + 0.5f;
			pivot.y = pivot.y - sh * 0.5f + 0.5f;

			for (int frame_number = slice->frame_number; frame_number < ase->frame_count; ++frame_number) {
				pivots[frame_number] = pivot;
			}
		}
	}

	// Build asset and store in flat array.
	uint64_t asset_id = (uint64_t)asize(g_ase_cache->assets);
	CF_SpriteAsset asset = { 0 };
	asset.path = unique_name;
	asset.ase = ase;
	asset.w = ase->w;
	asset.h = ase->h;
	CF_MEMCPY(&asset.animations, &animations, sizeof(animations));
	asset.slices = slices;
	asset.pivots = pivots;
	asset.center_patches = center_patches;
	asset.frame_ids = NULL;
	for (int i = 0; i < ids.size(); ++i) {
		apush(asset.frame_ids, ids[i]);
	}
	asset.blend_count = 1;
	asset.blend_frame_ids = NULL;
	apush(asset.blend_frame_ids, asset.frame_ids);
	asset.owned_pixel_buffers = NULL;
	apush(g_ase_cache->assets, asset);
	g_ase_cache->path_to_asset_id.insert(unique_name, asset_id);

	CF_SpriteAsset* cached_asset = g_ase_cache->assets + asset_id;
	s_sprite(cached_asset, sprite_out, asset_id);
	return cf_result_success();
}

CF_Result cf_aseprite_cache_load_from_memory(const char* unique_name, const void* data, int sz, CF_Sprite* sprite_out)
{
	// First see if this ase was already cached.
	unique_name = sintern(unique_name);
	uint64_t* asset_id_ptr = g_ase_cache->path_to_asset_id.try_find(unique_name);
	if (asset_id_ptr) {
		CF_SpriteAsset* asset = g_ase_cache->assets + *asset_id_ptr;
		s_sprite(asset, sprite_out, *asset_id_ptr);
		return cf_result_success();
	}

	return s_aseprite_cache_load_from_memory(unique_name, data, sz, sprite_out);
}

CF_Result cf_aseprite_cache_load(const char* aseprite_path, CF_Sprite* sprite)
{
	// First see if this ase was already cached.
	aseprite_path = sintern(aseprite_path);
	uint64_t* asset_id_ptr = g_ase_cache->path_to_asset_id.try_find(aseprite_path);
	if (asset_id_ptr) {
		CF_SpriteAsset* asset = g_ase_cache->assets + *asset_id_ptr;
		s_sprite(asset, sprite, *asset_id_ptr);
		return cf_result_success();
	}

	// Load the aseprite file.
	size_t sz = 0;
	void* data = cf_fs_read_entire_file_to_memory(aseprite_path, &sz);
	if (!data) return cf_result_error("Unable to open ase file at `aseprite_path`.");
	CF_DEFER(CF_FREE(data));

	return s_aseprite_cache_load_from_memory(aseprite_path, data, (int)sz, sprite);
}

void cf_aseprite_cache_unload(const char* aseprite_path)
{
	aseprite_path = sintern(aseprite_path);
	uint64_t* asset_id_ptr = g_ase_cache->path_to_asset_id.try_find(aseprite_path);
	if (!asset_id_ptr) return;

	CF_SpriteAsset* asset = g_ase_cache->assets + *asset_id_ptr;

	// Invalidate all frame IDs in the spritebatch.
	for (int i = 0; i < asize(asset->frame_ids); ++i) {
		uint64_t id = asset->frame_ids[i];
		g_ase_cache->id_to_pixels.remove(id);
		spritebatch_invalidate(&s_draw->sb, id);
	}

	s_free_asset(asset);
	asset->path = NULL;
	g_ase_cache->path_to_asset_id.remove(aseprite_path);
}

CF_Result cf_aseprite_cache_load_ase(const char* aseprite_path, ase_t** ase)
{
	aseprite_path = sintern(aseprite_path);
	CF_Sprite s = cf_sprite_defaults();
	CF_Result err = cf_aseprite_cache_load(aseprite_path, &s);
	if (cf_is_error(err)) return err;

	uint64_t* asset_id_ptr = g_ase_cache->path_to_asset_id.try_find(aseprite_path);
	if (!asset_id_ptr) return cf_result_error("Unable to load aseprite.");
	*ase = (g_ase_cache->assets + *asset_id_ptr)->ase;
	return cf_result_success();
}

void cf_aseprite_cache_reload_asset(uint64_t asset_id, ase_t* new_ase)
{
	CF_SpriteAsset* asset = g_ase_cache->assets + asset_id;
	CF_ASSERT(asset->path);
	CF_ASSERT(asset->ase); // Must be an ase-owned asset.

	int old_count = asize(asset->frame_ids);
	int new_count = new_ase->frame_count;
	int common = old_count < new_count ? old_count : new_count;

	// Reuse existing frame IDs for overlapping frames, replacing pixel data.
	for (int i = 0; i < common; ++i) {
		uint64_t id = asset->frame_ids[i];
		g_ase_cache->id_to_pixels.insert(id, new_ase->frames[i].pixels);
		spritebatch_invalidate(&s_draw->sb, id);
	}

	// Allocate new IDs for additional frames.
	for (int i = common; i < new_count; ++i) {
		uint64_t id = g_ase_cache->id_gen++;
		apush(asset->frame_ids, id);
		g_ase_cache->id_to_pixels.insert(id, new_ase->frames[i].pixels);
	}

	// Remove excess old frame IDs.
	for (int i = common; i < old_count; ++i) {
		uint64_t id = asset->frame_ids[i];
		g_ase_cache->id_to_pixels.remove(id);
		spritebatch_invalidate(&s_draw->sb, id);
	}
	if (new_count < old_count) {
		asetlen(asset->frame_ids, new_count);
	}

	// Free old animation data.
	CF_Animation** old_anim_vals = map_items(asset->animations);
	for (int i = 0; i < map_size(asset->animations); ++i) {
		afree(old_anim_vals[i]->frames);
		CF_FREE(old_anim_vals[i]);
	}
	map_free(asset->animations);
	afree(asset->slices);
	afree(asset->pivots);
	afree(asset->center_patches);

	// Rebuild animations from new ase.
	CF_MAP(CF_Animation*) animations = NULL;
	if (new_ase->tag_count) {
		for (int i = 0; i < new_ase->tag_count; ++i) {
			ase_tag_t* tag = new_ase->tags + i;
			int from = tag->from_frame;
			int to = tag->to_frame;
			CF_Animation* animation = (CF_Animation*)CF_ALLOC(sizeof(CF_Animation));
			CF_MEMSET(animation, 0, sizeof(CF_Animation));
			animation->name = sintern(tag->name);
			animation->play_direction = s_play_direction(tag->loop_animation_direction);
			animation->frame_offset = from;
			for (int j = from; j <= to; ++j) {
				CF_Frame frame;
				frame.delay = new_ase->frames[j].duration_milliseconds / 1000.0f;
				frame.id = asset->frame_ids[j];
				apush(animation->frames, frame);
			}
			map_set(animations, (uint64_t)animation->name, animation);
		}
	} else {
		CF_Animation* animation = (CF_Animation*)CF_ALLOC(sizeof(CF_Animation));
		CF_MEMSET(animation, 0, sizeof(CF_Animation));
		animation->name = sintern("default");
		animation->play_direction = CF_PLAY_DIRECTION_FORWARDS;
		for (int i = 0; i < new_count; ++i) {
			CF_Frame frame;
			frame.delay = new_ase->frames[i].duration_milliseconds / 1000.0f;
			frame.id = asset->frame_ids[i];
			apush(animation->frames, frame);
		}
		map_set(animations, (uint64_t)animation->name, animation);
	}
	CF_MEMCPY(&asset->animations, &animations, sizeof(animations));

	// Rebuild pivots and center_patches.
	CF_V2* pivots = NULL;
	CF_Aabb* center_patches = NULL;
	afit(pivots, new_count);
	afit(center_patches, new_count);
	for (int i = 0; i < new_count; ++i) {
		apush(pivots, cf_v2(0, 0));
		CF_Aabb zero_aabb = { 0 };
		apush(center_patches, zero_aabb);
	}

	// Rebuild slices.
	CF_SpriteSlice* slices = NULL;
	afit(slices, new_ase->slice_count);
	float sw = (float)new_ase->w;
	float sh = (float)new_ase->h;
	for (int i = 0; i < new_ase->slice_count; ++i) {
		ase_slice_t* slice = new_ase->slices + i;
		float x = (float)slice->origin_x - sw * 0.5f;
		float y = (float)slice->origin_y;
		float w = (float)slice->w;
		float h = (float)slice->h;
		y = sh - y;
		y = y - sh * 0.5f;
		CF_Aabb bb = make_aabb(V2(x, y - h), V2(x + w, y));
		const char* slice_name = sintern(slice->name);
		apush(slices, CF_SpriteSlice {
			.frame_index = slice->frame_number,
			.name = slice_name,
			.box = bb
		});
		if (slice->has_center_as_9_slice) {
			CF_V2 min = cf_v2((float)slice->center_x, (float)slice->center_y);
			CF_V2 max = cf_v2((float)slice->center_x + slice->center_w, (float)slice->center_y + slice->center_h);
			CF_Aabb center_patch = cf_make_aabb(min, max);
			for (int frame_number = slice->frame_number; frame_number < new_count; ++frame_number) {
				center_patches[frame_number] = center_patch;
			}
		}
		if (slice->has_pivot) {
			v2 pivot = V2((float)slice->pivot_x, (float)slice->pivot_y);
			pivot.x = pivot.x - sw * 0.5f + 0.5f;
			pivot.y = pivot.y - sh * 0.5f + 0.5f;
			for (int frame_number = slice->frame_number; frame_number < new_count; ++frame_number) {
				pivots[frame_number] = pivot;
			}
		}
	}
	asset->slices = slices;
	asset->pivots = pivots;
	asset->center_patches = center_patches;

	// Swap in the new ase, free the old one.
	cute_aseprite_free(asset->ase);
	asset->ase = new_ase;
	asset->w = new_ase->w;
	asset->h = new_ase->h;
}

uint64_t cf_aseprite_layer_mask(ase_t* ase, const char** layer_names, int count)
{
	uint64_t mask = 0;
	for (int i = 0; i < count; ++i) {
		for (int li = 0; li < ase->layer_count; ++li) {
			if (strcmp(ase->layers[li].name, layer_names[i]) == 0) {
				mask |= (1ULL << li);
			}
		}
	}
	return mask;
}

int cf_aseprite_cache_add_blend(const char* path, uint64_t layer_mask)
{
	path = sintern(path);
	uint64_t* asset_id_ptr = g_ase_cache->path_to_asset_id.try_find(path);
	CF_ASSERT(asset_id_ptr);
	CF_SpriteAsset* asset = g_ase_cache->assets + *asset_id_ptr;
	ase_t* ase = asset->ase;
	CF_ASSERT(ase);

	int frame_count = ase->frame_count;
	int pixel_bytes = ase->w * ase->h * (int)sizeof(ase_color_t);

	// Allocate pixel buffers and blend layers.
	ase_color_t** pixel_bufs = (ase_color_t**)CF_ALLOC(sizeof(ase_color_t*) * frame_count);
	for (int i = 0; i < frame_count; ++i) {
		pixel_bufs[i] = (ase_color_t*)CF_ALLOC(pixel_bytes);
		CF_MEMSET(pixel_bufs[i], 0, pixel_bytes);
	}
	cute_aseprite_blend_layers(ase, layer_mask, pixel_bufs);

	// Generate IDs, premultiply alpha, register pixels.
	dyna uint64_t* blend_ids = NULL;
	for (int i = 0; i < frame_count; ++i) {
		// Premultiply alpha.
		ase_color_t* pix = pixel_bufs[i];
		for (int p = 0; p < ase->w * ase->h; ++p) {
			float a = pix[p].a / 255.0f;
			pix[p].r = (uint8_t)(pix[p].r / 255.0f * a * 255.0f);
			pix[p].g = (uint8_t)(pix[p].g / 255.0f * a * 255.0f);
			pix[p].b = (uint8_t)(pix[p].b / 255.0f * a * 255.0f);
		}

		uint64_t id = g_ase_cache->id_gen++;
		g_ase_cache->id_to_pixels.insert(id, pixel_bufs[i]);
		apush(blend_ids, id);
		apush(asset->owned_pixel_buffers, (void*)pixel_bufs[i]);
	}
	CF_FREE(pixel_bufs);

	apush(asset->blend_frame_ids, blend_ids);
	int blend_index = asset->blend_count++;
	return blend_index;
}

uint64_t cf_register_external_sprite_asset(const char* name, int w, int h, CF_MAP(CF_Animation*) animations)
{
	name = sintern(name);
	uint64_t* existing = g_ase_cache->path_to_asset_id.try_find(name);
	if (existing) return *existing;

	uint64_t asset_id = (uint64_t)asize(g_ase_cache->assets);
	CF_SpriteAsset asset;
	CF_MEMSET(&asset, 0, sizeof(asset));
	asset.path = name;
	asset.ase = NULL; // External: caller owns animations.
	asset.w = w;
	asset.h = h;
	CF_MEMCPY(&asset.animations, &animations, sizeof(animations));
	apush(g_ase_cache->assets, asset);
	g_ase_cache->path_to_asset_id.insert(name, asset_id);
	return asset_id;
}
