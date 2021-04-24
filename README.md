<p align="center">
<img src=https://github.com/RandyGaul/cute_framework/blob/master/logo.png>
</p>

Cute Framework (CF for short) is the *cutest* framework available for making 2D games in C/C++. CF comprises of different features, where the various features avoid inter-dependencies. In this way using CF is about picking and choosing which pieces are needed for your game.

## Important Note on Version

CF is not quite ready for the official first release! This repository is public to prepare for first release, so expect breaking changes and use at your own peril, etc. Most notably the networking features are not ready.

You've been warned!

# Gettin' all Cute

Setting up an application and getting started is quite easy. Simply visit [the app docs](https://github.com/RandyGaul/cute_framework/tree/master/doc/app), grab the following code snippet for [app_make](https://github.com/RandyGaul/cute_framework/blob/master/doc/app/app_make.md), and off you go.

> Creating a window and closing it.

```cpp
#include <cute.h>
using namespace cute;

int main(int argc, const char** argv)
{
	// Create a window with a resolution of 640 x 480, along with a DirectX 11 context.
	app_t* app = app_make("Fancy Window Title", 50, 50, 640, 480, CUTE_APP_OPTIONS_D3D11_CONTEXT, argv[0]);

	while (app_is_running(app))
	{
		float dt = calc_dt();
		app_update(app, dt);
		// All your game logic and updates go here...
		app_present(app);
	}
	
	app_destroy(app);
	
	return 0;
}
```

# Docs by API Category

Select one of the categories below to learn more about them. Each category contains information about functions, structs, enums, and anything else relevant in the various Cute Framework header files.

[app](https://github.com/RandyGaul/cute_framework/tree/master/doc/app)  
[audio](https://github.com/RandyGaul/cute_framework/tree/master/doc/audio)  
[clipboard](https://github.com/RandyGaul/cute_framework/tree/master/doc/clipboard)  
[data structures](https://github.com/RandyGaul/cute_framework/tree/master/doc/data_structures)  
[ecs](https://github.com/RandyGaul/cute_framework/tree/master/doc/ecs)  
[graphics](https://github.com/RandyGaul/cute_framework/tree/master/doc/graphics)  
[math](https://github.com/RandyGaul/cute_framework/tree/master/doc/math)  
[serialization](https://github.com/RandyGaul/cute_framework/tree/master/doc/serialization)  
[string](https://github.com/RandyGaul/cute_framework/tree/master/doc/string)  
[time](https://github.com/RandyGaul/cute_framework/tree/master/doc/time)  
[window](https://github.com/RandyGaul/cute_framework/tree/master/doc/window)  

# Docs by API List

TODO

# Download

TODO

# Community and Support

Feel free to open up an [issue right here on GitHub](https://github.com/RandyGaul/cute_framework/issues) to ask any questions. If you'd like to make a pull request I highly recommend opening a GitHub issue first to start a discussion on any changes you would like to make.

Here's a [link to the discord chat](https://discord.gg/2DFHRmX) for Cute Framework and the [Cute Headers](https://github.com/RandyGaul/cute_headers). Feel free to pop in and ask questions, make suggestions, or have a discussion.

Another easy way to get a hold of the author of Cute Framework is on twitter [@randypgaul](https://twitter.com/RandyPGaul).

# Building from Source

It's highly recommended to download a prebuilt version of CF, but if you for some reason want to build from source yourself, then read on...

Install [cmake](https://cmake.org/).
Perform the usual cmake dance (make folder, -G to generate the build files, and the finally trigger the build), for example on Windows with Visual Studio 2019.

```cmake
mkdir build_msvc_2019 > nul 2> nul
cmake -G "Visual Studio 16 2019" -A x64 -Bbuild_msvc_2019 .
cmake --build build_msvc_2019 --config Debug
cmake --build build_msvc_2019 --config Release
```

Some scripts for running this cmake process are laying around in the top-level folder, such as `apple_make.sh` for apple machines, or `mingw.cmd` for building against a MingW compiler on Windows. Feel free to use or ignore these scripts as you wish.
