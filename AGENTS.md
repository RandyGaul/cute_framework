# AGENTS.md

This document provides essential context for AI assistants (Claude, GPT, Copilot, Cursor, etc.) working with the Cute Framework codebase

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

# Available samples include:
# imgui, spaceshooter, waves, metaballs, hello_triangle, etc.
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

### Documentation

The API reference is available in the docs/ directory, when run using mkdocs and the docs_parser binary.
Full documentation is availabe at https://randygaul.github.io/cute_framework/api_reference/.
