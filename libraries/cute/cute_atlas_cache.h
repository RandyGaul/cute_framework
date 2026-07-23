/*
	------------------------------------------------------------------------------
		Licensing information can be found at the end of the file.
	------------------------------------------------------------------------------

	cute_atlas_cache.h - v1.11

	To create implementation (the function definitions)
		#define ATLAS_CACHE_IMPLEMENTATION
	in *one* C/CPP file (translation unit) that includes this file

	SUMMARY:

		This header implements a runtime texture atlas cache. Images are referenced by a
		unique id; the atlas cache packs the images actually in use into large atlas
		textures on the fly, decays them out when they stop being used, and reports back
		texture handles + uv coordinates grouped by atlas. This avoids compiling texture
		atlases as a pre-process step, letting the game load images up individually,
		dramatically simplifying art pipelines.

		Think of it as an atlas *tool*: you tell it which images you are drawing this
		frame (`atlas_cache_push`), and it tells you which texture to bind and which uvs
		to use for each (`atlas_cache_flush` + your callback), while transparently
		managing atlas residency behind the scenes. What geometry you render with those
		uvs -- quads, command buffers, whatever -- is entirely your business; each entry
		carries a single opaque `udata` value so you can correlate reported entries back
		to your own draw data.

	MORE DETAILS:

		`atlas_cache_push` is used to push entries into a buffer. Rendering works
		by calling `atlas_cache_flush`. `atlas_cache_flush` will use a user-supplied
		callback to report batches. This callback is of type `submit_batch_fn`. The
		batches are reported as an array of `atlas_cache_entry_t` entries with valid
		texture ids and uvs filled in. Entries in a batch share the same texture handle
		(either from the same base image, or from the same internal atlas).

		cute_atlas_cache does not know anything about how to generate texture handles, or
		destroy them. As such, the user must supply two callbacks for creating handles and
		destroying them. These can be simple wrappers around, for example, `glGenTextures`
		and `glDeleteTextures`.

		Finally, cute_atlas_cache will periodically need access to pixels from images. These
		pixels are used to generate textures, or to build atlases (which in turn generate a
		texture). cute_atlas_cache does not need to know much about your images, other than
		the pixel stride. The user supplies callback of type `get_pixels_fn`, which lets
		cute_atlas_cache retreive the pixels associated with a particular image. The pixels
		can be stored in RAM and handed to cute_atlas_cache whenever requested, or the pixels
		can be fetched directly from disk and handed to cute_atlas_cache. It doesn't matter
		to cute_atlas_cache. Since `get_pixels_fn` can be called from `atlas_cache_flush` it
		is recommended to avoid file i/o within the `get_pixels_fn` callback, and instead try
		to already have pixels ready in RAM.

		The `atlas_cache_defrag` function performs atlas creation and texture management. It
		should be called periodically. It can be called once per game tick (once per render),
		or optionally called at a different frequency (once every N game ticks).

	PROS AND CONS:

		PROS
		- Texture atlases are completely hidden behind an api. The api in this header can
		  easily be implemented with different backends. For example on
		  some platforms bindless textures can be utilized in order to avoid texture
		  atlases entirely! Code using this API can have the backend implementation swapped
		  without requiring any user code to change.
		- Entries are batched in an effective manner to dramatically reduce draw call counts.
		- Supporting hotswapping or live-reloading of images can be trivialized due to
		  moving atlas creation out of the art-pipeline and into the run-time.
		- Since atlases are built at run-time and continually maintained, images are
		  guaranteed to be drawn at the same time on-screen as their atlas neighbors. This is
		  typically not the case for atlas preprocessors, as a *guess* must be made to try
		  and organize images together in atlases that need to be drawn at roughly the same
		  time.

		CONS
		- Performance hits in the `atlas_cache_defrag` function, and a little as well in
		  the `atlas_cache_flush` function. Extra run-time memory usage for bookkeeping,
		  which implies a RAM hit as well as more things to clog the CPU cache.
		- If each texture comes from a separate image on-disk, opening individual files on
		  disk can be very slow. For example on Windows just performing permissions and
		  related work to open a file is time-consuming. This can be mitigated by moving
		  assets into a single larger file, for example a .zip archive and read from using
		  a file io abstraction like PHYSFS.
		- For large numbers of separate images, some file abstraction is necessary to avoid
		  a large performance hit on opening/closing many individual files. This problem is
		  *not* solved by cute_atlas_cache.h, and instead should be solved by some separate
		  file abstraction system. PHYSFS is a good example of a solid file io abstraction.

	EXAMPLE USAGE:

		atlas_cache_config_t config;
		atlas_cache_set_default_config(&config);
		config.batch_callback = my_report_batches_function;
		config.get_pixels_callback = my_get_pixels_function;
		config.generate_texture_callback = my_make_texture_handle_function;
		config.delete_texture_callback = my_destroy_texture_handle_function;

		atlas_cache_t cache;
		atlas_cache_init(&cache, &config, NULL);

		while (game_is_running)
		{
			for (int i = 0; i < draw_count; ++i)
			{
				atlas_cache_entry_t entry = { 0 };
				entry.image_id = draws[i].image_id;
				entry.w = draws[i].image_width_in_pixels;
				entry.h = draws[i].image_height_in_pixels;
				entry.minx = 0; entry.miny = 0; // Local uv space -- 0 to 1 covers the
				entry.maxx = 1; entry.maxy = 1; // whole image (except premade atlases).
				entry.udata = (ATLAS_CACHE_U64)i; // Opaque, handed back in reported batches.
				atlas_cache_push(&cache, entry);
			}

			atlas_cache_tick(&cache);
			atlas_cache_defrag(&cache);
			atlas_cache_flush(&cache);
		}

	CUSTOMIZATION:

		The following macros can be defined before including this header with the
		ATLAS_CACHE_IMPLEMENTATION symbol defined, in order to customize the internal
		behavior of cute_atlas_cache.h. Search this header to find how each macro is
		defined and used. Note that MALLOC/FREE functions can optionally take a context
		parameter for custom allocation.

		ATLAS_CACHE_MALLOC
		ATLAS_CACHE_MEMCPY
		ATLAS_CACHE_MEMSET
		ATLAS_CACHE_MEMMOVE
		ATLAS_CACHE_ASSERT
		ATLAS_CACHE_ATLAS_FLIP_Y_AXIS_FOR_UV
		ATLAS_CACHE_ATLAS_EMPTY_COLOR
		ATLAS_CACHE_LOG

	Revision history:
		0.01 (11/20/2017) experimental release
		1.00 (04/14/2018) initial release
		1.01 (05/07/2018) modification for easier file embedding
		1.02 (02/03/2019) moved def of atlas_cache_t for easier embedding,
		                  inverted get pixels callback to let users have an easier time
		                  with memory management, added support for pixel padding along
		                  the edges of all textures (useful for certain shader effects)
		1.03 (08/18/2020) refactored `atlas_cache_push` so that sprites can have userdata
		1.04 (08/20/2021) qsort -> mergesort to avoid sort bugs, optional override
		                  `sprites_sorter_fn` sorting routines provided by Kariem, added
		                  new function `atlas_cache_prefetch`
		1.05 (12/10/2022) added `ATLAS_CACHE_ENTRY_GEOMETRY`, a way to put custom geom-
		                  etry in sprites. Added `atlas_cache_register_premade_atlas`, a
		                  way to inject premade atlases into atlas_cache
		1.06 (03/24/2023) added `atlas_cache_invalidate`, useful for updating pixels NOW
		1.07 (10/01/2024) Removed public `sort_bits` since it's not actually useful, and
		                  reused it internally to implement stable sorting (bugfix).
		1.08 (02/01/2026) Replaced external hashtable.h with simpler internal atlas_cache_map_t,
		                  fixed memory context bugs in atlas_cache_term and get_pixels.
		1.09 (07/21/2026) Reframed as a pure runtime atlas cache. The geometry payload
		                  (ATLAS_CACHE_ENTRY_GEOMETRY) is now documented as an opaque
		                  user correlation payload rather than "the sprite's geometry" --
		                  renderers that keep their own draw streams (e.g. command-buffer
		                  renderers) should pass a small index/id here and look their own
		                  data up when batches are reported. The default quad geometry
		                  remains for standalone users.
		1.10 (07/22/2026) Stripped out the last of the old spritebatch heritage. Removed
		                  the ATLAS_CACHE_ENTRY_GEOMETRY macro machinery (and its default
		                  quad geometry), ATLAS_CACHE_SPRITE_USERDATA, the optional
		                  `sprites_sorter_fn` callback, `atlas_cache_reset_function_ptrs`,
		                  and `atlas_cache_clear`. Entries now carry one opaque u64
		                  `udata` for correlating reported batches with your own draw
		                  data. Renamed internals from sprite to entry terminology (in-
		                  cluding the old `sb` param, now `cache`), fixed latent compile
		                  errors in atlas creation for custom ATLAS_CACHE_MALLOC macros
		                  that use their context parameter, and fixed `atlas_cache_fetch`
		                  to return uv coordinates for premade images.
		1.11 (07/23/2026) Fixed a use-after-free in `atlas_cache_defrag`: the lonely
		                  buffer's items pointer was captured before input processing,
		                  which can insert new entries and reallocate the map's backing
		                  storage; atlas creation then read the freed array and packed
		                  garbage blocks (visible as corrupted entries once enough
		                  distinct images accumulated to cross the map's capacity).
*/

/*
	Contributors:
		Kariem            1.04 - Optional sorter function `sprites_sorter_fn`
		Kariem            1.05 - Initial work on premade atlases
*/

#ifndef ATLAS_CACHE_H

#ifndef ATLAS_CACHE_U64
	#define ATLAS_CACHE_U64 unsigned long long
#endif // ATLAS_CACHE_U64

typedef struct atlas_cache_t atlas_cache_t;
typedef struct atlas_cache_config_t atlas_cache_config_t;
typedef struct atlas_cache_entry_t atlas_cache_entry_t;

// Entries are pushed into the atlas cache with this struct. All the fields
// should be set before calling `atlas_cache_push`, though `texture_id` and
// `sort_bits` can simply be set to zero.
//
// After entries are pushed via `atlas_cache_push` they are sorted, `texture_id`
// is assigned to a generated atlas, and each entry is handed back to you via the
// `submit_batch_fn` callback.
struct atlas_cache_entry_t
{
	// `image_id` must be a unique identifier for the image an entry references.
	// You must set this value!
	ATLAS_CACHE_U64 image_id;

	// `texture_id` can be set to zero. This value will be overwritten with a valid
	// texture id before batches are reported back to you. This id will map to an
	// atlas created internally. For premade atlases you must set this yourself.
	ATLAS_CACHE_U64 texture_id;

	// For internal use. Will get overwritten.
	int sort_bits;

	// Opaque user payload, never touched internally and handed back in reported
	// batches. Use it to correlate entries with your own draw data, e.g. an index
	// into your own array.
	ATLAS_CACHE_U64 udata;

	int w, h;         // width and height of this entry's image in pixels
	float minx, miny; // u coordinate - this will be overwritten, except for premade entries
	float maxx, maxy; // v coordinate - this will be overwritten, except for premade entries
};

// Pushes an entry onto an internal buffer. Does no other logic.
void atlas_cache_push(atlas_cache_t* cache, atlas_cache_entry_t entry);

// Ensures the image associated with your unique `image_id` is loaded up into the cache. This
// function pretends to draw an entry referencing `image_id` but doesn't actually do any
// drawing at all. Use this function as an optimization to pre-load images you know will be
// drawn very soon, e.g. prefetch all ten images within a single animation just as it starts
// playing.
void atlas_cache_prefetch(atlas_cache_t* cache, ATLAS_CACHE_U64 image_id, int w, int h);

// Useful for re-uploading pixels to the GPU.
// Invalidates the internal cache for a specific image. If this image resides in a texture atlas
// the entire atlas is recompiled. If the image resides in the lonely buffer only the individual
// texture gets recreated. You may want to beef up `lonely_buffer_count_till_flush` in the config
// `atlas_cache_config_t` if you want to invalidate images often -- this can help prevent constantly
// invalidating internal atlases and recompiling them, and instead get your dynamic textures into the
// lonely buffer.
void atlas_cache_invalidate(atlas_cache_t* cache, ATLAS_CACHE_U64 image_id);

// If a match for `image_id` is found, the texture id and uv coordinates are looked up and returned
// as an entry. This is sometimes useful to render images through an external mechanism, such as
// Dear ImGui. The return result will be valid until the next call to `atlas_cache_defrag`.
atlas_cache_entry_t atlas_cache_fetch(atlas_cache_t* cache, ATLAS_CACHE_U64 image_id, int w, int h);

// Increments internal timestamps on all textures, for use in `atlas_cache_defrag`.
void atlas_cache_tick(atlas_cache_t* cache);

// Sorts the internal entries and flushes the buffer built by `atlas_cache_push`. Will call
// the `submit_batch_fn` function for each batch of entries and return them as an array. Any `image_id`
// within the `atlas_cache_push` buffer that do not yet have a texture handle will request pixels
// from the image via `get_pixels_fn` and request a texture handle via `generate_texture_handle_fn`.
// Returns the number of batches created and submitted.
int atlas_cache_flush(atlas_cache_t* cache);

// All textures created so far by `atlas_cache_flush` will be considered as candidates for creating
// new internal texture atlases. Internal texture atlases compress images together inside of one
// texture to dramatically reduce draw calls. When an atlas is created, the most recently used `image_id`
// instances are prioritized, to ensure atlases are filled with images all drawn at the same time.
// As some textures cease to draw on screen, they "decay" over time. Once enough images in an atlas
// decay, the atlas is removed, and any "live" images in the atlas are used to create new atlases.
// Can be called every 1/N times `atlas_cache_flush` is called.
int atlas_cache_defrag(atlas_cache_t* cache);

int atlas_cache_init(atlas_cache_t* cache, atlas_cache_config_t* config, void* udata);
void atlas_cache_term(atlas_cache_t* cache);

typedef struct atlas_cache_premade_entry_t
{
	// `image_id` must be a unique identifier for the image an entry references. This id is *not* local
	// to a particular atlas, and instead is global to the entire atlas cache.
	ATLAS_CACHE_U64 image_id;
	int w, h;         // width and height of this entry's image in pixels
	float minx, miny; // u coordinate in the premade atlas
	float maxx, maxy; // v coordinate in the premade atlas
} atlas_cache_premade_entry_t;

// Registers a premade atlas into the atlas cache. This function is provided here mostly for
// convenience. Sometimes you may already have a pre-created atlas and want a simple way to draw all
// of its images through the cache.
//
// In terms of performance, premade atlases provide another way to tune the performance of batching.
// If you know ahead of time it's better to inject a premade atlas into the cache, instead of
// using `atlas_cache_push`, this can be a good option. For example, this makes sense for text
// rendering systems that create their own font texture atlas. Understanding the performance impact
// can become a lot simpler than flooding `atlas_cache_push` with a lot of unique glyphs used briefly.
void atlas_cache_register_premade_atlas(atlas_cache_t* cache, ATLAS_CACHE_U64 texture_id, int w, int h, int entry_count, atlas_cache_premade_entry_t* entries);

// Batches are submit via synchronous callback back to the user. This function is called
// from inside `atlas_cache_flush`. Each time `submit_batch_fn` is called an array of entries
// is handed to the user. The entries are intended to be further sorted by the user as desired
// (for example, additional sorting based on depth). `w` and `h` are the width/height, respectively,
// of the texture the batch of entries resides upon. w/h can be useful for knowing texture dim-
// ensions, which is needed to know texel size or other measurements.
typedef void (submit_batch_fn)(atlas_cache_entry_t* entries, int count, int texture_w, int texture_h, void* udata);

// cute_atlas_cache.h needs to know how to get the pixels of an image, generate textures handles (for
// example glGenTextures for OpenGL), and destroy texture handles. These functions are all called
// from within the `atlas_cache_defrag` function, and sometimes from `atlas_cache_flush`.

// Called when the pixels are needed from the user. `image_id` maps to a unique image, and is *not*
// related to `texture_id` at all. `buffer` must be filled in with `bytes_to_fill` number of bytes.
// The user is assumed to know the width/height of the image, and can optionally verify that
// `bytes_to_fill` matches the user's w * h * stride for this particular image.
typedef void (get_pixels_fn)(ATLAS_CACHE_U64 image_id, void* buffer, int bytes_to_fill, void* udata);

// Called with a new texture handle is needed. This will happen whenever a new atlas is created,
// and whenever new `image_id`s first appear to cute_atlas_cache, and have yet to find their way
// into an appropriate atlas.
typedef ATLAS_CACHE_U64 (generate_texture_handle_fn)(void* pixels, int w, int h, void* udata);

// Called whenever a texture handle is ready to be free'd up. This happens whenever a particular image
// or a particular atlas has not been used for a while, and is ready to be released.
typedef void (destroy_texture_handle_fn)(ATLAS_CACHE_U64 texture_id, void* udata);

// Initializes a set of good default paramaters. The users must still set
// the four callbacks inside of `config`.
void atlas_cache_set_default_config(atlas_cache_config_t* config);

struct atlas_cache_config_t
{
	int pixel_stride;
	int atlas_width_in_pixels;
	int atlas_height_in_pixels;
	int atlas_use_border_pixels;
	int ticks_to_decay_texture;         // number of ticks it takes for a texture handle to be destroyed via `destroy_texture_handle_fn`
	int lonely_buffer_count_till_flush; // Number of unique textures allowed to persist that are not a part of an atlas yet, each one allowed is another draw call.
	                                    // These are called "lonely textures", since they don't belong to any atlas yet. Set this to 0 if you want all textures to be
	                                    // immediately put into atlases. Setting a higher number, like 64, will buffer up 64 unique textures (which means up to an
	                                    // additional 64 draw calls) before flushing them into atlases. Too low of a lonely buffer count combined with a low tick
	                                    // to decay rate will cause performance problems where atlases are constantly created and immedately destroyed -- you have
	                                    // been warned! Use `ATLAS_CACHE_LOG` to gain some insight on what's going on inside the atlas_cache when tuning these settings.
	float ratio_to_decay_atlas;         // from 0 to 1, once ratio is less than `ratio_to_decay_atlas`, flush active textures in atlas to lonely buffer
	float ratio_to_merge_atlases;       // from 0 to 0.5, attempts to merge atlases with some ratio of empty space
	submit_batch_fn* batch_callback;
	get_pixels_fn* get_pixels_callback;
	generate_texture_handle_fn* generate_texture_callback;
	destroy_texture_handle_fn* delete_texture_callback;
	void* allocator_context;
};

#define ATLAS_CACHE_H
#endif

#if !defined(ATLAS_CACHE_INTERNAL_H)

// atlas_cache_map_t - A simple hash map for ATLAS_CACHE_U64 keys to fixed-size values.
// Uses open addressing with linear probing and maintains a dense item array for fast iteration.

typedef struct atlas_cache_map_slot_t
{
	unsigned key_hash;
	int item_index;
	int base_count;
} atlas_cache_map_slot_t;

typedef struct atlas_cache_map_t
{
	void* mem_ctx;
	int count;
	int item_size;
	atlas_cache_map_slot_t* slots;
	int slot_capacity;
	ATLAS_CACHE_U64* keys;
	int* item_slots;
	void* items;
	int item_capacity;
} atlas_cache_map_t;

typedef struct atlas_cache_internal_entry_t
{
	ATLAS_CACHE_U64 image_id;
	int sort_bits;
	ATLAS_CACHE_U64 udata;
	int w, h;                         // w/h of image in pixels
	float premade_minx, premade_miny; // u coordinate for premade
	float premade_maxx, premade_maxy; // v coordinate for premade
} atlas_cache_internal_entry_t;

typedef struct atlas_cache_internal_texture_t
{
	int timestamp;
	int w, h;
	float minx, miny;
	float maxx, maxy;
	ATLAS_CACHE_U64 image_id;
} atlas_cache_internal_texture_t;

typedef struct atlas_cache_internal_atlas_t
{
	ATLAS_CACHE_U64 texture_id;
	float volume_ratio;
	atlas_cache_map_t textures;
	struct atlas_cache_internal_atlas_t* next;
	struct atlas_cache_internal_atlas_t* prev;
} atlas_cache_internal_atlas_t;

typedef struct atlas_cache_internal_lonely_texture_t
{
	int timestamp;
	int w, h;
	ATLAS_CACHE_U64 image_id;
	ATLAS_CACHE_U64 texture_id;
} atlas_cache_internal_lonely_texture_t;

typedef struct atlas_cache_internal_premade_t
{
	int atlas_w, atlas_h;
	float minx, miny;
	float maxx, maxy;
	ATLAS_CACHE_U64 texture_id;
} atlas_cache_internal_premade_t;

struct atlas_cache_t
{
	int input_count;
	int input_capacity;
	atlas_cache_internal_entry_t* input_buffer;

	int entry_count;
	int entry_capacity;
	atlas_cache_entry_t* entries;
	atlas_cache_entry_t* entries_scratch;

	int key_buffer_count;
	int key_buffer_capacity;
	ATLAS_CACHE_U64* key_buffer;

	int pixel_buffer_size; // number of pixels
	void* pixel_buffer;

	atlas_cache_map_t premades;
	atlas_cache_map_t lonely_buffer;
	atlas_cache_map_t image_to_atlas;

	atlas_cache_internal_atlas_t* atlases;

	int sort_id;
	int pixel_stride;
	int atlas_width_in_pixels;
	int atlas_height_in_pixels;
	int atlas_use_border_pixels;
	int ticks_to_decay_texture;
	int lonely_buffer_count_till_flush;
	int lonely_buffer_count_till_decay;
	float ratio_to_decay_atlas;
	float ratio_to_merge_atlases;
	submit_batch_fn* batch_callback;
	get_pixels_fn* get_pixels_callback;
	generate_texture_handle_fn* generate_texture_callback;
	destroy_texture_handle_fn* delete_texture_callback;
	void* mem_ctx;
	void* udata;
};

#ifndef _CRT_SECURE_NO_WARNINGS
	#define _CRT_SECURE_NO_WARNINGS
#endif

#ifndef _CRT_NONSTDC_NO_DEPRECATE
	#define _CRT_NONSTDC_NO_DEPRECATE
#endif

#ifndef ATLAS_CACHE_MALLOC
	#include <stdlib.h>
	#define ATLAS_CACHE_MALLOC(size, ctx) malloc(size)
	#define ATLAS_CACHE_FREE(ptr, ctx) free(ptr)
#endif

#ifndef ATLAS_CACHE_MEMCPY
	#include <string.h>
	#define ATLAS_CACHE_MEMCPY(dst, src, n) memcpy(dst, src, n)
#endif

#ifndef ATLAS_CACHE_MEMSET
	#include <string.h>
	#define ATLAS_CACHE_MEMSET(ptr, val, n) memset(ptr, val, n)
#endif

#ifndef ATLAS_CACHE_MEMMOVE
	#include <string.h>
	#define ATLAS_CACHE_MEMMOVE(dst, src, n) memmove(dst, src, n)
#endif

#ifndef ATLAS_CACHE_ASSERT
	#include <assert.h>
	#define ATLAS_CACHE_ASSERT(condition) assert(condition)
#endif

// flips output uv coordinate's y. Can be useful to "flip image on load"
#ifndef ATLAS_CACHE_ATLAS_FLIP_Y_AXIS_FOR_UV
	#define ATLAS_CACHE_ATLAS_FLIP_Y_AXIS_FOR_UV 1
#endif

// flips output uv coordinate's y. Can be useful to "flip image on load"
#ifndef ATLAS_CACHE_LONELY_FLIP_Y_AXIS_FOR_UV
	#define ATLAS_CACHE_LONELY_FLIP_Y_AXIS_FOR_UV 1
#endif

#ifndef ATLAS_CACHE_ATLAS_EMPTY_COLOR
	#define ATLAS_CACHE_ATLAS_EMPTY_COLOR 0x00000000
#endif

#ifndef ATLAS_CACHE_LOG
	#if 0
		#define ATLAS_CACHE_LOG printf
	#else
		#define ATLAS_CACHE_LOG(...)
	#endif
#endif

#define ATLAS_CACHE_INTERNAL_H
#endif

#ifdef ATLAS_CACHE_IMPLEMENTATION
#ifndef ATLAS_CACHE_IMPLEMENTATION_ONCE
#define ATLAS_CACHE_IMPLEMENTATION_ONCE

// atlas_cache_map implementation

static unsigned atlas_cache_map_pow2ceil(unsigned v)
{
	--v;
	v |= v >> 1;
	v |= v >> 2;
	v |= v >> 4;
	v |= v >> 8;
	v |= v >> 16;
	++v;
	v += (v == 0);
	return v;
}

static unsigned atlas_cache_map_hash(ATLAS_CACHE_U64 key)
{
	key = (~key) + (key << 18);
	key = key ^ (key >> 31);
	key = key * 21;
	key = key ^ (key >> 11);
	key = key + (key << 6);
	key = key ^ (key >> 22);
	ATLAS_CACHE_ASSERT(key);
	return (unsigned)key;
}

static void atlas_cache_map_init(atlas_cache_map_t* map, int item_size, int initial_capacity, void* mem_ctx)
{
	initial_capacity = (int)atlas_cache_map_pow2ceil(initial_capacity >= 0 ? (unsigned)initial_capacity : 32U);
	map->mem_ctx = mem_ctx;
	map->count = 0;
	map->item_size = item_size;
	map->slot_capacity = (int)atlas_cache_map_pow2ceil((unsigned)(initial_capacity + initial_capacity / 2));
	int slots_size = (int)(map->slot_capacity * sizeof(*map->slots));
	map->slots = (atlas_cache_map_slot_t*)ATLAS_CACHE_MALLOC(slots_size, mem_ctx);
	ATLAS_CACHE_ASSERT(map->slots);
	ATLAS_CACHE_MEMSET(map->slots, 0, slots_size);
	map->item_capacity = (int)atlas_cache_map_pow2ceil((unsigned)initial_capacity);
	map->keys = (ATLAS_CACHE_U64*)ATLAS_CACHE_MALLOC(map->item_capacity * (sizeof(*map->keys) + sizeof(*map->item_slots) + map->item_size), mem_ctx);
	ATLAS_CACHE_ASSERT(map->keys);
	map->item_slots = (int*)(map->keys + map->item_capacity);
	map->items = (void*)(map->item_slots + map->item_capacity);
}


static void atlas_cache_map_term(atlas_cache_map_t* map)
{
	ATLAS_CACHE_FREE(map->keys, map->mem_ctx);
	ATLAS_CACHE_FREE(map->slots, map->mem_ctx);
}

static int atlas_cache_map_find_slot(atlas_cache_map_t const* map, ATLAS_CACHE_U64 key)
{
	int slot_mask = map->slot_capacity - 1;
	unsigned hash = atlas_cache_map_hash(key);
	int base_slot = (int)(hash & (unsigned)slot_mask);
	int base_count = map->slots[base_slot].base_count;
	int slot = base_slot;

	while (base_count > 0) {
		unsigned slot_hash = map->slots[slot].key_hash;
		if (slot_hash) {
			int slot_base = (int)(slot_hash & (unsigned)slot_mask);
			if (slot_base == base_slot) {
				--base_count;
				if (slot_hash == hash && map->keys[map->slots[slot].item_index] == key)
					return slot;
			}
		}
		slot = (slot + 1) & slot_mask;
	}
	return -1;
}

static void atlas_cache_map_expand_slots(atlas_cache_map_t* map)
{
	int old_capacity = map->slot_capacity;
	atlas_cache_map_slot_t* old_slots = map->slots;

	map->slot_capacity *= 2;
	int slot_mask = map->slot_capacity - 1;
	int size = (int)(map->slot_capacity * sizeof(*map->slots));
	map->slots = (atlas_cache_map_slot_t*)ATLAS_CACHE_MALLOC(size, map->mem_ctx);
	ATLAS_CACHE_ASSERT(map->slots);
	ATLAS_CACHE_MEMSET(map->slots, 0, size);

	for (int i = 0; i < old_capacity; ++i) {
		unsigned hash = old_slots[i].key_hash;
		if (hash) {
			int base_slot = (int)(hash & (unsigned)slot_mask);
			int slot = base_slot;
			while (map->slots[slot].key_hash)
				slot = (slot + 1) & slot_mask;
			map->slots[slot].key_hash = hash;
			int item_index = old_slots[i].item_index;
			map->slots[slot].item_index = item_index;
			map->item_slots[item_index] = slot;
			++map->slots[base_slot].base_count;
		}
	}
	ATLAS_CACHE_FREE(old_slots, map->mem_ctx);
}

static void atlas_cache_map_expand_items(atlas_cache_map_t* map)
{
	map->item_capacity *= 2;
	ATLAS_CACHE_U64* new_keys = (ATLAS_CACHE_U64*)ATLAS_CACHE_MALLOC(
		map->item_capacity * (sizeof(*map->keys) + sizeof(*map->item_slots) + map->item_size), map->mem_ctx);
	ATLAS_CACHE_ASSERT(new_keys);

	int* new_item_slots = (int*)(new_keys + map->item_capacity);
	void* new_items = (void*)(new_item_slots + map->item_capacity);

	ATLAS_CACHE_MEMCPY(new_keys, map->keys, map->count * sizeof(*map->keys));
	ATLAS_CACHE_MEMCPY(new_item_slots, map->item_slots, map->count * sizeof(*map->item_slots));
	ATLAS_CACHE_MEMCPY(new_items, map->items, (size_t)map->count * map->item_size);

	ATLAS_CACHE_FREE(map->keys, map->mem_ctx);
	map->keys = new_keys;
	map->item_slots = new_item_slots;
	map->items = new_items;
}

static void* atlas_cache_map_insert(atlas_cache_map_t* map, ATLAS_CACHE_U64 key, void const* item)
{
	ATLAS_CACHE_ASSERT(atlas_cache_map_find_slot(map, key) < 0);

	if (map->count >= (map->slot_capacity - map->slot_capacity / 3))
		atlas_cache_map_expand_slots(map);

	int slot_mask = map->slot_capacity - 1;
	unsigned hash = atlas_cache_map_hash(key);
	int base_slot = (int)(hash & (unsigned)slot_mask);
	int base_count = map->slots[base_slot].base_count;
	int slot = base_slot;
	int first_free = slot;

	while (base_count) {
		unsigned slot_hash = map->slots[slot].key_hash;
		if (slot_hash == 0 && map->slots[first_free].key_hash != 0) first_free = slot;
		int slot_base = (int)(slot_hash & (unsigned)slot_mask);
		if (slot_base == base_slot) --base_count;
		slot = (slot + 1) & slot_mask;
	}

	slot = first_free;
	while (map->slots[slot].key_hash)
		slot = (slot + 1) & slot_mask;

	if (map->count >= map->item_capacity)
		atlas_cache_map_expand_items(map);

	map->slots[slot].key_hash = hash;
	map->slots[slot].item_index = map->count;
	++map->slots[base_slot].base_count;

	void* dest = (void*)((uintptr_t)map->items + map->count * map->item_size);
	ATLAS_CACHE_MEMCPY(dest, item, map->item_size);
	map->keys[map->count] = key;
	map->item_slots[map->count] = slot;
	++map->count;
	return dest;
}

static void atlas_cache_map_remove(atlas_cache_map_t* map, ATLAS_CACHE_U64 key)
{
	int slot = atlas_cache_map_find_slot(map, key);
	ATLAS_CACHE_ASSERT(slot >= 0);

	int slot_mask = map->slot_capacity - 1;
	unsigned hash = map->slots[slot].key_hash;
	int base_slot = (int)(hash & (unsigned)slot_mask);
	--map->slots[base_slot].base_count;
	map->slots[slot].key_hash = 0;

	int index = map->slots[slot].item_index;
	int last_index = map->count - 1;
	if (index != last_index) {
		map->keys[index] = map->keys[last_index];
		map->item_slots[index] = map->item_slots[last_index];
		void* dst = (void*)((uintptr_t)map->items + index * map->item_size);
		void* src = (void*)((uintptr_t)map->items + last_index * map->item_size);
		ATLAS_CACHE_MEMCPY(dst, src, map->item_size);
		map->slots[map->item_slots[last_index]].item_index = index;
	}
	--map->count;
}

static void atlas_cache_map_clear(atlas_cache_map_t* map)
{
	map->count = 0;
	ATLAS_CACHE_MEMSET(map->slots, 0, map->slot_capacity * sizeof(*map->slots));
}

static void* atlas_cache_map_find(atlas_cache_map_t const* map, ATLAS_CACHE_U64 key)
{
	int slot = atlas_cache_map_find_slot(map, key);
	if (slot < 0) return 0;
	int index = map->slots[slot].item_index;
	return (void*)((uintptr_t)map->items + index * map->item_size);
}

static int atlas_cache_map_count(atlas_cache_map_t const* map)
{
	return map->count;
}

static void* atlas_cache_map_items(atlas_cache_map_t const* map)
{
	return map->items;
}

static void atlas_cache_map_swap(atlas_cache_map_t* map, int index_a, int index_b)
{
	if (index_a < 0 || index_a >= map->count || index_b < 0 || index_b >= map->count) return;

	int slot_a = map->item_slots[index_a];
	int slot_b = map->item_slots[index_b];
	map->item_slots[index_a] = slot_b;
	map->item_slots[index_b] = slot_a;

	ATLAS_CACHE_U64 temp_key = map->keys[index_a];
	map->keys[index_a] = map->keys[index_b];
	map->keys[index_b] = temp_key;

	void* item_a = (void*)((uintptr_t)map->items + index_a * map->item_size);
	void* item_b = (void*)((uintptr_t)map->items + index_b * map->item_size);

	// Use stack buffer for swap (item_size is typically small)
	char swap_temp[256];
	ATLAS_CACHE_ASSERT(map->item_size <= (int)sizeof(swap_temp));
	ATLAS_CACHE_MEMCPY(swap_temp, item_a, map->item_size);
	ATLAS_CACHE_MEMCPY(item_a, item_b, map->item_size);
	ATLAS_CACHE_MEMCPY(item_b, swap_temp, map->item_size);

	map->slots[slot_a].item_index = index_b;
	map->slots[slot_b].item_index = index_a;
}

int atlas_cache_init(atlas_cache_t* cache, atlas_cache_config_t* config, void* udata)
{
	// read config params
	if (!config | !cache) return 1;
	cache->sort_id = 0;
	cache->pixel_stride = config->pixel_stride;
	cache->atlas_width_in_pixels = config->atlas_width_in_pixels;
	cache->atlas_height_in_pixels = config->atlas_height_in_pixels;
	cache->atlas_use_border_pixels = config->atlas_use_border_pixels;
	cache->ticks_to_decay_texture = config->ticks_to_decay_texture;
	cache->lonely_buffer_count_till_flush = config->lonely_buffer_count_till_flush;
	cache->lonely_buffer_count_till_decay = cache->lonely_buffer_count_till_flush / 2;
	if (cache->lonely_buffer_count_till_decay <= 0) cache->lonely_buffer_count_till_decay = 1;
	cache->ratio_to_decay_atlas = config->ratio_to_decay_atlas;
	cache->ratio_to_merge_atlases = config->ratio_to_merge_atlases;
	cache->batch_callback = config->batch_callback;
	cache->get_pixels_callback = config->get_pixels_callback;
	cache->generate_texture_callback = config->generate_texture_callback;
	cache->delete_texture_callback = config->delete_texture_callback;
	cache->mem_ctx = config->allocator_context;
	cache->udata = udata;

	if (cache->atlas_width_in_pixels < 1 || cache->atlas_height_in_pixels < 1) return 1;
	if (cache->ticks_to_decay_texture < 1) return 1;
	if (cache->ratio_to_decay_atlas < 0 || cache->ratio_to_decay_atlas > 1.0f) return 1;
	if (cache->ratio_to_merge_atlases < 0 || cache->ratio_to_merge_atlases > 0.5f) return 1;
	if (!cache->batch_callback) return 1;
	if (!cache->get_pixels_callback) return 1;
	if (!cache->generate_texture_callback) return 1;
	if (!cache->delete_texture_callback) return 1;

	// initialize input buffer
	cache->input_count = 0;
	cache->input_capacity = 1024;
	cache->input_buffer = (atlas_cache_internal_entry_t*)ATLAS_CACHE_MALLOC(sizeof(atlas_cache_internal_entry_t) * cache->input_capacity, cache->mem_ctx);
	if (!cache->input_buffer) return 1;

	// initialize entry buffer
	cache->entry_count = 0;
	cache->entry_capacity = 1024;
	cache->entries = (atlas_cache_entry_t*)ATLAS_CACHE_MALLOC(sizeof(atlas_cache_entry_t) * cache->entry_capacity, cache->mem_ctx);
	cache->entries_scratch = (atlas_cache_entry_t*)ATLAS_CACHE_MALLOC(sizeof(atlas_cache_entry_t) * cache->entry_capacity, cache->mem_ctx);
	if (!cache->entries) return 1;
	if (!cache->entries_scratch) return 1;

	// initialize key buffer (for marking hash table entries for deletion)
	cache->key_buffer_count = 0;
	cache->key_buffer_capacity = 1024;
	cache->key_buffer = (ATLAS_CACHE_U64*)ATLAS_CACHE_MALLOC(sizeof(ATLAS_CACHE_U64) * cache->key_buffer_capacity, cache->mem_ctx);

	// initialize pixel buffer for grabbing pixel data from the user as needed
	cache->pixel_buffer_size = 1024;
	cache->pixel_buffer = ATLAS_CACHE_MALLOC(cache->pixel_buffer_size * cache->pixel_stride, cache->mem_ctx);

	// setup tables
	atlas_cache_map_init(&cache->lonely_buffer, sizeof(atlas_cache_internal_lonely_texture_t), 1024, cache->mem_ctx);
	atlas_cache_map_init(&cache->premades, sizeof(atlas_cache_internal_premade_t), 1024 * 10, cache->mem_ctx);
	atlas_cache_map_init(&cache->image_to_atlas, sizeof(atlas_cache_internal_atlas_t*), 16, cache->mem_ctx);

	cache->atlases = 0;

	return 0;
}

void atlas_cache_term(atlas_cache_t* cache)
{
	ATLAS_CACHE_FREE(cache->input_buffer, cache->mem_ctx);
	ATLAS_CACHE_FREE(cache->entries, cache->mem_ctx);
	ATLAS_CACHE_FREE(cache->entries_scratch, cache->mem_ctx);
	ATLAS_CACHE_FREE(cache->key_buffer, cache->mem_ctx);
	ATLAS_CACHE_FREE(cache->pixel_buffer, cache->mem_ctx);
	{
		int count = atlas_cache_map_count(&cache->lonely_buffer);
		atlas_cache_internal_lonely_texture_t* lonely = (atlas_cache_internal_lonely_texture_t*)atlas_cache_map_items(&cache->lonely_buffer);
		for (int i = 0; i < count; ++i) {
			if (lonely[i].texture_id != ~0)
				cache->delete_texture_callback(lonely[i].texture_id, cache->udata);
		}
	}
	atlas_cache_map_term(&cache->lonely_buffer);
	atlas_cache_map_term(&cache->premades);
	atlas_cache_map_term(&cache->image_to_atlas);

	if (cache->atlases)
	{
		atlas_cache_internal_atlas_t* atlas = cache->atlases;
		atlas_cache_internal_atlas_t* sentinel = cache->atlases;
		do
		{
			atlas_cache_map_term(&atlas->textures);
			cache->delete_texture_callback(atlas->texture_id, cache->udata);
			atlas_cache_internal_atlas_t* next = atlas->next;
			ATLAS_CACHE_FREE(atlas, cache->mem_ctx);
			atlas = next;
		}
		while (atlas != sentinel);
	}

	ATLAS_CACHE_MEMSET(cache, 0, sizeof(atlas_cache_t));
}

void atlas_cache_set_default_config(atlas_cache_config_t* config)
{
	config->pixel_stride = sizeof(char) * 4;
	config->atlas_width_in_pixels = 2048;
	config->atlas_height_in_pixels = 2048;
	config->atlas_use_border_pixels = 0;
	config->ticks_to_decay_texture = 60 * 30;
	config->lonely_buffer_count_till_flush = 64;
	config->ratio_to_decay_atlas = 0.5f;
	config->ratio_to_merge_atlases = 0.25f;
	config->batch_callback = 0;
	config->get_pixels_callback = 0;
	config->generate_texture_callback = 0;
	config->delete_texture_callback = 0;
	config->allocator_context = 0;
}

#define ATLAS_CACHE_CHECK_BUFFER_GROW(ctx, count, capacity, data, type) \
	do { \
		if (ctx->count == ctx->capacity) \
		{ \
			int new_capacity = ctx->capacity * 2; \
			void* new_data = ATLAS_CACHE_MALLOC(sizeof(type) * new_capacity, ctx->mem_ctx); \
			if (!new_data) return 0; \
			ATLAS_CACHE_MEMCPY(new_data, ctx->data, sizeof(type) * ctx->count); \
			ATLAS_CACHE_FREE(ctx->data, ctx->mem_ctx); \
			ctx->data = (type*)new_data; \
			ctx->capacity = new_capacity; \
		} \
	} while (0)


int atlas_cache_internal_fill_entry(atlas_cache_t* cache, atlas_cache_entry_t entry, atlas_cache_internal_entry_t* out)
{
	ATLAS_CACHE_ASSERT(entry.w <= cache->atlas_width_in_pixels);
	ATLAS_CACHE_ASSERT(entry.h <= cache->atlas_height_in_pixels);
	ATLAS_CACHE_CHECK_BUFFER_GROW(cache, input_count, input_capacity, input_buffer, atlas_cache_internal_entry_t);

	out->image_id = entry.image_id;
	out->sort_bits = cache->sort_id++;
	out->udata = entry.udata;
	out->w = entry.w;
	out->h = entry.h;

	out->premade_minx = entry.minx;
	out->premade_miny = entry.miny;
	out->premade_maxx = entry.maxx;
	out->premade_maxy = entry.maxy;

	return 1;
}

void atlas_cache_push(atlas_cache_t* cache, atlas_cache_entry_t entry)
{
	atlas_cache_internal_entry_t entry_out;
	atlas_cache_internal_fill_entry(cache, entry, &entry_out);
	cache->input_buffer[cache->input_count++] = entry_out;
}

void atlas_cache_register_premade_atlas(atlas_cache_t* cache, ATLAS_CACHE_U64 texture_id, int w, int h, int entry_count, atlas_cache_premade_entry_t* entries)
{
	for (int i = 0; i < entry_count; ++i) {
		atlas_cache_internal_premade_t premade;
		premade.atlas_w = w;
		premade.atlas_h = h;
		premade.minx = entries[i].minx;
		premade.miny = entries[i].miny;
		premade.maxx = entries[i].maxx;
		premade.maxy = entries[i].maxy;
		premade.texture_id = texture_id;
		void* find = atlas_cache_map_find(&cache->premades, entries[i].image_id);
		if (find) {
			ATLAS_CACHE_MEMCPY(find, &premade, sizeof(premade));
		} else {
			atlas_cache_map_insert(&cache->premades, entries[i].image_id, &premade);
		}
	}
}

int atlas_cache_internal_lonely_entry(atlas_cache_t* cache, ATLAS_CACHE_U64 image_id, int w, int h, atlas_cache_entry_t* entry_out, int skip_missing_textures);
atlas_cache_internal_premade_t* atlas_cache_internal_get_premade(atlas_cache_t* cache, ATLAS_CACHE_U64 image_id, atlas_cache_entry_t* entry_out);

void atlas_cache_prefetch(atlas_cache_t* cache, ATLAS_CACHE_U64 image_id, int w, int h)
{
	atlas_cache_internal_premade_t* premade = atlas_cache_internal_get_premade(cache, image_id, NULL);
	if(!premade) {
		void* atlas_ptr = atlas_cache_map_find(&cache->image_to_atlas, image_id);
		if (!atlas_ptr) atlas_cache_internal_lonely_entry(cache, image_id, w, h, NULL, 0);
	}
}

atlas_cache_entry_t atlas_cache_fetch(atlas_cache_t* cache, ATLAS_CACHE_U64 image_id, int w, int h)
{
	atlas_cache_entry_t s;
	ATLAS_CACHE_MEMSET(&s, 0, sizeof(s));

	atlas_cache_internal_premade_t* premade = (atlas_cache_internal_premade_t*)atlas_cache_map_find(&cache->premades, image_id);
	if(!premade)
	{
		void* atlas_ptr = atlas_cache_map_find(&cache->image_to_atlas, image_id);
		if (atlas_ptr) {
			atlas_cache_internal_atlas_t* atlas = *(atlas_cache_internal_atlas_t**)atlas_ptr;
			s.texture_id = atlas->texture_id;

			atlas_cache_internal_texture_t* tex = (atlas_cache_internal_texture_t*)atlas_cache_map_find(&atlas->textures, image_id);
			if (tex) {
				s.maxx = tex->maxx;
				s.maxy = tex->maxy;
				s.minx = tex->minx;
				s.miny = tex->miny;
			}
		}
		else {
			// Lonely textures live in 0..1 UV space; fetch returns the full image.
			s.minx = 0;
			s.miny = 0;
			s.maxx = 1.0f;
			s.maxy = 1.0f;
			atlas_cache_internal_lonely_entry(cache, image_id, w, h, &s, 0);
		}
	}
	else
	{
		ATLAS_CACHE_ASSERT(premade->texture_id != ~0);
		s.texture_id = premade->texture_id;
		s.minx = premade->minx;
		s.miny = premade->miny;
		s.maxx = premade->maxx;
		s.maxy = premade->maxy;
	}
	return s;
}

static int atlas_cache_internal_entry_less_than_or_equal(atlas_cache_entry_t* a, atlas_cache_entry_t* b)
{
	if (a->sort_bits < b->sort_bits) return 1;
	return a->texture_id <= b->texture_id;
}

void atlas_cache_internal_merge_sort_iteration(atlas_cache_entry_t* a, int lo, int split, int hi, atlas_cache_entry_t* b)
{
	int i = lo, j = split;
	for (int k = lo; k < hi; k++) {
		if (i < split && (j >= hi || atlas_cache_internal_entry_less_than_or_equal(a + i, a + j))) {
			b[k] = a[i];
			i = i + 1;
		} else {
			b[k] = a[j];
			j = j + 1;
		}
	}
}

void atlas_cache_internal_merge_sort_recurse(atlas_cache_entry_t* b, int lo, int hi, atlas_cache_entry_t* a)
{
	if (hi - lo <= 1) return;
	int split = (lo + hi) / 2;
	atlas_cache_internal_merge_sort_recurse(a, lo,  split, b);
	atlas_cache_internal_merge_sort_recurse(a, split, hi, b);
	atlas_cache_internal_merge_sort_iteration(b, lo, split, hi, a);
}

void atlas_cache_internal_merge_sort(atlas_cache_entry_t* a, atlas_cache_entry_t* b, int n)
{
	ATLAS_CACHE_MEMCPY(b, a, sizeof(atlas_cache_entry_t) * n);
	atlas_cache_internal_merge_sort_recurse(b, 0, n, a);
}

void atlas_cache_internal_sort_entries(atlas_cache_t* cache)
{
	atlas_cache_internal_merge_sort(cache->entries, cache->entries_scratch, cache->entry_count);
}

static inline void atlas_cache_internal_get_pixels(atlas_cache_t* cache, ATLAS_CACHE_U64 image_id, int w, int h)
{
	int size = cache->atlas_use_border_pixels ? cache->pixel_stride * (w + 2) * (h + 2) : cache->pixel_stride * w * h;
	if (size > cache->pixel_buffer_size)
	{
		ATLAS_CACHE_FREE(cache->pixel_buffer, cache->mem_ctx);
		cache->pixel_buffer_size = size;
		cache->pixel_buffer = ATLAS_CACHE_MALLOC(cache->pixel_buffer_size, cache->mem_ctx);
		if (!cache->pixel_buffer) return;
	}

	ATLAS_CACHE_MEMSET(cache->pixel_buffer, 0, size);
	int size_from_user = cache->pixel_stride * w * h;
	cache->get_pixels_callback(image_id, cache->pixel_buffer, size_from_user, cache->udata);

	if (cache->atlas_use_border_pixels) {
		// Expand image from top-left corner, offset by (1, 1).
		int w0 = w;
		int h0 = h;
		w += 2;
		h += 2;
		char* buffer = (char*)cache->pixel_buffer;
		int dst_row_stride = w * cache->pixel_stride;
		int src_row_stride = w0 * cache->pixel_stride;
		int src_row_offset = cache->pixel_stride;
		for (int i = 0; i < h - 2; ++i)
		{
			char* src_row = buffer + (h0 - i - 1) * src_row_stride;
			char* dst_row = buffer + (h - i - 2) * dst_row_stride + src_row_offset;
			ATLAS_CACHE_MEMMOVE(dst_row, src_row, src_row_stride);
		}

		// Clear the border pixels.
		int pixel_stride = cache->pixel_stride;
		ATLAS_CACHE_MEMSET(buffer, 0, dst_row_stride);
		for (int i = 1; i < h - 1; ++i)
		{
			ATLAS_CACHE_MEMSET(buffer + i * dst_row_stride, 0, pixel_stride);
			ATLAS_CACHE_MEMSET(buffer + i * dst_row_stride + src_row_stride + src_row_offset, 0, pixel_stride);
		}
		ATLAS_CACHE_MEMSET(buffer + (h - 1) * dst_row_stride, 0, dst_row_stride);
	}
}

static inline ATLAS_CACHE_U64 atlas_cache_internal_generate_texture_handle(atlas_cache_t* cache, ATLAS_CACHE_U64 image_id, int w, int h)
{
	atlas_cache_internal_get_pixels(cache, image_id, w, h);
	if (cache->atlas_use_border_pixels)
	{
		w += 2;
		h += 2;
	}
	return cache->generate_texture_callback(cache->pixel_buffer, w, h, cache->udata);
}

atlas_cache_internal_lonely_texture_t* atlas_cache_internal_lonelybuffer_push(atlas_cache_t* cache, ATLAS_CACHE_U64 image_id, int w, int h, int make_tex)
{
	atlas_cache_internal_lonely_texture_t texture;
	texture.timestamp = 0;
	texture.w = w;
	texture.h = h;
	texture.image_id = image_id;
	texture.texture_id = make_tex ? atlas_cache_internal_generate_texture_handle(cache, image_id, w, h) : ~0;
	return (atlas_cache_internal_lonely_texture_t*)atlas_cache_map_insert(&cache->lonely_buffer, image_id, &texture);
}

int atlas_cache_internal_lonely_entry(atlas_cache_t* cache, ATLAS_CACHE_U64 image_id, int w, int h, atlas_cache_entry_t* entry_out, int skip_missing_textures)
{
	atlas_cache_internal_lonely_texture_t* tex = (atlas_cache_internal_lonely_texture_t*)atlas_cache_map_find(&cache->lonely_buffer, image_id);

	if (skip_missing_textures)
	{
		if (!tex) atlas_cache_internal_lonelybuffer_push(cache, image_id, w, h, 0);
		return 1;
	}

	else
	{
		if (!tex) tex = atlas_cache_internal_lonelybuffer_push(cache, image_id, w, h, 1);
		else if (tex->texture_id == ~0) tex->texture_id = atlas_cache_internal_generate_texture_handle(cache, image_id, w, h);
		tex->timestamp = 0;

		if (entry_out) {
			entry_out->texture_id = tex->texture_id;
			// Preserve local UVs already set by the caller (0..1 texture space for lonely
			// textures). 9-slice and other partial-UV draws depend on this. Callers that
			// want the full texture (e.g. atlas_cache_fetch) should set 0..1 first.

			if (ATLAS_CACHE_LONELY_FLIP_Y_AXIS_FOR_UV)
			{
				float tmp = entry_out->miny;
				entry_out->miny = entry_out->maxy;
				entry_out->maxy = tmp;
			}
		}

		return 0;
	}
}

atlas_cache_internal_premade_t* atlas_cache_internal_get_premade(atlas_cache_t* cache, ATLAS_CACHE_U64 image_id, atlas_cache_entry_t* entry_out)
{
	atlas_cache_internal_premade_t* tex = (atlas_cache_internal_premade_t*)atlas_cache_map_find(&cache->premades, image_id);
	if (!tex) return NULL;
	ATLAS_CACHE_ASSERT(tex->texture_id != ~0);
	if (entry_out) {
		entry_out->texture_id = tex->texture_id;
	}
	return tex;
}

int atlas_cache_internal_push_entry(atlas_cache_t* cache, atlas_cache_internal_entry_t* s, int skip_missing_textures)
{
	int skipped_tex = 0;
	atlas_cache_entry_t entry;
	entry.image_id = s->image_id;
	entry.sort_bits = s->sort_bits;
	entry.udata = s->udata;
	entry.w = s->w;
	entry.h = s->h;

	entry.minx = s->premade_minx;
	entry.miny = s->premade_miny;
	entry.maxx = s->premade_maxx;
	entry.maxy = s->premade_maxy;

	atlas_cache_internal_premade_t* premade = atlas_cache_internal_get_premade(cache, s->image_id, &entry);

	if(!premade)
	{
		void* atlas_ptr = atlas_cache_map_find(&cache->image_to_atlas, s->image_id);
		if (atlas_ptr)
		{
			atlas_cache_internal_atlas_t* atlas = *(atlas_cache_internal_atlas_t**)atlas_ptr;
			entry.texture_id = atlas->texture_id;

			atlas_cache_internal_texture_t* tex = (atlas_cache_internal_texture_t*)atlas_cache_map_find(&atlas->textures, s->image_id);
			ATLAS_CACHE_ASSERT(tex);
			tex->timestamp = 0;
			entry.w = tex->w;
			entry.h = tex->h;

			// Entries are pushed with *local* uvs, remapped here onto the image's slot in
			// the atlas. Pass minx/miny as 0 and maxx/maxy as 1 to draw the full image;
			// values between 0-1 draw a portion (e.g. 9-slice), while values outside 0-1
			// will creep into neighboring images of the atlas.
			float dx = tex->maxx - tex->minx;
			float dy = tex->maxy - tex->miny;
			entry.minx = dx * entry.minx + tex->minx;
			entry.miny = dy * entry.miny + tex->miny;
			entry.maxx = dx * entry.maxx + tex->minx;
			entry.maxy = dy * entry.maxy + tex->miny;
		}
		else skipped_tex = atlas_cache_internal_lonely_entry(cache, s->image_id, s->w, s->h, &entry, skip_missing_textures);
	}
	else
	{
		entry.texture_id = premade->texture_id;
	}

	if (!skipped_tex)
	{
		if (cache->entry_count >= cache->entry_capacity) {
			int new_capacity = cache->entry_capacity * 2;
			void* new_data = ATLAS_CACHE_MALLOC(sizeof(atlas_cache_entry_t) * new_capacity, cache->mem_ctx);
			if (!new_data) return 0;
			ATLAS_CACHE_MEMCPY(new_data, cache->entries, sizeof(atlas_cache_entry_t) * cache->entry_count);
			ATLAS_CACHE_FREE(cache->entries, cache->mem_ctx);
			cache->entries = (atlas_cache_entry_t*)new_data;
			cache->entry_capacity = new_capacity;

			ATLAS_CACHE_FREE(cache->entries_scratch, cache->mem_ctx);
			cache->entries_scratch = (atlas_cache_entry_t*)ATLAS_CACHE_MALLOC(sizeof(atlas_cache_entry_t) * new_capacity, cache->mem_ctx);
			if (!cache->entries_scratch) return 0;
		}
		cache->entries[cache->entry_count++] = entry;
	}

	return skipped_tex;
}

void atlas_cache_internal_process_input(atlas_cache_t* cache, int skip_missing_textures)
{
	int skipped_index = 0;
	for (int i = 0; i < cache->input_count; ++i)
	{
		atlas_cache_internal_entry_t* s = cache->input_buffer + i;
		int skipped = atlas_cache_internal_push_entry(cache, s, skip_missing_textures);
		if (skip_missing_textures && skipped) cache->input_buffer[skipped_index++] = *s;
	}

	cache->input_count = skipped_index;
}

void atlas_cache_tick(atlas_cache_t* cache)
{
	atlas_cache_internal_atlas_t* atlas = cache->atlases;
	if (atlas)
	{
		atlas_cache_internal_atlas_t* sentinel = atlas;
		do
		{
			int texture_count = atlas_cache_map_count(&atlas->textures);
			atlas_cache_internal_texture_t* textures = (atlas_cache_internal_texture_t*)atlas_cache_map_items(&atlas->textures);
			for (int i = 0; i < texture_count; ++i) textures[i].timestamp += 1;
			atlas = atlas->next;
		}
		while (atlas != sentinel);
	}

	int texture_count = atlas_cache_map_count(&cache->lonely_buffer);
	atlas_cache_internal_lonely_texture_t* lonely_textures = (atlas_cache_internal_lonely_texture_t*)atlas_cache_map_items(&cache->lonely_buffer);
	for (int i = 0; i < texture_count; ++i) lonely_textures[i].timestamp += 1;
}

int atlas_cache_flush(atlas_cache_t* cache)
{
	// process input buffer, make any necessary lonely textures
	// convert user entries to internal format
	// lookup uv coordinates
	atlas_cache_internal_process_input(cache, 0);

	// patchup any missing lonely textures that may have come from atlases decaying and whatnot
	int texture_count = atlas_cache_map_count(&cache->lonely_buffer);
	atlas_cache_internal_lonely_texture_t* lonely_textures = (atlas_cache_internal_lonely_texture_t*)atlas_cache_map_items(&cache->lonely_buffer);
	for (int i = 0; i < texture_count; ++i)
	{
		atlas_cache_internal_lonely_texture_t* lonely = lonely_textures + i;
		if (lonely->texture_id == ~0) {
			lonely->texture_id = atlas_cache_internal_generate_texture_handle(cache, lonely->image_id, lonely->w, lonely->h);
		}
	}

	// Reset each flush, since it's only used to id entries on a per-sort basis.
	cache->sort_id = 0;

	// sort internal entry buffer and submit batches
	atlas_cache_internal_sort_entries(cache);

	int min = 0;
	int max = 0;
	int done = !cache->entry_count;
	int count = 0;
	while (!done)
	{
		ATLAS_CACHE_U64 id = cache->entries[min].texture_id;
		ATLAS_CACHE_U64 image_id = cache->entries[min].image_id;

		while (1)
		{
			if (max == cache->entry_count)
			{
				done = 1;
				break;
			}

			if (id != cache->entries[max].texture_id)
				break;

			++max;
		}

		int batch_count = max - min;
		if (batch_count)
		{
			int w, h;
			atlas_cache_internal_premade_t* premade = (atlas_cache_internal_premade_t*)atlas_cache_map_find(&cache->premades, image_id);
			if (!premade)
			{
				void* atlas_ptr = atlas_cache_map_find(&cache->image_to_atlas, image_id);

				if (atlas_ptr)
				{
					w = cache->atlas_width_in_pixels;
					h = cache->atlas_height_in_pixels;
				}

				else
				{
					atlas_cache_internal_lonely_texture_t* tex = (atlas_cache_internal_lonely_texture_t*)atlas_cache_map_find(&cache->lonely_buffer, image_id);
					ATLAS_CACHE_ASSERT(tex);
					w = tex->w;
					h = tex->h;
					if (cache->atlas_use_border_pixels)
					{
						w += 2;
						h += 2;
					}
				}
			} 
			else
			{
				w = premade->atlas_w;
				h = premade->atlas_h;
			}

			cache->batch_callback(cache->entries + min, batch_count, w, h, cache->udata);
			++count;
		}
		min = max;
	}

	cache->entry_count = 0;
	if (count > 1) {
		ATLAS_CACHE_LOG("Flushed %d batches.\n", count);
	}

	return count;
}

typedef struct
{
	int x;
	int y;
} atlas_cache_v2_t;

typedef struct
{
	int img_index;
	atlas_cache_v2_t size;
	atlas_cache_v2_t min;
	atlas_cache_v2_t max;
	int fit;
} atlas_cache_internal_integer_image_t;

static atlas_cache_v2_t atlas_cache_v2(int x, int y)
{
	atlas_cache_v2_t v;
	v.x = x;
	v.y = y;
	return v;
}

static atlas_cache_v2_t atlas_cache_sub(atlas_cache_v2_t a, atlas_cache_v2_t b)
{
	atlas_cache_v2_t v;
	v.x = a.x - b.x;
	v.y = a.y - b.y;
	return v;
}

static atlas_cache_v2_t atlas_cache_add(atlas_cache_v2_t a, atlas_cache_v2_t b)
{
	atlas_cache_v2_t v;
	v.x = a.x + b.x;
	v.y = a.y + b.y;
	return v;
}

typedef struct
{
	atlas_cache_v2_t size;
	atlas_cache_v2_t min;
	atlas_cache_v2_t max;
} atlas_cache_internal_atlas_node_t;

static atlas_cache_internal_atlas_node_t* atlas_cache_best_fit(int sp, int w, int h, atlas_cache_internal_atlas_node_t* nodes)
{
	int best_volume = INT_MAX;
	atlas_cache_internal_atlas_node_t *best_node = 0;
	int img_volume = w * h;

	for ( int i = 0; i < sp; ++i )
	{
		atlas_cache_internal_atlas_node_t *node = nodes + i;
		int can_contain = node->size.x >= w && node->size.y >= h;
		if ( can_contain )
		{
			int node_volume = node->size.x * node->size.y;
			if ( node_volume == img_volume ) return node;
			if ( node_volume < best_volume )
			{
				best_volume = node_volume;
				best_node = node;
			}
		}
	}

	return best_node;
}

static int atlas_cache_internal_image_less_than_or_equal(atlas_cache_internal_integer_image_t* a, atlas_cache_internal_integer_image_t* b)
{
	int perimeterA = 2 * (a->size.x + a->size.y);
	int perimeterB = 2 * (b->size.x + b->size.y);
	return perimeterB <= perimeterA;
}

void atlas_cache_internal_image_merge_sort_iteration(atlas_cache_internal_integer_image_t* a, int lo, int split, int hi, atlas_cache_internal_integer_image_t* b)
{
	int i = lo, j = split;
	for (int k = lo; k < hi; k++) {
		if (i < split && (j >= hi || atlas_cache_internal_image_less_than_or_equal(a + i, a + j))) {
			b[k] = a[i];
			i = i + 1;
		} else {
			b[k] = a[j];
			j = j + 1;
		}
	}
}

void atlas_cache_internal_image_merge_sort_recurse(atlas_cache_internal_integer_image_t* b, int lo, int hi, atlas_cache_internal_integer_image_t* a)
{
	if (hi - lo <= 1) return;
	int split = (lo + hi) / 2;
	atlas_cache_internal_image_merge_sort_recurse(a, lo,  split, b);
	atlas_cache_internal_image_merge_sort_recurse(a, split, hi, b);
	atlas_cache_internal_image_merge_sort_iteration(b, lo, split, hi, a);
}

void atlas_cache_internal_image_merge_sort(atlas_cache_internal_integer_image_t* a, atlas_cache_internal_integer_image_t* b, int n)
{
	ATLAS_CACHE_MEMCPY(b, a, sizeof(atlas_cache_internal_integer_image_t) * n);
	atlas_cache_internal_image_merge_sort_recurse(b, 0, n, a);
}

typedef struct atlas_cache_internal_atlas_image_t
{
	int img_index;    // index into the `imgs` array
	int w, h;         // pixel w/h of original image
	float minx, miny; // u coordinate
	float maxx, maxy; // v coordinate
	int fit;          // non-zero if image fit and was placed into the atlas
} atlas_cache_internal_atlas_image_t;

#define ATLAS_CACHE_CHECK( X, Y ) do { if ( !(X) ) { ATLAS_CACHE_LOG(Y); goto cache_err; } } while ( 0 )

void atlas_cache_make_atlas(atlas_cache_t* cache, atlas_cache_internal_atlas_t* atlas_out, const atlas_cache_internal_lonely_texture_t* imgs, int img_count)
{
	float iw, ih;
	int atlas_image_size, atlas_stride, sp;
	void* atlas_pixels = 0;
	int atlas_node_capacity = img_count * 2;
	atlas_cache_internal_integer_image_t* images = 0;
	atlas_cache_internal_integer_image_t* images_scratch = 0;
	atlas_cache_internal_atlas_node_t* nodes = 0;
	int pixel_stride = cache->pixel_stride;
	int atlas_width = cache->atlas_width_in_pixels;
	int atlas_height = cache->atlas_height_in_pixels;
	float volume_used = 0;

	images = (atlas_cache_internal_integer_image_t*)ATLAS_CACHE_MALLOC(sizeof(atlas_cache_internal_integer_image_t) * img_count, cache->mem_ctx);
	images_scratch = (atlas_cache_internal_integer_image_t*)ATLAS_CACHE_MALLOC(sizeof(atlas_cache_internal_integer_image_t) * img_count, cache->mem_ctx);
	nodes = (atlas_cache_internal_atlas_node_t*)ATLAS_CACHE_MALLOC(sizeof(atlas_cache_internal_atlas_node_t) * atlas_node_capacity, cache->mem_ctx);
	ATLAS_CACHE_CHECK(images, "out of mem");
	ATLAS_CACHE_CHECK(nodes, "out of mem");

	for (int i = 0; i < img_count; ++i)
	{
		const atlas_cache_internal_lonely_texture_t* img = imgs + i;
		atlas_cache_internal_integer_image_t* image = images + i;
		image->fit = 0;
		image->size = cache->atlas_use_border_pixels ? atlas_cache_v2(img->w + 2, img->h + 2) : atlas_cache_v2(img->w, img->h);
		image->img_index = i;
	}

	// Sort images from largest to smallest
	atlas_cache_internal_image_merge_sort(images, images_scratch, img_count);

	// stack pointer, the stack is the nodes array which we will
	// allocate nodes from as necessary.
	sp = 1;

	nodes[0].min = atlas_cache_v2(0, 0);
	nodes[0].max = atlas_cache_v2(atlas_width, atlas_height);
	nodes[0].size = atlas_cache_v2(atlas_width, atlas_height);

	// Nodes represent empty space in the atlas. Placing a texture into the
	// atlas involves splitting a node into two smaller pieces (or, if a
	// perfect fit is found, deleting the node).
	for (int i = 0; i < img_count; ++i)
	{
		atlas_cache_internal_integer_image_t* image = images + i;
		int width = image->size.x;
		int height = image->size.y;
		atlas_cache_internal_atlas_node_t *best_fit = atlas_cache_best_fit(sp, width, height, nodes);

		if (!best_fit) {
			image->fit = 0;
			continue;
		}

		image->min = best_fit->min;
		image->max = atlas_cache_add(image->min, image->size);

		if (best_fit->size.x == width && best_fit->size.y == height)
		{
			atlas_cache_internal_atlas_node_t* last_node = nodes + --sp;
			*best_fit = *last_node;
			image->fit = 1;

			continue;
		}

		image->fit = 1;

		if (sp == atlas_node_capacity)
		{
			int new_capacity = atlas_node_capacity * 2;
			atlas_cache_internal_atlas_node_t* new_nodes = (atlas_cache_internal_atlas_node_t*)ATLAS_CACHE_MALLOC(sizeof(atlas_cache_internal_atlas_node_t) * new_capacity, cache->mem_ctx);
			ATLAS_CACHE_CHECK(new_nodes, "out of mem");
			ATLAS_CACHE_MEMCPY(new_nodes, nodes, sizeof(atlas_cache_internal_atlas_node_t) * sp);
			ATLAS_CACHE_FREE(nodes, cache->mem_ctx);
			nodes = new_nodes;
			atlas_node_capacity = new_capacity;
		}

		atlas_cache_internal_atlas_node_t* new_node = nodes + sp++;
		new_node->min = best_fit->min;

		// Split bestFit along x or y, whichever minimizes
		// fragmentation of empty space
		atlas_cache_v2_t d = atlas_cache_sub(best_fit->size, atlas_cache_v2(width, height));
		if (d.x < d.y)
		{
			new_node->size.x = d.x;
			new_node->size.y = height;
			new_node->min.x += width;

			best_fit->size.y = d.y;
			best_fit->min.y += height;
		}

		else
		{
			new_node->size.x = width;
			new_node->size.y = d.y;
			new_node->min.y += height;

			best_fit->size.x = d.x;
			best_fit->min.x += width;
		}

		new_node->max = atlas_cache_add(new_node->min, new_node->size);
	}

	// Write the final atlas image, use ATLAS_CACHE_ATLAS_EMPTY_COLOR as base color
	atlas_stride = atlas_width * pixel_stride;
	atlas_image_size = atlas_width * atlas_height * pixel_stride;
	atlas_pixels = ATLAS_CACHE_MALLOC(atlas_image_size, cache->mem_ctx);
	ATLAS_CACHE_CHECK(atlas_pixels, "out of mem");
	ATLAS_CACHE_MEMSET(atlas_pixels, ATLAS_CACHE_ATLAS_EMPTY_COLOR, atlas_image_size);

	for (int i = 0; i < img_count; ++i)
	{
		atlas_cache_internal_integer_image_t* image = images + i;

		if (image->fit)
		{
			const atlas_cache_internal_lonely_texture_t* img = imgs + image->img_index;
			atlas_cache_internal_get_pixels(cache, img->image_id, img->w, img->h);
			char* pixels = (char*)cache->pixel_buffer;

			atlas_cache_v2_t min = image->min;
			atlas_cache_v2_t max = image->max;
			int atlas_offset = min.x * pixel_stride;
			int tex_stride = image->size.x * pixel_stride;

			for (int row = min.y, y = 0; row < max.y; ++row, ++y)
			{
				void* row_ptr = (char*)atlas_pixels + (row * atlas_stride + atlas_offset);
				ATLAS_CACHE_MEMCPY(row_ptr, pixels + y * tex_stride, tex_stride);
			}
		}
	}

	atlas_cache_map_init(&atlas_out->textures, sizeof(atlas_cache_internal_texture_t), img_count, cache->mem_ctx);
	atlas_out->texture_id = cache->generate_texture_callback(atlas_pixels, atlas_width, atlas_height, cache->udata);

	iw = 1.0f / (float)(atlas_width);
	ih = 1.0f / (float)(atlas_height);

	for (int i = 0; i < img_count; ++i)
	{
		atlas_cache_internal_integer_image_t* img = images + i;

		if (img->fit)
		{
			atlas_cache_v2_t min = img->min;
			atlas_cache_v2_t max = img->max;
			volume_used += img->size.x * img->size.y;

			float min_x = (float)min.x * iw;
			float min_y = (float)min.y * ih;
			float max_x = (float)max.x * iw;
			float max_y = (float)max.y * ih;

			// flip image on y axis
			if (ATLAS_CACHE_ATLAS_FLIP_Y_AXIS_FOR_UV)
			{
				float tmp = min_y;
				min_y = max_y;
				max_y = tmp;
			}

			atlas_cache_internal_texture_t texture;
			texture.w = img->size.x;
			texture.h = img->size.y;
			texture.timestamp = 0;
			texture.minx = min_x;
			texture.miny = min_y;
			texture.maxx = max_x;
			texture.maxy = max_y;
			ATLAS_CACHE_ASSERT(!(img->size.x < 0));
			ATLAS_CACHE_ASSERT(!(img->size.y < 0));
			ATLAS_CACHE_ASSERT(!(min_x < 0));
			ATLAS_CACHE_ASSERT(!(max_x < 0));
			ATLAS_CACHE_ASSERT(!(min_y < 0));
			ATLAS_CACHE_ASSERT(!(max_y < 0));
			texture.image_id = imgs[img->img_index].image_id;
			atlas_cache_map_insert(&atlas_out->textures, texture.image_id, &texture);
		}
	}

	// Need to adjust atlas_width and atlas_height in config params, as none of the images for this
	// atlas actually fit inside of the atlas! Either adjust the config, or stop sending giant images
	// to the atlas cache.
	ATLAS_CACHE_ASSERT(volume_used > 0);

	atlas_out->volume_ratio = volume_used / (atlas_width * atlas_height);

cache_err:
	// no specific error handling needed here (yet)

	ATLAS_CACHE_FREE(atlas_pixels, cache->mem_ctx);
	ATLAS_CACHE_FREE(nodes, cache->mem_ctx);
	ATLAS_CACHE_FREE(images, cache->mem_ctx);
	ATLAS_CACHE_FREE(images_scratch, cache->mem_ctx);
	return;
}

static int atlas_cache_internal_lonely_pred(atlas_cache_internal_lonely_texture_t* a, atlas_cache_internal_lonely_texture_t* b)
{
	return a->timestamp < b->timestamp;
}

static void atlas_cache_internal_qsort_lonely(atlas_cache_map_t* lonely_table, atlas_cache_internal_lonely_texture_t* items, int count)
{
	if (count <= 1) return;

	atlas_cache_internal_lonely_texture_t pivot = items[count - 1];
	int low = 0;
	for (int i = 0; i < count - 1; ++i)
	{
		if (atlas_cache_internal_lonely_pred(items + i, &pivot))
		{
			atlas_cache_map_swap(lonely_table, i, low);
			low++;
		}
	}

	atlas_cache_map_swap(lonely_table, low, count - 1);
	atlas_cache_internal_qsort_lonely(lonely_table, items, low);
	atlas_cache_internal_qsort_lonely(lonely_table, items + low + 1, count - 1 - low);
}

int atlas_cache_internal_buffer_key(atlas_cache_t* cache, ATLAS_CACHE_U64 key)
{
	ATLAS_CACHE_CHECK_BUFFER_GROW(cache, key_buffer_count, key_buffer_capacity, key_buffer, ATLAS_CACHE_U64);
	cache->key_buffer[cache->key_buffer_count++] = key;
	return 0;
}

void atlas_cache_internal_remove_table_entries(atlas_cache_t* cache, atlas_cache_map_t* table)
{
	for (int i = 0; i < cache->key_buffer_count; ++i) atlas_cache_map_remove(table, cache->key_buffer[i]);
	cache->key_buffer_count = 0;
}

void atlas_cache_internal_flush_atlas(atlas_cache_t* cache, atlas_cache_internal_atlas_t* atlas, atlas_cache_internal_atlas_t** sentinel, atlas_cache_internal_atlas_t** next)
{
	int ticks_to_decay_texture = cache->ticks_to_decay_texture;
	int texture_count = atlas_cache_map_count(&atlas->textures);
	atlas_cache_internal_texture_t* textures = (atlas_cache_internal_texture_t*)atlas_cache_map_items(&atlas->textures);

	for (int i = 0; i < texture_count; ++i)
	{
		atlas_cache_internal_texture_t* atlas_texture = textures + i;
		if (atlas_texture->timestamp < ticks_to_decay_texture)
		{
			int w = atlas_texture->w;
			int h = atlas_texture->h;
			if (cache->atlas_use_border_pixels)
			{
				w -= 2;
				h -= 2;
			}
			atlas_cache_internal_lonely_texture_t* lonely_texture = atlas_cache_internal_lonelybuffer_push(cache, atlas_texture->image_id, w, h, 0);
			lonely_texture->timestamp = atlas_texture->timestamp;
		}
		atlas_cache_map_remove(&cache->image_to_atlas, atlas_texture->image_id);
	}

	if (cache->atlases == atlas)
	{
		if (atlas->next == atlas) cache->atlases = 0;
		else cache->atlases = atlas->prev;
	}

	// handle loop end conditions if sentinel was removed from the chain
	if (sentinel && next)
	{
		if (*sentinel == atlas)
		{
			ATLAS_CACHE_LOG("\t\tsentinel was also atlas: %p\n", *sentinel);
			if ((*next)->next != *sentinel)
			{
				ATLAS_CACHE_LOG("\t\t*next = (*next)->next : %p = (*next)->%p\n", *next, (*next)->next);
				*next = (*next)->next;
			}

			ATLAS_CACHE_LOG("\t\t*sentinel = *next : %p =  %p\n", *sentinel, *next);
 			*sentinel = *next;

		}
	}

	atlas->next->prev = atlas->prev;
	atlas->prev->next = atlas->next;
	atlas_cache_map_term(&atlas->textures);
	cache->delete_texture_callback(atlas->texture_id, cache->udata);
	ATLAS_CACHE_FREE(atlas, cache->mem_ctx);
}

void atlas_cache_invalidate(atlas_cache_t* cache, ATLAS_CACHE_U64 image_id)
{
	void* atlas_ptr = atlas_cache_map_find(&cache->image_to_atlas, image_id);
	if (atlas_ptr) {
		atlas_cache_internal_atlas_t* atlas = *(atlas_cache_internal_atlas_t**)atlas_ptr;
		atlas_cache_internal_flush_atlas(cache, atlas, 0, 0);
	}
	if (atlas_cache_map_find(&cache->lonely_buffer, image_id)) {
		atlas_cache_map_remove(&cache->lonely_buffer, image_id);
	}
}

void atlas_cache_internal_log_chain(atlas_cache_internal_atlas_t* atlas)
{
	if (atlas)
	{
		atlas_cache_internal_atlas_t* sentinel = atlas;
		ATLAS_CACHE_LOG("sentinel: %p\n", sentinel);
		do
		{
			atlas_cache_internal_atlas_t* next = atlas->next;
			ATLAS_CACHE_LOG("\tatlas %p\n", atlas);
			atlas = next;
		}
		while (atlas != sentinel);
	}
}

int atlas_cache_defrag(atlas_cache_t* cache)
{
	// remove decayed atlases and flush them to the lonely buffer
	// only flush textures that are not decayed
	int ticks_to_decay_texture = cache->ticks_to_decay_texture;
	float ratio_to_decay_atlas = cache->ratio_to_decay_atlas;
	atlas_cache_internal_atlas_t* atlas = cache->atlases;
	if (atlas)
	{
		atlas_cache_internal_log_chain(atlas);
		atlas_cache_internal_atlas_t* sentinel = atlas;
		do
		{
			atlas_cache_internal_atlas_t* next = atlas->next;
			int texture_count = atlas_cache_map_count(&atlas->textures);
			atlas_cache_internal_texture_t* textures = (atlas_cache_internal_texture_t*)atlas_cache_map_items(&atlas->textures);
			int decayed_texture_count = 0;
			for (int i = 0; i < texture_count; ++i) if (textures[i].timestamp >= ticks_to_decay_texture) decayed_texture_count++;

			float ratio;
			if (!decayed_texture_count) ratio = 0;
			else ratio = (float)texture_count / (float)decayed_texture_count;
			if (ratio > ratio_to_decay_atlas)
			{
				ATLAS_CACHE_LOG("flushed atlas %p\n", atlas);
				atlas_cache_internal_flush_atlas(cache, atlas, &sentinel, &next);
			}
			atlas = next;
		}
		while (atlas != sentinel);
	}

	// merge mostly empty atlases
	float ratio_to_merge_atlases = cache->ratio_to_merge_atlases;
	atlas = cache->atlases;
	if (atlas)
	{
		int sp = 0;
		atlas_cache_internal_atlas_t* merge_stack[2];

		atlas_cache_internal_atlas_t* sentinel = atlas;
		do
		{
			atlas_cache_internal_atlas_t* next = atlas->next;

			ATLAS_CACHE_ASSERT(sp >= 0 && sp <= 2);
			if (sp == 2)
			{
				ATLAS_CACHE_LOG("merged 2 atlases\n");
				atlas_cache_internal_flush_atlas(cache, merge_stack[0], &sentinel, &next);
				atlas_cache_internal_flush_atlas(cache, merge_stack[1], &sentinel, &next);
				sp = 0;
			}

			float ratio = atlas->volume_ratio;
			if (ratio < ratio_to_merge_atlases) merge_stack[sp++] = atlas;

			atlas = next;
		}
		while (atlas != sentinel);

		if (sp == 2)
		{
			ATLAS_CACHE_LOG("merged 2 atlases (out of loop)\n");
			atlas_cache_internal_flush_atlas(cache, merge_stack[0], 0, 0);
			atlas_cache_internal_flush_atlas(cache, merge_stack[1], 0, 0);
		}
	}

	// remove decayed textures from the lonely buffer
	int lonely_buffer_count_till_decay = cache->lonely_buffer_count_till_decay;
	int lonely_count = atlas_cache_map_count(&cache->lonely_buffer);
	atlas_cache_internal_lonely_texture_t* lonely_textures = (atlas_cache_internal_lonely_texture_t*)atlas_cache_map_items(&cache->lonely_buffer);
	if (lonely_count >= lonely_buffer_count_till_decay)
	{
		atlas_cache_internal_qsort_lonely(&cache->lonely_buffer, lonely_textures, lonely_count);
		int index = 0;
		while (1)
		{
			if (index == lonely_count) break;
			if (lonely_textures[index].timestamp >= ticks_to_decay_texture) break;
			++index;
		}
		for (int i = index; i < lonely_count; ++i)
		{
			ATLAS_CACHE_U64 texture_id = lonely_textures[i].texture_id;
			if (texture_id != ~0) cache->delete_texture_callback(texture_id, cache->udata);
			atlas_cache_internal_buffer_key(cache, lonely_textures[i].image_id);
			ATLAS_CACHE_LOG("lonely texture decayed\n");
		}
		atlas_cache_internal_remove_table_entries(cache, &cache->lonely_buffer);
		lonely_count -= lonely_count - index;
		ATLAS_CACHE_ASSERT(lonely_count == atlas_cache_map_count(&cache->lonely_buffer));
	}

	// process input, but don't make textures just yet
	atlas_cache_internal_process_input(cache, 1);
	lonely_count = atlas_cache_map_count(&cache->lonely_buffer);
	// Processing input can insert new lonely entries, and growing the map past its
	// item capacity reallocates the backing items array -- re-fetch the pointer, or
	// the atlas creation below reads freed memory and packs garbage blocks.
	lonely_textures = (atlas_cache_internal_lonely_texture_t*)atlas_cache_map_items(&cache->lonely_buffer);

	// while greater than lonely_buffer_count_till_flush elements in lonely buffer
	// grab lonely_buffer_count_till_flush of them and make an atlas
	int lonely_buffer_count_till_flush = cache->lonely_buffer_count_till_flush;
	int stuck = 0;
	while (lonely_count > lonely_buffer_count_till_flush && !stuck)
	{
		atlas = (atlas_cache_internal_atlas_t*)ATLAS_CACHE_MALLOC(sizeof(atlas_cache_internal_atlas_t), cache->mem_ctx);
		if (cache->atlases)
		{
			atlas->prev = cache->atlases;
			atlas->next = cache->atlases->next;
			cache->atlases->next->prev = atlas;
			cache->atlases->next = atlas;
		}

		else
		{
			atlas->next = atlas;
			atlas->prev = atlas;
			cache->atlases = atlas;
		}

		atlas_cache_make_atlas(cache, atlas, lonely_textures, lonely_count);
		ATLAS_CACHE_LOG("making atlas\n");

		int tex_count_in_atlas = atlas_cache_map_count(&atlas->textures);
		if (tex_count_in_atlas != lonely_count)
		{
			int hit_count = 0;
			for (int i = 0; i < lonely_count; ++i)
			{
				ATLAS_CACHE_U64 key = lonely_textures[i].image_id;
				if (atlas_cache_map_find(&atlas->textures, key))
				{
					atlas_cache_internal_buffer_key(cache, key);
					ATLAS_CACHE_U64 texture_id = lonely_textures[i].texture_id;
					if (texture_id != ~0) cache->delete_texture_callback(texture_id, cache->udata);
					atlas_cache_map_insert(&cache->image_to_atlas, key, &atlas);
					ATLAS_CACHE_LOG("removing lonely texture for atlas%s\n", texture_id != ~0 ? "" : " (tex was ~0)" );
				}
				else
				{
					hit_count++;
					ATLAS_CACHE_ASSERT(lonely_textures[i].w <= cache->atlas_width_in_pixels);
					ATLAS_CACHE_ASSERT(lonely_textures[i].h <= cache->atlas_height_in_pixels);
				}
			}
			atlas_cache_internal_remove_table_entries(cache, &cache->lonely_buffer);

			lonely_count = atlas_cache_map_count(&cache->lonely_buffer);

			if (!hit_count)
			{
				// TODO
				// handle case where none fit in atlas
				stuck = 1;
				ATLAS_CACHE_ASSERT(0);
			}
		}

		else
		{
			for (int i = 0; i < lonely_count; ++i)
			{
				ATLAS_CACHE_U64 key = lonely_textures[i].image_id;
				ATLAS_CACHE_U64 texture_id = lonely_textures[i].texture_id;
				if (texture_id != ~0) cache->delete_texture_callback(texture_id, cache->udata);
				atlas_cache_map_insert(&cache->image_to_atlas, key, &atlas);
				ATLAS_CACHE_LOG("(fast path) removing lonely texture for atlas%s\n", texture_id != ~0 ? "" : " (tex was ~0)" );
			}
			atlas_cache_map_clear(&cache->lonely_buffer);
			lonely_count = 0;
			break;
		}
	}

	return 1;
}

#endif // ATLAS_CACHE_IMPLEMENTATION_ONCE
#endif // ATLAS_CACHE_IMPLEMENTATION

/*
	------------------------------------------------------------------------------
	This software is available under 2 licenses - you may choose the one you like.
	------------------------------------------------------------------------------
	ALTERNATIVE A - zlib license
	Copyright (c) 2026 Randy Gaul https://randygaul.github.io/
	This software is provided 'as-is', without any express or implied warranty.
	In no event will the authors be held liable for any damages arising from
	the use of this software.
	Permission is granted to anyone to use this software for any purpose,
	including commercial applications, and to alter it and redistribute it
	freely, subject to the following restrictions:
	  1. The origin of this software must not be misrepresented; you must not
	     claim that you wrote the original software. If you use this software
	     in a product, an acknowledgment in the product documentation would be
	     appreciated but is not required.
	  2. Altered source versions must be plainly marked as such, and must not
	     be misrepresented as being the original software.
	  3. This notice may not be removed or altered from any source distribution.
	------------------------------------------------------------------------------
	ALTERNATIVE B - Public Domain (www.unlicense.org)
	This is free and unencumbered software released into the public domain.
	Anyone is free to copy, modify, publish, use, compile, sell, or distribute this 
	software, either in source code form or as a compiled binary, for any purpose, 
	commercial or non-commercial, and by any means.
	In jurisdictions that recognize copyright laws, the author or authors of this 
	software dedicate any and all copyright interest in the software to the public 
	domain. We make this dedication for the benefit of the public at large and to 
	the detriment of our heirs and successors. We intend this dedication to be an 
	overt act of relinquishment in perpetuity of all present and future rights to 
	this software under copyright law.
	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
	AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN 
	ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION 
	WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
	------------------------------------------------------------------------------
*/
