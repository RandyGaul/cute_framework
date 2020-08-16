cmake_minimum_required(VERSION 2.8.4)
project(cute_framework)

# Force 64bit builds.
if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
	message(FATAL_ERROR "Only 64-bit build is supported.")
	return()
endif()

# Disable annoying MSVC warnings.
if(MSVC)
	add_definitions(-D_CRT_SECURE_NO_WARNINGS)
	add_definitions(-D_CRT_SECURE_NO_DEPRECATE)
endif()

# Common directories for compiler/linker path.
include_directories(src libraries test libraries/datalibrary)
link_directories(shared_libs)

# PhysicsFS, statically linked.
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
endif()

add_library(physfs STATIC ${PHYSFS_SRCS})

# Cute Framework shared library.
set(CUTE_SRCS
	src/cute_app.cpp
	src/cute_audio.cpp
	src/cute_buffer.cpp
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
	src/cute_log.cpp
	src/cute_hashtable.cpp
	src/cute_ecs.cpp
	src/cute_string.cpp
	src/cute_string_utils.cpp
	src/cute_typeless_array.cpp
	src/cute_math.cpp
	src/cute_sprite.cpp
	src/cute_window.cpp
	src/cute_image.cpp
	src/cute_font.cpp
	src/cute_camera.cpp
	src/cute_file_index.cpp
	src/cute_gfx.cpp

	src/internal/cute_transport_internal.cpp
	src/internal/cute_ecs_internal.cpp
	src/internal/cute_dx11.cpp

	src/internal/imgui/imgui_impl_dx11.cpp
	src/internal/imgui/imgui_impl_sdl.cpp

	libraries/imgui/imgui.cpp
	libraries/imgui/imgui_demo.cpp
	libraries/imgui/imgui_draw.cpp
	libraries/imgui/imgui_widgets.cpp

	libraries/glad/glad.c
)

set(CUTE_HDRS
	src/cute_alloc.h
	src/cute_app.h
	src/cute_audio.h
	src/cute_buffer.h
	src/cute_c_runtime.h
	src/cute_circular_buffer.h
	src/cute_client.h
	src/cute_clipboard.h
	src/cute_concurrency.h
	src/cute_crypto.h
	src/cute_crypto_utils.h
	src/cute_defines.h
	src/cute_error.h
	src/cute_file_system.h
	src/cute_file_system_utils.h
	src/cute_handle_table.h
	src/cute_input.h
	src/cute_net.h
	src/cute_server.h
	src/cute_timer.h
	src/cute_version.h
	src/cute_memory_pool.h
	src/cute_protocol.h
	src/cute_doubly_list.h
	src/cute_kv.h
	src/cute_kv_utils.h
	src/cute_base64.h
	src/cute_log.h
	src/cute_array.h
	src/cute_hashtable.h
	src/cute_dictionary.h
	src/cute_ecs.h
	src/cute_string.h
	src/cute_string_utils.h
	src/cute_typeless_array.h
	src/cute_defer.h
	src/cute_math.h
	src/cute_sprite.h
	src/cute_lru_cache.h
	src/cute_window.h
	src/cute_debug_printf.h
	src/cute_image.h
	src/cute_color.h
	src/cute_font.h
	src/cute_camera.h
	src/cute_file_index.h
	src/cute.h
	src/cute_gfx.h

	src/internal/cute_app_internal.h
	src/internal/cute_audio_internal.h
	src/internal/cute_crypto_internal.h
	src/internal/cute_defines_internal.h
	src/internal/cute_file_system_internal.h
	src/internal/cute_input_internal.h
	src/internal/cute_net_internal.h
	src/internal/cute_serialize_internal.h
	src/internal/cute_protocol_internal.h
	src/internal/cute_transport_internal.h
	src/internal/cute_object_table_internal.h
	src/internal/cute_ecs_internal.h
	src/internal/cute_dx11.h

	src/internal/imgui/imgui_impl_dx11.h
	src/internal/imgui/imgui_impl_sdl.h

	libraries/glad/glad.h
	libraries/imgui/imgui.h
	libraries/sokol/sokol_gfx.h
)

# Handle DirectX 9 link directories.
#if (MSVC)
#	link_directories($ENV{DXSDK_DIR}/Lib/x64)
#endif()

add_library(cute SHARED ${CUTE_SRCS} ${CUTE_HDRS})
target_link_libraries(cute PRIVATE SDL2 physfs libsodium)
target_compile_definitions(cute PRIVATE CUTE_EXPORT)

# Handle DirectX 9 include directories.
#if (MSVC)
#	target_include_directories(cute PRIVATE $ENV{DXSDK_DIR}/Include)
#endif()

# Cute unit tests executable (optional, defaulted to also build).
option(CUTE_FRAMEWORK_BUILD_TESTS "Build the cute framework unit tests." ON)
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
	)

	add_executable(tests ${CUTE_TEST_SRCS} ${CUTE_TEST_HDRS})
	target_link_libraries(tests PRIVATE cute)

	# Copy over prebuilt binaries to output directory.
	add_custom_command(TARGET tests POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory "${PROJECT_SOURCE_DIR}/shared_libs" $<TARGET_FILE_DIR:tests>)

	# Copy over any test data in the test/test_data folder.
	add_custom_command(TARGET tests POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory "${PROJECT_SOURCE_DIR}/test/test_data" $<TARGET_FILE_DIR:tests>)

	# For convenience make tests the startup project in Visual Studio.
	# Also set working directory to the target output folder.
	if (MSVC)
		set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT tests)
		set_property(TARGET tests PROPERTY VS_DEBUGGER_WORKING_DIRECTORY $<TARGET_FILE_DIR:tests>)
	endif()
endif()

# Cute unit tests executable (optional, defaulted to also build).
option(CUTE_FRAMEWORK_BUILD_BLOCK_MAN "Build the Block Man game." ON)
if (CUTE_FRAMEWORK_BUILD_BLOCK_MAN)
	set(CUTE_BLOCK_MAN_SRCS
		block_man/main.cpp

		libraries/imgui/imgui.cpp
		libraries/imgui/imgui_demo.cpp
		libraries/imgui/imgui_draw.cpp
		libraries/imgui/imgui_widgets.cpp
	)
	add_executable(block_man ${CUTE_BLOCK_MAN_SRCS})
	target_link_libraries(block_man PRIVATE cute)

	# For convenience set working directory to the target output folder.
	if (MSVC)
		set_property(TARGET block_man PROPERTY VS_DEBUGGER_WORKING_DIRECTORY $<TARGET_FILE_DIR:block_man>)
	endif()

	# Copy over any test data in the test/test_data folder.
	add_custom_command(TARGET block_man POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory "${PROJECT_SOURCE_DIR}/block_man/data" $<TARGET_FILE_DIR:block_man>/data)
endif()
