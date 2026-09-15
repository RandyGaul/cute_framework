/*
	Cute Framework
	Copyright (C) 2026 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

#ifdef __cplusplus
#error "This code must be compiled as C, not C++"
#endif

// Dear ImGui's C bindings must work from a plain C translation unit, internal header included
// (DockBuilder, window lookups), with no extra defines or include-order tricks.
#include <dcimgui.h>
#include <dcimgui_internal.h>

#include "test_harness.h"

TEST_CASE(test_dcimgui_internal_header_in_c)
{
	// ImFontAtlasBuilder holds an ImVector of stb rect-pack nodes. It is only a complete type
	// when dcimgui_internal.h resolved stbrp_node_im in C.
	REQUIRE(sizeof(ImFontAtlasBuilder) > 0);
	ImGuiWindow* (*find_window)(const char*) = ImGui_FindWindowByName;
	ImGuiDockNode* (*get_node)(ImGuiID) = ImGui_DockBuilderGetNode;
	REQUIRE(find_window != NULL);
	REQUIRE(get_node != NULL);
	return true;
}

TEST_SUITE(test_dcimgui_c)
{
	RUN_TEST_CASE(test_dcimgui_internal_header_in_c);
}
