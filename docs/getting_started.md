Cute Framework (CF for short) is the *cutest* framework available for making 2D and 3D games in C++. CF provides a portable foundational layer for building games in C/C++ without baggage, gnarly dependencies, or cryptic APIs. CF runs almost anywhere, including Windows, MacOS, iOS, Android, Linux, and more!

> [!NOTE]
> Cute Framework documentation covers the C API, however, the vast majority of CF also has associated C++ wrapper APIs that sit along-side the C API in each header.

## Download and Setup

CF must be built from source using Cmake. Cmake provides one of the only reliable ways to setup and build C/C++ programs in a cross-platform manner. If you're new to Cmake there are some step-by-step instructions just below written specifically for getting your project up and running. These steps are a great way to learn about cross-platform developement in general, not just for CF!

### Building from Source

Make sure you have a compiler installed that you're familiar with beforehand. If you're new to C/C++ I highly recommend using Microsoft Visual Studio (Community Edition), for Windows users. If you're MacOS XCode (and command line tools) are recommended. For Linux you'll probably use g++.

1. Download and install CMake (v3.14 or higher, you can just get the latest version). CMake is for easy cross-platform building. Also install [git](https://git-scm.com/downloads). If you're new to git and a Windows user it's highly recommended to use [Github Desktop](https://desktop.github.com/).
2. Copy CMakeLists.txt ([this one here](https://github.com/RandyGaul/cute_framework_project_template/blob/main/CMakeLists.txt)) into the top-level of your project directory.
3. Find + replace "mygame".
4. Make a folder called `src` in the top-level of your project, and place your initial `main.cpp` there.
5. Run CMake on your project folder. If you need help with this step, try reading the [CF + CMake 101 section here](https://github.com/RandyGaul/cute_framework_project_template#cmake-101-walkthrough).

### CMake Options

Pass these to CMake with `-D<OPTION>=ON` or `-D<OPTION>=OFF`.

| Option | Default | Description |
| --- | --- | --- |
| `CF_FRAMEWORK_STATIC` | `ON` | Build CF as a static library. `OFF` builds a shared library. |
| `CF_CUTE_SHADERC` | `ON` | Build `cute-shaderc`, the offline shader compiler. See [Shader Compilation](topics/shader_compilation.md). |
| `CF_CUTE_SYM` | `ON` | Build `cute-sym`, the symbol table tool used by the crash reporter. |
| `CF_FRAMEWORK_APPLE_FRAMEWORK` | `OFF` | Build CF as an Apple Framework. |
| `CF_USE_SYSTEM_SDL` | `OFF` | Use an SDL3 you already have installed instead of downloading and building SDL3 from source. |
| `CF_FRAMEWORK_BUILD_TESTS` | `ON` | Build the unit tests (only when CF is the top-level project). |
| `CF_FRAMEWORK_BUILD_SAMPLES` | `ON` | Build the samples (only when CF is the top-level project). |
| `CF_BUILD_DOCSPARSER` | `ON` | Build the documentation generator (only when CF is the top-level project). |

#### Using an Installed SDL3

By default CF downloads SDL3 and builds it alongside CF. Set `CF_USE_SYSTEM_SDL=ON` to link against an SDL3 that's already installed, for example from your package manager or your own build. CF finds it with CMake's `find_package(SDL3)`, so point CMake at it with `CMAKE_PREFIX_PATH` (the install prefix) or `SDL3_DIR` (the folder holding `SDL3Config.cmake`) if it isn't in a default location.

```bash
cmake -B build -DCF_USE_SYSTEM_SDL=ON -DCMAKE_PREFIX_PATH=/path/to/sdl3/install
```

- CF needs SDL3 3.4.0 or any newer SDL3 release (SDL 4 is not accepted). Configuration stops with an error listing the versions it found if none fit.
- A static CF build links SDL's static library when the package has one, and otherwise its shared library. A shared CF build links SDL's shared library.
- On Windows, when the SDL3 used is a shared library, CF's own tests and samples get `SDL3.dll` copied next to them. Your own executables need `SDL3.dll` beside them or on `PATH`.
- On Windows, CF reaches into SDL's private D3D12 structs to silence one debug-layer warning. An installed SDL doesn't include the sources CF normally reads those layouts from, so CF uses offsets recorded from SDL 3.4.0. With any other SDL version that warning simply stays on in debug builds.
- The option has no effect on Emscripten, which always uses its own SDL3 port.

## Example Game Window

> Creating a window and closing it.

```cpp
#include <cute.h>
using namespace Cute;

int main(int argc, char* argv[])
{
	// Create a window with a resolution of 640 x 480.
	CF_Result result = make_app("Fancy Window Title", 0, 0, 0, 640, 480, CF_APP_OPTIONS_WINDOW_POS_CENTERED_BIT, argv[0]);
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
