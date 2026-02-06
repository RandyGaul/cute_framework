cmake_minimum_required(VERSION 4.2)
project(cute_framework
	DESCRIPTION "The cutest framework out there for creating 2D games in C++!"
	HOMEPAGE_URL "https://github.com/RandyGaul/cute_framework"
	LANGUAGES C CXX
	VERSION 1.1.0
)

# Must have at least C++20 and C23.
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_STANDARD 23)

include(GNUInstallDirs)
# These are needed for how we use FetchContent.
include(FetchContent)
Set(FETCHCONTENT_QUIET FALSE)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Todo - Fix how turning some of these off breaks the build.
option(CF_FRAMEWORK_STATIC "Build static library for Cute Framework." ON)
option(CF_RUNTIME_SHADER_COMPILATION "Build CF with online shader compilation support (requires python 3.x installation)." ON)
option(CF_CUTE_SHADERC "Build cute-shaderc, an offline shader compiler (requires python 3.x installation)." ON)
option(CF_FRAMEWORK_APPLE_FRAMEWORK "Build CF libraries as Apple Framework" OFF)

# Make sure all libraries are placed into the same output folder.
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

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
endif()

if(CF_FRAMEWORK_WITH_HTTPS MATCHES OFF)
	add_definitions(-DCF_NO_HTTPS)
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
	src/cute_draw.cpp
	src/cute_image.cpp
	src/cute_graphics.cpp
	src/cute_graphics_sdlgpu.cpp
	src/cute_graphics_gles.cpp
	src/cute_graphics_glad.cpp
	src/cute_aseprite_cache.cpp
	src/cute_png_cache.cpp
	src/cute_https.cpp
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

if (NOT EMSCRIPTEN)
	set(CF_SRCS ${CF_SRCS}
		libraries/SDL3_shadercross/SDL_shadercross.c
	)
endif()

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
	include/cute_draw.h
	include/cute_debug_printf.h
	include/cute_image.h
	include/cute_color.h
	include/cute.h
	include/cute_graphics.h
	include/cute_rnd.h
	include/cute_sprite.h
	include/cute_png_cache.h
	include/cute_https.h
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
	src/internal/cute_png_cache_internal.h
	src/internal/cute_draw_internal.h
	src/internal/cute_font_internal.h
	src/internal/cute_graphics_internal.h
	src/internal/cute_aseprite_cache_internal.h
	src/internal/cute_alloc_internal.h
	src/internal/yyjson.h
	src/data/builtin_shaders_bytecode.h
)

if(CF_FRAMEWORK_STATIC)
	add_library(cute STATIC ${CF_SRCS} ${CF_HDRS})
	target_compile_definitions(cute PUBLIC CF_STATIC)
else()
	add_library(cute SHARED ${CF_SRCS} ${CF_HDRS})
endif()
target_compile_definitions(cute PRIVATE CF_EXPORT)

if(EMSCRIPTEN)
	message(STATUS "Configuring CF for Web (Emscripten)")

	# Emscripten forces off a couple options that don't make sense for web builds.
	set(CF_RUNTIME_SHADER_COMPILATION OFF CACHE BOOL "" FORCE)
	set(CF_CUTE_SHADERC OFF CACHE BOOL "" FORCE)
	set(CF_FRAMEWORK_APPLE_FRAMEWORK OFF CACHE BOOL "" FORCE)
	set(PHYSFS_BUILD_SHARED OFF CACHE BOOL "" FORCE)
	set(PHYSFS_BUILD_STATIC ON  CACHE BOOL "" FORCE)

	target_compile_options(cute PUBLIC -sUSE_SDL=3)
	target_compile_options(cute PRIVATE -gsplit-dwarf)

	target_link_options(cute INTERFACE
		-sUSE_SDL=3
		-sUSE_WEBGL2=1 -sMIN_WEBGL_VERSION=2 -sMAX_WEBGL_VERSION=2
		-sFULL_ES3=1
		-sALLOW_MEMORY_GROWTH=1
		-sFILESYSTEM=1
		-sASYNCIFY=1
		-sASYNCIFY_STACK_SIZE=2040000
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
	set(CF_LINK_LIBS ${CF_LINK_LIBS} ${IOKIT} ${FOUNDATION} ${SECURITY} ${QUARTZCORE} ${METAL} ${METALKIT} ${NETWORK})
endif()

if(LINUX)
	find_package(OpenGL REQUIRED)
	set(CF_LINK_LIBS ${CF_LINK_LIBS} ${IOKIT} ${FOUNDATION} ${SECURITY} ${OPENGL_LIBRARIES})
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
		GIT_PROGRESS TRUE
	)
	FetchContent_MakeAvailable(s2n)
	set(CF_LINK_LIBS ${CF_LINK_LIBS} s2n)
endif()

# MinGW needs these since gcc ignores pragma links in C/C++.
if(WINDOWS AND NOT MSVC)
	set(CF_LINK_LIBS ${CF_LINK_LIBS} ws2_32 secur32 shlwapi)
endif()

# This function should be used if you have specified a target-specific runtime output directory,
# or if you do not wish to specify a default runtime output directory.
function(target_dxc_copy TARGET)
	get_target_property(cute_SOURCE_DIR cute SOURCE_DIR)
	set(DXC_DLL_DIR "${cute_SOURCE_DIR}/libraries/dxc")

	add_custom_command(TARGET ${TARGET} POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_if_different "${DXC_DLL_DIR}/dxcompiler.dll" "$<TARGET_FILE_DIR:${TARGET}>/dxcompiler.dll"
		COMMAND ${CMAKE_COMMAND} -E copy_if_different "${DXC_DLL_DIR}/dxil.dll" "$<TARGET_FILE_DIR:${TARGET}>/dxil.dll"
	)
endfunction()

# Check for Windows platform and specify dxcompiler.dll and dxil.dll locations.
if (WINDOWS)
	set(DXC_DLL_DIR "${CMAKE_CURRENT_SOURCE_DIR}/libraries/dxc")

	# Check if this project is being included as a subproject.
	if (NOT CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
		# Set default runtime output directory if not defined.
		if (NOT DEFINED CMAKE_RUNTIME_OUTPUT_DIRECTORY)
			set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIG>")
		endif()

		# Iterate over the configuration types.
		# ...Try and support custom user output directories.
		foreach (CONFIG_TYPE ${CMAKE_CONFIGURATION_TYPES})
			string(TOUPPER ${CONFIG_TYPE} CONFIG_TYPE_UPPER)
			if (DEFINED CMAKE_RUNTIME_OUTPUT_DIRECTORY_${CONFIG_TYPE_UPPER})
				set(DEST_DIR "${CMAKE_RUNTIME_OUTPUT_DIRECTORY_${CONFIG_TYPE_UPPER}}")
			else()
				set(DEST_DIR "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<CONFIG>")
			endif()

			if (DEST_DIR)
				add_custom_command(TARGET cute POST_BUILD
					COMMAND ${CMAKE_COMMAND} -E copy_if_different "${DXC_DLL_DIR}/dxcompiler.dll" "${DEST_DIR}/dxcompiler.dll"
					COMMAND ${CMAKE_COMMAND} -E copy_if_different "${DXC_DLL_DIR}/dxil.dll" "${DEST_DIR}/dxil.dll"
				)
			endif()
		endforeach()
	endif()
endif()

if(NOT EMSCRIPTEN) # Emscripten provides it's own SDL3.
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
		DOWNLOAD_EXTRACT_TIMESTAMP ON
	)
	FetchContent_MakeAvailable(SDL3)
	if(CF_FRAMEWORK_STATIC)
		set(CF_LINK_LIBS ${CF_LINK_LIBS} SDL3::SDL3-static)
	else ()
		set(CF_LINK_LIBS ${CF_LINK_LIBS} SDL3::SDL3)
	endif()
	target_include_directories(cute PUBLIC ${SDL3_INCLUDE_DIRS})
endif()

if (CF_RUNTIME_SHADER_COMPILATION OR CF_CUTE_SHADERC)
	# glslang for shader compilation.
	set(GLSLANG_TESTS_DEFAULT OFF CACHE BOOL "Do not build glslang tests.")
	set(BUILD_SHARED_LIBS OFF CACHE BOOL "Do not build glslang as a shared lib.")
	set(ENABLE_OPT ON CACHE BOOL "Enable optimization.")
	set(SKIP_SPIRV_TOOLS_INSTALL OFF CACHE BOOL "Don't install the SPIR-V Tools as they are built.")
	set(SPIRV_SKIP_EXECUTABLES ON CACHE BOOL "Don't build any SPIR-V Tools executables/tests.")
	set(SPIRV_WERROR OFF CACHE BOOL "Turn off SPIR-V warnings as error.")
	set(ENABLE_GLSLANG_BINARIES OFF CACHE BOOL "Turn off some standalone binary tools for glslang.")
	set(GLSLANG_ENABLE_INSTALL OFF CACHE BOOL "Disable installation option for glslang.")
	find_package(Python3 REQUIRED)
	FetchContent_Declare(
		glslang
		URL https://github.com/KhronosGroup/glslang/archive/refs/tags/16.0.0.zip
		DOWNLOAD_EXTRACT_TIMESTAMP ON
		PATCH_COMMAND ${Python3_EXECUTABLE} update_glslang_sources.py
	)
	FetchContent_MakeAvailable(glslang)
endif()

add_subdirectory(tools)

# SPIRV-Cross for SDL_shadercross
if(NOT EMSCRIPTEN)  # SPIRV-Cross is not needed for web.
	# Set all SPIRV-Cross options BEFORE FetchContent_Declare/MakeAvailable
	set(SPIRV_CROSS_CLI OFF CACHE BOOL "Turn off building SPIRV-Cross CLI.")
	set(SPIRV_CROSS_ENABLE_TESTS OFF CACHE BOOL "Disable SPIRV-Cross tests")
	set(SPIRV_CROSS_ENABLE_GLSLANG_INSTALL OFF CACHE BOOL "Disable GLSLANG install")
	set(SPIRV_CROSS_SHARED OFF CACHE BOOL "Do not build SPIRV-Cross as shared library")
	set(SPIRV_CROSS_STATIC ON CACHE BOOL "Build SPIRV-Cross as static library")
	set(SPIRV_CROSS_C_API ON CACHE BOOL "Enable the C API")
	set(SPIRV_CROSS_EXCEPTIONS_TO_ASSERTIONS ON CACHE BOOL "Turn off exceptions in SPIRV-Cross.")
	set(SPIRV_CROSS_ENABLE_REFLECT OFF CACHE BOOL "Turn off reflection in SPIRV-Cross.")
	set(SPIRV_CROSS_SKIP_INSTALL OFF CACHE BOOL "Turn off install targets in SPIRV-Cross.")
	set(SPIRV_CROSS_ENABLE_UTIL OFF CACHE BOOL "Turn off utils in SPIRV-Cross.")
	set(SPIRV_CROSS_ENABLE_CPP OFF CACHE BOOL "Turn off CPP target support in SPIRV-Cross.")
	if(CF_FRAMEWORK_STATIC)
		set(SPIRV_CROSS_FORCE_PIC OFF CACHE BOOL "Force position-independent code for all targets.")
	else()
		set(SPIRV_CROSS_FORCE_PIC ON CACHE BOOL "Force position-independent code for all targets.")
	endif()
	if(WINDOWS)
		set(SPIRV_CROSS_ENABLE_MSL OFF CACHE BOOL "Turn off MSL support when on Windows.")
	endif()
	if(APPLE)
		set(SPIRV_CROSS_ENABLE_HLSL OFF CACHE BOOL "Turn off HLSL support when on Apple.")
	endif()
	FetchContent_Declare(
		SPIRV_CROSS
		GIT_REPOSITORY https://github.com/KhronosGroup/SPIRV-Cross.git
		GIT_TAG 0a88b2d5c08708d45692b7096a0a84e7bfae366c
		UPDATE_DISCONNECTED TRUE
		GIT_PROGRESS TRUE
	)
	FetchContent_MakeAvailable(SPIRV_CROSS)
	set(CF_LINK_LIBS ${CF_LINK_LIBS} spirv-cross-c)
endif()

# Some platform specific settings.
if(MINGW)
	set(CF_LINK_LIBS ${CF_LINK_LIBS} d3d11 crypt32)
elseif(WINDOWS)
	set(CF_LINK_LIBS ${CF_LINK_LIBS} crypt32)
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

# Precompile builtin shaders
if (CF_CUTE_SHADERC)
	add_custom_command(
		OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/src/data/builtin_shaders_bytecode.h
		COMMAND cute-shaderc
			-type=builtin
			-oheader=${CMAKE_CURRENT_SOURCE_DIR}/src/data/builtin_shaders_bytecode.h
		DEPENDS tools/builtin_shaders.h
		DEPENDS cute-shaderc
	)
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
target_include_directories(cute PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/tools)

# Install target
install(TARGETS cute
	BUNDLE DESTINATION .
	RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
	ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
	FRAMEWORK DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
