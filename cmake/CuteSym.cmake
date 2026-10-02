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

function(cf_symbols TARGET)
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
