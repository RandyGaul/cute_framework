#include <cute.h>
using namespace Cute;

#include <imgui.h>

#ifndef CF_RUNTIME_SHADER_COMPILATION
#include "recolor_data/recolor_shd.h"
#endif

int main(int argc, char* argv[])
{
	CF_Result result = make_app("Recolor", 0, 0, 0, 720, 480, CF_APP_OPTIONS_WINDOW_POS_CENTERED_BIT, argv[0]);
	if (is_error(result)) return -1;
#ifdef CF_RUNTIME_SHADER_COMPILATION
	cf_shader_directory("/recolor_data");
	CF_Shader recolor = make_draw_shader("recolor.shd");
#else
	CF_Shader recolor = cf_make_draw_shader_from_bytecode(s_recolor_bytecode);
#endif
	app_init_imgui();

	CF_Sprite girl = cf_make_demo_sprite();
	sprite_play(girl, "spin");
	girl.scale = V2(4,4);

	while (app_is_running()) {
		app_update();

		static CF_Color color = color_red();
		static float strength = 0.5f;
		ImGui::Begin("Color Picker", NULL, ImGuiWindowFlags_AlwaysAutoResize);
		ImGui::ColorPicker3("Color", (float*)&color);
		ImGui::DragFloat("Strength", &strength, 0.01f, 0, 1);
		ImGui::End();

		draw_push_shader(recolor);
		draw_push_vertex_attributes(color.r, color.g, color.b, strength);
		sprite_update(girl);
		sprite_draw(girl);

		app_draw_onto_screen(true);
	}

	destroy_app();

	return 0;
}
