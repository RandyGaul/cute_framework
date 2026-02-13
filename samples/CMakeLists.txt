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

# Helper function to add shader compilation custom commands
# Usage: add_shader(SHADER_SOURCE TYPE VARNAME [OUTPUT_HEADER])
#   SHADER_SOURCE: Path to .shd file relative to samples dir (e.g., "metaballs.shd" or "waves_data/waves.shd")
#   TYPE: Shader type (draw, vertex, fragment, compute)
#   VARNAME: Variable name for the bytecode struct
#   OUTPUT_HEADER: Optional output header path; defaults to SHADER_SOURCE with .shd replaced by _shd.h
function(add_shader SHADER_SOURCE TYPE VARNAME)
	# Determine output header path
	if (ARGC GREATER 3)
		set(OUTPUT_HEADER ${ARGV3})
	else()
		string(REGEX REPLACE "\\.shd$" "_shd.h" OUTPUT_HEADER "${SHADER_SOURCE}")
	endif()

	add_custom_command(
		OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/${OUTPUT_HEADER}
		COMMAND cute-shaderc
		-type=${TYPE}
		-varname=${VARNAME}
		-oheader=${CMAKE_CURRENT_SOURCE_DIR}/${OUTPUT_HEADER}
		${CMAKE_CURRENT_SOURCE_DIR}/${SHADER_SOURCE}
		DEPENDS ${SHADER_SOURCE}
		DEPENDS cute-shaderc
	)
endfunction()

# Helper function to copy .shd files to build output for runtime shader compilation
function(add_sample_shader TARGET_NAME SHADER_FILE)
	add_custom_command(TARGET ${TARGET_NAME} PRE_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_if_different
			${CMAKE_CURRENT_SOURCE_DIR}/${SHADER_FILE}
			$<TARGET_FILE_DIR:${TARGET_NAME}>/${SHADER_FILE}
	)
	if (EMSCRIPTEN)
		target_link_options(${TARGET_NAME} PRIVATE
			"SHELL:--preload-file ${CMAKE_CURRENT_SOURCE_DIR}/${SHADER_FILE}@/${SHADER_FILE}")
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
# Disabled on Windows -- MSVC crashes for internal reasons when compiling these.
if(NOT WIN32)
	add_sample(clay clay.c)
	add_sample(clay_ui_animations clay_ui_animations.c)
endif()
add_sample(9_slice 9_slice.c)
add_sample(basic_camera basic_camera.c)
add_sample(imgui_texture imgui_texture.c)
add_sample(input_binding input_binding.c)
if (NOT EMSCRIPTEN)
	add_sample(compute compute.c)
	add_sample(galaxy galaxy.c)
	add_sample(hrc hrc.c)
endif()

# Data directories are now copied wholesale for affected samples (see below)

# Pre-compile shaders for certain samples.
if (CF_CUTE_SHADERC)
	# Draw shaders
	add_shader(spaceshooter_data/flash.shd draw s_flash_shd_bytecode)
	add_shader(waves_data/waves.shd draw s_waves_shd_bytecode)
	add_shader(metaballs_data/metaballs.shd draw metaballs metaballs_data/metaballs_shd.h)
	add_shader(shallow_water_data/shallow_water.shd draw s_shallow_water_shd_bytecode)
	add_shader(recolor_data/recolor.shd draw s_recolor_shd_bytecode recolor_data/recolor_shd.h)
	add_shader(fluid_sim_data/fluid_sim.shd draw s_fluid_sim_shd_bytecode fluid_sim_data/fluid_sim_shd.h)
	add_shader(screen_shatter_data/screen_shatter.shd draw s_screen_shatter_shd_bytecode screen_shatter_data/screen_shatter_shd.h)
	add_shader(sprite_shatter_data/sprite_shatter.shd draw s_sprite_shatter_shd_bytecode sprite_shatter_data/sprite_shatter_shd.h)
	add_shader(sprite_slice_data/sprite_slice.shd draw s_sprite_slice_shd_bytecode sprite_slice_data/sprite_slice_shd.h)

	# Low-level shaders (vertex/fragment)
	add_shader(hello_triangle_data/hello_triangle_vs.shd vertex s_hello_triangle_vs_bytecode hello_triangle_data/hello_triangle_vs_shd.h)
	add_shader(hello_triangle_data/hello_triangle_fs.shd fragment s_hello_triangle_fs_bytecode hello_triangle_data/hello_triangle_fs_shd.h)
	add_shader(basic_instancing_data/basic_instancing_vs.shd vertex s_basic_instancing_vs_bytecode basic_instancing_data/basic_instancing_vs_shd.h)
	add_shader(basic_instancing_data/basic_instancing_fs.shd fragment s_basic_instancing_fs_bytecode basic_instancing_data/basic_instancing_fs_shd.h)
	add_shader(basic_indexed_rendering_data/basic_indexed_rendering_vs.shd vertex s_basic_indexed_rendering_vs_bytecode basic_indexed_rendering_data/basic_indexed_rendering_vs_shd.h)
	add_shader(basic_indexed_rendering_data/basic_indexed_rendering_fs.shd fragment s_basic_indexed_rendering_fs_bytecode basic_indexed_rendering_data/basic_indexed_rendering_fs_shd.h)
endif()

# Ensure that sample data is included in web build
if (EMSCRIPTEN)
	target_link_options(waves PRIVATE --preload-file "${CMAKE_CURRENT_SOURCE_DIR}/waves_data@/waves_data")
	target_link_options(spaceshooter PRIVATE --preload-file "${CMAKE_CURRENT_SOURCE_DIR}/spaceshooter_data@/spaceshooter_data")
	target_link_options(shallow_water PRIVATE --preload-file "${CMAKE_CURRENT_SOURCE_DIR}/shallow_water_data@/shallow_water_data")
	target_link_options(import_spritesheet PRIVATE --preload-file "${CMAKE_CURRENT_SOURCE_DIR}/import_spritesheet_data@/import_spritesheet_data")
	target_link_options(pivot PRIVATE --preload-file "${CMAKE_CURRENT_SOURCE_DIR}/pivot_data@/pivot_data")
	target_link_options(9_slice PRIVATE --preload-file "${CMAKE_CURRENT_SOURCE_DIR}/9_slice_data@/9_slice_data")
	target_link_options(scratch PRIVATE --preload-file "${CMAKE_CURRENT_SOURCE_DIR}/../assets/CF_Logo_Pixel.png@/app_icon.png")
	target_link_options(recolor PRIVATE --preload-file "${CMAKE_CURRENT_SOURCE_DIR}/recolor_data@/recolor_data")
	target_link_options(fluid_sim PRIVATE --preload-file "${CMAKE_CURRENT_SOURCE_DIR}/fluid_sim_data@/fluid_sim_data")
	target_link_options(metaballs PRIVATE --preload-file "${CMAKE_CURRENT_SOURCE_DIR}/metaballs_data@/metaballs_data")
	target_link_options(screen_shatter PRIVATE --preload-file "${CMAKE_CURRENT_SOURCE_DIR}/screen_shatter_data@/screen_shatter_data")
	target_link_options(sprite_shatter PRIVATE --preload-file "${CMAKE_CURRENT_SOURCE_DIR}/sprite_shatter_data@/sprite_shatter_data")
	target_link_options(sprite_slice PRIVATE --preload-file "${CMAKE_CURRENT_SOURCE_DIR}/sprite_slice_data@/sprite_slice_data")
	target_link_options(hello_triangle PRIVATE --preload-file "${CMAKE_CURRENT_SOURCE_DIR}/hello_triangle_data@/hello_triangle_data")
	target_link_options(basicinstancing PRIVATE --preload-file "${CMAKE_CURRENT_SOURCE_DIR}/basic_instancing_data@/basic_instancing_data")
	target_link_options(basicindexedrendering PRIVATE --preload-file "${CMAKE_CURRENT_SOURCE_DIR}/basic_indexed_rendering_data@/basic_indexed_rendering_data")
endif ()

# Copy over some folders for certain samples.
add_custom_command(TARGET spaceshooter PRE_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/spaceshooter_data $<TARGET_FILE_DIR:spaceshooter>/spaceshooter_data)
add_custom_command(TARGET waves PRE_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/waves_data $<TARGET_FILE_DIR:waves>/waves_data)
add_custom_command(TARGET shallow_water PRE_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/shallow_water_data $<TARGET_FILE_DIR:shallow_water>/shallow_water_data)
add_custom_command(TARGET import_spritesheet PRE_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/import_spritesheet_data $<TARGET_FILE_DIR:import_spritesheet>/import_spritesheet_data)
add_custom_command(TARGET pivot PRE_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/pivot_data $<TARGET_FILE_DIR:pivot>/pivot_data)
add_custom_command(TARGET scratch PRE_BUILD COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/../assets/CF_Logo_Pixel.png $<TARGET_FILE_DIR:scratch>/app_icon.png)
add_custom_command(TARGET 9_slice PRE_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/9_slice_data $<TARGET_FILE_DIR:9_slice>/9_slice_data)
add_custom_command(TARGET recolor PRE_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/recolor_data $<TARGET_FILE_DIR:recolor>/recolor_data)
add_custom_command(TARGET fluid_sim PRE_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/fluid_sim_data $<TARGET_FILE_DIR:fluid_sim>/fluid_sim_data)
add_custom_command(TARGET metaballs PRE_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/metaballs_data $<TARGET_FILE_DIR:metaballs>/metaballs_data)
add_custom_command(TARGET screen_shatter PRE_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/screen_shatter_data $<TARGET_FILE_DIR:screen_shatter>/screen_shatter_data)
add_custom_command(TARGET sprite_shatter PRE_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/sprite_shatter_data $<TARGET_FILE_DIR:sprite_shatter>/sprite_shatter_data)
add_custom_command(TARGET sprite_slice PRE_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/sprite_slice_data $<TARGET_FILE_DIR:sprite_slice>/sprite_slice_data)
add_custom_command(TARGET hello_triangle PRE_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/hello_triangle_data $<TARGET_FILE_DIR:hello_triangle>/hello_triangle_data)
add_custom_command(TARGET basicinstancing PRE_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/basic_instancing_data $<TARGET_FILE_DIR:basicinstancing>/basic_instancing_data)
add_custom_command(TARGET basicindexedrendering PRE_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/basic_indexed_rendering_data $<TARGET_FILE_DIR:basicindexedrendering>/basic_indexed_rendering_data)
if(NOT EMSCRIPTEN)
add_custom_command(TARGET hrc PRE_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/hrc_data $<TARGET_FILE_DIR:hrc>/hrc_data)
endif()
