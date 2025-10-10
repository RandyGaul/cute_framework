#include <cute.h>
#include <stdio.h>

#include <dcimgui.h>

int main(int argc, char* argv[])
{
	CF_Result result = cf_make_app("Dear ImGui", 0, 0, 0, 640, 480, CF_APP_OPTIONS_WINDOW_POS_CENTERED_BIT, argv[0]);
	if (cf_is_error(result)) return -1;

	cf_app_init_imgui();

	while (cf_app_is_running())
	{
		cf_app_update(NULL);

		// Brief demo.
		static bool hello = true;
		static bool mutate = false;
		static bool big_demo = false;
		if (hello) {
			ImGui_Begin("Hello", &hello, 0);
			ImGui_Text("Formatting some text! Press X to %s\n", mutate ? "mutate." : "MUTATE!!!");
			if (ImGui_ButtonEx("Press me!", (ImVec2){0,0})) {
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

		cf_app_draw_onto_screen(true);
	}

	cf_destroy_app();

	return 0;
}
