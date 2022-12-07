#include <cute.h>
#include <imgui/imgui.h>
#include <sokol/sokol_gfx_imgui.h>
using namespace Cute;

int main(int argc, const char** argv)
{
	int options = APP_OPTIONS_DEFAULT_GFX_CONTEXT | APP_OPTIONS_WINDOW_POS_CENTERED | APP_OPTIONS_RESIZABLE;
	Result result = make_app("Fancy Window Title", 0, 0, 640, 480, options, argv[0]);
	if (is_error(result)) return -1;

	Sprite s = cf_make_sprite("test_data/girl.aseprite");
	s.play("spin");

	auto imgui = cf_app_init_imgui(false);
	sg_imgui_t* sg_imgui = app_get_sokol_imgui();
	float w = 640/2;
	float h = 480/2;
	camera_dimensions(w, h);
	float t = 0;

	draw_push_antialias(true);

	while (app_is_running()) {
		float dt = calc_dt();
		t += dt;
		app_update(dt);
		float opacity = (sinf(t)+1)*0.5f;
		s.opacity = opacity;
		s.update(dt);
		s.draw();
		camera_look_at(w/4, 0);
		camera_rotate(t);
		draw_push_layer(-1);
		CF_Color c = color_purple();
		c.a = opacity;
		draw_push_color(c);
		draw_line(V2(0,0), V2(100,100), 0);
		draw_pop_color();
		draw_pop_layer();
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
