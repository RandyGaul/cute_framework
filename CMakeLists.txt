cmake_minimum_required(VERSION 3.14)
project(cute_framework)

# Must have at least C++14.
set(CMAKE_CXX_STANDARD 14)

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
option(CUTE_FRAMEWORK_STATIC "Build static library for Cute Framework." ON)
option(CUTE_FRAMEWORK_WITH_HTTPS "Build Cute Framework with mbedtls for HTTPS support (Apache 2.0 license)." ON)
option(CUTE_FRAMEWORK_BUILD_TESTS "Build the cute framework unit tests." ON)

# Platform detection.
if(CMAKE_SYSTEM_NAME MATCHES "Emscripten")
	set(EMSCRIPTEN TRUE)
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

# Disable SSE for ARM Macbooks.
if(APPLE AND "${CMAKE_OSX_ARCHITECTURES}" MATCHES "arm")
	add_definitions(-DCUTE_SOUND_SCALAR_MODE)
endif()

# Common directories for compiler/linker path.
include_directories(src libraries test)

# Cute Framework shared library.
set(CUTE_SRCS
	src/cute_app.cpp
	src/cute_audio.cpp
	src/cute_circular_buffer.cpp
	src/cute_clipboard.cpp
	src/cute_concurrency.cpp
	src/cute_file_system.cpp
	src/cute_handle_table.cpp
	src/cute_input.cpp
	src/cute_timer.cpp
	src/cute_version.cpp
	src/cute_memory_pool.cpp
	src/cute_kv.cpp
	src/cute_kv_utils.cpp
	src/cute_base64.cpp
	src/cute_hashtable.cpp
	src/cute_ecs.cpp
	src/cute_string.cpp
	src/cute_string_utils.cpp
	src/cute_typeless_array.cpp
	src/cute_math.cpp
	src/cute_batch.cpp
	src/cute_window.cpp
	src/cute_image.cpp
	src/cute_font.cpp
	src/cute_gfx.cpp
	src/cute_aseprite_cache.cpp
	src/cute_png_cache.cpp
	src/cute_https.cpp
	src/cute_joypad.cpp
	src/cute_a_star.cpp
	src/cute_aabb_tree.cpp
	src/cute_strpool.cpp
	src/cute_symbol.cpp
	src/cute_haptics.cpp
	src/cute_utf8.cpp
	src/cute_sprite.cpp
	src/cute_coroutine.cpp
	src/cute_networking.cpp
	src/cute_guid.cpp

	src/internal/cute_ecs_internal.cpp
	src/internal/cute_dx11.cpp

	src/internal/imgui/imgui_impl_sdl.cpp

	libraries/imgui/imgui.cpp
	libraries/imgui/imgui_demo.cpp
	libraries/imgui/imgui_draw.cpp
	libraries/imgui/imgui_tables.cpp
	libraries/imgui/imgui_widgets.cpp
)

set(CUTE_PUBLIC_HDRS
	include/cute_alloc.h
	include/cute_app.h
	include/cute_audio.h
	include/cute_c_runtime.h
	include/cute_circular_buffer.h
	include/cute_clipboard.h
	include/cute_concurrency.h
	include/cute_defines.h
	include/cute_result.h
	include/cute_file_system.h
	include/cute_file_system_utils.h
	include/cute_handle_table.h
	include/cute_input.h
	include/cute_timer.h
	include/cute_version.h
	include/cute_memory_pool.h
	include/cute_doubly_list.h
	include/cute_kv.h
	include/cute_kv_utils.h
	include/cute_base64.h
	include/cute_array.h
	include/cute_hashtable.h
	include/cute_dictionary.h
	include/cute_ecs.h
	include/cute_string.h
	include/cute_string_utils.h
	include/cute_typeless_array.h
	include/cute_defer.h
	include/cute_math.h
	include/cute_batch.h
	include/cute_lru_cache.h
	include/cute_window.h
	include/cute_debug_printf.h
	include/cute_image.h
	include/cute_color.h
	include/cute_font.h
	include/cute.h
	include/cute_gfx.h
	include/cute_rnd.h
	include/cute_sprite.h
	include/cute_aseprite_cache.h
	include/cute_png_cache.h
	include/cute_https.h
	include/cute_joypad.h
	include/cute_priority_queue.h
	include/cute_a_star.h
	include/cute_aabb_tree.h
	include/cute_strpool.h
	include/cute_symbol.h
	include/cute_haptics.h
	include/cute_utf8.h
	include/cute_coroutine.h
	include/cute_networking.h
	include/cute_guid.h
)

set(IMGUI_HDRS
	libraries/imgui/imgui.h
	libraries/imgui/imconfig.h
	libraries/imgui/imgui_internal.h
	libraries/imgui/imstb_rectpack.h
	libraries/imgui/imstb_textedit.h
	libraries/imgui/imstb_truetype.h
)

set(SOKOL_HDRS
	libraries/sokol/sokol_gfx.h
	libraries/sokol/sokol_gfx_imgui.h
)

set(CUTE_HDRS
	${CUTE_PUBLIC_HDRS}
	${IMGUI_HDRS}
	${SOKOL_HDRS}

	src/internal/cute_app_internal.h
	src/internal/cute_audio_internal.h
	src/internal/cute_file_system_internal.h
	src/internal/cute_font_internal.h
	src/internal/cute_input_internal.h
	src/internal/cute_serialize_internal.h
	src/internal/cute_ecs_internal.h
	src/internal/cute_dx11.h
	src/internal/cute_png_cache_internal.h

	src/internal/imgui/sokol_imgui.h
	src/internal/imgui/imgui_impl_sdl.h

	src/shaders/sprite_shader.h
	src/shaders/font_shader.h
	src/shaders/upscale_shader.h
)

if(CUTE_FRAMEWORK_STATIC)
	add_library(cute STATIC ${CUTE_SRCS} ${CUTE_HDRS})
	target_compile_definitions(cute PUBLIC CUTE_STATIC)
else()
	add_library(cute SHARED ${CUTE_SRCS} ${CUTE_HDRS})
endif()
target_compile_definitions(cute PRIVATE CUTE_EXPORT)

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

if(MACOSX)
	set(PHYSFS_SRCS ${PHYSFS_SRCS}
		libraries/physfs/physfs_platform_apple.m
	)
	find_library(IOKIT IOKit)
	find_library(FOUNDATION Foundation)
	find_library(SECURITY Security)
	find_package(OpenGL REQUIRED)
	set(CUTE_LINK_LIBS ${CUTE_LINK_LIBS} ${IOKIT} ${FOUNDATION} ${SECURITY} ${OPENGL_LIBRARIES})
endif()

if(LINUX)
	find_package(OpenGL REQUIRED)
	set(CUTE_LINK_LIBS ${CUTE_LINK_LIBS} ${IOKIT} ${FOUNDATION} ${SECURITY} ${OPENGL_LIBRARIES})
endif()

add_library(physfs STATIC ${PHYSFS_SRCS})
set(CUTE_LINK_LIBS ${CUTE_LINK_LIBS} physfs)

# Optional mbedtls for HTTPS support.
if(CUTE_FRAMEWORK_WITH_HTTPS)
	option(ENABLE_PROGRAMS "Build mbed TLS programs." OFF)
	option(ENABLE_TESTING "Build mbed TLS tests." OFF)
	fetch_content_with_folder(
		mbedtls
		GIT_REPOSITORY https://github.com/Mbed-TLS/mbedtls
		GIT_TAG v3.1.0
		GIT_PROGRESS TRUE
	)
	set(CUTE_LINK_LIBS ${CUTE_LINK_LIBS} mbedx509 mbedtls mbedcrypto)
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
		GIT_TAG release-2.0.14
		GIT_PROGRESS TRUE
	)
endif()
set(CUTE_LINK_LIBS ${CUTE_LINK_LIBS} SDL2-static)
target_include_directories(cute PUBLIC ${SDL2_INCLUDE_DIRS})

# Some platform specific settings.
if(EMSCRIPTEN)
	target_compile_options(cute PUBLIC -O1 -fno-rtti -fno-exceptions)
	target_link_libraries(cute PRIVATE "-s USE_WEBGL2=1 -s ASSERTIONS=1 -s MAX_WEBGL_VERSION=2 -s USE_SDL=2 -s ALLOW_MEMORY_GROWTH=1 -O1 -s ASYNCIFY=1")
elseif(MINGW)
	set(CUTE_LINK_LIBS ${CUTE_LINK_LIBS} d3d11 crypt32)
elseif(WINDOWS)
	set(CUTE_LINK_LIBS ${CUTE_LINK_LIBS} crypt32)
endif()

# Link up all dependencies to Cute.
target_link_libraries(cute PUBLIC ${CUTE_LINK_LIBS})

# Cute unit tests executable (optional, defaulted to also build).
if (CUTE_FRAMEWORK_BUILD_TESTS)
	set(CUTE_TEST_SRCS test/main.cpp)
	set(CUTE_TEST_HDRS
		test/test_circular_buffer.h
		test/test_handle.h
		test/test_harness.h
		test/test_doubly_list.h
		test/test_kv.h
		test/test_base64.h
		test/test_audio.h
		test/test_ecs.h
		test/test_lru_cache.h
		test/test_array.h
		test/test_aseprite.h
		test/test_png_cache.h
		test/test_sprite.h
		test/test_coroutine.h
		test/test_string.h
	)

	add_executable(tests ${CUTE_TEST_SRCS} ${CUTE_TEST_HDRS})
	target_link_libraries(tests PRIVATE cute)

	if(EMSCRIPTEN)
		set(CMAKE_EXECUTABLE_SUFFIX ".html")
		target_compile_options(tests PUBLIC -O1 -fno-rtti -fno-exceptions)
		target_link_options(tests PRIVATE -o tests.html --preload-file ${PROJECT_SOURCE_DIR}/test/test_data --emrun -O1)
	else()
		# Copy over any test data in the test/test_data folder.
		add_custom_command(TARGET tests POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory "${PROJECT_SOURCE_DIR}/test/test_data" $<TARGET_FILE_DIR:tests>/test_data)
	endif()

	# For convenience make tests the startup project in Visual Studio.
	# Also set working directory to the target output folder.
	if (MSVC)
		set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT tests)
		set_property(TARGET tests PROPERTY VS_DEBUGGER_WORKING_DIRECTORY $<TARGET_FILE_DIR:tests>)
	endif()
endif()

# Propogate public headers to other cmake scripts including this subdirectory.
target_include_directories(cute PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
target_include_directories(cute PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/libraries>)

install(TARGETS cute EXPORT cute DESTINATION "lib")
install(FILES ${CUTE_PUBLIC_HDRS} DESTINATION "include/cute")
install(FILES ${IMGUI_HDRS} DESTINATION "include/cute/imgui")
install(FILES ${SOKOL_HDRS} DESTINATION "include/cute/sokol")
