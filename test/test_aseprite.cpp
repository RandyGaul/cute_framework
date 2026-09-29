/*
	Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

#include "test_harness.h"

#include <cute.h>
using namespace Cute;

#ifndef CF_STATIC
#	define CUTE_ASEPRITE_IMPLEMENTATION
#	include <cute/cute_aseprite.h>
#endif

#include <internal/cute_aseprite_cache_internal.h>

#include <internal/cute_girl.h>

/* Load an aseprite file and destroy it. */
TEST_CASE(test_aseprite_make_destroy)
{
	ase_t* ase = cute_aseprite_load_from_memory(girl_data, girl_sz, NULL);
	cute_aseprite_free(ase);
	return true;
}

/* Build a sprite from scratch, save it, load it back: every pixel and every frame survives. */
TEST_CASE(test_aseprite_create_save_load)
{
	ase_t* ase = cute_aseprite_create(5, 3, 2, NULL);
	REQUIRE(ase->frame_count == 2 && ase->layer_count == 1);
	for (int f = 0; f < 2; ++f) {
		ase_color_t* px = (ase_color_t*)ase->frames[f].cels[0].pixels;
		for (int i = 0; i < 15; ++i) {
			ase_color_t c = { (uint8_t)(i * 16 + f), (uint8_t)(255 - i), (uint8_t)(f * 100), (uint8_t)(i % 2 ? 255 : 0) };
			px[i] = c;
		}
	}
	ase->frames[1].duration_milliseconds = 250;
	int size = 0;
	void* data = cute_aseprite_save_to_memory(ase, &size, NULL);
	REQUIRE(data && size > 128);
	ase_t* back = cute_aseprite_load_from_memory(data, size, NULL);
	REQUIRE(back && back->w == 5 && back->h == 3 && back->frame_count == 2 && back->layer_count == 1);
	REQUIRE(!strcmp(back->layers[0].name, "Layer 1"));
	REQUIRE(back->frames[1].duration_milliseconds == 250);
	for (int f = 0; f < 2; ++f) {
		const ase_color_t* a = (const ase_color_t*)ase->frames[f].cels[0].pixels;
		const ase_color_t* b = (const ase_color_t*)back->frames[f].cels[0].pixels;
		REQUIRE(back->frames[f].cel_count == 1 && back->frames[f].cels[0].w == 5 && back->frames[f].cels[0].h == 3);
		REQUIRE(!memcmp(a, b, 15 * sizeof(ase_color_t)));
	}
	cf_free(data);
	cute_aseprite_free(back);
	cute_aseprite_free(ase);
	return true;
}

/* A real file with layers, tags, and a palette: load, save, load again, same composite. */
TEST_CASE(test_aseprite_resave_girl)
{
	ase_t* ase = cute_aseprite_load_from_memory(girl_data, girl_sz, NULL);
	REQUIRE(ase);
	int size = 0;
	void* data = cute_aseprite_save_to_memory(ase, &size, NULL);
	REQUIRE(data && size > 0);
	ase_t* back = cute_aseprite_load_from_memory(data, size, NULL);
	REQUIRE(back && back->w == ase->w && back->h == ase->h && back->frame_count == ase->frame_count);
	REQUIRE(back->layer_count == ase->layer_count && back->tag_count == ase->tag_count);
	REQUIRE(back->palette.entry_count == ase->palette.entry_count);
	for (int f = 0; f < ase->frame_count; ++f) {
		REQUIRE(back->frames[f].duration_milliseconds == ase->frames[f].duration_milliseconds);
		REQUIRE(!memcmp(back->frames[f].pixels, ase->frames[f].pixels, sizeof(ase_color_t) * (size_t)(ase->w * ase->h)));
	}
	for (int t = 0; t < ase->tag_count; ++t) {
		REQUIRE(!strcmp(back->tags[t].name, ase->tags[t].name));
		REQUIRE(back->tags[t].from_frame == ase->tags[t].from_frame && back->tags[t].to_frame == ase->tags[t].to_frame);
	}
	cf_free(data);
	cute_aseprite_free(back);
	cute_aseprite_free(ase);
	return true;
}

TEST_SUITE(test_aseprite)
{
	RUN_TEST_CASE(test_aseprite_make_destroy);
	RUN_TEST_CASE(test_aseprite_create_save_load);
	RUN_TEST_CASE(test_aseprite_resave_girl);
}
