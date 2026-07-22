# AGENTS.md

This document provides essential context for AI assistants (Claude, GPT, Copilot, Cursor, etc.) working with the Cute Framework codebase.

## Project Overview

Cute Framework is a **2D game development framework** written in C/C++ designed to be portable, lightweight, and easy to use. It provides a complete solution for creating 2D games across multiple platforms including Windows, macOS, Linux, iOS, Android, and web browsers (via Emscripten).

**Key Features**:
- Cross-platform support with SDL3 as the platform abstraction layer
- Modern C++20 codebase with CMake build system (requires CMake 4.2+)
- Comprehensive graphics pipeline with shader cross-compilation
- Full audio, input, networking, and file system support
- 50+ sample programs demonstrating framework capabilities
- Extensive documentation at https://randygaul.github.io/cute_framework/

## Code Style and Conventions

When contributing to Cute Framework, follow these established coding conventions:

### Naming Conventions

**Functions**:
- Public API: `cf_` prefix with snake_case (e.g., `cf_make_app`, `cf_draw_sprite`)
- Internal functions: `s_` prefix for static functions (e.g., `s_init_video`)
- C++ namespace functions: snake_case without prefix (e.g., `sprite_draw`, `app_update`)

**Types and Structures**:
- Structs: `CF_` prefix with PascalCase (e.g., `CF_Sprite`, `CF_Canvas`, `CF_Material`)
- Enums: `CF_` prefix with UPPER_SNAKE_CASE (e.g., `CF_DISPLAY_ORIENTATION_LANDSCAPE`)
- Typedefs: Same as the underlying type convention

**Macros and Constants**:
- Macros: `CF_` prefix with UPPER_SNAKE_CASE (e.g., `CF_INLINE`, `CF_GLOBAL`, `CF_STATIC_ASSERT`)
- Constants: UPPER_SNAKE_CASE or follow enum convention

**Variables**:
- Local variables: snake_case (e.g., `display_id`, `rect`)
- Member variables: snake_case
- Global variables: Typically avoided, but when necessary use descriptive names

### File Organization

**Header Files** (.h):
- Include guards: `CF_[FILENAME]_H` (e.g., `CF_GRAPHICS_H`)
- Copyright notice at the top
- C API wrapped in `extern "C"` for C++ compatibility
- Comprehensive Doxygen-style documentation for public APIs
- Related functions grouped together with `@related` tags
- Many headers define enums via X-macro lists (`CF_*_DEFS`)

**Implementation Files** (.cpp/.c):
- Copyright notice at the top
- System/external includes first, then internal headers
- Static functions and variables at file scope with `s_` prefix
- Logical grouping of related functions

### Documentation Style

**API Documentation**:
```c
/**
 * @function function_name
 * @category category_name
 * @brief    Brief one-line description.
 * @param    param_name    Description of parameter.
 * @return   Description of return value.
 * @remarks  Additional details and usage notes.
 * @related  Related functions for cross-reference.
 */
```

### Code Formatting

**Indentation and Spacing**:
- Use tabs for indentation
- Opening braces on the same line for functions and control structures
- Space after control keywords: `if (`, `while (`, `for (`
- No space after function names: `function_name(`

**Line Length**:
- Generally keep lines under 120 characters
- Break long function signatures across multiple lines if needed

### C++ Specific Conventions

**Namespace**:
- Primary namespace: `Cute` (using `namespace Cute;` in implementation files)
- C++ wrapper functions use snake_case without cf_ prefix

**Modern C++ Features**:
- Target C++20 standard
- Use `CF_INLINE` macro for inline functions (cross-platform compatibility)
- Prefer stack allocation and value semantics where appropriate

### Platform-Specific Code

**Conditional Compilation**:
- Use `#ifdef CF_WINDOWS`, `#ifdef CF_APPLE`, `#ifdef CF_LINUX`, `#ifdef CF_EMSCRIPTEN`
- Platform detection handled in the `# Platform detection.` block near the top of CMakeLists.txt

**Portability Macros**:
- `CF_INLINE` - Platform-appropriate inline keyword
- `CF_GLOBAL` - Global variable declaration
- `CF_STATIC_ASSERT` - Compile-time assertions

### Memory Management

- Prefer stack allocation when possible
- Use `cf_alloc`/`cf_free` for dynamic allocation (not malloc/free)
- RAII patterns in C++ code
- Clear ownership semantics for allocated resources

### Error Handling

- Return `CF_Result` for operations that can fail
- Use `is_error(result)` to check for errors
- Provide meaningful error messages via result system

### Deprecation Pattern

When renaming public API functions:
- The old name remains the real implementation/declaration
- The new name is a `CF_INLINE` forwarding function
- The old name gets `@deprecated` in its doc comment
- C++ wrappers in `namespace Cute` are updated in tandem

## Build Commands

### Building the Project
```bash
# Standard build (debug)
cmake -B build/debug -S . -DCMAKE_BUILD_TYPE=Debug
cmake --build build/debug

# Release build
cmake -B build/release -S . -DCMAKE_BUILD_TYPE=Release
cmake --build build/release

# Web build (Emscripten)
emcmake cmake -B build/web -S .
cmake --build build/web

# Build with specific options
cmake -B build -S . \
  -DCF_FRAMEWORK_STATIC=ON \
  -DCF_CUTE_SHADERC=ON
```

### Running Tests
```bash
# Build and run all tests
cmake --build build/debug
./build/debug/tests

# The test executable uses pico_unit framework
# Test files are in test/ directory with pattern test_*.cpp
```

### Building Samples
```bash
# Samples are automatically built with the main project
# Run a specific sample
./build/debug/[sample_name]

# Key sample programs (50+ available, see samples/):
# Basic: hello_triangle, basic_sprite, basic_input, basic_shapes, basic_camera
# Graphics: basic_instancing, basic_indexed_rendering, stencil_pie_chart, draw_to_texture, compute
# Effects: metaballs, waves, fluid_sim, shallow_water, rainbow_liquid
# Games: spaceshooter, platformer
# ImGui: imgui, imgui_texture
```

## Architecture Overview

### Core Systems

**Graphics Pipeline**:
- SDL_gpu-based renderer (src/cute_graphics_sdlgpu.cpp)
- OpenGL ES 3 renderer for web builds using Emscripten (src/cute_graphics_glad.cpp)
- Runtime shader compilation always available via CF's own GLSL-to-SPIR-V compiler (libraries/cute/cute_spirv.h)
- SPIRV-Cross translates SPIR-V for whichever backend is active

**Component Structure**:
- `src/cute_*.cpp` - Core framework components (app, audio, graphics, input, etc.)
- `src/internal/cute_*_internal.h` - Internal implementation headers
- `include/cute_*.h` - Public API headers
- Single header entry point: `include/cute.h`
- `libraries/` - Vendored dependencies (imgui/cimgui, glad, stb, pico, minicoro, single-header cute libs)
- `tools/` - Shader compiler front-end (cute-shader), offline compiler (cute-shaderc), docs_parser

**Rendering Architecture**:
- Mesh system with vertex attributes (CF_MeshInternal)
- Buffer management (vertex, index, instance buffers)
- Canvas-based rendering with default and custom canvases
- Shader uniforms mapped from CF_ShaderInfo types

**ImGui Integration**:
- Wrapper at src/cute_imgui.cpp with internal header cute_imgui_internal.h

### Platform Support

The framework uses SDL3 as the platform abstraction layer and supports:
- Windows (D3D12/Vulkan)
- macOS/iOS (Metal)
- Linux (Vulkan/OpenGL)
- Web (WebGL2/OpenGL ES3 via Emscripten)

Platform detection happens in the `# Platform detection.` block near the top of CMakeLists.txt, with specific build configurations for each target.

### Dependencies

**Vendored** (in libraries/):
- SDL3_shadercross (SPIR-V to backend shader translation)
- imgui + cimgui (immediate mode GUI)
- glad (OpenGL loader for web builds)
- stb, pico (pico_unit), edubart (minicoro), dxc
- cute/ (single-header cute libs: ckit.h, cute_spirv.h, cute_net.h, cute_sound.h, ...)

**Fetched via CMake FetchContent**:
- SDL3 (release archive; Emscripten provides its own)
- PhysicsFS (virtual filesystem)
- SPIRV-Cross (shader translation)
- s2n (TLS, Linux only)

**Internal Libraries** (in src/internal/):
- yyjson (JSON parsing)
- Various internal headers for subsystems

### Shader System

The framework has a sophisticated shader compilation pipeline:
- Runtime compilation always available; CF ships its own GLSL-to-SPIR-V compiler (libraries/cute/cute_spirv.h, no external dependencies)
- Front-end library cute-shader (tools/cute_shader.cpp); SPIRV-Cross translates SPIR-V for the active backend
- Offline compiler tool cute-shaderc (tools/cute_shaderc.cpp, CF_CUTE_SHADERC option)
- Builtin shaders in tools/builtin_shaders.h
- Compiler unit tests: cute-spirv-tests (tools/cute_spirv_test.c)

### File Organization

**Directory Structure**:
```
cute_framework/
├── include/              # Public API headers (42 files)
│   ├── cute.h           # Single header entry point (includes all)
│   └── cute_*.h         # Individual subsystem headers
├── src/                 # Implementation files (33 C++ files)
│   ├── cute_*.cpp       # Core framework components
│   ├── internal/        # Internal implementation headers (+ yyjson)
│   └── data/            # Embedded data (joypad mapping db, fonts)
├── tools/               # cute-shader, cute-shaderc, docs_parser, cute-spirv-tests
├── libraries/           # Vendored dependencies
│   ├── cute/            # Single-header cute libs (ckit.h, cute_spirv.h, ...)
│   ├── imgui/, cimgui/  # Dear ImGui + C bindings
│   └── [glad, stb, pico, SDL3_shadercross, ...]
├── samples/             # 50+ example programs
├── test/                # Unit tests (20+ modules using pico_unit)
├── docs/                # MkDocs documentation source
├── CMakeLists.txt       # Main build configuration
├── README.md            # Project overview
└── AGENTS.md            # This file - AI assistant guide
```

**Key Configuration Files**:
- `CMakeLists.txt` - Platform detection (top `# Platform detection.` block) and build options
- `mkdocs.yml` - Documentation site configuration
- `web.cmd` - Emscripten web build helper

### Documentation

**Building Documentation**:
```bash
# Generate API reference
./build/debug/docs_parser

# Serve documentation locally
mkdocs serve

# Build documentation site
mkdocs build
```

The API reference is available in the docs/ directory, when run using mkdocs and the docs_parser binary.
Full documentation is available at https://randygaul.github.io/cute_framework/api_reference/.

### Testing

**Test Infrastructure**:
- Framework: pico_unit (lightweight C++ testing)
- Location: `test/` directory
- Pattern: `test_*.cpp` files
- Coverage: Core functionality, math, strings, collections, etc.

```bash
# Run all tests
./build/debug/tests

# Tests are automatically built with the project
# Test results are printed to console with pass/fail status
```
