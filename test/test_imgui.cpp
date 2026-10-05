/*
	Cute Framework
	Copyright (C) 2026 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

#include "test_harness.h"
#include "test_app_shared.h"

#include <cute.h>
#include <imgui.h>
#include <internal/cute_graphics_internal.h>

using namespace Cute;

// ImGui starts once per app and stays up until cf_destroy_app, so this runs on a private app.

#define W 320
#define H 240

static int s_error_count()
{
#ifdef CF_WEBGPU
	if (cf_query_backend() == CF_BACKEND_TYPE_WEBGPU) return cf_webgpu_error_count();
#endif
	return 0;
}

// Only the renderer backend moves a texture to OK, by creating or updating it.
static bool s_imgui_textures_made()
{
	ImVector<ImTextureData*>& textures = ImGui::GetPlatformIO().Textures;
	if (textures.Size == 0) return false;
	for (int i = 0; i < textures.Size; ++i) {
		if (textures[i]->Status != ImTextureStatus_OK) return false;
	}
	return true;
}

// A window with text, widgets, and images of a sampled texture, a canvas target, and a fetched
// sprite frame (which delays the atlas defrag until after ImGui renders), presented for
// several frames.
TEST_CASE(test_imgui_frame)
{
	if (!test_make_private_app(W, H)) return true; // Headless CI: no display/GPU.
	TestPrivateAppGuard app_guard;

	REQUIRE(cf_app_init_imgui());
	CF_Pixel pixels[16 * 16];
	for (int i = 0; i < 16 * 16; ++i) pixels[i] = (i & 1) ? cf_pixel_white() : cf_pixel_black();
	CF_Texture tex = cf_make_texture(cf_texture_defaults(16, 16));
	cf_texture_update(tex, pixels, sizeof(pixels));
	CF_Canvas canvas = cf_make_canvas(cf_canvas_defaults(64, 64));
	CF_Sprite sprite = cf_make_demo_sprite();

	int errors_before = s_error_count();
	float slider = 0.5f;
	bool check = true;
	for (int frame = 0; frame < 4; ++frame) {
		cf_app_update(NULL);
		// Hidden test apps are minimized, which ImGui reads as a zero-size display and skips
		// rendering. Restart the frame at the window's real size.
		ImGui::EndFrame();
		ImGui::GetIO().DisplaySize = ImVec2(W, H);
		ImGui::NewFrame();

		cf_draw_box(cf_make_aabb(cf_v2(-10, -10), cf_v2(10, 10)), 1, 0);
		cf_render_to(canvas, true);
		cf_sprite_update(&sprite);
		cf_draw_sprite(&sprite);

		ImGui::SetNextWindowPos(ImVec2(8, 8));
		ImGui::SetNextWindowSize(ImVec2(240, 200));
		ImGui::Begin("Test");
		ImGui::Text("Same glyphs every frame"); // New glyphs would leave the font atlas mid-update.
		ImGui::Button("Button");
		ImGui::SliderFloat("Slider", &slider, 0, 1);
		ImGui::Checkbox("Check", &check);
		ImGui::Image((ImTextureID)cf_texture_handle(tex), ImVec2(32, 32));
		ImGui::SameLine();
		ImGui::Image((ImTextureID)cf_texture_handle(cf_canvas_get_target(canvas)), ImVec2(32, 32));
		ImGui::SameLine();
		CF_TemporaryImage image = cf_fetch_image(&sprite);
		ImGui::Image((ImTextureID)cf_texture_handle(image.tex), ImVec2((float)image.w, (float)image.h), ImVec2(image.u.x, image.v.y), ImVec2(image.v.x, image.u.y));
		ImGui::End();

		cf_app_draw_onto_screen(true);
	}
	cf_gpu_sync();

	REQUIRE(ImGui::GetDrawData()->TotalVtxCount > 0);
	REQUIRE(s_imgui_textures_made());
	REQUIRE(s_error_count() == errors_before);

	cf_destroy_canvas(canvas);
	cf_destroy_texture(tex);
	return true;
}

TEST_SUITE(test_imgui)
{
	RUN_TEST_CASE(test_imgui_frame);
}
