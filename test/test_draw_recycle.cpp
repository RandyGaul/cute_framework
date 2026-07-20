/*
	Cute Framework
	Copyright (C) 2026 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

#include "test_harness.h"

#include <cute.h>
using namespace Cute;

#include <cstdlib>

// Regression test for per-frame heap churn in the draw command list.
//
// Every CF_Command embeds a Cute::Array<spritebatch_sprite_t> "items" buffer -- the sprite
// instances belonging to that command. Historically, every processed command was destroyed
// at the end of each frame (in cf_render_layers_to's cleanup loop, and again in
// cf_app_draw_onto_screen's cmds.clear()), which freed that buffer. The next frame had to
// re-allocate and re-grow it from scratch via capacity-doubling, even when the scene's
// draw-command shape was perfectly stable frame to frame. This test proves that steady-state
// rendering causes zero items-buffer allocations or frees, by installing a counting allocator
// and watching cf_alloc/cf_free traffic across frames with a static and a changing draw-command
// count.

namespace
{
	struct AllocStats
	{
		int alloc_count = 0;
		int free_count = 0;
	};

	AllocStats* s_stats = NULL;

	// NOTE: `udata` is not used here -- cf_alloc/cf_free/cf_calloc/cf_realloc (cute_alloc.cpp)
	// currently call the overridden allocator's function pointers with a hardcoded NULL instead
	// of forwarding CF_Allocator::udata, so udata can't carry our AllocStats* through. That's a
	// separate, pre-existing bug outside this test's scope; we sidestep it with a file-local
	// static pointer instead.

	void* s_count_alloc(size_t size, void* udata)
	{
		CF_UNUSED(udata);
		s_stats->alloc_count++;
		return malloc(size);
	}

	void s_count_free(void* ptr, void* udata)
	{
		CF_UNUSED(udata);
		if (ptr) s_stats->free_count++;
		free(ptr);
	}

	void* s_count_calloc(size_t size, size_t count, void* udata)
	{
		CF_UNUSED(udata);
		s_stats->alloc_count++;
		return calloc(size, count);
	}

	void* s_count_realloc(void* ptr, size_t size, void* udata)
	{
		CF_UNUSED(udata);
		s_stats->alloc_count++;
		return realloc(ptr, size);
	}

	// Draws `count` filled boxes, each pushed under its own (strictly increasing) layer value.
	// Since a new CF_Command is only started when a pushed draw variable actually changes, this
	// guarantees `count` distinct commands are created this frame, each owning exactly one
	// spritebatch_sprite_t item -- the worst case for per-command items-buffer churn.
	void s_draw_frame(int count)
	{
		for (int i = 0; i < count; ++i) {
			draw_push_layer(i);
			draw_box_fill(make_aabb(V2(0, 0), 1.0f, 1.0f));
		}
		app_update();
		app_draw_onto_screen(true);
	}

	// Runs one measured frame at a fixed command count and returns how many cf_alloc/cf_free
	// calls it caused.
	AllocStats s_measure_frame(int count)
	{
		int allocs_before = s_stats->alloc_count;
		int frees_before = s_stats->free_count;
		s_draw_frame(count);
		AllocStats delta;
		delta.alloc_count = s_stats->alloc_count - allocs_before;
		delta.free_count = s_stats->free_count - frees_before;
		return delta;
	}

	// REQUIRE() (pico_unit.h) expands to a plain early `return false;` on failure -- exactly
	// the case this test exists to hit. Without RAII cleanup, a failing REQUIRE below would
	// leave the process-global allocator override pointing at s_stats, which is about to go
	// out of scope: every cf_alloc/cf_free call for the rest of the test run (i.e. the whole
	// engine, including all later test suites) would then dereference a dangling pointer.
	struct AllocatorGuard
	{
		AllocatorGuard(AllocStats* stats)
		{
			s_stats = stats;
			CF_Allocator allocator = { stats, s_count_alloc, s_count_free, s_count_calloc, s_count_realloc };
			cf_allocator_override(allocator);
		}

		~AllocatorGuard()
		{
			cf_allocator_restore_default();
			s_stats = NULL;
		}
	};

	struct AppGuard
	{
		~AppGuard() { destroy_app(); }
	};
}

TEST_CASE(test_draw_command_items_buffers_are_recycled)
{
	REQUIRE(!is_error(make_app(NULL, 0, 0, 0, 0, CF_APP_OPTIONS_HIDDEN_BIT | CF_APP_OPTIONS_NO_AUDIO_BIT, NULL)));
	AppGuard app_guard;

	AllocStats stats;
	AllocatorGuard allocator_guard(&stats);

	// Prime every size-dependent buffer (command items, vertex scratch, the outer command list)
	// at the largest command count we'll use below, so later measurements aren't polluted by
	// legitimate one-time growth allocations unrelated to the recycling behavior under test.
	for (int i = 0; i < 4; ++i) s_draw_frame(96);

	// Steady state at a fixed command count: once buffers are primed, drawing the exact same
	// shape of scene again should not touch the allocator at all.
	for (int i = 0; i < 4; ++i) s_draw_frame(32);
	AllocStats steady_small = s_measure_frame(32);
	REQUIRE(steady_small.alloc_count == 0);
	REQUIRE(steady_small.free_count == 0);

	// Scaling the per-frame command count back up should not free anything -- the buffers for
	// the extra commands are still sitting in the pool from the priming frames above.
	AllocStats grow = s_measure_frame(96);
	REQUIRE(grow.free_count == 0);

	// ...and steady state at that larger count is equally alloc/free free.
	for (int i = 0; i < 4; ++i) s_draw_frame(96);
	AllocStats steady_large = s_measure_frame(96);
	REQUIRE(steady_large.alloc_count == 0);
	REQUIRE(steady_large.free_count == 0);

	// Scaling back down: the pool now holds more buffers than this frame needs. Nothing should
	// be freed just because fewer commands were drawn.
	AllocStats shrink = s_measure_frame(32);
	REQUIRE(shrink.free_count == 0);

	return true;
}

TEST_SUITE(test_draw_recycle)
{
	RUN_TEST_CASE(test_draw_command_items_buffers_are_recycled);
}
