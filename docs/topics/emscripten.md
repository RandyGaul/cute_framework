# Web Builds with Emscripten

Cute Framework supports web builds with a GLES3 backend renderer. Getting started with Emscripten is a bit challenging, so hopefully this page can help get you started. Once you get your game building for the web it's usually quite a breeze after the initial setup.

!!! Note

    Emscripten builds automatically disable CF's [HTTPS support](../api_reference.md#web), since web builds suffer from very poor support of this feature.

## Install Emscripten

Make sure [emscripten is installed](https://emscripten.org/docs/getting_started/downloads.html) on your machine, along with [Perl](https://strawberryperl.com/).

!!! tip

    Be sure to install any [platform-specific](https://emscripten.org/docs/getting_started/downloads.html#platform-specific-notes) dependencies first.

!!! note

    For windows users performing setup with the command prompt you should omit `./` from the Emscripten docs. Instead of typing `./emsdk install latest`, try typing `emsdk install latest`.

1. Run `./emsdk install latest` (or `emsdk install latest` for Windows users).
2. Run `./emsdk activate latest` (or `emsdk activate latest` for Windows users).
3. Windows uers only: Optionally run step 2 with `--permanent` flag (recommended), e.g. `emsdk activate latest --permanent`.

You will likely need to call `source ./emsdk_env.sh` on Linux/MacOS to setup environment variables. For Windows users `--permanent` may be buggy and not correctly, in this case you must call `emsdk_env.bat` manually each time you open a new command prompt.

## Build CF

If on Windows go ahead and run the `emscripten.cmd` file. This will build libcute.a. If you're using something like Ninja the commands will be slightly different; consult the [emscripten docs](https://emscripten.org/docs/compiling/Building-Projects.html#integrating-with-a-build-system) if you need help.

## Build your Game

Additionally you can add something like the following to your cmake build script for your own project.

```cmake
if(${CMAKE_SYSTEM_NAME} MATCHES "Emscripten")
	set(CMAKE_EXECUTABLE_SUFFIX ".html")
	target_compile_options(your_game PUBLIC -O1 -fno-rtti -fno-exceptions)
	target_link_options(your_game PRIVATE -o your_game.html --preload-file ${CMAKE_SOURCE_DIR}/content --emrun -O1)
endif()
```

Also don't forget to call `emscripten_set_main_loop` from your `main` function! The easiest way is to use the macro `CF_EMSCRIPTEN` to setup your main loop like so:

```c
#ifdef CF_EMSCRIPTEN
	// Receives a function to call and some user data to provide it.
	emscripten_set_main_loop(update, 60, true);
#else
	while (app_is_running()) {
		update();
	}
	destroy_app();
#endif
```

Where `upduate` will be run once per game tick, and replace your usually main loop body.

## Example Game Project

The [Cute Snake](https://github.com/RandyGaul/cute_snake/blob/master/README.md) game project is a good example of how to setup an Emscripten build with CMake.
