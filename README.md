<p align="center">
<img src=https://github.com/RandyGaul/cute_framework/blob/master/logo.png>
<br>
<img alt="GitHub Workflow Status" src="https://img.shields.io/github/actions/workflow/status/randygaul/cute_framework/build.yml">
<img alt="Discord" src="https://img.shields.io/discord/432009046833233930?label=discord">
</p>

Cute Framework (CF for short) is the *cutest* framework available for making 2D games in C++. CF provides a portable foundational layer for building 2D games in C/C++ without baggage, gnarly dependencies, or cryptic APIs. CF is easy to build and great for getting projects off the ground. Written with a portable pure C API CF runs almost anywhere, including Windows, MacOS, iOS, Android, Linux, and more!

# Getting Started

1. Download and install CMake v3.14+ (for easy cross-platform building)
2. Copy + paste [CMakeLists.txt](https://github.com/RandyGaul/cute_framework_project_template/blob/main/CMakeLists.txt) from the [CF project template](https://github.com/RandyGaul/cute_framework_project_template). Find + replace "my_project_name".
3. Run CMake on your project folder. Cmake will automagically download and hook up CF.
4. Use the below code snippet for your first `main.cpp`!

> Creating a window and closing it.

```cpp
#include <cute.h>
using namespace Cute;

int main(int argc, const char** argv)
{
	// Create a window with a resolution of 640 x 480.
	int options = APP_OPTIONS_DEFAULT_GFX_CONTEXT | APP_OPTIONS_WINDOW_POS_CENTERED;
	Result result = make_app("Fancy Window Title", 0, 0, 640, 480, options, argv[0]);
	if (is_error(result)) return -1;

	while (app_is_running())
	{
		app_update();
		// All your game logic and updates go here...
		app_draw_onto_screen();
	}

	destroy_app();

	return 0;
}
```

# Resources

## Topics

* link
* link
* link
* link

## API Reference

* link
* link
* link

## Samples

- [Cute Snake, example game implemented in CF](https://github.com/RandyGaul/cute_snake)

## Tutorials

## Community / Ask for Help

Here's a [link to the discord chat](https://discord.gg/2DFHRmX) for Cute Framework and the [Cute Headers](https://github.com/RandyGaul/cute_headers). Feel free to pop in and ask questions, make suggestions, or have a discussion.

Feel free to open up an [issue right here on GitHub](https://github.com/RandyGaul/cute_framework/issues) to ask any questions. If you'd like to make a pull request I highly recommend opening a GitHub issue first to start a discussion on any changes you would like to make.
