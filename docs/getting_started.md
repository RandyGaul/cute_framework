[](/header.md ':include')

<br>

Cute Framework (CF for short) is the *cutest* framework available for making 2D games in C++. CF provides a portable foundational layer for building 2D games in C/C++ without baggage, gnarly dependencies, or cryptic APIs. CF runs almost anywhere, including Windows, MacOS, iOS, Android, Linux, and more!

!> **Note** Cute Framework is currently in flux for it's v1.0 release! These notes/docs will be changed as they get rolled out and come online.

## Download and Setup

For now CF must be built from source using Cmake. Cmake provides one of the only reliable ways to setup and build C/C++ programs in a cross-platform manner. If you're new to Cmake there are some step-by-step instructions just below written specifically for getting your project up and running. These steps are a great way to learn about cross-platform developement in general, not just for CF!

### Building from Source

Another option for those familiar with CMake is to build from source with CMake. Make sure you have a compiler installed that you're familiar with beforehand. If you're new to C/C++ I highly recommend using Microsoft Visual Studio (Community Edition), for Windows users. If you're MacOS XCode (and command line tools) are recommended. For Linux you'll probably use g++.

1. Download and install CMake (v3.14 or higher, you can just get the latest version). CMake is for easy cross-platform building. Also install [git](https://git-scm.com/downloads). If you're new to git and a Windows user it's highly recommended to use [Github Desktop](https://desktop.github.com/).
2. Copy CMakeLists.txt ([this one here](https://github.com/RandyGaul/cute_framework_project_template/blob/main/CMakeLists.txt)) into the top-level of your project directory.
3. Find + replace "my_project_name".
4. Make a folder called `src` in the top-level of your project, and place your initial `main.cpp` there.
5. Run CMake on your project folder. If you need help with this step, try reading the [CMake 101 section here](https://github.com/RandyGaul/cute_framework_project_template#cmake-101-walkthrough).

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
	if (is_error(result)) {
		printf("Error: %s\n", result.details);
		return -1;
	}

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
