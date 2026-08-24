# The shader compiler front-end (cute_shader.cpp) builds directly into the cute
# target. It sits on cute_spirv.h, CF's own GLSL -> SPIR-V compiler with
# transpiler backends for HLSL/MSL/GLSL ES 300 -- no external dependencies.
target_sources(cute PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/cute_shader.cpp)

# The CF_RUNTIME_SHADER_COMPILATION macro stays defined for backwards
# compatibility with user code using the documented #ifdef idiom.
target_compile_definitions(cute PUBLIC CF_RUNTIME_SHADER_COMPILATION)

# cute-spirv-tests, the compiler's unit test suite. Runs standalone (no SDL);
# validates emitted modules with spirv-val when CSPV_SPIRV_VAL points at it.
if (CMAKE_CURRENT_SOURCE_DIR STREQUAL "${CMAKE_SOURCE_DIR}/tools")
	add_executable(cute-spirv-tests ${CMAKE_CURRENT_SOURCE_DIR}/cute_spirv_test.c)
	set_source_files_properties(${CMAKE_CURRENT_SOURCE_DIR}/cute_spirv_test.c PROPERTIES LANGUAGE CXX)
	target_include_directories(cute-spirv-tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/../libraries ${CMAKE_CURRENT_SOURCE_DIR}/../include)
	target_compile_definitions(cute-spirv-tests PRIVATE _CRT_SECURE_NO_WARNINGS)
	set_target_properties(cute-spirv-tests PROPERTIES FOLDER "tools")
endif()

# cute-model-fuzz, a mutation fuzzer for cute_model.h. Standalone (ckit only), so it
# builds cleanly under a sanitizer:
#     cl /TP /EHsc /fsanitize=address /Zi /Od /I libraries tools\cute_model_fuzz.c
# See the file header for usage.
if (CMAKE_CURRENT_SOURCE_DIR STREQUAL "${CMAKE_SOURCE_DIR}/tools")
	add_executable(cute-model-fuzz ${CMAKE_CURRENT_SOURCE_DIR}/cute_model_fuzz.c)

	# cute-jpg-fuzz, the same mutation-fuzzer treatment for cute_jpg.h. Standalone.
	add_executable(cute-jpg-fuzz ${CMAKE_CURRENT_SOURCE_DIR}/cute_jpg_fuzz.c)
	set_source_files_properties(${CMAKE_CURRENT_SOURCE_DIR}/cute_jpg_fuzz.c PROPERTIES LANGUAGE CXX)
	target_include_directories(cute-jpg-fuzz PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../libraries)
	target_compile_definitions(cute-jpg-fuzz PRIVATE _CRT_SECURE_NO_WARNINGS)
	set_target_properties(cute-jpg-fuzz PROPERTIES FOLDER "tools" RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
	set_source_files_properties(${CMAKE_CURRENT_SOURCE_DIR}/cute_model_fuzz.c PROPERTIES LANGUAGE CXX)
	target_include_directories(cute-model-fuzz PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../libraries)
	target_compile_definitions(cute-model-fuzz PRIVATE _CRT_SECURE_NO_WARNINGS)
	set_target_properties(cute-model-fuzz PROPERTIES FOLDER "tools" RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

	# The cute_h264.h harness. Standalone like the fuzzers above. cute-h264-test encodes synthetic
	# patterns and cute-h264-decode decodes them back, which h264_bconf.py and h264_refconf.py
	# drive against ffmpeg for bit-exact conformance sweeps; the fuzz targets feed damaged streams
	# and containers (seed corpus: h264_fuzz_seed.264) and are meant to run under a sanitizer.
	foreach(H264_TOOL test decode cabac-test mp4-test fuzz fuzz-one mp4-fuzz)
		string(REPLACE "-" "_" H264_SRC ${H264_TOOL})
		add_executable(cute-h264-${H264_TOOL} ${CMAKE_CURRENT_SOURCE_DIR}/h264_${H264_SRC}.c)
		target_include_directories(cute-h264-${H264_TOOL} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../libraries)
		target_compile_definitions(cute-h264-${H264_TOOL} PRIVATE _CRT_SECURE_NO_WARNINGS)
		set_target_properties(cute-h264-${H264_TOOL} PROPERTIES FOLDER "tools" RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
		if (UNIX)
			target_link_libraries(cute-h264-${H264_TOOL} PRIVATE m) # sin/cos in the test pattern
		endif()
	endforeach()
endif()

# cute-shaderc, an offline shader compiler
if (CF_CUTE_SHADERC)
	set(CUTE_SHADERC_SRCS
		${CMAKE_CURRENT_SOURCE_DIR}/cute_shaderc.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/cute_shader.cpp)
	# Standalone tools need a ckit implementation TU (CF apps get it from the
	# cute library's src/cute_ckit.cpp).
	list(APPEND CUTE_SHADERC_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/cute_shader_ckit.cpp)
	# ...and cf_alloc/cf_free themselves, so allocations go through the same
	# knob as everywhere else in CF (CF apps get it from the cute library),
	# plus the CF_ASSERT handler cute_alloc.cpp relies on.
	list(APPEND CUTE_SHADERC_SRCS
		${CMAKE_CURRENT_SOURCE_DIR}/../src/cute_alloc.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/cute_shader_assert.cpp)
	add_executable(cute-shaderc ${CUTE_SHADERC_SRCS})
	target_include_directories(cute-shaderc PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../include)
	set_target_properties(cute-shaderc PROPERTIES FOLDER "tools")
	install(TARGETS cute-shaderc
	BUNDLE DESTINATION .
	RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
	ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
endif()

# docsparser, a standalone documentation generator (only depends on ckit.h)
# Only build when CF is the top-level project (not when used as subdirectory)
cmake_path(GET CMAKE_CURRENT_SOURCE_DIR PARENT_PATH CF_ROOT_DIR)
if (CF_ROOT_DIR STREQUAL CMAKE_SOURCE_DIR OR CF_BUILD_DOCSPARSER)
	add_executable(docsparser ${CMAKE_CURRENT_SOURCE_DIR}/docs_parser.c)
	target_include_directories(docsparser PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../libraries)
	if(MSVC)
		target_compile_options(docsparser PRIVATE /std:clatest /experimental:c11atomics)
	endif()
	set_target_properties(docsparser PROPERTIES
		FOLDER "tools"
		RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
	)
endif()
