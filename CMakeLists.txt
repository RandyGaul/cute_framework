cmake_minimum_required(VERSION 3.0)
project(cute_framework)

# Must have at least C++14.
set(CMAKE_CXX_STANDARD 14)

# Used to hide dependencies in filters for Visual Studio.
# From: https://stackoverflow.com/a/45176243
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
define_property(
	TARGET
	PROPERTY FOLDER
	INHERITED
	BRIEF_DOCS "Set the folder name."
	FULL_DOCS  "Use to organize targets in an IDE."
)
function(add_subdirectory_with_folder _folder _folder_name)
	add_subdirectory(${_folder} ${ARGN} EXCLUDE_FROM_ALL)
	set_property(DIRECTORY "${_folder}" PROPERTY FOLDER "${_folder_name}")
endfunction()

option(CUTE_FRAMEWORK_STATIC "Build static library for Cute Framework." ON)
option(CUTE_FRAMEWORK_WITH_HTTPS "Build Cute Framework with mbedtls for HTTPS support (Apache 2.0 license)." ON)
option(CUTE_FRAMEWORK_WITH_HYDROGEN "Build Cute Framework with cryptography + authentication support from Hydrogen for networking support (ISC license)." ON)
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

# Defines for cross-platform shader support.
if(WINDOWS)
elseif(LINUX)
	#set(CMAKE_POSITION_INDEPENDENT_CODE TRUE)
elseif(MACOSX)
	#set(CMAKE_POSITION_INDEPENDENT_CODE TRUE)
elseif(EMSCRIPTEN)
endif()

# Disable annoying MSVC warnings.
if(MSVC)
	add_definitions(-D_CRT_SECURE_NO_WARNINGS)
	add_definitions(-D_CRT_SECURE_NO_DEPRECATE)
	add_compile_options("$<$<C_COMPILER_ID:MSVC>:/utf-8>")
	add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")
endif()

# Common directories for compiler/linker path.
include_directories(src libraries test)

# Optional Hydrogen for security.
if(CUTE_FRAMEWORK_WITH_HYDROGEN)
	set(HYDROGEN_HDRS
		libraries/libhydrogen/hydrogen.h
	)
	set(HYDROGEN_SRCS
		libraries/libhydrogen/hydrogen.c
	)
	include_directories(libraries/libhydrogen)
endif()

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
endif()

add_library(physfs STATIC ${PHYSFS_SRCS})

# Optional mbedtls for HTTPS support.
if(CUTE_FRAMEWORK_WITH_HTTPS)
	add_subdirectory_with_folder(libraries/mbedtls/mbedtls-2.24.0 "mbedtls")
endif()

# SDL2 for platform support.
# Just don't build the shared library at all, it's not needed.
set(SDL_SHARED_ENABLED_BY_DEFAULT OFF)

# Static linking for simplicity.
# TODO - Replace with FetchContent?
if(EMSCRIPTEN)
	# Emscripten provides its own SDL2.
else()
	add_subdirectory_with_folder(libraries/SDL2/SDL2-2.0.14 "SDL2")
endif()

# Cute Framework shared library.
set(CUTE_SRCS
	src/cute_app.cpp
	src/cute_audio.cpp
	src/cute_circular_buffer.cpp
	src/cute_client.cpp
	src/cute_clipboard.cpp
	src/cute_concurrency.cpp
	src/cute_crypto.cpp
	src/cute_error.cpp
	src/cute_file_system.cpp
	src/cute_handle_table.cpp
	src/cute_input.cpp
	src/cute_net.cpp
	src/cute_server.cpp
	src/cute_timer.cpp
	src/cute_version.cpp
	src/cute_memory_pool.cpp
	src/cute_protocol.cpp
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

	src/internal/cute_transport_internal.cpp
	src/internal/cute_ecs_internal.cpp
	src/internal/cute_dx11.cpp

	src/internal/imgui/imgui_impl_sdl.cpp

	libraries/imgui/imgui.cpp
	libraries/imgui/imgui_demo.cpp
	libraries/imgui/imgui_draw.cpp
	libraries/imgui/imgui_tables.cpp
	libraries/imgui/imgui_widgets.cpp
)

if(EMSCRIPTEN)
	set(GLAD_SRCS libraries/glad/gles3/glad/glad.c)
	set(GLAD_HDRS
		libraries/glad/gles3/glad/glad.h
		libraries/glad/gles3/KHR/khrplatform.h
	)
	set(GLAD_INCLUDE_DIRECTORY libraries/glad/gles3)
else()
	set(GLAD_SRCS libraries/glad/gl33/glad/glad.c)
	set(GLAD_HDRS
		libraries/glad/gl33/glad/glad.h
		libraries/glad/gl33/KHR/khrplatform.h
	)
	set(GLAD_INCLUDE_DIRECTORY libraries/glad/gl33)
endif()

set(CUTE_PUBLIC_HDRS
	include/cute_alloc.h
	include/cute_app.h
	include/cute_audio.h
	include/cute_c_runtime.h
	include/cute_circular_buffer.h
	include/cute_client.h
	include/cute_clipboard.h
	include/cute_concurrency.h
	include/cute_crypto.h
	include/cute_defines.h
	include/cute_error.h
	include/cute_file_system.h
	include/cute_file_system_utils.h
	include/cute_handle_table.h
	include/cute_input.h
	include/cute_net.h
	include/cute_server.h
	include/cute_timer.h
	include/cute_version.h
	include/cute_memory_pool.h
	include/cute_protocol.h
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
)

set(CUTE_IMGUI_HDRS
	libraries/imgui/imgui.h
)

set(CUTE_SOKOL_HDRS
	include/sokol/sokol_gfx.h
)

set(CUTE_CUTE_HDRS
	include/cute/cute_aseprite.h
	include/cute/cute_c2.h
	include/cute/cute_coroutine.h
	include/cute/cute_font.h
	include/cute/cute_path.h
	include/cute/cute_png.h
	include/cute/cute_sound.h
	include/cute/cute_spritebatch.h
	include/cute/cute_sync.h
	include/cute/cute_utf.h
)

set(CUTE_HDRS
	${CUTE_PUBLIC_HDRS}
	${CUTE_IMGUI_HDRS}
	${CUTE_SOKOL_HDRS}
	${CUTE_CUTE_HDRS}

	src/internal/cute_app_internal.h
	src/internal/cute_audio_internal.h
	src/internal/cute_crypto_internal.h
	src/internal/cute_file_system_internal.h
	src/internal/cute_font_internal.h
	src/internal/cute_input_internal.h
	src/internal/cute_net_internal.h
	src/internal/cute_serialize_internal.h
	src/internal/cute_protocol_internal.h
	src/internal/cute_transport_internal.h
	src/internal/cute_object_table_internal.h
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
	add_library(cute STATIC ${CUTE_SRCS} ${CUTE_HDRS} ${GLAD_SRCS} ${GLAD_HDRS} ${HYDROGEN_SRCS} ${HYDROGEN_HDRS})
	target_compile_definitions(cute PUBLIC CUTE_STATIC)
else()
	add_library(cute SHARED ${CUTE_SRCS} ${CUTE_HDRS} ${GLAD_SRCS} ${GLAD_HDRS} ${HYDROGEN_SRCS} ${HYDROGEN_HDRS})
endif()
target_include_directories(cute PRIVATE ${GLAD_INCLUDE_DIRECTORY})
include_directories(cute PRIVATE "include")

if(MACOSX)
	# These are needed to link to PHYSFS.
	find_library(IOKIT IOKit)
	find_library(FOUNDATION Foundation)
	find_library(SECURITY Security)

	target_link_libraries(cute PRIVATE ${SDL2_LIBRARIES} physfs ${IOKIT} ${FOUNDATION} ${SECURITY})
elseif(EMSCRIPTEN)
	target_compile_options(cute PUBLIC -O1 -fno-rtti -fno-exceptions)
	target_link_libraries(cute PRIVATE physfs "-s USE_WEBGL2=1 -s ASSERTIONS=1  -s MAX_WEBGL_VERSION=2 -s USE_SDL=2 -s ALLOW_MEMORY_GROWTH=1 -O1")
else()
	if(MINGW)
		target_link_libraries(cute PRIVATE SDL2-static physfs d3d11 crypt32)
	elseif(WINDOWS)
		target_link_libraries(cute PRIVATE SDL2-static physfs crypt32)
	else()
		target_link_libraries(cute PRIVATE SDL2-static physfs)
	endif()
endif()

if(CUTE_FRAMEWORK_WITH_HTTPS)
	target_link_libraries(cute PRIVATE mbedx509 mbedtls mbedcrypto)
endif()

target_compile_definitions(cute PRIVATE CUTE_EXPORT)

# Cute unit tests executable (optional, defaulted to also build).
if (CUTE_FRAMEWORK_BUILD_TESTS)
	set(CUTE_TEST_SRCS test/main.cpp)
	set(CUTE_TEST_HDRS
		test/test_circular_buffer.h
		test/test_crypto.h
		test/test_handle.h
		test/test_harness.h
		test/test_replay_buffer.h
		test/test_socket.h
		test/test_connect_token.h
		test/test_packets.h
		test/test_hashtable.h
		test/test_encryption_map.h
		test/test_doubly_list.h
		test/test_connect_token_cache.h
		test/test_protocol_client_server.h
		test/test_sequence_buffer.h
		test/test_transport.h
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
		test/test_client_server.h
	)

	add_executable(tests ${CUTE_TEST_SRCS} ${CUTE_TEST_HDRS})
	target_link_libraries(tests PRIVATE cute)

	if(EMSCRIPTEN)
		set(CMAKE_EXECUTABLE_SUFFIX ".html")
		target_compile_options(tests PUBLIC -O1 -fno-rtti -fno-exceptions)
		target_link_options(tests PRIVATE -o tests.html --preload-file ${CMAKE_SOURCE_DIR}/test/test_data --emrun -O1)
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

install(TARGETS cute EXPORT cute DESTINATION "lib")
install(FILES ${CUTE_PUBLIC_HDRS} DESTINATION "include/cute")
install(FILES ${CUTE_SOKOL_HDRS} DESTINATION "include/cute/sokol")
install(FILES ${CUTE_CUTE_HDRS} DESTINATION "include/cute/cute")
