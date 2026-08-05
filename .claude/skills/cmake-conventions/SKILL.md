---
name: cmake-conventions
description: Modern CMake conventions for Cute Framework, SDL-inspired. Reference before writing or modifying any CMakeLists.txt or cmake/ file in this repo.
user-invocable: false
---

# CMake Conventions (SDL-inspired)

Cute Framework is a framework consumed by other projects. Every CMake change
must keep it well-behaved both standalone and as a dependency (FetchContent /
add_subdirectory / installed package). SDL3 is the reference implementation
of these practices.

## Consumability rules

- **Namespaced targets.** Consumers link `cute::cute`, never bare `cute`.
  Any new library target gets `add_library(cute::<name> ALIAS <name>)`.
- **Export sets.** Installed targets use
  `install(TARGETS ... EXPORT cute-targets ...)` +
  `install(EXPORT cute-targets NAMESPACE cute:: DESTINATION lib/cmake/cute)`.
- **Config package.** `find_package(cute CONFIG)` must work:
  `configure_package_config_file` + `write_basic_package_version_file`
  (SameMajorVersion). The config file declares dependencies with
  `find_dependency` — a static cute must propagate what it links.
- **Headers.** Public headers are attached to the target via
  `target_sources(cute PUBLIC FILE_SET HEADERS BASE_DIRS include FILES ...)`
  and installed through the FILE_SET (CMake ≥ 4.2 is required, so FILE_SET
  is always available).
- **Interface hygiene.** Every public include dir carries BOTH generator
  expressions: `$<BUILD_INTERFACE:...>` and `$<INSTALL_INTERFACE:...>`.
- **No global state.** Never `add_definitions`, bare `add_compile_options`,
  or `set(CMAKE_*_FLAGS ...)` at directory scope for things a consumer would
  inherit. Use `target_compile_definitions/options/features(... PRIVATE|PUBLIC)`.
  Anything that must be global (output dirs, folders) goes behind
  `if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)` — i.e. only when cute is
  the top-level project.
- **Options.** All options are `CF_`-prefixed, declared with `option()`, and
  work in any combination when cute is a subproject.
- **Never** hardcode compiler flags a consumer can't override; prefer
  `target_compile_features(cute PUBLIC cxx_std_20 c_std_23)` over setting
  global CMAKE_CXX_STANDARD for consumers.

## Repo-specific rules

- **Vendored SDL3 must win the include-path race.** The build vendors SDL
  via FetchContent; its include dirs must stay ahead of any system SDL
  (use `BEFORE` where needed). Rationale: a Homebrew `CPATH` export can leak
  a newer system SDL3 with an incompatible ABI into the build.
- **Registration points** (a file that exists but is unregistered builds
  green and does nothing):
  - new `src/*.cpp` → `CF_SRCS` in the root `CMakeLists.txt`
  - new public header → `#include` in `include/cute.h`
  - new `test/test_*.cpp` → `CF_TEST_SRCS` in `test/CMakeLists.txt` AND
    `TEST_SUITE(...)`/`RUN_TRACED(...)` in `test/main.cpp`
  - new sample → `add_sample(<target> <source>)` in `samples/CMakeLists.txt`
- **Emscripten sample assets:** a sample with a `<name>_data/` folder needs
  `target_link_options(<target> PRIVATE --preload-file
  "${CMAKE_CURRENT_SOURCE_DIR}/<name>_data@/<name>_data")` inside the
  `if (EMSCRIPTEN)` block of `samples/CMakeLists.txt`, or the web build
  ships a sample that exits at startup.
- **Platform detection** happens once, near the top of the root
  CMakeLists.txt (EMSCRIPTEN first so it doesn't fall into the UNIX path);
  extend that block rather than sprinkling `if(APPLE)` checks.
- **Version files** `include/cute_version.h` and `src/cute_version.cpp` are
  generated via `configure_file` — edit the `.in` templates.

## Checklist for any CMake change

1. Does it still configure as a subproject? Quick check:
   a scratch consumer with `FetchContent_Declare(cute SOURCE_DIR <repo>)`.
2. Did you introduce any directory-scope flags/definitions? Move them onto
   targets.
3. New target? Add the `cute::` alias and decide install/export membership.
4. Anything user-visible (option, target name) documented in README/docs?
