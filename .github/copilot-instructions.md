# Cute Framework – Copilot Instructions

2D game framework in C/C++20. SDL3 platform layer. CMake 4.2+. Targets: Windows, macOS, Linux, iOS, Android, Web (Emscripten).

## Build & Test

```bash
# Debug build + tests (the normal workflow)
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build
./build/tests

# Release
cmake -B build/release -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build/release

# Emscripten
emcmake cmake -B build/web && cmake --build build/web
```

Key CMake options: `-DCF_FRAMEWORK_STATIC=ON|OFF`, `-DCF_RUNTIME_SHADER_COMPILATION=ON|OFF`, `-DCF_CUTE_SHADERC=ON|OFF`  
Tests use **pico_unit**. Test files: `test/test_*.cpp`. Test binary: `./build/tests`.

## Directory Layout

```
include/cute_*.h      Public API headers (single umbrella: include/cute.h)
src/cute_*.cpp        Implementations
src/internal/         Internal headers (cute_*_internal.h) — not for public API
libraries/            Vendored deps (imgui, physfs, SDL3, glad, stb, …)
samples/              40+ standalone samples
test/                 pico_unit tests
src/cute_shader/      Shader compilation subsystem
src/data/             Builtin shader bytecode cache
```

## Naming

| Kind | Convention | Example |
|---|---|---|
| Public C function | `cf_` + snake_case | `cf_make_sprite` |
| Public C type/struct/enum | `CF_` + PascalCase | `CF_Sprite` |
| Enum value / macro / constant | `CF_` + UPPER_SNAKE | `CF_PIXEL_FORMAT_R8` |
| Internal/static function | `s_` + snake_case | `s_init_video` |
| C++ namespace wrapper | snake_case (no prefix) | `sprite_draw` |

C++ namespace: `Cute`. In `.cpp` files: `using namespace Cute;`.

## Code Style

- Tabs for indentation; braces on same line; `if (` / `while (` / `for (` with space; no space before `(` in calls.
- Lines ≤ 120 chars.
- Dynamic allocation: `cf_alloc`/`cf_free` (never `malloc`/`free`).
- Error returns: `CF_Result`; check with `is_error(result)`.
- Lifecycle: `cf_make_<name>` / `cf_destroy_<name>`.
- Platform guards: `CF_WINDOWS`, `CF_APPLE`, `CF_LINUX`, `CF_EMSCRIPTEN`, `CF_IOS`, `CF_ANDROID`.
- Portability macros: `CF_INLINE`, `CF_GLOBAL`, `CF_STATIC_ASSERT`, `CF_API`, `CF_CALL`.

## Header Structure

```c
/*
	Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/
#ifndef CF_<NAME>_H   // strip "cute_", uppercase
#define CF_<NAME>_H

#include "cute_defines.h"
// CF headers: quotes; system/SDL headers: angle brackets

#ifdef __cplusplus
extern "C" {
#endif

// ... C API declarations with /** @function / @struct / @enum ... */ doc blocks ...

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
namespace Cute {
// ... C++ wrappers (snake_case, no cf_ prefix) ...
} // namespace Cute
#endif

#endif // CF_<NAME>_H
```

## API Documentation (all public symbols)

```c
/**
 * @function cf_example
 * @category graphics
 * @brief    One-line summary.
 * @param    x   Description.
 * @return   Description.
 * @remarks  Extended notes.
 * @related  CF_Foo cf_make_foo cf_destroy_foo
 */
```

Tag order: `@function`/`@struct`/`@enum` → `@category` → `@brief` → `@param` → `@return` → `@remarks` → `@example` → `@related`.  
Structs use `/* @member */` inline; enums use X-macro `CF_<NAME>_DEFS` pattern with `/* @entry */`.  
Never use `///`; always `/** ... */`.

## Adding a New Subsystem

1. `include/cute_<name>.h` — public API (see header structure above).
2. `src/cute_<name>.cpp` — implementation.
3. Add `.cpp` to `CF_SRCS` in `CMakeLists.txt`.
4. Add `.h` to `CF_PUBLIC_HDRS` in `CMakeLists.txt`.
5. Add `#include "cute_<name>.h"` to `include/cute.h` (maintain existing ordering).
6. If internal state needed: `src/internal/cute_<name>_internal.h`.

## Shader Files

- `*_shd.h` files are **generated** by `build/cute-shaderc` — do not edit manually.
- `include/cute_shader_bytecode.h` defines shared shader structures — **hand-written**, edit normally.
- Builtin shaders: `src/cute_shader/builtin_shaders.h`; bytecode cache: `src/data/builtin_shaders_bytecode.h`.

## Code Review

Comments must be succinct and compact. Audience are expert framework authors — skip obvious explanations, be direct.

## Common Pitfalls

- The build requires CMake **4.2+** (`cmake_minimum_required(VERSION 4.2)` in CMakeLists.txt). Older CMake will fail.
- Web builds: tests binary is not built for Emscripten (no `./build/tests` step).
- CI uses `cmake -B build -G Ninja` (no explicit `build/debug` subdir) — match this in local workflows.
- `include/cute_version.h` and `src/cute_version.cpp` are **generated** from `.in` templates by CMake `configure_file`; do not edit the generated files directly.
