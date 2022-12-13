#include <cute.h>
using namespace Cute;

#include <imgui/imgui.h>
#include <sokol/sokol_gfx_imgui.h>

int main(int argc, const char** argv)
{
	int options = APP_OPTIONS_DEFAULT_GFX_CONTEXT | APP_OPTIONS_WINDOW_POS_CENTERED | APP_OPTIONS_RESIZABLE;
	Result result = make_app("Development Scratch", 0, 0, 640, 480, options, argv[0]);
	if (is_error(result)) return -1;

	auto imgui = cf_app_init_imgui(false);
	sg_imgui_t* sg_imgui = app_get_sokol_imgui();

	float w = 640/1;
	float h = 480/1;
	camera_dimensions(w, h);

	make_font("test_data/ProggyClean.ttf", "ProggyClean");
	font_add_codepoints("ProggyClean", ascii_latin());
	font_build("ProggyClean", 13);
	font_build("ProggyClean", 26);
	font_build("ProggyClean", 13 * 5);
	font_build("ProggyClean", 13 * 10);

	while (app_is_running()) {
		float dt = calc_dt();
		app_update(dt);

		cf_draw_push_font_size(13);
		cf_draw_text("ProggyClean", "The quick brown fox jumps over the lazy dog. 1234567890", V2(-100,0));
		cf_draw_pop_font_size();

		cf_draw_push_font_size(26);
		cf_draw_text("ProggyClean", "Some bigger text.", V2(-100,-30));
		cf_draw_pop_font_size();

		cf_draw_push_font_size(13 * 5);
		cf_draw_text("ProggyClean", "huge text okkkk", V2(-150,-90));
		cf_draw_pop_font_size();

		cf_draw_push_font_size(13 * 10);
		cf_draw_text("ProggyClean", "MASSIVE", V2(-150,-200));
		cf_draw_pop_font_size();

		if (ImGui::BeginMainMenuBar()) {
			if (ImGui::BeginMenu("sokol-gfx")) {
				ImGui::MenuItem("Buffers", 0, &sg_imgui->buffers.open);
				ImGui::MenuItem("Images", 0, &sg_imgui->images.open);
				ImGui::MenuItem("Shaders", 0, &sg_imgui->shaders.open);
				ImGui::MenuItem("Pipelines", 0, &sg_imgui->pipelines.open);
				ImGui::MenuItem("Passes", 0, &sg_imgui->passes.open);
				ImGui::MenuItem("Calls", 0, &sg_imgui->capture.open);
				ImGui::EndMenu();
			}
			ImGui::EndMainMenuBar();
		}

		app_present();
	}

	destroy_app();

	return 0;
}
