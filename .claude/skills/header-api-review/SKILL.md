---
name: header-api-review
description: Cute Framework public header conventions and review checklist. Reference before creating or modifying any file in include/.
user-invocable: false
---

# CF Header Conventions

## File Naming
- Public headers: `cute_<name>.h` in `include/`
- Source files: `cute_<name>.cpp` in `src/`

## Generated vs Hand-Written Headers
- **Generated** (do NOT edit manually): `*_shd.h` shader bytecode files produced by `build/cute-shaderc`
- **Hand-written** (edit normally): all other `include/cute_*.h` files, including `cute_shader_bytecode.h` which defines shared shader structures and is NOT generated

## Include Guards
Format: `CF_<NAME>_H` where `<NAME>` is the filename with `cute_` prefix stripped, uppercased.
- `cute_graphics.h` → `CF_GRAPHICS_H`
- `cute_sprite.h` → `CF_SPRITE_H`
- `cute.h` → `CF_H`

## Copyright Header
Every file must begin with exactly:
```
/*
	Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/
```

## Header Structure
```c
#ifndef CF_NAME_H
#define CF_NAME_H

#include "cute_defines.h"
// ... other includes with quotes for CF headers, angle brackets for system/SDL ...

//--------------------------------------------------------------------------------------------------
// C API

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

// ... C declarations ...

#ifdef __cplusplus
}
#endif // __cplusplus

//--------------------------------------------------------------------------------------------------
// C++ API

#ifdef __cplusplus
namespace Cute {
// ... C++ wrappers ...
} // namespace Cute
#endif // __cplusplus

#endif // CF_NAME_H
```

## Naming Conventions
- Public C functions: `cf_` prefix, snake_case → `cf_make_sprite`, `cf_draw_line`
- Public C structs/enums/typedefs: `CF_` prefix → `CF_Sprite`, `CF_PIXEL_FORMAT_R8`
- X-macro enum patterns: `CF_<NAME>_DEFS` macro + `CF_<NAME>` enum typedef
- Static internal functions: `s_` prefix — never in public headers
- C++ namespace: `namespace Cute`, functions snake_case without `cf_` prefix

## Lifecycle Functions
Prefer `cf_make_<name>(...)` for creation and `cf_destroy_<name>(...)` for destruction.
Avoid other lifecycle verbs unless strongly motivated.

## Deprecation Pattern
Old name stays as the real implementation; new name is a `CF_INLINE` forwarder (or vice versa).
The deprecated symbol's doc comment must include `@deprecated Use cf_new_name instead.`
```c
/**
 * @function cf_old_function
 * @category example
 * @brief    Does the thing.
 * @deprecated Use cf_new_function instead.
 * @related  cf_new_function
 */
CF_INLINE void cf_old_function(int x) { cf_new_function(x); }
```

## Documentation Format

All public declarations use `/** ... */` block comments with ` * ` on every interior line.
**Never use `///` style comments for documentation.**

### Tag ordering (follow this sequence)
1. `@function` / `@struct` / `@enum` — declaration kind; value is the symbol name
2. `@category` — groups symbol in docs (e.g. `graphics`, `audio`, `input`, `allocator`, `sprite`)
3. `@brief` — one-line summary
4. `@param` — one entry per parameter (omit if none)
5. `@return` — return value description (omit for `void`)
6. `@remarks` — extended notes, caveats, usage details (optional)
7. `@example` — code example with `>` title (optional)
8. `@related` — space-separated list of related symbols (highly recommended)

### Function / typedef / macro
```c
/**
 * @function cf_noise2
 * @category noise
 * @brief    Generates a random value given a 2D coordinate.
 * @param    noise  The noise settings.
 * @param    x      Noise at this x-component.
 * @param    y      Noise at this y-component.
 * @return   Returns a random value at the specified point.
 * @remarks  You're probably looking for image generation functions such as `cf_noise_pixels` or
 *           `cf_noise_fbm_pixels`. This function is fairly low-level.
 * @related  CF_Noise cf_make_noise cf_destroy_noise cf_noise2 cf_noise3 cf_noise4
 */
CF_API float CF_CALL cf_noise2(CF_Noise noise, float x, float y);
```

### Struct — with `// @end` marker and `/* @member */` inline comments
```c
/**
 * @struct   CF_Result
 * @category utility
 * @brief    Information about the result of a function, containing any potential error details.
 * @remarks  Check if a result is an error or not with `cf_is_error`.
 * @related  CF_Result cf_is_error cf_result_make cf_result_error cf_result_success
 */
typedef struct CF_Result
{
	/* @member Either 0 for success, or -1 for failure. */
	int code;

	/* @member String containing details about any error encountered. */
	const char* details;
} CF_Result;
// @end
```

### Enum — X-macro pattern with `/* @entry */` and `/* @end */`
```c
/**
 * @enum     CF_PlayDirection
 * @category sprite
 * @brief    The direction a sprite plays frames.
 * @related  CF_PlayDirection cf_play_direction_to_string CF_Animation
 */
#define CF_PLAY_DIRECTION_DEFS \
	/* @entry Flips through the frames of an animation forwards. */ \
	CF_ENUM(PLAY_DIRECTION_FORWARDS, 0) \
	/* @entry Flips through the frames of an animation backwards. */ \
	CF_ENUM(PLAY_DIRECTION_BACKWARDS, 1) \
	/* @end */

typedef enum CF_PlayDirection
{
	#define CF_ENUM(K, V) CF_##K = V,
	CF_PLAY_DIRECTION_DEFS
	#undef CF_ENUM
} CF_PlayDirection;
```

### @example — indented code, title after `>`
```c
 * @example > Creating a dynamic array and freeing it afterwards.
 *           dyna int* a = NULL;
 *           apush(a, 5);
 *           afree(a);
```
Code lines are indented to align under the title text (no triple-backtick fences in `@example`).

### @remarks — embedded code uses fenced blocks
When `@remarks` contains a multi-line code snippet, use fenced ` ```c ` blocks:
```c
 * @remarks  The members are stored tightly as an array. To access them:
 *
 *           ```c
 *           for (int i = 0; i < info.num_uniforms; ++i) {
 *               printf("%s\n", info.uniforms[i].block_name);
 *           }
 *           ```
```

### Formatting rules
- **@param alignment**: pad the parameter name so all descriptions align in a column
- **@remarks continuation**: indent continuation lines to align with the first word of the description
- **@related**: single space-separated line; include both the type (`CF_Foo`) and related functions
- **Inline code**: backticks around all symbol names in prose: `` `cf_make_noise` ``, `` `CF_Noise` ``
- **Links**: standard Markdown `[text](url)` for external docs
- **"Default X" / "Out parameter"**: suffix in `@param` descriptions where applicable

## Includes
- CF headers: quotes → `#include "cute_defines.h"`
- System/SDL headers: angle brackets → `#include <SDL3/SDL.h>`
- `cute_defines.h` is almost always needed (provides `CF_INLINE`, `CF_API`, `CF_GLOBAL`, etc.)

## New Header Checklist
- [ ] Copyright header (exact format)
- [ ] Include guard `CF_<NAME>_H`
- [ ] `#include "cute_defines.h"` at minimum
- [ ] `extern "C" { ... }` wrapping for C section
- [ ] All public C functions: `cf_` prefix
- [ ] All public types: `CF_` prefix
- [ ] `/** ... */` Doxygen comments on all public declarations with `@function`/`@struct`/`@enum`, `@category`, `@brief`, and `@related` tags
- [ ] `namespace Cute` C++ wrappers section
- [ ] Added to `include/cute.h` umbrella include (follow that file's existing include ordering conventions)
- [ ] Source file `cute_<name>.cpp` added to top-level `CMakeLists.txt` (in the `CF_SRCS` list)
