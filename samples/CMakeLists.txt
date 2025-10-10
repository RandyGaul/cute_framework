if (EMSCRIPTEN)
	add_custom_target(copy-emscripten-shell
		COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different ${CMAKE_CURRENT_SOURCE_DIR}/emscripten ${CMAKE_BINARY_DIR}/emscripten
	)
	add_library(cute-emscripten-shell INTERFACE)
	add_dependencies(cute-emscripten-shell copy-emscripten-shell)
	target_link_options(cute-emscripten-shell INTERFACE --shell-file "${CMAKE_CURRENT_SOURCE_DIR}/emscripten/shell.html")
endif ()

function(add_sample TARGET_NAME SOURCE_FILE)
	add_executable(${TARGET_NAME} ${SOURCE_FILE})
	target_link_libraries(${TARGET_NAME} PRIVATE cute)
	if (APPLE)
		set_target_properties(${TARGET_NAME} PROPERTIES
			MACOSX_BUNDLE_GUI_IDENTIFIER "com.cuteframework.${TARGET_NAME}"
			MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
			MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
		)
	endif()
	set_target_properties(${TARGET_NAME} PROPERTIES
		FOLDER "samples"
		RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
	)
	if (WINDOWS)
		target_dxc_copy(${TARGET_NAME})
	endif()
	if (EMSCRIPTEN)
		set_target_properties(${TARGET_NAME} PROPERTIES OUTPUT_NAME "${TARGET_NAME}" SUFFIX ".html")
		target_compile_options(${TARGET_NAME} PRIVATE -O1 -fno-rtti -fno-exceptions -gsplit-dwarf)
		target_link_options(${TARGET_NAME} PRIVATE -o ${TARGET_NAME}.html -sASYNCIFY=1 -O1 -gseparate-dwarf)
		target_link_libraries(${TARGET_NAME} PRIVATE cute-emscripten-shell)
	endif()
endfunction()

add_sample(easysprite easy_sprite.c)
add_sample(basicserialization basic_serialization.c)
add_sample(imgui imgui.c)
add_sample(textdrawing text_drawing.cpp)
add_sample(basicsprite basic_sprite.cpp)
add_sample(basicshapes basic_shapes.cpp)
add_sample(windowresizing window_resizing.cpp)
add_sample(basicinput basic_input.c)
add_sample(windowevents window_events.c)
add_sample(window window.cpp)
add_sample(docsparser docs_parser.cpp)
add_sample(scratch scratch.cpp)
if (NOT EMSCRIPTEN)
	add_sample(https https.c)
endif()
add_sample(spaceshooter spaceshooter.cpp)
add_sample(draw_to_texture draw_to_texture.c)
add_sample(hello_triangle hello_triangle.c)
add_sample(basicinstancing basic_instancing.c)
add_sample(basicindexedrendering basic_indexed_rendering.c)
add_sample(waves waves.cpp)
add_sample(shallow_water shallow_water.cpp)
add_sample(noise noise.c)
add_sample(fetch_image fetch_image.cpp)
add_sample(metaballs metaballs.cpp)
add_sample(timestep timestep.cpp)
add_sample(joypad joypad.c)
add_sample(outline_stencil outline_stencil.cpp)
add_sample(recolor recolor.cpp)
add_sample(rainbow_liquid rainbow_liquid.cpp)
add_sample(import_spritesheet import_spritesheet.cpp)
add_sample(stencil_pie_chart stencil_pie_chart.c)
add_sample(platformer platformer.cpp)
add_sample(polygon polygon.cpp)
add_sample(scissor scissor.c)
add_sample(ime ime.c)
add_sample(pivot pivot.cpp)
add_sample(fluid_sim fluid_sim.cpp)
add_sample(sprite_slice sprite_slice.cpp)
add_sample(sprite_shatter sprite_shatter.cpp)
add_sample(screen_shatter screen_shatter.cpp)
add_sample(font_debug font_debug.cpp)
add_sample(clay clay.c)
add_sample(9_slice 9_slice.c)

# Pre-compile shaders for certain samples.
if (CF_CUTE_SHADERC)
	add_custom_command(
		OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/spaceshooter_data/flash_shd.h
		COMMAND cute-shaderc
		-type=draw
		-varname=s_flash_shd_bytecode
		-oheader=${CMAKE_CURRENT_SOURCE_DIR}/spaceshooter_data/flash_shd.h
		${CMAKE_CURRENT_SOURCE_DIR}/spaceshooter_data/flash.shd
		DEPENDS spaceshooter_data/flash.shd
		DEPENDS cute-shaderc
	)
	add_custom_command(
		OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/waves_data/waves_shd.h
		COMMAND cute-shaderc
		-type=draw
		-varname=s_waves_shd_bytecode
		-oheader=${CMAKE_CURRENT_SOURCE_DIR}/waves_data/waves_shd.h
		${CMAKE_CURRENT_SOURCE_DIR}/waves_data/waves.shd
		DEPENDS waves_data/waves.shd
		DEPENDS cute-shaderc
	)
endif()

# Ensure that sample data is included in web build
if (EMSCRIPTEN)
	target_link_options(waves PRIVATE --preload-file "${CMAKE_CURRENT_SOURCE_DIR}/waves_data@/waves_data")
	target_link_options(spaceshooter PRIVATE --preload-file "${CMAKE_CURRENT_SOURCE_DIR}/spaceshooter_data@/spaceshooter_data")
endif ()

# Copy over some folders for certain samples.
add_custom_command(TARGET spaceshooter PRE_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/spaceshooter_data $<TARGET_FILE_DIR:spaceshooter>/spaceshooter_data)
add_custom_command(TARGET waves PRE_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/waves_data $<TARGET_FILE_DIR:waves>/waves_data)
add_custom_command(TARGET shallow_water PRE_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/shallow_water_data $<TARGET_FILE_DIR:shallow_water>/shallow_water_data)
add_custom_command(TARGET import_spritesheet PRE_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/import_spritesheet_data $<TARGET_FILE_DIR:import_spritesheet>/import_spritesheet_data)
add_custom_command(TARGET pivot PRE_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/pivot_data $<TARGET_FILE_DIR:pivot>/pivot_data)
add_custom_command(TARGET scratch PRE_BUILD COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/../assets/CF_Logo_Pixel.png $<TARGET_FILE_DIR:scratch>/app_icon.png)
add_custom_command(TARGET 9_slice PRE_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/9_slice_data $<TARGET_FILE_DIR:9_slice>/9_slice_data)
