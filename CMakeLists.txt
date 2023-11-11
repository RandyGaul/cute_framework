cmake_minimum_required(VERSION 3.14)
project(cute_framework)

# Must have at least C++17.
set(CMAKE_CXX_STANDARD 17)

# These are needed for how we use FetchContent.
include(FetchContent)
Set(FETCHCONTENT_QUIET FALSE)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
define_property(
	TARGET
	PROPERTY FOLDER
	INHERITED
	BRIEF_DOCS "Set the folder name."
	FULL_DOCS  "Use to organize targets in an IDE."
)
function(fetch_content_with_folder _name)
	FetchContent_Declare(
		${_name}
		${ARGN}
		GIT_PROGRESS TRUE
	)
	FetchContent_Populate(${_name})
	string(TOLOWER "${_name}" lc_name)
	add_subdirectory(${${lc_name}_SOURCE_DIR} ${${lc_name}_BINARY_DIR} EXCLUDE_FROM_ALL)
	set_property(DIRECTORY "${${lc_name}_SOURCE_DIR}" PROPERTY FOLDER "${_name}")
endfunction()

# Todo - Fix how turning some of these off breaks the build.
option(CF_FRAMEWORK_STATIC "Build static library for Cute Framework." ON)
option(CF_FRAMEWORK_BUILD_TESTS "Build the cute framework unit tests." ON)
option(CF_FRAMEWORK_BUILD_SAMPLES "Build the cute framework sample programs." ON)

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
	src/cute_circular_buffer.cpp
	src/cute_clipboard.cpp
	src/cute_multithreading.cpp
	src/cute_file_system.cpp
	src/cute_handle_table.cpp
	src/cute_input.cpp
	src/cute_time.cpp
	src/cute_version.cpp
	src/cute_json.cpp
	src/cute_base64.cpp
	src/cute_hashtable.cpp
	src/cute_ecs.cpp
	src/cute_string.cpp
	src/cute_typeless_array.cpp
	src/cute_math.cpp
	src/cute_draw.cpp
	src/cute_image.cpp
	src/cute_graphics.cpp
	src/cute_aseprite_cache.cpp
	src/cute_png_cache.cpp
	src/cute_https.cpp
	src/cute_joypad.cpp
	src/cute_a_star.cpp
	src/cute_aabb_tree.cpp
	src/cute_symbol.cpp
	src/cute_haptics.cpp
	src/cute_sprite.cpp
	src/cute_coroutine.cpp
	src/cute_networking.cpp
	src/cute_guid.cpp
	src/cute_alloc.cpp
	src/cute_result.cpp
	src/cute_aabb_grid.cpp

	src/internal/cute_dx11.cpp
	src/internal/yyjson.c

	libraries/cimgui/imgui/backends/imgui_impl_sdl.cpp
	
	libraries/cimgui/imgui/imgui.cpp
	libraries/cimgui/imgui/imgui_demo.cpp
	libraries/cimgui/imgui/imgui_draw.cpp
	libraries/cimgui/imgui/imgui_tables.cpp
	libraries/cimgui/imgui/imgui_widgets.cpp

	libraries/cimgui/cimgui.cpp
)
if(APPLE)
	set(CF_SRCS ${CF_SRCS}
		src/internal/cute_metal.mm
		src/internal/cute_tls.m
	)
else()
	set(CF_SRCS ${CF_SRCS}
		src/internal/cute_metal.cpp
	)
endif()

set(CF_PUBLIC_HDRS
	include/cute_alloc.h
	include/cute_app.h
	include/cute_audio.h
	include/cute_user_config.h
	include/cute_c_runtime.h
	include/cute_circular_buffer.h
	include/cute_clipboard.h
	include/cute_multithreading.h
	include/cute_defines.h
	include/cute_result.h
	include/cute_file_system.h
	include/cute_handle_table.h
	include/cute_input.h
	include/cute_time.h
	include/cute_version.h
	include/cute_doubly_list.h
	include/cute_json.h
	include/cute_base64.h
	include/cute_array.h
	include/cute_hashtable.h
	include/cute_ecs.h
	include/cute_string.h
	include/cute_typeless_array.h
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
	include/cute_a_star.h
	include/cute_aabb_tree.h
	include/cute_symbol.h
	include/cute_haptics.h
	include/cute_coroutine.h
	include/cute_networking.h
	include/cute_guid.h
	include/cute_routine.h
	include/cute_aabb_grid.h
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

set(SOKOL_HDRS
	libraries/sokol/sokol_gfx.h
	libraries/sokol/sokol_gfx_imgui.h
)

set(CF_HDRS
	${CF_PUBLIC_HDRS}
	${IMGUI_HDRS}
	${CIMGUI_HDRS}
	${SOKOL_HDRS}

	src/internal/cute_app_internal.h
	src/internal/cute_file_system_internal.h
	src/internal/cute_input_internal.h
	src/internal/cute_serialize_internal.h
	src/internal/cute_ecs_internal.h
	src/internal/cute_dx11.h
	src/internal/cute_metal.h
	src/internal/cute_png_cache_internal.h
	src/internal/cute_draw_internal.h
	src/internal/cute_font_internal.h
	src/internal/cute_graphics_internal.h
	src/internal/cute_aseprite_cache_internal.h
	src/internal/cute_alloc_internal.h
	src/internal/yyjson.h

	src/internal/imgui/sokol_imgui.h
	libraries/cimgui/imgui/backends/imgui_impl_sdl.h

	src/shaders/sprite_shader.h
	src/shaders/backbuffer_shader.h
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
	fetch_content_with_folder(
		s2n
		GIT_REPOSITORY https://github.com/aws/s2n-tls
		GIT_TAG v1.3.46
		GIT_PROGRESS TRUE
	)
	set(CF_LINK_LIBS ${CF_LINK_LIBS} s2n)
endif()

# MinGW needs these since gcc ignores pragma links in C/C++.
if(WINDOWS AND NOT MSVC)
	set(CF_LINK_LIBS ${CF_LINK_LIBS} ws2_32 secur32 shlwapi)
endif()

# SDL2 for platform support.
# Just don't build the shared library at all, it's not needed.
set(SDL_SHARED_ENABLED_BY_DEFAULT OFF)
if(EMSCRIPTEN)
	# Emscripten provides its own SDL2.
else()
	fetch_content_with_folder(
		SDL2
		GIT_REPOSITORY https://github.com/libsdl-org/SDL
		GIT_TAG release-2.26.1
		GIT_PROGRESS TRUE
	)
	set(CF_LINK_LIBS ${CF_LINK_LIBS} SDL2main SDL2-static)
endif()
target_include_directories(cute PUBLIC ${SDL2_INCLUDE_DIRS})

# Some platform specific settings.
if(EMSCRIPTEN)
	target_compile_options(cute PUBLIC -O1 -fno-rtti -fno-exceptions)
	set_target_properties(cute PROPERTIES COMPILE_FLAGS "-s USE_SDL=2")
	target_link_libraries(cute PRIVATE "-s USE_WEBGL2=1 -s ASSERTIONS=1 -s MAX_WEBGL_VERSION=2 -s USE_SDL=2 -s ALLOW_MEMORY_GROWTH=1 -O1 -s ASYNCIFY=1")
elseif(MINGW)
	set(CF_LINK_LIBS ${CF_LINK_LIBS} d3d11 crypt32)
elseif(WINDOWS)
	set(CF_LINK_LIBS ${CF_LINK_LIBS} crypt32)
elseif(APPLE)
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

# Link up all dependencies to Cute.
target_link_libraries(cute PUBLIC ${CF_LINK_LIBS})

# Cute unit tests executable (optional, defaulted to also build).
if (CF_FRAMEWORK_BUILD_TESTS)
	set(CF_TEST_SRCS test/main.cpp
		test/test_array.cpp
		test/test_aseprite.cpp
		test/test_audio.cpp
		test/test_base64.cpp
		test/test_circular_buffer.cpp
		test/test_coroutine.cpp
		test/test_doubly_list.cpp
		test/test_ecs.cpp
		test/test_handle.cpp
		test/test_hashtable.cpp
		test/test_path.cpp
		test/test_png_cache.cpp
		test/test_sprite.cpp
		test/test_string.cpp
		test/test_json.cpp
		test/test_aabb.cpp
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
	add_executable(basicecs samples/basic_ecs.cpp)
	add_executable(basicserialization samples/basic_serialization.c)
	add_executable(imgui samples/imgui.c)
	add_executable(textdrawing samples/text_drawing.cpp)
	add_executable(basicsprite samples/basic_sprite.cpp)
	add_executable(basicshapes samples/basic_shapes.cpp)
	add_executable(windowresizing samples/window_resizing.cpp)
	add_executable(basicinput samples/basic_input.c)
	add_executable(windowevents samples/window_events.c)
	add_executable(window samples/window.cpp)
	add_executable(tint samples/tint.cpp)
	add_executable(docsparser samples/docs_parser.cpp)
	add_executable(scratch samples/scratch.cpp)
	add_executable(echo samples/echo.cpp)
	add_executable(https samples/https.c)
	add_executable(spaceshooter samples/spaceshooter.cpp)
	add_executable(draw_to_texture samples/draw_to_texture.c)
	set(SAMPLE_EXECUTABLES
		easysprite
		basicecs
		basicserialization
		imgui
		textdrawing
		basicsprite
		basicshapes
		windowresizing
		basicinput
		windowevents
		window
		tint
		docsparser
		scratch
		echo
		https
		spaceshooter
		draw_to_texture
	)

	foreach(CURRENT_TARGET ${SAMPLE_EXECUTABLES})
		target_link_libraries("${CURRENT_TARGET}" PRIVATE cute)
		if (APPLE)
			set_target_properties("${CURRENT_TARGET}" PROPERTIES
				MACOSX_BUNDLE_GUI_IDENTIFIER "com.cuteframework.${CURRENT_TARGET}"
				MACOSX_BUNDLE_BUNDLE_VERSION "1.0.0"
				MACOSX_BUNDLE_SHORT_VERSION_STRING "1.0.0"
			)
		endif()
		set_target_properties("${CURRENT_TARGET}" PROPERTIES FOLDER "samples")
	endforeach()

	# Copy over some folders for certain samples.
	add_custom_command(TARGET spaceshooter PRE_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/samples/spaceshooter_data $<TARGET_FILE_DIR:spaceshooter>)
endif()

# Propogate public headers to other cmake scripts including this subdirectory.
target_include_directories(cute PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
target_include_directories(cute PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/libraries>)
target_include_directories(cute PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/libraries/cimgui>)
target_include_directories(cute PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/libraries/cimgui/imgui>)

# These are not properly tested!!!
# install(TARGETS cute EXPORT cute DESTINATION "lib")
# install(FILES ${CF_PUBLIC_HDRS} DESTINATION "include/cute")
# install(FILES ${IMGUI_HDRS} DESTINATION "include/cute/imgui")
# install(FILES ${CIMGUI_HDRS} DESTINATION "include/cute/cimgui")
# install(FILES ${SOKOL_HDRS} DESTINATION "include/cute/sokol")
