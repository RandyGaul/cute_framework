<p align="center">
<img src=https://github.com/RandyGaul/cute_framework/blob/master/logo.png>
</p>

Cute Framework (CF for short) is the *cutest* framework available for making 2D games in C++. CF comprises of different features, where the various features avoid inter-dependencies. In this way using CF is about picking and choosing which pieces are needed for your game. Here's a [video from the Handmade Seattle conference](https://media.handmade-seattle.com/cute-framework/) talking all about CF if you're interested in some more juicy background deets.

CF is not quite ready for the official first release! This repository is public to prepare for first release, so expect breaking changes and use at your own peril, etc.

# Gettin' all Cute

Setting up an application and getting started is quite easy. Simply visit [the app docs](https://randygaul.github.io/cute_framework/#/app/), grab the following code snippet for [app_make](https://randygaul.github.io/cute_framework/#/app/app_make), and off you go.

> Creating a window and closing it.

```cpp
#include <cute.h>
using namespace cute;

int main(int argc, const char** argv)
{
	// Create a window with a resolution of 640 x 480.
	error_t err = app_make("Fancy Window Title", 50, 50, 640, 480, CUTE_APP_OPTIONS_DEFAULT_GFX_CONTEXT, argv[0]);
	if (is_error(err)) return -1;

	while (app_is_running())
	{
		float dt = calc_dt();
		app_update(dt);
		// All your game logic and updates go here...
		app_present();
	}

	app_destroy();

	return 0;
}
```

# Docs by API Category

Select one of the categories below to learn more about them. Each category contains information about functions, structs, enums, and anything else relevant in the various Cute Framework header files.

[app](https://randygaul.github.io/cute_framework/#/app/)  
[audio](https://randygaul.github.io/cute_framework/#/audio/)  
[clipboard](https://randygaul.github.io/cute_framework/#/clipboard/)  
[data structures](https://randygaul.github.io/cute_framework/#/data_structures/)  
[ecs](https://randygaul.github.io/cute_framework/#/ecs/)  
[graphics](https://randygaul.github.io/cute_framework/#/graphics/)  
[math](https://randygaul.github.io/cute_framework/#/math/)  
[networking](https://randygaul.github.io/cute_framework/#/networking/)  
[serialization](https://randygaul.github.io/cute_framework/#/serialization/)  
[string](https://randygaul.github.io/cute_framework/#/string/)  
[time](https://randygaul.github.io/cute_framework/#/time/)  
[window](https://randygaul.github.io/cute_framework/#/window/)  

# Docs by API List

TODO

# Examples, Tutorials, and Articles

- [Cute Snake, example game implemented in CF](https://github.com/RandyGaul/cute_snake)
- [KV Serialization in CF docs](https://randygaul.github.io/cute_framework/#/serialization/)
- [ECS in CF docs](https://randygaul.github.io/cute_framework/#/ecs/)

# Download

Fow now it's recommended to build CF from source, at least until CF hits a first official release. See the Building from Source section below.

Prebuilt binaries for Windows are available in the [releases section](https://github.com/RandyGaul/cute_framework/releases). Please build and install from source for Mac/Linux users. Note - CF is designed for *64-bit only*.

# Community and Support

Feel free to open up an [issue right here on GitHub](https://github.com/RandyGaul/cute_framework/issues) to ask any questions. If you'd like to make a pull request I highly recommend opening a GitHub issue first to start a discussion on any changes you would like to make.

Here's a [link to the discord chat](https://discord.gg/2DFHRmX) for Cute Framework and the [Cute Headers](https://github.com/RandyGaul/cute_headers). Feel free to pop in and ask questions, make suggestions, or have a discussion.

Another easy way to get a hold of the author of Cute Framework is on twitter [@randypgaul](https://twitter.com/RandyPGaul).

# Building from Source

Install [cmake](https://cmake.org/). Then perform the usual cmake dance (make folder, -G to generate the build files, and then finally trigger the build), for example on Windows with Visual Studio 2019.

```cmake
mkdir build_msvc_2019 > nul 2> nul
cmake -G "Visual Studio 16 2019" -A x64 -Bbuild_msvc_2019 .
cmake --build build_msvc_2019 --config Debug
cmake --build build_msvc_2019 --config Release
```

Some scripts for running this cmake process are laying around in the top-level folder, such as `build_bash.sh` for apple/linux machines, or `mingw.cmd` for building against a MingW compiler on Windows. Feel free to use or ignore these scripts as you wish.

Once built go ahead and use cmake to install the headers and shared library for CF.

```cmake
cmake --install your_build_folder_name
```

## Prebuilt Releases

Prebuilt releases are planned for Windows and MacOS, but not actively setup right now since CF has yet to hit first release. Building from source is recommended for now.

# Emscripten Builds

Make sure [emscripten is installed](https://emscripten.org/docs/getting_started/downloads.html) on your machine. If on Windows go ahead and run the `emscripten.cmd` file. This will build libcute.a. Though if you're using something Ninja the commands will be slightly different, as you'll need to consult [emscripten docs](https://emscripten.org/docs/compiling/Building-Projects.html#integrating-with-a-build-system).

Additionally you can add something like the following to your cmake build script for your own project.

```cmake
if(${CMAKE_SYSTEM_NAME} MATCHES "Emscripten")
	set(CMAKE_EXECUTABLE_SUFFIX ".html")
	target_compile_options(your_game PUBLIC -O1 -fno-rtti -fno-exceptions)
	target_link_options(your_game PRIVATE -o your_game.html --preload-file ${CMAKE_SOURCE_DIR}/content --emrun -O1)
endif()
```

Also don't forget to call `emscripten_set_main_loop` from your `main` function!
