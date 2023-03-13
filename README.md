<p align="center">
<img src=https://github.com/RandyGaul/cute_framework/blob/master/assets/CF_Banner_Hifi_1280.png>
<br>
<br>
<img alt="GitHub Workflow Status" src="https://img.shields.io/github/actions/workflow/status/randygaul/cute_framework/build.yml">
<img alt="Discord" src="https://img.shields.io/discord/432009046833233930?label=discord">
</p>

[Cute Framework](https://randygaul.github.io/cute_framework/#/) (CF) is the *cutest* framework available for making 2D games in C++. It provides a portable foundational layer for building 2D games in C/C++ without baggage, gnarly dependencies, or cryptic APIs. CF runs almost anywhere, including Windows, MacOS, iOS, Android, Linux, and more!

> **NOTE**: Cute Framework is currently in flux for it's v1.0 release! These notes/docs will be changed as they get rolled out and come online.

## Download and Setup

~~The easiest option is to head over and pickup the [latest pre-built version](https://github.com/randygaul/cute_framework/releases/latest)~~ This link is currently out of date, and awaiting the soon to be v1.0 release (see below for building latest from source). You can link your project against CF as either a static or shared library. Be sure to also visit the [Cute Framwork documentation site](https://randygaul.github.io/cute_framework/#/).

### Building from Source

Another option for those familiar with CMake is to build from source with CMake.

1. Download and install CMake v3.14+ (for easy cross-platform building), and [git](https://git-scm.com/downloads). 
2. Copy CMakeLists.txt ([this one here](https://github.com/RandyGaul/cute_framework_project_template/blob/main/CMakeLists.txt)) into the top-level of your project directory.
3. Find + replace "my_project_name".
4. Make a folder called `src` in the top-level of your project, and place your initial `main.cpp` there.
5. Run CMake on your project folder.

## Example Game Window

> Creating a window and closing it.

```cpp
#include <cute.h>
using namespace Cute;

int main(int argc, char* argv[])
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

The [documentation website](https://randygaul.github.io/cute_framework/#/) is the go-to place for finding all the resources available. Here are some quick-links to get you started:

- [Topics and Tutorials](https://randygaul.github.io/cute_framework/#/topics_and_tutorials)
- [Samples](https://randygaul.github.io/cute_framework/#/samples)
- [API Reference](https://randygaul.github.io/cute_framework/#/api_reference)

If you're stuck and need help then check out the [Discord chat](https://discord.gg/2DFHRmX). Feel free to pop in and ask questions, make suggestions, or have a discussion. General gamedev chatting unrelated to CF is also welcome!

Feel free to open up an [issue right here on GitHub](https://github.com/RandyGaul/cute_framework/issues) to ask any questions. If you'd like to make a pull request I highly recommend opening a GitHub issue first to start a discussion on any changes you would like to make.

<p align="center"><img src=https://github.com/RandyGaul/cute_framework/blob/master/assets/CF_Logo_Pixel_2x.png></p>
