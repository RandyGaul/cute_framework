<p align="center">
<img src=https://github.com/RandyGaul/cute_framework/blob/master/assets/CF_Banner_Hifi_1280.png>
<br>
<br>
<img alt="GitHub Workflow Status" src="https://img.shields.io/github/actions/workflow/status/randygaul/cute_framework/build.yml">
<img alt="Discord" src="https://img.shields.io/discord/432009046833233930?label=discord">
</p>

[Cute Framework](https://randygaul.github.io/cute_framework/#/) (CF) is the *cutest* framework available for making 2D games in C++. It provides a portable foundational layer for building 2D games in C/C++ without baggage, gnarly dependencies, or cryptic APIs. CF runs almost anywhere, including Windows, MacOS, iOS, Android, Linux, Browsers, and more!

> **NOTE**: Cute Framework is currently in flux for it's v1.0 release! These notes/docs will be changed as they get rolled out and come online.

## Download and Setup

~~The easiest option is to head over and pickup the [latest pre-built version](https://github.com/randygaul/cute_framework/releases/latest)~~ This link is currently out of date, and awaiting the soon to be v1.0 release (see below for building latest from source). You can link your project against CF as either a static or shared library. Be sure to also visit the [Cute Framework documentation site](https://randygaul.github.io/cute_framework/#/).

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

### Building from Source

Another option for those familiar with CMake is to build from source with CMake. Make sure you have a compiler installed that you're familiar with beforehand. If you're new to C/C++ I highly recommend using Microsoft Visual Studio (Community Edition), for Windows users. If you don't like Visual Studio you can try gcc/g++ (MinGW), [tdm-gcc](https://jmeubank.github.io/tdm-gcc/) is recommended in this case. If you're MacOS, XCode (and the command line tools) are recommended. For Linux you'll probably use g++.

It's highly recommended to use our [Cmake project template](https://github.com/RandyGaul/cute_framework_project_template#cmake-101-walkthrough) and follow along these steps with it!

1. Download and install CMake (v3.14 or higher, you can just get the latest version). CMake is for easy cross-platform building. Also install [git](https://git-scm.com/downloads). If you're new to git and a Windows user it's highly recommended to use [Github Desktop](https://desktop.github.com/).
2. Copy CMakeLists.txt ([this one here](https://github.com/RandyGaul/cute_framework_project_template/blob/main/CMakeLists.txt)) into the top-level of your project directory.
3. Find + replace "mygame" to your game name (no underscores, spaces, or special characters allowed).
4. Make a folder called `src` in the top-level of your project, and place your initial `main.cpp` there.
5. Run CMake on your project folder. If you need help with this step, try reading a setup guide for CF here: [CF - CMake 101](https://github.com/RandyGaul/cute_framework_project_template#cmake-101-walkthrough).

> **Note** For Linux users make sure to you have OpenGL, gcc/g++, etc. installed and setup. You can try these commands:
```cpp
sudo apt-get update -qq
sudo apt-get install build-essential gcc-multilib cmake
sudo apt-get install libasound2-dev libpulse-dev 
sudo apt-get install -y --no-install-recommends libglfw3 libglfw3-dev libx11-dev libxcursor-dev libxrandr-dev libxinerama-dev libxi-dev libxext-dev libxfixes-dev
```
> **Note** On WSL2 you may also need `sudo apt install libpulse0`.

> **Note** For non-Apple ARM platforms (like Raspberry/Orange Pi) you may need to define CUTE_SOUND_SCALAR_MODE to disable SSE intrinsics. Ideally CF could use preprocessor directives to define this for you -- pull requests are highly appreciated here!

# Resources

The [documentation website](https://randygaul.github.io/cute_framework/#/) is the go-to place for finding all the resources available. Here are some quick-links to get you started:

- [Topics and Tutorials](https://randygaul.github.io/cute_framework/#/topics/)
- [Samples](https://randygaul.github.io/cute_framework/#/samples)
- [API Reference](https://randygaul.github.io/cute_framework/#/api_reference)

If you're stuck and need help then check out the [Discord chat](https://discord.gg/2DFHRmX). Feel free to pop in and ask questions, make suggestions, or have a discussion. General gamedev chatting unrelated to CF is also welcome!

Feel free to open up an [issue right here on GitHub](https://github.com/RandyGaul/cute_framework/issues) to ask any questions. If you'd like to make a pull request I highly recommend opening a GitHub issue first to start a discussion on any changes you would like to make.

<p align="center"><img src=https://github.com/RandyGaul/cute_framework/blob/master/assets/CF_Logo_Pixel_2x.png></p>
