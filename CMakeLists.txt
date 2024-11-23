cmake_minimum_required(VERSION 3.14)
project(cute_framework)

# Must have at least C++20.
set(CMAKE_CXX_STANDARD 20)

# These are needed for how we use FetchContent.
include(FetchContent)
Set(FETCHCONTENT_QUIET FALSE)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Todo - Fix how turning some of these off breaks the build.
option(CF_FRAMEWORK_STATIC "Build static library for Cute Framework." ON)
option(CF_RUNTIME_SHADER_COMPILATION "Build CF with online shader compilation support (requires python 3.x installation)." ON)
option(CF_CUTE_SHADERC "Build cute-shaderc, an offline shader compiler (requires python 3.x installation)." ON)
option(CF_FRAMEWORK_APPLE_FRAMEWORK "Build CF libraries as Apple Framework" OFF)

# Platform detection.
if(CMAKE_SYSTEM_NAME MATCHES "Emscripten")
	set(EMSCRIPTEN TRUE)
	# Also disable samples/tests. These should be supported/added back in at some point.
	set(CF_FRAMEWORK_BUILD_SAMPLES OFF)
	set(CF_FRAMEWORK_BUILD_TESTS OFF)
elseif(WIN32)
	set(WINDOWS TRUE)
elseif(UNIX AND NOT APPLE)
	if(CMAKE_SYSTEM_NAME MATCHES ".*Linux")
		set(LINUX TRUE)
	else()
		message(FATAL_ERROR, "No supported platform detected.")
	endif()
elseif(APPLE)
	enable_language(OBJC)
	if(CMAKE_SYSTEM_NAME MATCHES ".*MacOS.*" OR CMAKE_SYSTEM_NAME MATCHES ".*Darwin.*")
		set(MACOSX TRUE)
	elseif(CMAKE_SYSTEM_NAME MATCHES ".*iOS.*")
		set(IOS TRUE)
	else()
		message(FATAL_ERROR, "No supported platform detected.")
	endif()
else()
	message(FATAL_ERROR, "No supported platform detected.")
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

# Common directories for compiler/linker path.
include_directories(src libraries test)

# Cute Framework shared library.
set(CF_SRCS
	src/cute_app.cpp
	src/cute_array.cpp
	src/cute_audio.cpp
	src/cute_clipboard.cpp
	src/cute_multithreading.cpp
	src/cute_file_system.cpp
	src/cute_input.cpp
	src/cute_time.cpp
	src/cute_version.cpp
	src/cute_json.cpp
	src/cute_base64.cpp
	src/cute_hashtable.cpp
	src/cute_string.cpp
	src/cute_math.cpp
	src/cute_draw.cpp
	src/cute_image.cpp
	src/cute_graphics.cpp
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

	libraries/cimgui/imgui/imgui.cpp
	libraries/cimgui/imgui/imgui_demo.cpp
	libraries/cimgui/imgui/imgui_draw.cpp
	libraries/cimgui/imgui/imgui_tables.cpp
	libraries/cimgui/imgui/imgui_widgets.cpp

	libraries/cimgui/cimgui.cpp
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
	include/cute_hashtable.h
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
	libraries/cimgui/imgui/imgui.h
	libraries/cimgui/imgui/imconfig.h
	libraries/cimgui/imgui/imgui_internal.h
	libraries/cimgui/imgui/imstb_rectpack.h
	libraries/cimgui/imgui/imstb_textedit.h
	libraries/cimgui/imgui/imstb_truetype.h
)

set(CIMGUI_HDRS
	libraries/cimgui/cimgui.h
)

set(CF_HDRS
	${CF_PUBLIC_HDRS}
	${IMGUI_HDRS}
	${CIMGUI_HDRS}

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

# PhysicsFS, always statically linked.
set(PHYSFS_SRCS
	libraries/physfs/physfs_archiver_7z.c
	libraries/physfs/physfs_archiver_dir.c
	libraries/physfs/physfs_archiver_grp.c
	libraries/physfs/physfs_archiver_hog.c
	libraries/physfs/physfs_archiver_iso9660.c
	libraries/physfs/physfs_archiver_mvl.c
	libraries/physfs/physfs_archiver_qpak.c
	libraries/physfs/physfs_archiver_slb.c
	libraries/physfs/physfs_archiver_unpacked.c
	libraries/physfs/physfs_archiver_vdf.c
	libraries/physfs/physfs_archiver_wad.c
	libraries/physfs/physfs_archiver_zip.c
	libraries/physfs/physfs_byteorder.c
	libraries/physfs/physfs_casefolding.h
	libraries/physfs/physfs_internal.h
	libraries/physfs/physfs_lzmasdk.h
	libraries/physfs/physfs_miniz.h
	libraries/physfs/physfs_platform_haiku.cpp
	libraries/physfs/physfs_platform_os2.c
	libraries/physfs/physfs_platform_posix.c
	libraries/physfs/physfs_platform_qnx.c
	libraries/physfs/physfs_platform_unix.c
	libraries/physfs/physfs_platform_windows.c
	libraries/physfs/physfs_platform_winrt.cpp
	libraries/physfs/physfs_platforms.h
	libraries/physfs/physfs_unicode.c
	libraries/physfs/physfs.c
	libraries/physfs/physfs.h
)

if(APPLE)
	set(PHYSFS_SRCS ${PHYSFS_SRCS}
		libraries/physfs/physfs_platform_apple.m
	)
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

add_library(physfs STATIC ${PHYSFS_SRCS})
set(CF_LINK_LIBS ${CF_LINK_LIBS} physfs)

# Add s2n for Linux builds.
if(LINUX)
	# Ensure tests are not built.
	set(BUILD_TESTING OFF CACHE BOOL "Disable building s2n tests." FORCE)

	FetchContent_Declare(
		s2n
		URL https://github.com/aws/s2n-tls/archive/refs/tags/v1.3.46.zip
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

# SDL for platform support.
# Just don't build the shared library at all, it's not needed.
set(SDL_SHARED_ENABLED_BY_DEFAULT OFF)
if(EMSCRIPTEN)
	# Emscripten provides its own SDL.
else()
	set(SDL_STATIC ${CF_FRAMEWORK_STATIC} CACHE BOOL "Build SDL as a static library")
	set(SDL_SHARED $<NOT:${CF_FRAMEWORK_STATIC}> CACHE BOOL "Do not build SDL as a shared library")
	set(SDL_TEST_LIBRARY OFF CACHE BOOL "Do not build SDL tests")
	set(SDL_EXAMPLES OFF CACHE BOOL "Do not build SDL examples")
	FetchContent_Declare(
		SDL3
		URL https://github.com/libsdl-org/SDL/archive/d9e6fe0b7a22dae7835c62a120f1a22585b0d9c0.zip
		DOWNLOAD_EXTRACT_TIMESTAMP ON
		CMAKE_ARGS -DSDL_SHARED=$<NOT:${CF_FRAMEWORK_STATIC}> -DSDL_STATIC=${CF_FRAMEWORK_STATIC} -DSDL_TEST_LIBRARY=OFF -DSDL_TESTS=OFF -DSDL_EXAMPLES=OFF
	)
	FetchContent_MakeAvailable(SDL3)
	if(CF_FRAMEWORK_STATIC)
		set(CF_LINK_LIBS ${CF_LINK_LIBS} SDL3::SDL3-static)
	else ()
		set(CF_LINK_LIBS ${CF_LINK_LIBS} SDL3::SDL3)
	endif()
endif()
target_include_directories(cute PUBLIC ${SDL3_INCLUDE_DIRS})

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
		URL https://github.com/KhronosGroup/glslang/archive/refs/tags/14.3.0.zip
		DOWNLOAD_EXTRACT_TIMESTAMP ON
		PATCH_COMMAND ${Python3_EXECUTABLE} update_glslang_sources.py
	)
	FetchContent_MakeAvailable(glslang)

	# cute-shader, a glslang wrapper
	set(CUTE_SHADER_SRCS src/cute_shader/cute_shader.cpp)
	add_library(cute-shader STATIC ${CUTE_SHADER_SRCS})
	target_link_libraries(cute-shader PRIVATE glslang glslang-default-resource-limits SPIRV)
	if (NOT MSVC AND NOT CF_FRAMEWORK_STATIC)
		target_compile_options(cute-shader PRIVATE -fPIC)
	endif()
	target_include_directories(cute-shader PUBLIC include)  # For cute_shader_info.h

	# Linking cute-shader for online shader compilation (optional).
	if (CF_RUNTIME_SHADER_COMPILATION)
		target_compile_definitions(cute PUBLIC CF_RUNTIME_SHADER_COMPILATION)
		set(CF_LINK_LIBS ${CF_LINK_LIBS} cute-shader)
	endif()

	# cute-shaderc, an offline shader compiler
	if (CF_CUTE_SHADERC)
		set(CUTE_SHADERC_SRCS src/cute_shader/cute_shaderc.cpp)
		add_executable(cute-shaderc ${CUTE_SHADERC_SRCS})
		target_link_libraries(cute-shaderc cute-shader)
	endif()
endif()

# SPIRV-Cross for SDL_gpu_shadercross
set(SPIRV_CROSS_CLI OFF CACHE BOOL "Turn off building SPIRV-Cross CLI.")
FetchContent_Declare(
	SPIRV_CROSS
	GIT_REPOSITORY https://github.com/KhronosGroup/SPIRV-Cross.git
	GIT_TAG main
	CMAKE_ARGS -DSPIRV_CROSS_EXCEPTIONS_TO_ASSERTIONS=ON -DSPIRV_CROSS_CLI=OFF -DSPIRV_CROSS_SHARED=OFF -DSPIRV_CROSS_STATIC=ON
	GIT_SHALLOW TRUE
	GIT_PROGRESS TRUE
)
set(SPIRV_CROSS_ENABLE_TESTS OFF CACHE BOOL "Disable SPIRV-Cross tests")
set(SPIRV_CROSS_ENABLE_GLSLANG_INSTALL OFF CACHE BOOL "Disable GLSLANG install")
set(SPIRV_CROSS_SHARED OFF CACHE BOOL "Build SPIRV-Cross as shared library")
set(SPIRV_CROSS_STATIC ON CACHE BOOL "Do not build SPIRV-Cross as static library")
set(SPIRV_CROSS_C_API ON CACHE BOOL "Enable the C API")
set(SPIRV_CROSS_EXCEPTIONS_TO_ASSERTIONS ON CACHE BOOL "Turn off exceptions in SPIRV-Cross.")
set(SPIRV_CROSS_ENABLE_REFLECT OFF CACHE BOOL "Turn off reflection in SPIRV-Cross.")
set(SPIRV_CROSS_SKIP_INSTALL OFF CACHE BOOL "Turn off install targets in SPIRV-Cross.")
set(SPIRV_CROSS_ENABLE_UTIL OFF CACHE BOOL "Turn off utils in SPIRV-Cross.")
set(SPIRV_CROSS_ENABLE_CPP OFF CACHE BOOL "Turn off CPP target support in SPIRV-Cross.")
set(SPIRV_CROSS_FORCE_PIC $<NOT:${CF_FRAMEWORK_STATIC}> CACHE BOOL "Force position-independent code for all targets.")
if (WINDOWS)
	set(SPIRV_CROSS_ENABLE_MSL OFF CACHE BOOL "Turn off MSL support when on windows.")
endif()
if (APPLE)
	set(SPIRV_CROSS_ENABLE_HLSL OFF CACHE BOOL "Turn off HLSL support when on windows.")
endif()
FetchContent_MakeAvailable(SPIRV_CROSS)
set(CF_LINK_LIBS ${CF_LINK_LIBS} spirv-cross-c)

# Some platform specific settings.
if(EMSCRIPTEN)
	target_compile_options(cute PUBLIC -O1 -fno-rtti -fno-exceptions)
	set_target_properties(cute PROPERTIES COMPILE_FLAGS "-s USE_SDL=3")
	target_link_libraries(cute PRIVATE "-s USE_WEBGL2=1 -s ASSERTIONS=1 -s MAX_WEBGL_VERSION=2 -s USE_SDL=3 -s ALLOW_MEMORY_GROWTH=1 -O1 -s ASYNCIFY=1")
elseif(MINGW)
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
			FRAMEWORK_VERSION "1.0.0"
			MACOSX_FRAMEWORK_IDENTIFIER "com.cuteframework.static"
		)
		else()
		set_target_properties(cute PROPERTIES
			FRAMEWORK_VERSION "1.0.0"
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
		DEPENDS src/cute_shader/builtin_shaders.h
		DEPENDS cute-shaderc
	)
endif()

# Link up all dependencies to Cute.
target_link_libraries(cute PUBLIC ${CF_LINK_LIBS})

if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
	option(CF_FRAMEWORK_BUILD_TESTS "Build the cute framework unit tests." ON)
	option(CF_FRAMEWORK_BUILD_SAMPLES "Build the cute framework sample programs." ON)
	# Cute unit tests executable (optional, defaulted to also build).
	if (CF_FRAMEWORK_BUILD_TESTS)
		set(CF_TEST_SRCS test/main.cpp
			test/test_array.cpp
			test/test_aseprite.cpp
			test/test_audio.cpp
			test/test_base64.cpp
			test/test_coroutine.cpp
			test/test_doubly_list.cpp
			test/test_hashtable.cpp
			test/test_path.cpp
			test/test_png_cache.cpp
			test/test_sprite.cpp
			test/test_string.cpp
			test/test_json.cpp
			test/test_markups.cpp
			)
		set(CF_TEST_HDRS test/test_harness.h)

		add_executable(tests ${CF_TEST_SRCS} ${CF_TEST_HDRS})
		target_link_libraries(tests PRIVATE cute)

		if(EMSCRIPTEN)
			set(CMAKE_EXECUTABLE_SUFFIX ".html")
			target_compile_options(tests PUBLIC -O1 -fno-rtti -fno-exceptions)
			target_link_options(tests PRIVATE -o tests.html --emrun -O1)
		endif()

		# For convenience make tests the startup project in Visual Studio.
		# Also set working directory to the target output folder.
		if (MSVC)
			set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT tests)
			set_property(TARGET tests PROPERTY VS_DEBUGGER_WORKING_DIRECTORY $<TARGET_FILE_DIR:tests>)
		endif()
	endif()

	# Cute sample prgrams (optional, defaulted to also build).
	if (CF_FRAMEWORK_BUILD_SAMPLES)
		add_executable(easysprite samples/easy_sprite.c)
		add_executable(basicserialization samples/basic_serialization.c)
		add_executable(imgui samples/imgui.c)
		add_executable(textdrawing samples/text_drawing.cpp)
		add_executable(basicsprite samples/basic_sprite.cpp)
		add_executable(basicshapes samples/basic_shapes.cpp)
		add_executable(windowresizing samples/window_resizing.cpp)
		add_executable(basicinput samples/basic_input.c)
		add_executable(windowevents samples/window_events.c)
		add_executable(window samples/window.cpp)
		add_executable(docsparser samples/docs_parser.cpp)
		add_executable(scratch samples/scratch.cpp)
		add_executable(https samples/https.c)
		add_executable(spaceshooter samples/spaceshooter.cpp samples/spaceshooter_data/flash_shd.h)
		add_executable(draw_to_texture samples/draw_to_texture.c)
		add_executable(hello_triangle samples/hello_triangle.c)
		add_executable(waves samples/waves.cpp samples/waves_data/waves_shd.h)
		add_executable(shallow_water samples/shallow_water.cpp)
		add_executable(noise samples/noise.c)
		add_executable(fetch_image samples/fetch_image.cpp)
		add_executable(metaballs samples/metaballs.cpp)
		add_executable(timestep samples/timestep.cpp)
		add_executable(joypad samples/joypad.c)
		add_executable(outline_stencil samples/outline_stencil.cpp)
		add_executable(recolor samples/recolor.cpp)
		add_executable(rainbow_liquid samples/rainbow_liquid.cpp)
		add_executable(import_spritesheet samples/import_spritesheet.cpp)
		add_executable(stencil_pie_chart samples/stencil_pie_chart.c)
		add_executable(platformer samples/platformer.cpp)
		add_executable(polygon samples/polygon.cpp)
		add_executable(scissor samples/scissor.c)
		add_executable(ime samples/ime.c)
		add_executable(pivot samples/pivot.cpp)
		set(SAMPLE_EXECUTABLES
			easysprite
			basicserialization
			imgui
			textdrawing
			basicsprite
			basicshapes
			windowresizing
			basicinput
			windowevents
			window
			docsparser
			scratch
			https
			spaceshooter
			draw_to_texture
			hello_triangle
			waves
			shallow_water
			noise
			fetch_image
			metaballs
			timestep
			joypad
			outline_stencil
			recolor
			rainbow_liquid
			import_spritesheet
			stencil_pie_chart
			platformer
			polygon
			scissor
			ime
			pivot
		)

		foreach(CURRENT_TARGET ${SAMPLE_EXECUTABLES})
			target_link_libraries(${CURRENT_TARGET} PRIVATE cute)
			if (APPLE)
				set_target_properties(${CURRENT_TARGET} PROPERTIES
					MACOSX_BUNDLE_GUI_IDENTIFIER "com.cuteframework.${CURRENT_TARGET}"
					MACOSX_BUNDLE_BUNDLE_VERSION "1.0.0"
					MACOSX_BUNDLE_SHORT_VERSION_STRING "1.0.0"
				)
			endif()
			set_target_properties(${CURRENT_TARGET} PROPERTIES FOLDER "samples")
			if (WINDOWS)
				target_dxc_copy(${CURRENT_TARGET})
			endif()
		endforeach()

		# Pre-compile shaders for certain samples.
		if (CF_CUTE_SHADERC)
			add_custom_command(
				OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/samples/spaceshooter_data/flash_shd.h
				COMMAND cute-shaderc
					-type=draw
					-varname=s_flash_shd_bytecode
					-oheader=${CMAKE_CURRENT_SOURCE_DIR}/samples/spaceshooter_data/flash_shd.h
					${CMAKE_CURRENT_SOURCE_DIR}/samples/spaceshooter_data/flash.shd
				DEPENDS samples/spaceshooter_data/flash.shd
				DEPENDS cute-shaderc
			)
			add_custom_command(
				OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/samples/waves_data/waves_shd.h
				COMMAND cute-shaderc
					-type=draw
					-varname=s_waves_shd_bytecode
					-oheader=${CMAKE_CURRENT_SOURCE_DIR}/samples/waves_data/waves_shd.h
					${CMAKE_CURRENT_SOURCE_DIR}/samples/waves_data/waves.shd
				DEPENDS samples/waves_data/waves.shd
				DEPENDS cute-shaderc
			)
		endif()

		# Copy over some folders for certain samples.
		add_custom_command(TARGET spaceshooter PRE_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/samples/spaceshooter_data $<TARGET_FILE_DIR:spaceshooter>/spaceshooter_data)
		add_custom_command(TARGET waves PRE_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/samples/waves_data $<TARGET_FILE_DIR:waves>/waves_data)
		add_custom_command(TARGET shallow_water PRE_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/samples/shallow_water_data $<TARGET_FILE_DIR:shallow_water>/shallow_water_data)
		add_custom_command(TARGET import_spritesheet PRE_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/samples/import_spritesheet_data $<TARGET_FILE_DIR:import_spritesheet>/import_spritesheet_data)
		add_custom_command(TARGET pivot PRE_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/samples/pivot_data $<TARGET_FILE_DIR:pivot>/pivot_data)
	endif()
endif()

# Propogate public headers to other cmake scripts including this subdirectory.
target_include_directories(cute PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
target_include_directories(cute PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/libraries>)
target_include_directories(cute PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/libraries/cimgui>)
target_include_directories(cute PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/libraries/cimgui/imgui>)
