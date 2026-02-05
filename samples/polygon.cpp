#include <cute.h>
#include <stdio.h>
#include <imgui.h>

using namespace Cute;

int main(int argc, char* argv[])
{
	make_app("Polygon", 0, 0, 0, 1000, 600, CF_APP_OPTIONS_WINDOW_POS_CENTERED_BIT, argv[0]);
	app_init_imgui();

	int index = 0;
	v2 polygon[] = {
		{ 200+-70, 10 },
		{ 200+-80, -10 },
		{ 200+-20, -70 },
		{ 200+20, -70 },
		{ 200+70, -30 },
		{ 200+80, -10 },
		{ 200+20, 50 },
		{ 200+-20, 50 },
	};

	while (app_is_running()) {
		app_update();

		static float chubbiness = 0;
		static CF_Color color = color_white();
		static float aa_scale = 1.0f;
		static bool aa = true;
		ImGui::Begin("Polygon");
		ImGui::SliderFloat("chubbiness", &chubbiness, 0, 50);
		ImGui::ColorPicker4("color", &color.r);
		ImGui::Checkbox("antialias on/off", &aa);
		ImGui::SliderFloat("antialias scale", &aa_scale, 0, 200);
		ImGui::End();

		draw_push_color(color);
		draw_push_shape_aa_scale(aa_scale);
		draw_push_shape_aa(aa);
		draw_polygon_fill(polygon, CF_ARRAY_SIZE(polygon), chubbiness);

		if (!ImGui::GetIO().WantCaptureMouse) {
			if (mouse_just_pressed(CF_MOUSE_BUTTON_LEFT)) {
				v2 p = screen_to_world(V2(mouse_x(), mouse_y()));
				polygon[index] = p;
				index = index + 1 == 8 ? 0 : index + 1;
			}
		}

		app_draw_onto_screen(true);
	}

	destroy_app();

	return 0;
}