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
	add_subdirectory(${_folder} ${ARGN})
	set_property(DIRECTORY "${_folder}" PROPERTY FOLDER "${_folder_name}")
endfunction()

option(CUTE_FRAMEWORK_STATIC "Build static library for Cute Framework." ON)
option(CUTE_FRAMEWORK_WITH_PLATFORM "Build Cute Framework with platform support from SDL2 (zlib license)." ON)
option(CUTE_FRAMEWORK_WITH_HTTPS "Build Cute Framework with mbedtls for HTTPS support (Apache 2.0 license)." ON)
option(CUTE_FRAMEWORK_WITH_SODIUM "Build Cute Framework with cryptography + authentication support from Sodium for networking support (ISC license)." ON)
option(CUTE_FRAMEWORK_BUILD_TESTS "Build the cute framework unit tests." ON)
option(CUTE_FRAMEWORK_BUILD_BLOCK_MAN "Build the Block Man game." ON)
option(CUTE_PONG "Build Pong by Tony." ON)

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
	add_definitions(-DCUTE_WINDOWS)
elseif(LINUX)
	add_definitions(-DCUTE_LINUX)
	set(CMAKE_POSITION_INDEPENDENT_CODE TRUE)
elseif(MACOSX)
	add_definitions(-DCUTE_MACOSX)
	set(CMAKE_POSITION_INDEPENDENT_CODE TRUE)
elseif(EMSCRIPTEN)
	add_definitions(-DCUTE_EMSCRIPTEN)
endif()

# Disable annoying MSVC warnings.
if(MSVC)
	add_definitions(-D_CRT_SECURE_NO_WARNINGS)
	add_definitions(-D_CRT_SECURE_NO_DEPRECATE)
endif()

# Common directories for compiler/linker path.
include_directories(src libraries test)

# Optional Sodium for security.
if(CUTE_FRAMEWORK_WITH_SODIUM)
	add_definitions(-DSODIUM_STATIC)
	if(WINDOWS OR MACOSX)
		set(sodium_USE_STATIC_LIBS ON)

		# FindSodium.cmake expects sodium_DIR to be set.
		if(MINGW)
			set(sodium_DIR prebuilt_libs/mingw/sodium)
		else()
			set(sodium_DIR prebuilt_libs/windows/sodium)
		endif()

		# This adds the current directory to the cmake path, so it can
		# use FindSodium.cmake script to locate the sodium folders.
		list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/.")
		find_package(Sodium REQUIRED)
	elseif(EMSCRIPTEN)
		link_directories(prebuilt_libs/emscripten)
	endif()
	# FindSodium.cmake has set sodium_INCLUDE_DIR.
	include_directories(${sodium_INCLUDE_DIR})
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

# Optional SDL2 for platform support.
if(CUTE_FRAMEWORK_WITH_PLATFORM)
	# Just don't build the shared library at all, it's not needed.
	set(SDL_SHARED_ENABLED_BY_DEFAULT OFF)

	if(WINDOWS)
		# This is for static linking on Windows.
		add_subdirectory_with_folder(libraries/SDL2/SDL2-2.0.14 "SDL2")
	endif()
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

	src/internal/cute_transport_internal.cpp
	src/internal/cute_ecs_internal.cpp
	src/internal/cute_dx11.cpp

	src/internal/imgui/imgui_impl_sdl.cpp

	libraries/imgui/imgui.cpp
	libraries/imgui/imgui_demo.cpp
	libraries/imgui/imgui_draw.cpp
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

set(CUTE_HDRS
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

	src/internal/imgui/sokol_imgui.h
	src/internal/imgui/imgui_impl_sdl.h

	src/shaders/sprite_shader.h
	src/shaders/font_shader.h
	src/shaders/upscale_shader.h

	libraries/imgui/imgui.h

	include/sokol/sokol_gfx.h
)

if(CUTE_FRAMEWORK_STATIC)
	add_definitions(-DCUTE_STATIC)
	add_library(cute STATIC ${CUTE_SRCS} ${CUTE_HDRS} ${GLAD_SRCS} ${GLAD_HDRS})
else()
	add_library(cute SHARED ${CUTE_SRCS} ${CUTE_HDRS} ${GLAD_SRCS} ${GLAD_HDRS})
endif()
target_include_directories(cute PRIVATE ${GLAD_INCLUDE_DIRECTORY})
include_directories(cute PRIVATE "include")

if(MACOSX)
	find_package(SDL2 REQUIRED)

	# These are needed to link to PHYSFS.
	find_library(IOKIT IOKit)
	find_library(FOUNDATION Foundation)

	target_link_libraries(cute PRIVATE ${SDL2_LIBRARIES} sodium physfs ${IOKIT} ${FOUNDATION})
elseif(EMSCRIPTEN)
	target_compile_options(cute PUBLIC -O1 -fno-rtti -fno-exceptions)
	target_link_libraries(cute PRIVATE sodium physfs "-s USE_WEBGL2=1 -s ASSERTIONS=1  -s MAX_WEBGL_VERSION=2 -s USE_SDL=2 -s ALLOW_MEMORY_GROWTH=1 -O1")
else()
	if(MINGW)
		target_link_libraries(cute PRIVATE SDL2-static sodium physfs d3d11)
	else()
		target_link_libraries(cute PRIVATE SDL2-static sodium physfs)
	endif()
endif()

if(CUTE_FRAMEWORK_WITH_HTTPS)
	target_link_libraries(cute PRIVATE mbedx509 mbedtls mbedcrypto)
endif()

target_compile_definitions(cute PRIVATE CUTE_EXPORT)

# Cute block man demo game.
if (CUTE_FRAMEWORK_BUILD_BLOCK_MAN)
	set(CUTE_BLOCK_MAN_SRCS
		block_man/main.cpp
		block_man/world.h
		block_man/world.cpp
		block_man/serialize.h
		block_man/serialize.cpp
		block_man/shaders/light_shader.h
	)

	set(CUTE_BLOCK_MAN_IMGUI_SRCS
		libraries/imgui/imgui.cpp
		libraries/imgui/imgui_demo.cpp
		libraries/imgui/imgui_draw.cpp
		libraries/imgui/imgui_widgets.cpp
	)

	set(CUTE_BLOCK_MAN_COMPONENT_SRCS
		block_man/components/animator.h
		block_man/components/board_piece.h
		block_man/components/ice_block.h
		block_man/components/player.h
		block_man/components/reflection.h
		block_man/components/transform.h
		block_man/components/shadow.h
		block_man/components/mochi.h
		block_man/components/fire.h
		block_man/components/light.h
		block_man/components/oil.h
		block_man/components/lamp.h
		block_man/components/ladder.h
	)

	set(CUTE_BLOCK_MAN_SYSTEM_SRCS
		block_man/systems/animator_system.h
		block_man/systems/animator_system.cpp
		block_man/systems/board_system.h
		block_man/systems/board_system.cpp
		block_man/systems/ice_block_system.h
		block_man/systems/ice_block_system.cpp
		block_man/systems/player_system.h
		block_man/systems/player_system.cpp
		block_man/systems/reflection_system.h
		block_man/systems/reflection_system.cpp
		block_man/systems/transform_system.h
		block_man/systems/transform_system.cpp
		block_man/systems/shadow_system.h
		block_man/systems/shadow_system.cpp
		block_man/systems/mochi_system.h
		block_man/systems/mochi_system.cpp
		block_man/systems/light_system.h
		block_man/systems/light_system.cpp
	)

	add_executable(block_man
		${CUTE_BLOCK_MAN_SRCS}
		${CUTE_BLOCK_MAN_IMGUI_SRCS}
		${CUTE_BLOCK_MAN_COMPONENT_SRCS}
		${CUTE_BLOCK_MAN_SYSTEM_SRCS}
	)
	source_group("game" FILES ${CUTE_BLOCK_MAN_SRCS})
	source_group("imgui" FILES ${CUTE_BLOCK_MAN_IMGUI_SRCS})
	source_group("components" FILES ${CUTE_BLOCK_MAN_COMPONENT_SRCS})
	source_group("systems" FILES ${CUTE_BLOCK_MAN_SYSTEM_SRCS})
	target_link_libraries(block_man PRIVATE cute)
	if(EMSCRIPTEN)
		set(CMAKE_EXECUTABLE_SUFFIX ".html")
		target_compile_options(block_man PUBLIC -O1 -fno-rtti -fno-exceptions)
		target_link_options(block_man PRIVATE -o block_man.html --preload-file ../block_man/data --emrun -O1)
	endif()
	target_include_directories(block_man PUBLIC "${PROJECT_SOURCE_DIR}/block_man")

	# Copy over prebuilt binaries to output directory.
	if(EMSCRIPTEN)
		add_custom_command(TARGET tests POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory "${PROJECT_SOURCE_DIR}/prebuilt_libs/emscripten" $<TARGET_FILE_DIR:tests>)
	endif()

	# For convenience set working directory to the target output folder.
	if (MSVC)
		set_property(TARGET block_man PROPERTY VS_DEBUGGER_WORKING_DIRECTORY $<TARGET_FILE_DIR:block_man>)
	endif()
endif()

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
		test/test_client_server.h
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
	)

	add_executable(tests ${CUTE_TEST_SRCS} ${CUTE_TEST_HDRS})
	target_link_libraries(tests PRIVATE cute)

	# Copy over prebuilt binaries to output directory.
	if(EMSCRIPTEN)
		add_custom_command(TARGET tests POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory "${PROJECT_SOURCE_DIR}/prebuilt_libs/emscripten" $<TARGET_FILE_DIR:tests>)
	endif()

	# Copy over any test data in the test/test_data folder.
	add_custom_command(TARGET tests POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory "${PROJECT_SOURCE_DIR}/test/test_data" $<TARGET_FILE_DIR:tests>/test_data)

	# For convenience make tests the startup project in Visual Studio.
	# Also set working directory to the target output folder.
	if (MSVC)
		set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT tests)
		set_property(TARGET tests PROPERTY VS_DEBUGGER_WORKING_DIRECTORY $<TARGET_FILE_DIR:tests>)
	endif()
endif()

# ----------------------------------------------------------------------------------------

# Pong by Tony.
if (CUTE_PONG)
	set(CUTE_PONG_SRCS
		pong/main.cpp
		pong/pong_global.h
		pong/pong_utils.h
		pong/pong_graphics.h
		pong/pong_game.h
	)

	set(CUTE_PONG_IMGUI_SRCS
		libraries/imgui/imgui.cpp
		libraries/imgui/imgui_demo.cpp
		libraries/imgui/imgui_draw.cpp
		libraries/imgui/imgui_widgets.cpp
	)

	add_executable(pong
		${CUTE_PONG_SRCS}
		${CUTE_PONG_IMGUI_SRCS}
	)
	source_group("pong" FILES ${CUTE_PONG_SRCS})
	source_group("imgui" FILES ${CUTE_PONG_IMGUI_SRCS})
	target_link_libraries(pong PRIVATE cute)
	if(EMSCRIPTEN)
		set(CMAKE_EXECUTABLE_SUFFIX ".html")
		target_compile_options(pong PUBLIC -O1 -fno-rtti -fno-exceptions)
		target_link_options(pong PRIVATE -o pong.html --preload-file ../pong/data --emrun -O1)
	endif()
	target_include_directories(pong PUBLIC "${PROJECT_SOURCE_DIR}/pong")

	# Copy over prebuilt binaries to output directory.
	if(EMSCRIPTEN)
		add_custom_command(TARGET tests POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory "${PROJECT_SOURCE_DIR}/prebuilt_libs/emscripten" $<TARGET_FILE_DIR:tests>)
	endif()

	# For convenience set working directory to the target output folder.
	if (MSVC)
		set_property(TARGET pong PROPERTY VS_DEBUGGER_WORKING_DIRECTORY $<TARGET_FILE_DIR:pong>)
	endif()
endif()
