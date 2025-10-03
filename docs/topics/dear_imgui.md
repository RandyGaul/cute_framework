# Dear ImGui

[Dear ImGui](https://github.com/ocornut/imgui) is a free C/C++ library for building tools and debug UIs. CF includes it by default, ready to use.

## Setup Dear ImGui

Call [`cf_app_init_imgui`](../app/cf_app_init_imgui.md) once before your main loop. Then you can draw Dear ImGui windows:

```cpp
static bool hello = true;
static bool mutate = false;
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
}
```

Result:

<p align="center">
<img src=https://github.com/RandyGaul/cute_framework/blob/master/assets/imgui.png?raw=true>
</p>

## Sample Program

You can find [this sample program](https://github.com/RandyGaul/cute_framework/blob/master/samples/imgui.c) to run the example in the previous section.

```cpp
#include <cute.h>
#include <stdio.h>

#include <dcimgui.h>

int main(int argc, char* argv[])
{
	CF_Result result = cf_make_app("Basic Input", 0, 0, 0, 640, 480, CF_APP_OPTIONS_WINDOW_POS_CENTERED, argv[0]);
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

		cf_app_draw_onto_screen();
	}

	cf_destroy_app();

	return 0;
}
```

## Learning Dear ImGui

The [Dear ImGui](https://github.com/ocornut/imgui) GitHub has extensive learning resources. Check out the [Big Demo(https://github.com/ocornut/imgui/blob/master/imgui_demo.cpp) source code] to see practical examples of every feature.

## C vs C++

These examples use the C API (`dcimgui.h` with ImGui prefix). For C++, use `imgui.h` with ImGui:: prefix instead.
C++ is recommended - it provides default parameters that make the API easier to use. Use the C API only if you're working in plain C.

## Making Tools

Dear ImGui lets you build development tools directly in your game: level editors, tile editors, debug inspectors, entity editors, and value tweakers. Use it to visualize and modify your game data while it's running. CF includes Dear ImGui by default.

Dear ImGui has limited visual customization. Don't use it for your actual game UI (menus, HUD, player-facing interfaces) - it's a development tool, not meant to ship in your final product. That said, nothing technically prevents you from using it anyway.
