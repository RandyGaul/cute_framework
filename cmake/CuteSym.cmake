# cf_symbols(<target> [EMBED | FILE] [RESERVE <bytes>])
#
# Gives a target's crash reports names and line numbers (include/cute_crash.h). The precondition is
# debug info, which this function turns on for the target (a PDB on MSVC, -g elsewhere; nothing
# about the shipped code changes). After every link cute-sym reads that debug info into one table:
#
#   EMBED  the table is patched into a slot reserved inside the binary itself (RESERVE bytes,
#          default 8 MB: a Debug build of a large program needs that, a Release build a fraction),
#          so nothing ships beside it. A table larger than the slot fails the build with both
#          sizes in the message. Must run before code signing, which a post-build step does.
#   FILE   the table is written beside the binary as <file name>.sym.
#
# Neither is required: without it reports carry raw module-relative addresses and the developer
# resolves them later with `cute-sym resolve` and tables kept from the build.
set(CF_SYM_SLOT_IN "${CMAKE_CURRENT_LIST_DIR}/cute_crash_slot.c.in")

# For CF's own CMakeLists, where the cute target is made: a SHARED cute library gets a table beside it
# after every link, so a frame inside CF resolves in shared builds too. The table only exists when the
# library was built with debug info (cf_symbols on any target turns that on); otherwise the step is a
# no-op, not an error.
function(cf_symbols_cute)
	if (NOT TARGET cute-sym OR EMSCRIPTEN)
		return()
	endif()
	get_target_property(CUTE_TYPE cute TYPE)
	if (NOT CUTE_TYPE STREQUAL "SHARED_LIBRARY")
		return()
	endif()
	add_dependencies(cute cute-sym)
	add_custom_command(TARGET cute POST_BUILD
		COMMAND cute-sym "$<TARGET_FILE:cute>" -o "$<TARGET_FILE:cute>.sym" --optional
		COMMENT "cute-sym: the cute library's symbol table, when it has debug info"
		VERBATIM)
endfunction()

function(cf_symbols TARGET)
	if (EMSCRIPTEN)
		return() # No native binary to read and no reporter on the web: nothing to do.
	endif()
	cmake_parse_arguments(ARG "EMBED;FILE" "RESERVE" "" ${ARGN})
	if (NOT TARGET cute-sym)
		message(FATAL_ERROR "cf_symbols(${TARGET}): the cute-sym target is missing; set CF_CUTE_SYM ON.")
	endif()
	if (NOT ARG_RESERVE)
		set(ARG_RESERVE 8388608)
	endif()
	if (NOT ARG_EMBED AND NOT ARG_FILE)
		set(ARG_EMBED ON)
	endif()

	# Debug info for the target AND for the framework it links statically: a frame inside CF needs
	# its line too. Nothing about the generated code changes.
	foreach(T ${TARGET} cute)
		if (NOT TARGET ${T})
			continue()
		endif()
		if (MSVC)
			set_property(TARGET ${T} PROPERTY MSVC_DEBUG_INFORMATION_FORMAT "ProgramDatabase")
		else()
			target_compile_options(${T} PRIVATE -g)
		endif()
	endforeach()
	if (MSVC)
		target_link_options(${TARGET} PRIVATE /DEBUG:FULL)
	endif()
	if (TARGET cute AND MSVC)
		target_link_options(cute PRIVATE /DEBUG:FULL)
	endif()
	add_dependencies(${TARGET} cute-sym)

	if (ARG_EMBED)
		set(CF_SYM_RESERVE ${ARG_RESERVE})
		set(SLOT_SRC "${CMAKE_CURRENT_BINARY_DIR}/${TARGET}_cute_crash_slot.c")
		configure_file("${CF_SYM_SLOT_IN}" "${SLOT_SRC}" @ONLY)
		target_sources(${TARGET} PRIVATE "${SLOT_SRC}")
		add_custom_command(TARGET ${TARGET} POST_BUILD
			COMMAND cute-sym "$<TARGET_FILE:${TARGET}>" --embed
			COMMENT "cute-sym: embedding the symbol table in ${TARGET}"
			VERBATIM)
	else()
		add_custom_command(TARGET ${TARGET} POST_BUILD
			COMMAND cute-sym "$<TARGET_FILE:${TARGET}>" -o "$<TARGET_FILE:${TARGET}>.sym"
			COMMENT "cute-sym: writing the symbol table beside ${TARGET}"
			VERBATIM)
	endif()
endfunction()
