#include <cute.h>
#include <dcimgui.h>

using namespace Cute;

int main(int argc, char* argv[])
{
	int options = CF_APP_OPTIONS_WINDOW_POS_CENTERED_BIT | CF_APP_OPTIONS_GFX_OPENGL_BIT | CF_APP_OPTIONS_GFX_DEBUG_BIT;
	CF_Result result = make_app("Basic Sprite", 0, 0, 0, 640, 480, options, argv[0]);
	if (is_error(result)) return -1;

	cf_app_init_imgui();
	CF_Sprite girl = cf_make_demo_sprite();

	sprite_play(girl, "spin");
	girl.scale = V2(4,4);

	while (app_is_running()) {
		app_update();

		sprite_update(girl);
		sprite_draw(girl);

		// Brief demo.
		static bool hello = true;
		static bool mutate = false;
		static bool big_demo = false;
		if (hello) {
			ImGui_Begin("Hello", &hello, 0);
			ImGui_Text("Formatting some text! Press X to %s\n", mutate ? "mutate." : "MUTATE!!!");
			if (ImGui_Button("Press me!")) {
				printf("Clicked!\n");
			}
			static char buffer[256] = "...";
			ImGui_InputText("string", buffer, sizeof(buffer), 0);
			static float f;
			ImGui_SliderFloat("float", &f, 0, 10);
			if (ImGui_Button("Big Demo")) {
				big_demo = true;
			}
			ImGui_End();

			if (big_demo) {
				ImGui_ShowDemoWindow(&big_demo);
			}
		}

		if (cf_key_just_pressed(CF_KEY_SPACE)) {
			hello = true;
		}

		if (cf_key_just_pressed(CF_KEY_X)) {
			mutate = !mutate;
		}

		app_draw_onto_screen(true);
	}

	destroy_app();

	return 0;
}
