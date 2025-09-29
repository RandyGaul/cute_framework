# AGENTS.md

This document provides essential context for AI assistants (Claude, GPT, Copilot, Cursor, etc.) working with the Cute Framework codebase.

## Project Overview

Cute Framework is a **2D game development framework** written in C/C++ designed to be portable, lightweight, and easy to use. It provides a complete solution for creating 2D games across multiple platforms including Windows, macOS, Linux, iOS, Android, and web browsers (via Emscripten).

**Key Features**:
- Cross-platform support with SDL3 as the platform abstraction layer
- Modern C++20 codebase with CMake build system (requires CMake 3.14+)
- Comprehensive graphics pipeline with shader cross-compilation
- Full audio, input, networking, and file system support
- 40+ sample programs demonstrating framework capabilities
- Extensive documentation at https://randygaul.github.io/cute_framework/

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
  -DCF_RUNTIME_SHADER_COMPILATION=ON \
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

# Key sample programs (40+ available):
# Basic: hello_triangle, basic_sprite, basic_input, basic_networking
# Graphics: instancing, indexed_rendering, stencil, render_to_texture
# Effects: metaballs, waves, fluid_sim, water, particles
# Games: spaceshooter, platformer, tetris
# ImGui: imgui, imgui_custom_font, imgui_backend
# Tools: docs_parser (generates API documentation)
```

## Architecture Overview

### Core Systems

**Graphics Pipeline**:
- SDL_gpu-based renderer
- OpenGL ES 3 renderer for web builds using Emscripten
- Shader system supports runtime compilation via cute_shader/ subsystem
- Bytecode generation for cross-platform shader support

**Component Structure**:
- `src/cute_*.cpp` - Core framework components (app, audio, graphics, input, etc.)
- `src/internal/cute_*_internal.h` - Internal implementation headers
- `include/cute_*.h` - Public API headers
- Single header entry point: `include/cute.h`
- `libraries/` - External vendored dependencies (imgui, minicoro, stb, etc.

**Rendering Architecture**:
- Mesh system with vertex attributes (CF_MeshInternal)
- Buffer management (vertex, index, instance buffers)
- Canvas-based rendering with default and custom canvases
- Shader uniforms mapped from CF_ShaderInfo types

**ImGui Integration**:
- Wrapper at src/cute_imgui.cpp with internal header cute_imgui_internal.h

### Platform Support

The framework uses SDL3 as the platform abstraction layer and supports:
- Windows (D3D11/D3D12/Vulkan)
- macOS/iOS (Metal)
- Linux (Vulkan/OpenGL)
- Web (WebGL2/OpenGL ES3 via Emscripten)

Platform detection happens in CMakeLists.txt:30-55, with specific build configurations for each target.

### Dependencies

**External Libraries** (in libraries/):
- SDL3 (fetched via CMake)
- SDL3_shadercross (shader cross-compilation)
- PhysicsFS (virtual filesystem)
- imgui (immediate mode GUI)
- glad (OpenGL loader)

**Internal Libraries** (in src/internal/):
- yyjson (JSON parsing)
- Various internal headers for subsystems

### Shader System

The framework has a sophisticated shader compilation pipeline:
- Runtime compilation support (when CF_RUNTIME_SHADER_COMPILATION=ON)
- Offline compiler tool (cute-shaderc)
- Cross-platform bytecode generation
- Builtin shaders in src/cute_shader/builtin_shaders.h
- Bytecode cache in src/data/builtin_shaders_bytecode.h

### File Organization

**Directory Structure**:
```
cute_framework/
├── include/              # Public API headers (42 files)
│   ├── cute.h           # Single header entry point (includes all)
│   └── cute_*.h         # Individual subsystem headers
├── src/                 # Implementation files (37 C++ files)
│   ├── cute_*.cpp       # Core framework components
│   ├── internal/        # Internal implementation headers
│   └── cute_shader/     # Shader compilation system
├── libraries/           # External dependencies
│   ├── dear_bindings/   # ImGui with SDL3 backend
│   ├── physfs/          # Virtual filesystem
│   └── [various single-header libs]
├── samples/             # 40+ example programs
├── test/                # Unit tests (15+ modules using pico_unit)
├── docs/                # MkDocs documentation source
├── CMakeLists.txt       # Main build configuration
├── README.md            # Project overview
└── AGENTS.md            # This file - AI assistant guide
```

**Key Configuration Files**:
- `CMakeLists.txt:30-55` - Platform detection and configuration
- `mkdocs.yml` - Documentation site configuration
- `msvc2022.cmd` - Windows Visual Studio build helper
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
