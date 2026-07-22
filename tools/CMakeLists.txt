if (TRUE) # Runtime shader compilation is always available; the compiler is tiny.
	# cute-shader: the shader compiler front-end library, built on cute_spirv.h
	# (CF's own GLSL -> SPIR-V compiler, no external dependencies). SPIRV-Cross
	# translates the SPIR-V for whichever backend is active.
	set(CUTE_SHADER_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/cute_shader.cpp)
	add_library(cute-shader STATIC ${CUTE_SHADER_SRCS})
	target_link_libraries(cute-shader PRIVATE spirv-cross-c)
	install(TARGETS cute-shader
	BUNDLE DESTINATION .
	RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
	ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
	if (NOT MSVC AND NOT CF_FRAMEWORK_STATIC)
		set_target_properties(cute-shader PROPERTIES POSITION_INDEPENDENT_CODE TRUE)
	endif()
	target_include_directories(cute-shader PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../include)

	# The CF_RUNTIME_SHADER_COMPILATION macro stays defined for backwards
	# compatibility with user code using the documented #ifdef idiom.
	target_compile_definitions(cute PUBLIC CF_RUNTIME_SHADER_COMPILATION)
	set(CF_LINK_LIBS ${CF_LINK_LIBS} cute-shader PARENT_SCOPE)

	# cute-spirv-tests, the compiler's unit test suite. Runs standalone (no SDL);
	# validates emitted modules with spirv-val when CSPV_SPIRV_VAL points at it.
	if (CMAKE_CURRENT_SOURCE_DIR STREQUAL "${CMAKE_SOURCE_DIR}/tools")
		add_executable(cute-spirv-tests ${CMAKE_CURRENT_SOURCE_DIR}/cute_spirv_test.c)
		set_source_files_properties(${CMAKE_CURRENT_SOURCE_DIR}/cute_spirv_test.c PROPERTIES LANGUAGE CXX)
		target_include_directories(cute-spirv-tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/../libraries ${CMAKE_CURRENT_SOURCE_DIR}/../include)
		target_compile_definitions(cute-spirv-tests PRIVATE _CRT_SECURE_NO_WARNINGS)
		set_target_properties(cute-spirv-tests PROPERTIES FOLDER "tools")
	endif()

	# cute-shaderc, an offline shader compiler
	if (CF_CUTE_SHADERC)
		set(CUTE_SHADERC_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/cute_shaderc.cpp)
		# Standalone tools need a ckit implementation TU (CF apps get it from the
		# cute library's src/cute_ckit.cpp).
		list(APPEND CUTE_SHADERC_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/cute_shader_ckit.cpp)
		add_executable(cute-shaderc ${CUTE_SHADERC_SRCS})
		target_link_libraries(cute-shaderc cute-shader)
		set_target_properties(cute-shaderc PROPERTIES FOLDER "tools")
		install(TARGETS cute-shaderc
		BUNDLE DESTINATION .
		RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
		LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
		ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
	endif()
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
