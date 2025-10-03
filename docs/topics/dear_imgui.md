# Dear ImGui

[Dear ImGui](https://github.com/ocornut/imgui) is a free to use C/C++ library for creating tools or debug user-interfaces (UI). It's an absolutely splendid tool for game development, and comes baked right into CF's source code, ready to use right out of the box.

## Setup Dear ImGui

Before using Dear ImGui you must call [`cf_app_init_imgui`](../app/cf_app_init_imgui.md). You can call this before your main loop just once. Once done you may draw Dear ImGui debug windows like this one:

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

Which produces this window:

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

The [Dear ImGui](https://github.com/ocornut/imgui) page has tons of information about learning and getting started. The source code for the "Big Demo" window in the previous section can be found [here](https://github.com/ocornut/imgui/blob/master/imgui_demo.cpp). This is a great way to see examples of how to use Dear ImGui.

## C vs C++

The above examples are shown with the [dear_bindings](https://github.com/dearimgui/dear_bindings) API, which is a C wrapper around the C++ Dear ImGui library. If instead you want to use C++, include `<imgui.h>` instead of `<dcimgui.h>`, and use `ImGui::` instead of the `ImGui` prefix.

The C++ API is rather preferred since it adds in a lot of default parameters. But, if you're just using plain C then `<dcimgui.h>` is here for you.

## Making Tools

Dear ImGui lets you build development tools directly in your game: level editors, tile editors, debug inspectors, entity editors, and value tweakers. Use it to visualize and modify your game data while it's running. CF includes Dear ImGui by default.

Dear ImGui has limited visual customization. Don't use it for your actual game UI (menus, HUD, player-facing interfaces) - it's a development tool, not meant to ship in your final product. That said, nothing technically prevents you from using it anyway.
