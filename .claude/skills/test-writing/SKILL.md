---
name: test-writing
description: How to write and run Cute Framework tests — harness macros, registration points, headless runs, and suite traps. Use when adding or modifying anything under test/.
---

# Writing Cute Framework Tests

The test suite is one binary (`build/tests`) built from `test/`, using
pico_unit (`libraries/pico/pico_unit.h`) via `test/test_harness.h`.

## Adding a test

Three registration points — miss one and the test silently never runs:

1. Create `test/test_<topic>.cpp`:

   ```c
   #include "test_harness.h"
   #include <cute.h>
   using namespace Cute;

   TEST_CASE(test_<topic>_does_thing)
   {
       REQUIRE(1 + 1 == 2);
       return true;
   }

   TEST_SUITE(test_<topic>)
   {
       RUN_TEST_CASE(test_<topic>_does_thing);
   }
   ```

2. Add `test_<topic>.cpp` to `CF_TEST_SRCS` in `test/CMakeLists.txt`.
3. In `test/main.cpp`: add `TEST_SUITE(test_<topic>);` to the declarations
   AND `RUN_TRACED(test_<topic>);` to the run list.

Macros: `REQUIRE(cond)` (truthy), `CHECK(x)` = `REQUIRE(!(x))` (for
0-means-success results), `CHECK_POINTER(x)`. Test cases return `true`.

## Running

- Build: `cmake --build build --target tests`
- All: `./build/tests`
- **CLI filters by SUITE name only**: `./build/tests test_draw3d test_mrt`.
  Passing a CASE name silently runs nothing (Total: 0) — that is the trap.
- `CF_TEST_DUMP=1` writes readback dumps (`build/dump_*.png`) for graphics
  tests. Linux CI runs everything under
  `xvfb-run -a -s "-screen 0 1280x720x24"` with `SDL_AUDIODRIVER=dummy`
  and `LIBGL_ALWAYS_SOFTWARE=1`.

## Traps (all learned the hard way)

- **Baseline first.** Run the full suite on a clean master BEFORE judging
  your branch — this Retina Mac has known display-dependent failures.
  Compare failure lists, not pass percentages.
- **`AppDestroyGuard` is a reserved name.** Defining a same-named struct
  with a different inline dtor in another test file is an ODR violation —
  the linker silently merges them and you get a segfault in an unrelated
  suite. Use a distinct guard-struct name per file (`OwnedAppGuard` etc.).
- **Display-query tests must run before any app create/destroy** in a
  suite: `cf_destroy_app` calls `SDL_Quit()`, after which
  `cf_display_count()` reports 0.
- **Apps are shared between tests** via `test_app_shared.h` fixtures
  (`test_make_app`/`test_destroy_app`); don't create raw apps in graphics
  tests — reuse the fixture, and read its header before touching lifecycle.
- **Graphics readback:** results are only valid after submit — draw, call
  the readback helper from `test_app_shared.h`, THEN assert pixels.
- **TDD default:** write the failing test, watch it fail
  (`./build/tests test_<topic>`), then implement.
