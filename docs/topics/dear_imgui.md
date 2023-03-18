[](../header.md ':include')

<br>

[Dear ImGui](https://github.com/ocornut/imgui) is a free to use C/C++ library for creating tools or debug user-interfaces (UI). It's an absolutely splendid tool for game development, and comes baked right into CF's source code, ready to use right out of the box.

## Setup Dear ImGui

Before using Dear ImGui you must call [`cf_app_init_imgui`](https://randygaul.github.io/cute_framework/#/app/cf_app_init_imgui). You can call this before your main loop just once. Once done you may draw Dear ImGui debug windows like this one:

```cpp
static bool hello = true;
static bool mutate = false;
if (hello) {
	igBegin("Hello", &hello, 0);
	igText("Formatting some text! Press X to %s\n", mutate ? "mutate." : "MUTATE!!!");
	if (igButton("Press me!", (ImVec2){0,0})) {
		printf("Clicked!\n");
	}
	static char buffer[256] = "...";
	igInputText("string", buffer, sizeof(buffer), 0, NULL, NULL);
	static float f;
	igSliderFloat("float", &f, 0, 10, "%3f", 0);
	if (igButton("Big Demo", (ImVec2){0,0})) {
		big_demo = true;
	}
	igEnd();
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

#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include <cimgui.h>

int main(int argc, char* argv[])
{
	int options = CF_APP_OPTIONS_DEFAULT_GFX_CONTEXT | CF_APP_OPTIONS_WINDOW_POS_CENTERED;
	CF_Result result = cf_make_app("Basic Input", 0, 0, 640, 480, options, argv[0]);
	if (cf_is_error(result)) return -1;

	cf_app_init_imgui(false);

	while (cf_app_is_running())
	{
		cf_app_update(NULL);

		// Brief demo.
		static bool hello = true;
		static bool mutate = false;
		static bool big_demo = false;
		if (hello) {
			igBegin("Hello", &hello, 0);
			igText("Formatting some text! Press X to %s\n", mutate ? "mutate." : "MUTATE!!!");
			if (igButton("Press me!", (ImVec2){0,0})) {
				printf("Clicked!\n");
			}
			static char buffer[256] = "...";
			igInputText("string", buffer, sizeof(buffer), 0, NULL, NULL);
			static float f;
			igSliderFloat("float", &f, 0, 10, "%3f", 0);
			if (igButton("Big Demo", (ImVec2){0,0})) {
				big_demo = true;
			}
			igEnd();

			if (big_demo) {
				igShowDemoWindow(&big_demo);
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

The above examples are shown with the [cimgui](https://github.com/cimgui/cimgui) API, which is a C wrapper around the C++ Dear ImGui library. If instead you want to use C++, include `<imgui.h>` instead of `<cimgui.h>`, and use `ImGui::` instead of the `ig` prefix.

The C++ API is rather preferred since it adds in a lot of default parameters. But, if you're just using plain C then cimgu is here for you.

## Making Tools

Dear ImGui is great for making all kinds of development tools, such as level editors, tile editors, debug inspection UIs, tools for saving or editing entities/values, etc. It's a general purpose tool to visualize and tweak/edit data in your game. We have included Dear ImGui in CF out of the box since it's such an incredibly useful feature.

However, Dear ImGui is not very customizable in terms of its looks. For this reason, it's not recommended to use Dear ImGui for your actual in-game UI (such as your start menu, or other clickable UI elements). It's not really designed that way, to actually ship with your final game product. Of course, there's nothing stopping you from doing so... You've been warned.
