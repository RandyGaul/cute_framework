<p align="center">
<img src=https://github.com/RandyGaul/cute_framework/blob/master/assets/CF_Banner_Hifi_1280.png>
<br>
<br>
<img alt="GitHub Workflow Status" src="https://img.shields.io/github/actions/workflow/status/randygaul/cute_framework/build.yml">
<img alt="Discord" src="https://img.shields.io/discord/432009046833233930?label=discord">
</p>

[Cute Framework](https://randygaul.github.io/cute_framework/#/) (CF) is the *cutest* framework available for making 2D games in C++. It provides a portable foundational layer for building 2D games in C/C++ without baggage, gnarly dependencies, or cryptic APIs. CF runs almost anywhere, including Windows, MacOS, iOS, Android, Linux, Browsers, and more!

## Download and Setup

Cute Frameowrk is designed to be built from source. You can link your project against CF as either a static or shared library. Be sure to visit the [Cute Framework documentation site](https://randygaul.github.io/cute_framework/#/) for more in-depth details and steps.

## Example Game Window

> Creating a window and closing it.

```cpp
#include <cute.h>
using namespace Cute;

int main(int argc, char* argv[])
{
	// Create a window with a resolution of 640 x 480.
	Result result = make_app("Fancy Window Title", 0, 0, 640, 480, APP_OPTIONS_WINDOW_POS_CENTERED, argv[0]);
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

Building CF should be done with CMake. The reason CMake is chosen, is it's one of the only working cross-platform build generators in existence, that actually works pretty much everywhere. Make sure you have a compiler installed that you're familiar with beforehand. If you're new to C/C++ I highly recommend using Microsoft Visual Studio (Community Edition), for Windows users. If you don't like Visual Studio you can try gcc/g++ (MinGW), [tdm-gcc](https://jmeubank.github.io/tdm-gcc/) is recommended in this case. If you're MacOS, XCode (and the command line tools) are recommended. For Linux you'll probably use g++.

It's highly recommended to use CF's [Cmake project template](https://github.com/RandyGaul/cute_framework_project_template#cmake-101-walkthrough) and follow along these steps with it!

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

# Contributing

The main ways to contribute to CF are:

- Bug reporting/fixes
- Adding new sample code
- Editing the docs

Read on below for instructions on each style of contribution. If you wish to add in new features to CF please open a GitHub issue to discuss the proposal first, to avoid putting in effort on a PR before receiving any feedback and avoid wasted work.

## Bug Reporting/Fixes

Simply open up a GitHub issue for reporting bugs. Be sure to describe as much detail as you can to understand or debug the problem.

For bug fixes please create a GitHub pull request. Try to be careful to match CF code style, and describe your decision making and the problem involved.

## Adding new Sample Code

The CF [samples](https://github.com/RandyGaul/cute_framework/tree/master/samples) are quite easy to extend. Simply copy + paste one of the other samples to get started, such as one of the simpler ones, perhaps [basic_sprite](https://github.com/RandyGaul/cute_framework/blob/master/samples/basic_sprite.cppb). or [basic_input.c](https://github.com/RandyGaul/cute_framework/blob/master/samples/basic_input.c).

Open up CF's [CmakeLists.txt file](https://github.com/RandyGaul/cute_framework/blob/master/CMakeLists.txt) to hook up the sample to the build system. Search for `CF_FRAMEWORK_BUILD_SAMPLES` to find a list of executable targets for all the samples via `add_executable`. Add your new sample here like so for the example "new_sample":

```cmake
...
add_executable(waves samples/waves.cpp)
add_executable(shallow_water samples/shallow_water.cpp)
add_executable(noise samples/noise.c)
add_executable(new_sample samples/new_sample.c)
```

Just below also add in a line via `set(SAMPLE_EXECUTABLES` like so:

```cmake
		hello_triangle
		waves
		shallow_water
		noise
		new_sample
	)
```

If your sample needs access to files on disk, such as assets like images or audio, create a folder in CF's samples folder. Name it "new_sample_data", where "new_sample" is the name of your new sample. Then add in a line to CF's [CmakeLists.txt file](https://github.com/RandyGaul/cute_framework/blob/master/CMakeLists.txt) to copy over the assets to the build folder when building.

```cmake
	add_custom_command(TARGET spaceshooter PRE_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/samples/spaceshooter_data $<TARGET_FILE_DIR:spaceshooter>/spaceshooter_data)
	add_custom_command(TARGET waves PRE_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/samples/waves_data $<TARGET_FILE_DIR:waves>/waves_data)
	add_custom_command(TARGET shallow_water PRE_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/samples/shallow_water_data $<TARGET_FILE_DIR:shallow_water>/shallow_water_data)
	add_custom_command(TARGET shallow_water PRE_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/samples/new_sample $<TARGET_FILE_DIR:shallow_water>/new_sample)
```

And that's it! Regenerate your project and you will be able to build your new sample. The next step is to add in your sample to [CF's documentation](https://randygaul.github.io/cute_framework/#/samples). You should [edit this file](https://github.com/RandyGaul/cute_framework/blob/master/docs/samples.md) to add your sample to the list of CF samples.

Once confirmed working as intended, open a pull request to add in your new sample!

## Editing the Docs

All of CF's docs for the [CF website](https://randygaul.github.io/cute_framework/#/) are located here in GitHub [under the docs folder](https://github.com/RandyGaul/cute_framework/tree/master/docs). Editing any of these docs is a good way to contribute to CF through a pull-request.

Please note that the reference pages for functions/structs are automatically generated by CF's [docs parser + generator](https://github.com/RandyGaul/cute_framework/blob/master/samples/docs_parser.cpp). You can run this executable to regenerate all of the docs files for all of CF. If you wish to edit any of the reference pages be sure to edit the appropriate file in CF's actual source code and regenerate the docs by running the CF sample [docs_parser](https://github.com/RandyGaul/cute_framework/blob/master/samples/docs_parser.cpp). You can then create a pull-request for any of the regenerated docs files.

<p align="center"><img src=https://github.com/RandyGaul/cute_framework/blob/master/assets/CF_Logo_Pixel_2x.png></p>
