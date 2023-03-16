[](../header.md ':include')

<br>

!> This page is marked as _incomplete_, and will be refined for v1.0 launch of CF.

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
