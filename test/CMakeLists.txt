set(CF_TEST_SRCS
	main.cpp
	test_array.cpp
	test_aseprite.cpp
	test_audio.cpp
	test_base64.cpp
	test_color.cpp
	test_coroutine.cpp
	test_doubly_list.cpp
	test_hashtable.cpp
	test_path.cpp
	test_png_cache.cpp
	test_sprite.cpp
	test_string.cpp
	test_json.cpp
	test_markups.cpp
	test_math.cpp
	test_math.c
	test_ckit.c
)
set(CF_TEST_HDRS test_harness.h)

add_executable(tests ${CF_TEST_SRCS} ${CF_TEST_HDRS})
target_link_libraries(tests PRIVATE cute)
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
