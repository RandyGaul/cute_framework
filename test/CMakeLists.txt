set(CF_TEST_SRCS
	main.cpp
	test_alloc.cpp
	test_arith.cpp
	test_app.cpp
	test_array.cpp
	test_aseprite.cpp
	test_video.cpp
	test_audio.cpp
	test_base64.cpp
	test_color.cpp
	test_coroutine.cpp
	test_doubly_list.cpp
	test_hashtable.cpp
	test_path.cpp
	test_custom_sprite.cpp
	test_sprite.cpp
	test_string.cpp
	test_json.cpp
	test_markups.cpp
	test_draw_tiled.cpp
	test_atlas_uvs.cpp
	test_graphics_3d.cpp
	test_shader_reload.cpp
	test_shader_directory.cpp
	test_canvas_clear.cpp
	test_compute.cpp
	test_mrt.cpp
	test_texture_types.cpp
	test_shadow_sampling.cpp
	test_instancing.cpp
	test_draw3d.cpp
	test_uniform_arrays.cpp
	test_gles.cpp
	test_math.cpp
	test_math.c
	test_math3d.cpp
	test_math3d.c
	test_physics.cpp
	test_networking.cpp
	test_ckit.c
	test_model.cpp
	test_jpg.cpp
	test_dds.cpp
	test_sym.cpp
	test_crash.cpp
	test_app_shared.cpp
)
set(CF_TEST_HDRS test_harness.h test_app_shared.h jpg_test_vectors.h)

add_executable(tests ${CF_TEST_SRCS} ${CF_TEST_HDRS})
target_link_libraries(tests PRIVATE cute)
# test_crash drives the crashme sample as a child process; test_sym reads the tests binary's own debug info.
if (CF_CUTE_SYM AND NOT EMSCRIPTEN AND NOT CMAKE_CROSSCOMPILING)
	cf_symbols(tests FILE)
	target_compile_definitions(tests PRIVATE CF_TEST_HAVE_SYMBOLS)
	if (CF_FRAMEWORK_BUILD_SAMPLES)
		add_dependencies(tests crashme)
		target_compile_definitions(tests PRIVATE CF_TEST_HAVE_CRASHME)
	endif()
endif()
target_include_directories(tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../src)
set_target_properties(tests PROPERTIES
	RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)
# For convenience make tests the startup project in Visual Studio.
# Also set working directory to the target output folder.
if (MSVC)
	set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT tests)
	set_property(TARGET tests PROPERTY VS_DEBUGGER_WORKING_DIRECTORY $<TARGET_FILE_DIR:tests>)
endif()
