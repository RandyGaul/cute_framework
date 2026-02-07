if (CF_RUNTIME_SHADER_COMPILATION OR CF_CUTE_SHADERC)
	# cute-shader, a glslang wrapper library
	set(CUTE_SHADER_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/cute_shader.cpp)
	add_library(cute-shader STATIC ${CUTE_SHADER_SRCS})
	target_link_libraries(cute-shader PRIVATE glslang glslang-default-resource-limits SPIRV spirv-cross-c)
	install(TARGETS cute-shader
	BUNDLE DESTINATION .
	RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
	ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
	if (NOT MSVC AND NOT CF_FRAMEWORK_STATIC)
		set_target_properties(cute-shader PROPERTIES POSITION_INDEPENDENT_CODE TRUE)
	endif()
	target_include_directories(cute-shader PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../include)

	# Linking cute-shader for online shader compilation (optional).
	if (CF_RUNTIME_SHADER_COMPILATION)
		target_compile_definitions(cute PUBLIC CF_RUNTIME_SHADER_COMPILATION)
		set(CF_LINK_LIBS ${CF_LINK_LIBS} cute-shader PARENT_SCOPE)
	endif()

	# cute-shaderc, an offline shader compiler
	if (CF_CUTE_SHADERC)
		set(CUTE_SHADERC_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/cute_shaderc.cpp)
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
