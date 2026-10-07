cmake_minimum_required(VERSION 4.2)
project(cute_framework
	DESCRIPTION "The cutest framework out there for creating 2D and 3D games in C++!"
	HOMEPAGE_URL "https://github.com/RandyGaul/cute_framework"
	LANGUAGES C CXX
	VERSION 1.1.1
)

# Must have at least C++20 and C23.
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_STANDARD 23)

include(cmake/StandardProjectSettings.cmake)
include(cmake/CuteSym.cmake)
include(GNUInstallDirs)
# These are needed for how we use FetchContent.
include(FetchContent)
set(FETCHCONTENT_QUIET FALSE)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Todo - Fix how turning some of these off breaks the build.
option(CF_FRAMEWORK_STATIC "Build static library for Cute Framework." ON)
option(CF_CUTE_SHADERC "Build cute-shaderc, an offline shader compiler." ON)
option(CF_CUTE_SYM "Build cute-sym, the crash reporter's symbol table tool (cf_symbols needs it)." ON)
option(CF_FRAMEWORK_APPLE_FRAMEWORK "Build CF libraries as Apple Framework" OFF)
option(CF_WEBGPU "Build the optional WebGPU graphics backend (emdawnwebgpu on the web, wgpu-native on desktop)." OFF)
set(CF_WGPU_NATIVE_DIR "" CACHE PATH "Desktop CF_WEBGPU builds: a wgpu-native release (include/webgpu/webgpu.h, lib/).")
option(CF_USE_SYSTEM_SDL "Use an installed SDL3 package (found via CMAKE_PREFIX_PATH or SDL3_DIR) instead of downloading and building SDL3." OFF)

# Make sure all libraries are placed into the same output folder.
if (CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
	set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
	set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
	set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
endif ()

# Platform detection.
# Put Emscripten first so it doesn't fall into the generic UNIX/Linux path.
if(CMAKE_SYSTEM_NAME STREQUAL "Emscripten" OR DEFINED EMSCRIPTEN)
	set(EMSCRIPTEN TRUE)

elseif(CMAKE_SYSTEM_NAME STREQUAL "Android" OR DEFINED ANDROID)
	set(ANDROID TRUE)

elseif(WIN32)
	set(WINDOWS TRUE)

elseif(APPLE)
	enable_language(OBJC)
	if(CMAKE_SYSTEM_NAME MATCHES ".*MacOS.*" OR CMAKE_SYSTEM_NAME MATCHES ".*Darwin.*")
		set(MACOSX TRUE)
	elseif(CMAKE_SYSTEM_NAME MATCHES ".*iOS.*")
		set(IOS TRUE)
	else()
		message(FATAL_ERROR "No supported Apple platform detected.")
	endif()

elseif(UNIX AND NOT APPLE)
	if(CMAKE_SYSTEM_NAME MATCHES ".*Linux")
		set(LINUX TRUE)
	else()
		message(FATAL_ERROR "No supported UNIX platform detected.")
	endif()

else()
	message(FATAL_ERROR "No supported platform detected.")
endif()

# Disable annoying MSVC warnings.
if(MSVC)
	add_definitions(-D_CRT_SECURE_NO_WARNINGS)
	add_definitions(-D_CRT_SECURE_NO_DEPRECATE)
	add_compile_options("$<$<C_COMPILER_ID:MSVC>:/utf-8>")
	add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")
	add_compile_options(/MP)
endif()

configure_file(
	"${CMAKE_CURRENT_SOURCE_DIR}/include/cute_version.h.in"
	"${CMAKE_CURRENT_SOURCE_DIR}/include/cute_version.h"
	@ONLY
)
configure_file(
	"${CMAKE_CURRENT_SOURCE_DIR}/src/cute_version.cpp.in"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/cute_version.cpp"
	@ONLY
)

# Common directories for compiler/linker path.
include_directories(src libraries test)

# Cute Framework shared library.
set(CF_SRCS
	src/cute_app.cpp
	src/cute_binding.cpp
	src/cute_ckit.cpp
	src/cute_audio.cpp
	src/cute_clipboard.cpp
	src/cute_multithreading.cpp
	src/cute_file_system.cpp
	src/cute_input.cpp
	src/cute_time.cpp
	src/cute_version.cpp
	src/cute_json.cpp
	src/cute_base64.cpp
	src/cute_math.cpp
	src/cute_math3d.cpp
	src/cute_physics.cpp
	src/cute_draw.cpp
	src/cute_draw3d.cpp
	src/cute_image.cpp
	src/cute_video.cpp
	src/cute_model.cpp
	src/cute_graphics.cpp
	src/cute_graphics_sdlgpu.cpp
	src/cute_graphics_gles.cpp
	src/cute_graphics_glad.cpp
	src/cute_aseprite_cache.cpp
	src/cute_custom_sprite.cpp
	src/cute_https.cpp
	src/cute_crash.cpp
	src/cute_joypad.cpp
	src/cute_symbol.cpp
	src/cute_sprite.cpp
	src/cute_coroutine.cpp
	src/cute_networking.cpp
	src/cute_guid.cpp
	src/cute_alloc.cpp
	src/cute_result.cpp
	src/cute_noise.cpp
	src/cute_imgui.cpp

	src/internal/yyjson.c

	libraries/imgui/imgui.cpp
	libraries/imgui/imgui_demo.cpp
	libraries/imgui/imgui_draw.cpp
	libraries/imgui/imgui_tables.cpp
	libraries/imgui/imgui_widgets.cpp
	libraries/imgui/backends/imgui_impl_sdl3.cpp
	libraries/imgui/dcimgui.cpp
	libraries/imgui/dcimgui_internal.cpp
	libraries/imgui/backends/imgui_impl_sdlgpu3.cpp
	libraries/imgui/backends/imgui_impl_opengl3.cpp

	libraries/glad/glad.c
)

if(APPLE)
	set(CF_SRCS ${CF_SRCS}
		src/internal/cute_tls.m
	)
endif()

set(CF_PUBLIC_HDRS
	include/cute_alloc.h
	include/cute_app.h
	include/cute_audio.h
	include/cute_user_config.h
	include/cute_c_runtime.h
	include/cute_clipboard.h
	include/cute_multithreading.h
	include/cute_defines.h
	include/cute_result.h
	include/cute_file_system.h
	include/cute_input.h
	include/cute_time.h
	include/cute_version.h
	include/cute_doubly_list.h
	include/cute_json.h
	include/cute_base64.h
	include/cute_array.h
	include/cute_map.h
	include/cute_string.h
	include/cute_defer.h
	include/cute_math.h
	include/cute_math3d.h
	include/cute_draw3d.h
	include/cute_draw.h
	include/cute_debug_printf.h
	include/cute_image.h
	include/cute_video.h
	include/cute_color.h
	include/cute.h
	include/cute_main_callbacks.h
	include/cute_graphics.h
	include/cute_rnd.h
	include/cute_sprite.h
	include/cute_custom_sprite.h
	include/cute_https.h
	include/cute_crash.h
	include/cute_joypad.h
	include/cute_priority_queue.h
	include/cute_symbol.h
	include/cute_coroutine.h
	include/cute_networking.h
	include/cute_guid.h
	include/cute_routine.h
	include/cute_noise.h
)

set(IMGUI_HDRS
	libraries/imgui/imgui.h
	libraries/imgui/imconfig.h
	libraries/imgui/imgui_internal.h
	libraries/imgui/imstb_rectpack.h
	libraries/imgui/imstb_textedit.h
	libraries/imgui/imstb_truetype.h
	libraries/imgui/dcimgui.h
	libraries/imgui/dcimgui_internal.h
)

set(CF_HDRS
	${CF_PUBLIC_HDRS}
	${IMGUI_HDRS}

	src/internal/cute_imgui_internal.h
	src/internal/cute_app_internal.h
	src/internal/cute_input_internal.h
	src/internal/cute_serialize_internal.h
	src/internal/cute_custom_sprite_internal.h
	src/internal/cute_draw_internal.h
	src/internal/cute_font_internal.h
	src/internal/cute_graphics_internal.h
	src/internal/cute_aseprite_cache_internal.h
	src/internal/cute_alloc_internal.h
	src/internal/cute_file_system_internal.h
	src/internal/yyjson.h
)

if(CF_FRAMEWORK_STATIC)
	add_library(cute STATIC ${CF_SRCS} ${CF_HDRS})
	target_compile_definitions(cute PUBLIC CF_STATIC)
else()
	add_library(cute SHARED ${CF_SRCS} ${CF_HDRS})
endif()
target_compile_definitions(cute PRIVATE CF_EXPORT)

# Optional WebGPU backend. OFF leaves the build exactly as it was.
if(CF_WEBGPU)
	target_sources(cute PRIVATE
		src/cute_graphics_webgpu.cpp
		libraries/imgui/backends/imgui_impl_wgpu.cpp
	)
	target_compile_definitions(cute PUBLIC CF_WEBGPU)
	if(EMSCRIPTEN)
		target_compile_options(cute PUBLIC --use-port=emdawnwebgpu)
		target_link_options(cute INTERFACE --use-port=emdawnwebgpu)
		# The C++20 module scan (emscan-deps) can't see the headers the port adds, and fails on
		# webgpu/webgpu.h for a C++20 consumer project. CF has no modules to scan.
		set_target_properties(cute PROPERTIES CXX_SCAN_FOR_MODULES OFF)
	else()
		# wgpu-native v29.0.1.1, the release zip for this platform, fetched unless
		# CF_WGPU_NATIVE_DIR points at an unpacked release (include/webgpu/webgpu.h, lib/).
		if(NOT CF_WGPU_NATIVE_DIR)
			set(CF_WGPU_NATIVE_URL https://github.com/gfx-rs/wgpu-native/releases/download/v29.0.1.1)
			# The target, not the host: Visual Studio generators leave CMAKE_SYSTEM_PROCESSOR on the
			# host for -A Win32 and -A ARM64.
			if(MSVC)
				string(TOLOWER "${CMAKE_C_COMPILER_ARCHITECTURE_ID}" CF_WGPU_ARCH)
			else()
				string(TOLOWER "${CMAKE_SYSTEM_PROCESSOR}" CF_WGPU_ARCH)
			endif()
			if(CMAKE_OSX_ARCHITECTURES)
				string(TOLOWER "${CMAKE_OSX_ARCHITECTURES}" CF_WGPU_ARCH)
			endif()
			if(WIN32 AND MSVC AND CF_WGPU_ARCH MATCHES "^(amd64|x86_64|x64)$")
				set(CF_WGPU_ZIP wgpu-windows-x86_64-msvc-release.zip)
				set(CF_WGPU_SHA256 7e67d7445c42aeb85e30f88930fd8d7d83ee769e3390aeb1ada75ebf3cf78132)
			elseif(WIN32 AND MSVC AND CF_WGPU_ARCH MATCHES "^(arm64|aarch64)$")
				set(CF_WGPU_ZIP wgpu-windows-aarch64-msvc-release.zip)
				set(CF_WGPU_SHA256 4a876421a8c1e5fe72f849b3722214280fe485cb1c56f77f8b0c82414be5b29f)
			elseif(MACOSX AND CF_WGPU_ARCH MATCHES "^(arm64|aarch64)$")
				set(CF_WGPU_ZIP wgpu-macos-aarch64-release.zip)
				set(CF_WGPU_SHA256 a5797a37b1adf720bcd5dcffb291edbbd5b7b14be0a3874c28e6393a655a7a3e)
			elseif(MACOSX AND CF_WGPU_ARCH STREQUAL "x86_64")
				set(CF_WGPU_ZIP wgpu-macos-x86_64-release.zip)
				set(CF_WGPU_SHA256 8e2f7378548ddd0e2cf21e7d864dda46e953f0af724855a33778b85ead206d41)
			elseif(LINUX AND CF_WGPU_ARCH MATCHES "^(x86_64|amd64)$")
				set(CF_WGPU_ZIP wgpu-linux-x86_64-release.zip)
				set(CF_WGPU_SHA256 95a4d90c071005a98d03eab348beaa6b07e16eb00d1dcdb9f8348f75eb97ec5a)
			elseif(LINUX AND CF_WGPU_ARCH MATCHES "^(aarch64|arm64)$")
				set(CF_WGPU_ZIP wgpu-linux-aarch64-release.zip)
				set(CF_WGPU_SHA256 015fcdf1dbae82e614a783cc38017e5399ae0927a889fe9b69c9b664bc61b47a)
			else()
				message(FATAL_ERROR "CF_WEBGPU: no wgpu-native release is fetched for this platform (${CMAKE_SYSTEM_NAME}, ${CF_WGPU_ARCH}). Set CF_WGPU_NATIVE_DIR to an unpacked wgpu-native release.")
			endif()
			FetchContent_Declare(
				wgpu_native
				URL      ${CF_WGPU_NATIVE_URL}/${CF_WGPU_ZIP}
				URL_HASH SHA256=${CF_WGPU_SHA256}
				DOWNLOAD_EXTRACT_TIMESTAMP ON
				UPDATE_DISCONNECTED TRUE
				SOURCE_SUBDIR  cmake_is_unused
			)
			FetchContent_MakeAvailable(wgpu_native)
			set(CF_WGPU_NATIVE_ROOT ${wgpu_native_SOURCE_DIR})
		else()
			set(CF_WGPU_NATIVE_ROOT ${CF_WGPU_NATIVE_DIR})
		endif()

		if(WIN32)
			set(CF_WGPU_NATIVE_IMPLIB ${CF_WGPU_NATIVE_ROOT}/lib/wgpu_native.dll.lib)
			set(CF_WGPU_NATIVE_RUNTIME ${CF_WGPU_NATIVE_ROOT}/lib/wgpu_native.dll)
		elseif(APPLE)
			set(CF_WGPU_NATIVE_RUNTIME ${CF_WGPU_NATIVE_ROOT}/lib/libwgpu_native.dylib)
		else()
			set(CF_WGPU_NATIVE_RUNTIME ${CF_WGPU_NATIVE_ROOT}/lib/libwgpu_native.so)
		endif()
		if(NOT EXISTS ${CF_WGPU_NATIVE_ROOT}/include/webgpu/webgpu.h OR NOT EXISTS ${CF_WGPU_NATIVE_ROOT}/include/webgpu/wgpu.h)
			message(FATAL_ERROR "CF_WEBGPU: ${CF_WGPU_NATIVE_ROOT} is not a wgpu-native release: include/webgpu/webgpu.h and wgpu.h are missing.")
		endif()
		foreach(CF_WGPU_FILE ${CF_WGPU_NATIVE_RUNTIME} ${CF_WGPU_NATIVE_IMPLIB})
			if(NOT EXISTS ${CF_WGPU_FILE})
				message(FATAL_ERROR "CF_WEBGPU: ${CF_WGPU_FILE} is missing from the wgpu-native release at ${CF_WGPU_NATIVE_ROOT}.")
			endif()
		endforeach()

		# An imported target, so `$<TARGET_RUNTIME_DLLS:your_game>` lists the shared library for
		# programs outside CF's build. Copy it beside your executable with:
		#   add_custom_command(TARGET your_game POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different
		#     $<TARGET_RUNTIME_DLLS:your_game> $<TARGET_FILE_DIR:your_game> COMMAND_EXPAND_LISTS)
		# A project that already defines wgpu_native keeps its own target.
		if(NOT TARGET wgpu_native)
			add_library(wgpu_native SHARED IMPORTED GLOBAL)
			set_target_properties(wgpu_native PROPERTIES IMPORTED_LOCATION ${CF_WGPU_NATIVE_RUNTIME})
			if(WIN32)
				set_target_properties(wgpu_native PROPERTIES IMPORTED_IMPLIB ${CF_WGPU_NATIVE_IMPLIB})
			elseif(NOT APPLE)
				# The release .so has no SONAME; linked by path, executables would record that path.
				set_target_properties(wgpu_native PROPERTIES IMPORTED_NO_SONAME TRUE)
			endif()
			install(IMPORTED_RUNTIME_ARTIFACTS wgpu_native RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
			if(WIN32)
				install(FILES ${CF_WGPU_NATIVE_IMPLIB} DESTINATION ${CMAKE_INSTALL_LIBDIR})
			endif()
		endif()
		target_link_libraries(cute PUBLIC wgpu_native)
		# No public CF header includes webgpu.h.
		target_include_directories(cute PRIVATE $<BUILD_INTERFACE:${CF_WGPU_NATIVE_ROOT}/include>)
		target_compile_definitions(cute PRIVATE IMGUI_IMPL_WEBGPU_BACKEND_DAWN) # wgpu-native v25+ speaks the standard header, which is the path imgui calls Dawn.

		# CF's own executables all land in CMAKE_RUNTIME_OUTPUT_DIRECTORY, so one copy serves them.
		if(CMAKE_RUNTIME_OUTPUT_DIRECTORY)
			get_property(CF_MULTI_CONFIG GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
			if(CF_MULTI_CONFIG)
				set(CF_WGPU_RUNTIME_DIR "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<CONFIG>")
			else()
				set(CF_WGPU_RUNTIME_DIR "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
			endif()
			add_custom_command(TARGET cute POST_BUILD
				COMMAND ${CMAKE_COMMAND} -E make_directory "${CF_WGPU_RUNTIME_DIR}"
				COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:wgpu_native>" "${CF_WGPU_RUNTIME_DIR}"
			)
		endif()
	endif()
endif()

# Box2D v3.1.1 (MIT), fetched shallow at configure time -- the release tag is the pin
# (GIT_SHALLOW only composes with branch/tag names, not commit hashes, and a tag keeps the
# download at a single commit's worth of history). SOURCE_SUBDIR points at a directory with
# no CMakeLists on purpose: Box2D's own build is skipped entirely, because CF compiles the
# sources itself as an object library folded into cute -- the C sources need C17 (the rest
# of CF compiles as C++), and shared builds define box2d_EXPORTS so b2* symbols export from
# the DLL for direct user calls.
FetchContent_Declare(
	box2d
	GIT_REPOSITORY https://github.com/erincatto/box2d.git
	GIT_TAG        v3.1.1
	GIT_SHALLOW    TRUE
	GIT_PROGRESS   TRUE
	UPDATE_DISCONNECTED TRUE
	SOURCE_SUBDIR  cmake_is_unused
)
FetchContent_MakeAvailable(box2d)
file(GLOB CF_BOX2D_SRCS ${box2d_SOURCE_DIR}/src/*.c)
add_library(cute-box2d OBJECT ${CF_BOX2D_SRCS})
target_include_directories(cute-box2d PRIVATE
	${box2d_SOURCE_DIR}/include
	${box2d_SOURCE_DIR}/src
)
set_target_properties(cute-box2d PROPERTIES
	C_STANDARD 17
	C_STANDARD_REQUIRED YES
	C_EXTENSIONS YES
	POSITION_INDEPENDENT_CODE ON
)
if(NOT CF_FRAMEWORK_STATIC)
	target_compile_definitions(cute-box2d PRIVATE box2d_EXPORTS)
endif()
if(EMSCRIPTEN)
	# Matches upstream: wasm SIMD via the SSE2 emulation path.
	target_compile_options(cute-box2d PRIVATE -msimd128 -msse2)
endif()
target_sources(cute PRIVATE $<TARGET_OBJECTS:cute-box2d>)

# Box3D v0.1.0 (MIT), fetched shallow at configure time -- identical treatment to Box2D
# above: the release tag is the pin, Box3D's own CMake is skipped via the unused
# SOURCE_SUBDIR, and CF compiles the sources itself as a C17 object library folded into
# cute with b3* symbols exported on shared builds.
FetchContent_Declare(
	box3d
	GIT_REPOSITORY https://github.com/erincatto/box3d.git
	GIT_TAG        v0.1.0
	GIT_SHALLOW    TRUE
	GIT_PROGRESS   TRUE
	UPDATE_DISCONNECTED TRUE
	SOURCE_SUBDIR  cmake_is_unused
)
FetchContent_MakeAvailable(box3d)
file(GLOB CF_BOX3D_SRCS ${box3d_SOURCE_DIR}/src/*.c)
add_library(cute-box3d OBJECT ${CF_BOX3D_SRCS})
target_include_directories(cute-box3d PRIVATE
	${box3d_SOURCE_DIR}/include
	${box3d_SOURCE_DIR}/src
)
set_target_properties(cute-box3d PROPERTIES
	C_STANDARD 17
	C_STANDARD_REQUIRED YES
	C_EXTENSIONS YES
	POSITION_INDEPENDENT_CODE ON
)
if(NOT CF_FRAMEWORK_STATIC)
	target_compile_definitions(cute-box3d PRIVATE box3d_EXPORTS)
endif()
if(EMSCRIPTEN)
	target_compile_options(cute-box3d PRIVATE -msimd128 -msse2)
endif()
target_sources(cute PRIVATE $<TARGET_OBJECTS:cute-box3d>)

if(EMSCRIPTEN)
	message(STATUS "Configuring CF for Web (Emscripten)")

	# Emscripten forces off a couple options that don't make sense for web builds.
	set(CF_FRAMEWORK_APPLE_FRAMEWORK OFF CACHE BOOL "" FORCE)
	set(PHYSFS_BUILD_SHARED OFF CACHE BOOL "" FORCE)
	set(PHYSFS_BUILD_STATIC ON  CACHE BOOL "" FORCE)

	target_compile_options(cute PUBLIC -sUSE_SDL=3)
	target_compile_options(cute PRIVATE -gsplit-dwarf)
	# CF requires WebGL 2, but imgui_impl_opengl3 auto-selects its ES2 path on Emscripten, which
	# compiles out glBindSampler: CF's sampler objects then stay bound to unit 0 during ImGui draws.
	target_compile_definitions(cute PRIVATE IMGUI_IMPL_OPENGL_ES3)

	target_link_options(cute INTERFACE
		-sUSE_SDL=3
		-sUSE_WEBGL2=1 -sMIN_WEBGL_VERSION=2 -sMAX_WEBGL_VERSION=2
		-sFULL_ES3=1
		-sALLOW_MEMORY_GROWTH=1
		-sFILESYSTEM=1
		-sASYNCIFY=1
		-sASYNCIFY_STACK_SIZE=2040000
		# Skip ASYNCIFY instrumentation for functions proven never to suspend
		# (never reach cf_sleep / a coroutine yield / a fiber swap). This severs
		# the allocator hub (cf_alloc & co, an indirect call) and the cspv shader
		# compiler from the instrumented set, shrinking the wasm-opt link pass and
		# the output module. See cmake/asyncify_remove.txt for the list + contract.
		-sASYNCIFY_REMOVE=@${CMAKE_CURRENT_SOURCE_DIR}/cmake/asyncify_remove.txt
	)

	target_compile_options(cute PRIVATE -O1 -fno-rtti -fno-exceptions)
endif()

if(ANDROID)
	message(STATUS "Configuring CF for Android")

	# Android-specific options.
	set(CF_CUTE_SHADERC OFF CACHE BOOL "" FORCE)
	set(CF_FRAMEWORK_APPLE_FRAMEWORK OFF CACHE BOOL "" FORCE)
	set(PHYSFS_BUILD_SHARED OFF CACHE BOOL "" FORCE)
	set(PHYSFS_BUILD_STATIC ON CACHE BOOL "" FORCE)

	# Link Android-specific libraries.
	# GLESv3 needed for imgui OpenGL3 backend (VAO functions).
	set(CF_LINK_LIBS ${CF_LINK_LIBS} android log GLESv3 EGL)
endif()

# PhysicsFS for virtual file system and archive support.
if(CF_FRAMEWORK_STATIC)
	set(PHYSFS_BUILD_SHARED OFF CACHE BOOL "Build PhysFS as shared library")
	set(PHYSFS_BUILD_STATIC ON CACHE BOOL "Build PhysFS as static library")
else()
	set(PHYSFS_BUILD_SHARED ON CACHE BOOL "Build PhysFS as shared library")
	set(PHYSFS_BUILD_STATIC OFF CACHE BOOL "Build PhysFS as static library")
endif()
set(PHYSFS_BUILD_TEST OFF CACHE BOOL "Do not build PhysFS tests.")
set(PHYSFS_BUILD_DOCS OFF CACHE BOOL "Do not build PhysFS docs.")
FetchContent_Declare(
	physfs
	GIT_REPOSITORY https://github.com/icculus/physfs.git
	GIT_TAG        9d18d36b5a5207b72f473f05e1b2834e347d8144
	UPDATE_DISCONNECTED TRUE
	EXCLUDE_FROM_ALL
)
FetchContent_MakeAvailable(physfs)

if(APPLE)
	find_library(IOKIT IOKit)
	find_library(FOUNDATION Foundation)
	find_library(SECURITY Security)
	find_library(QUARTZCORE QuartzCore)
	find_library(METAL Metal)
	find_library(METALKIT MetalKit)
	find_library(NETWORK Network)
	# For the VideoToolbox hardware video-encode backend in cute_video.cpp.
	find_library(VIDEOTOOLBOX VideoToolbox)
	find_library(COREMEDIA CoreMedia)
	find_library(COREVIDEO CoreVideo)
	find_library(COREFOUNDATION CoreFoundation)
	set(CF_LINK_LIBS ${CF_LINK_LIBS} ${IOKIT} ${FOUNDATION} ${SECURITY} ${QUARTZCORE} ${METAL} ${METALKIT} ${NETWORK} ${VIDEOTOOLBOX} ${COREMEDIA} ${COREVIDEO} ${COREFOUNDATION})
endif()

if(LINUX)
	find_package(OpenGL REQUIRED)
	set(CF_LINK_LIBS ${CF_LINK_LIBS} ${OPENGL_LIBRARIES})
endif()

if(CF_FRAMEWORK_STATIC)
	set(CF_LINK_LIBS ${CF_LINK_LIBS} PhysFS::PhysFS-static)
else ()
	set(CF_LINK_LIBS ${CF_LINK_LIBS} PhysFS::PhysFS)
endif()

# Add s2n for Linux builds.
if(LINUX)
	# Ensure tests are not built.
	set(BUILD_TESTING OFF CACHE BOOL "Disable building s2n tests." FORCE)

	FetchContent_Declare(
		s2n
		URL https://github.com/aws/s2n-tls/archive/refs/tags/v1.6.3.zip
		DOWNLOAD_EXTRACT_TIMESTAMP ON
		UPDATE_DISCONNECTED TRUE
		EXCLUDE_FROM_ALL
	)
	FetchContent_MakeAvailable(s2n)
	set(CF_LINK_LIBS ${CF_LINK_LIBS} s2n)
endif()

# MinGW needs these since gcc ignores pragma links in C/C++.
if(WINDOWS AND NOT MSVC)
	set(CF_LINK_LIBS ${CF_LINK_LIBS} ws2_32 secur32 shlwapi)
endif()

# Deprecated no-op, kept so existing user CMake scripts don't break. CF no longer
# ships or uses dxcompiler.dll/dxil.dll -- D3D12 shaders go through the system's
# FXC (d3dcompiler_47.dll) fed by cute_spirv's HLSL transpiler backend.
function(target_dxc_copy TARGET)
endfunction()

if(EMSCRIPTEN AND CF_USE_SYSTEM_SDL)
	message(STATUS "CF_USE_SYSTEM_SDL has no effect on Emscripten: SDL3 always comes from Emscripten's port (-sUSE_SDL=3).")
endif()

if(NOT EMSCRIPTEN AND CF_USE_SYSTEM_SDL)
	set(SDL_REQUIRED_VERSION 3.4.0)
	# SDL3 keeps API and ABI stable across 3.x, so any later 3.x release works. SDL's own version
	# file accepts any newer version, so the range is what keeps SDL 4 out. GLOBAL makes the SDL3::
	# targets visible project-wide, as they are with the FetchContent build.
	find_package(SDL3 ${SDL_REQUIRED_VERSION}...<4.0.0 CONFIG GLOBAL)
	if(NOT SDL3_FOUND OR NOT TARGET SDL3::SDL3)
		set(CF_SDL_CONSIDERED "")
		if(SDL3_CONSIDERED_CONFIGS)
			set(CF_SDL_CONSIDERED " Rejected: ${SDL3_CONSIDERED_CONFIGS} (versions ${SDL3_CONSIDERED_VERSIONS}).")
		endif()
		message(FATAL_ERROR
			"CF_USE_SYSTEM_SDL is ON but no usable SDL3 package was found. CF needs SDL3 ${SDL_REQUIRED_VERSION} "
			"or a newer SDL3 release (not SDL 4).${CF_SDL_CONSIDERED} Point CMAKE_PREFIX_PATH or SDL3_DIR at an "
			"SDL3 install, or set CF_USE_SYSTEM_SDL=OFF to download and build SDL3 instead.")
	endif()
	# Match the FetchContent build: static CF links static SDL, shared CF links shared SDL. Static CF
	# falls back to a shared SDL when that's all the package has (common for distro packages).
	if(CF_FRAMEWORK_STATIC AND TARGET SDL3::SDL3-static)
		set(CF_SDL_TARGET SDL3::SDL3-static)
	else()
		set(CF_SDL_TARGET SDL3::SDL3)
	endif()
	message(STATUS "Using installed SDL3 ${SDL3_VERSION} (${CF_SDL_TARGET}) from ${SDL3_DIR}")
	set(CF_LINK_LIBS ${CF_LINK_LIBS} ${CF_SDL_TARGET})

	# On Windows a shared SDL's DLL lives in the install prefix, so cf_copy_sdl_dll puts it next to CF's
	# own executables. Packages differ in how SDL3::SDL3 reaches the library (a real target, an ALIAS,
	# or an INTERFACE wrapper), so follow it to whatever library is actually linked.
	if(WIN32)
		set(CF_SDL_LINKED ${CF_SDL_TARGET})
		get_target_property(CF_SDL_ALIASED ${CF_SDL_LINKED} ALIASED_TARGET)
		if(CF_SDL_ALIASED)
			set(CF_SDL_LINKED ${CF_SDL_ALIASED})
		endif()
		get_target_property(CF_SDL_TYPE ${CF_SDL_LINKED} TYPE)
		if(CF_SDL_TYPE STREQUAL "INTERFACE_LIBRARY")
			get_target_property(CF_SDL_WRAPPED ${CF_SDL_LINKED} INTERFACE_LINK_LIBRARIES)
			foreach(CF_SDL_ITEM IN LISTS CF_SDL_WRAPPED)
				if(TARGET ${CF_SDL_ITEM})
					get_target_property(CF_SDL_ITEM_TYPE ${CF_SDL_ITEM} TYPE)
					if(CF_SDL_ITEM_TYPE STREQUAL "SHARED_LIBRARY")
						set(CF_SDL_LINKED ${CF_SDL_ITEM})
						set(CF_SDL_TYPE SHARED_LIBRARY)
					endif()
				endif()
			endforeach()
		endif()
		if(CF_SDL_TYPE STREQUAL "SHARED_LIBRARY")
			set(CF_SDL_DLL_TARGET ${CF_SDL_LINKED})
		endif()
	endif()

	# An installed SDL ships no private sources for gen_d3d12_layout, so use the offsets recorded from
	# SDL 3.4.0's sources. cute_graphics_sdlgpu.cpp only applies them when the runtime SDL is 3.4.0.
	if(WIN32)
		if(NOT SDL3_VERSION VERSION_EQUAL 3.4.0)
			message(STATUS "D3D12 debug-layer warning filter disabled: its layout offsets were recorded from SDL 3.4.0, found ${SDL3_VERSION}.")
		endif()
		file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/generated")
		configure_file(tools/d3d12_layout_sdl_3_4_0.h "${CMAKE_CURRENT_BINARY_DIR}/generated/d3d12_layout.gen.h" COPYONLY)
		target_include_directories(cute PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/generated")
	endif()
elseif(NOT EMSCRIPTEN) # Emscripten provides it's own SDL3.
	# SDL for platform support.
	set(SDL_REQUIRED_VERSION 3.4.0)
	# Just don't build the shared library at all, it's not needed.
	set(SDL_SHARED_ENABLED_BY_DEFAULT OFF)
	if(CF_FRAMEWORK_STATIC)
		set(SDL_STATIC ON CACHE BOOL "Build SDL as a static library")
		set(SDL_SHARED OFF CACHE BOOL "Do not build SDL as a shared library")
	else()
		set(SDL_STATIC OFF CACHE BOOL "Do not build SDL as a static library")
		set(SDL_SHARED ON CACHE BOOL "Build SDL as a shared library")
	endif()
	set(SDL_TEST_LIBRARY OFF CACHE BOOL "Do not build SDL tests")
	set(SDL_EXAMPLES OFF CACHE BOOL "Do not build SDL examples")
	FetchContent_Declare(
		SDL3
		URL https://github.com/libsdl-org/SDL/releases/download/release-${SDL_REQUIRED_VERSION}/SDL3-${SDL_REQUIRED_VERSION}.zip
		URL_HASH SHA256=9ac2debb493e0d3e13dbd2729fb91f4bfeb00a0f4dff5e04b73cc9bac276b38d
		DOWNLOAD_EXTRACT_TIMESTAMP ON
		UPDATE_DISCONNECTED TRUE
		EXCLUDE_FROM_ALL
	)
	FetchContent_MakeAvailable(SDL3)
	if(CF_FRAMEWORK_STATIC)
		set(CF_LINK_LIBS ${CF_LINK_LIBS} SDL3::SDL3-static)
	else ()
		set(CF_LINK_LIBS ${CF_LINK_LIBS} SDL3::SDL3)
	endif()
	target_include_directories(cute PUBLIC ${SDL3_INCLUDE_DIRS})

	# Build-time tool to extract D3D12 struct layout offsets from SDL3 private headers.
	if(WIN32)
		set(D3D12_LAYOUT_HEADER "${CMAKE_CURRENT_BINARY_DIR}/generated/d3d12_layout.gen.h")
		file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/generated")
		add_executable(gen_d3d12_layout EXCLUDE_FROM_ALL tools/gen_d3d12_layout.c)
		add_custom_command(
			OUTPUT ${D3D12_LAYOUT_HEADER}
			COMMAND gen_d3d12_layout
				"${sdl3_SOURCE_DIR}/src/gpu/SDL_sysgpu.h"
				"${sdl3_SOURCE_DIR}/src/gpu/d3d12/SDL_gpu_d3d12.c"
				"${D3D12_LAYOUT_HEADER}"
			DEPENDS gen_d3d12_layout
				"${sdl3_SOURCE_DIR}/src/gpu/SDL_sysgpu.h"
				"${sdl3_SOURCE_DIR}/src/gpu/d3d12/SDL_gpu_d3d12.c"
			COMMENT "Generating D3D12 struct layout offsets"
		)
		add_custom_target(gen_d3d12_layout_target DEPENDS ${D3D12_LAYOUT_HEADER})
		add_dependencies(cute gen_d3d12_layout_target)
		target_include_directories(cute PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/generated")
	endif()
endif()

# Copies an installed SDL3.dll next to TARGET. A no-op unless CF_USE_SYSTEM_SDL resolved to a shared SDL on Windows.
function(cf_copy_sdl_dll TARGET)
	if(CF_SDL_DLL_TARGET)
		add_custom_command(TARGET ${TARGET} POST_BUILD
			COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:${CF_SDL_DLL_TARGET}>" "$<TARGET_FILE_DIR:${TARGET}>"
		)
	endif()
endfunction()

add_subdirectory(tools)
if (CF_CUTE_SYM)
	cf_symbols_cute()
endif()

# Some platform specific settings.
if(MINGW)
	set(CF_LINK_LIBS ${CF_LINK_LIBS} d3d11 crypt32 mfplat mfreadwrite mfuuid ole32)
elseif(WINDOWS)
	set(CF_LINK_LIBS ${CF_LINK_LIBS} crypt32 mfplat mfreadwrite mfuuid ole32)
elseif(LINUX)
	target_compile_options(cute PRIVATE -msse4.1)
elseif(APPLE)
	if(CF_FRAMEWORK_APPLE_FRAMEWORK)
		set_target_properties(cute PROPERTIES FRAMEWORK TRUE)
		if (CF_FRAMEWORK_STATIC)
		set_target_properties(cute PROPERTIES
			FRAMEWORK_VERSION "${PROJECT_VERSION}"
			MACOSX_FRAMEWORK_IDENTIFIER "com.cuteframework.static"
		)
		else()
		set_target_properties(cute PROPERTIES
			FRAMEWORK_VERSION "${PROJECT_VERSION}"
			MACOSX_FRAMEWORK_IDENTIFIER "com.cuteframework.shared"
		)
		endif()
	endif()
endif()


# Link up all dependencies to Cute.
target_link_libraries(cute PUBLIC ${CF_LINK_LIBS})

if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
	option(CF_FRAMEWORK_BUILD_TESTS "Build the cute framework unit tests." ON)
	option(CF_FRAMEWORK_BUILD_SAMPLES "Build the cute framework sample programs." ON)
	option(CF_BUILD_DOCSPARSER "Build the docsparser documentation generator." ON)

	# Cute unit tests executable (optional, defaulted to also build).
	if (CF_FRAMEWORK_BUILD_TESTS)
		add_subdirectory(test)
	endif()

	# Cute sample programs (optional, defaulted to also build).
	if (CF_FRAMEWORK_BUILD_SAMPLES)
		add_subdirectory(samples)
	endif()
endif()

# Propogate public headers to other cmake scripts including this subdirectory.
target_include_directories(cute PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
target_include_directories(cute PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/libraries>)
target_include_directories(cute PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/libraries/imgui>)
target_include_directories(cute PUBLIC $<BUILD_INTERFACE:${box2d_SOURCE_DIR}/include>)
target_include_directories(cute PUBLIC $<BUILD_INTERFACE:${box3d_SOURCE_DIR}/include>)
target_include_directories(cute PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/tools)

# Install target
install(TARGETS cute
	BUNDLE DESTINATION .
	RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
	ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
	FRAMEWORK DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
