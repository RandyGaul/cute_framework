# Cute Framework

The *cutest* framework out there for creating 2D games in C++ - runs on Windows/Apple/Linux and Android machines!

Cute is carefully designed to implement high quality APIs by anticipating common game developer needs and requirements. Cute prides itself on a lean-and-mean style with the utmost dedication to practicality. Cute is open source under the zlib license, making it free to use for commercial purposes without limitations.

## Download Cute

Click here to download the latest release of Cute!

## Snippets

Working with cute typically involves working with concise code-snippets that anticipate exactly what you will need as a developer. Check out some full example programs here.

#### Hello World

> Rendering a hello world string centered on the screen.
```cpp
#include <cute.h>

cute::app_t* app = cute::app_make("Fancy Window Title", x, y, w, h);

while (cute::is_running(app))
{
	int font_x = 0, font_y = 0;
	cute::font_print(app, font_x, font_y, "Hello, world!");
	cute::app_update(app);
}

```

#### Drawing a Sprite

> A ghost appears when pressing space (boo!).
```cpp
#include <cute.h>

cute::app_t* app = cute::app_make("Ghost Goes Boo!", x, y, w, h);
cute::sprite_t ghost = cute::sprite_make("ghost.png");

while (cute::is_running(app))
{
	if (cute::key_is_pressed(app, cute::KEY_SPACE)) {
		cute::sprite_push(app, ghost);
	}

	cute::app_update(app);
}
```

#### Playing Audio

> An empty screen accompanied by some music.
```cpp
#include <cute.h>

cute::app_t* app = cute::app_make("Did you hear something?", x, y, w, h);
cute::audio_t* song = cute::audio_load_ogg("rad_song.ogg");

cute::music_play(app, song);

while (cute::is_running(app))
{
	cute::app_update(app);
}
```

## Examples

Here are a few examples built with Cute.

* Snake
* Blue Armor-Suit Man
* Cave Cards

## Documentation by Features

Cute covers all the low level guts required to build 2D games by implementing the "common slice" of what games typically need the most. This leaves the exciting game implementation up to you! You won't find high level game-specific features here; cute stays lean-and-mean by focusing on knocking out the toughest problems games run into before getting off the ground. Check out the documentation categories.

Note: This has temporarily become a big TODO list. This list will eventually need to be converted into documentation categories.

* [ ] [cute](https://github.com/RandyGaul/cute_framework/blob/master/doc/cute_t.md)
* [ ] audio
	* [x] load wav
	* [x] load ogg
	* [x] stream wav
	* [x] stream ogg
	* [ ] stream then switch ogg
	* [ ] stream then crossfade ogg
	* [ ] music
	* [ ] sounds
* [ ] event
	* [ ] poll event
	* [ ] push event
	* [ ] grab data out of event
	* [ ] free event data
* [x] file system
	* [x] file io
	* [x] directory/archive mounting
* [ ] graphics
	* [ ] sprite batching
	* [ ] shader
	* [ ] vertex/index buffers
	* [ ] blend states
	* [ ] fbo (full-screen effect)
	* [ ] render to texture
	* [ ] debug rendering (line, shape, frames)
	* [ ] draw calls
	* [ ] projection
	* [ ] textures, wrap mode
	* [ ] scissor
	* [ ] viewport, resizing
	* [ ] pixel upscaling
	* [ ] frame-based animation
	* [ ] raster font
	* [ ] image loading
	* [ ] pixel upscale shader
	* [ ] universal MVP in shaders
	* [ ] CPU culling with DBVH

	* [ ] network simulator	* [ ] matrix helpers
	* [ ] d3d9
	* [ ] GL 3.2
	* [ ] GLES 2.0
	* [ ] color and helpers
* [ ] input
	* [ ] mouse
		* [ ] cursor
	* [ ] keyboard
	* [ ] gamepad
	* [ ] text input
	* [ ] drag n drop file
* [ ] math
* [ ] collision detection
* [x] concurrency
* [x] time
* [ ] net
	* [x] socket
	* [ ] connection handhsake
	* [ ] reliability
		* [ ] packet ack and resend
		* [ ] fragmentation and reassembly
	* [ ] relay server
		* [ ] broadcast to all, to all but one, to one
		* [ ] accept new connection
		* [ ] disconnect client
		* [ ] look for timed out clients
		* [ ] thread to pull packets and queue them
		* [ ] poll server packets (deque)
	* [x] security
	* [ ] network simulator
	* [ ] packet loss and RTT estimator
	* [ ] loopback client
* [ ] utf8 (for localization support)
* [ ] serialization
* [ ] data structures
	* [x] buffer
	* [ ] hash table
	* [ ] doubly linked list
	* [ ] dbvh
	* [ ] string
* [x] allocators
* [ ] ini
* [ ] camera
	* [ ] track an entity
	* [ ] set position
	* [ ] set destination + lerp time
	* [ ] state machine driven
* [ ] logging
	* [ ] to stderr
	* [ ] to log file
* [ ] entity
	* [ ] vtable
	* [ ] entity list
	* [ ] composition mechanism
* [ ] profile
	* [ ] record capture data
	* [ ] render capture data to screen
	* [ ] print capture data
	* [ ] interpolate capture data
* [ ] automated release package builder
* [ ] build/distro options
	* [ ] copy + paste all source into project
	* [ ] build shared libs themselves
	* [ ] download prebuilt release folder
* [ ] cmake support
* [ ] single-file-header packer (still requires shared lib dependencies)
* [ ] error
	* [ ] thread local
	* [x] error strings and handler
	* [ ] dialogue box

## Support

Cute is actively developed and the author, Randy Gaul, uses it for his own games. Just open a [github issue](https://github.com/RandyGaul/cute_framework/issues/new) to ask any questions you might have :)

## Contributing

Contributions are welcome, so feel free to open a [pull-request](https://github.com/RandyGaul/cute_framework/pulls). To make it as easy as possible to accept new pull-requests it is a good idea to open up a discussion as a [github issue](https://github.com/RandyGaul/cute_framework/issues/new) to talk about changes before making the commitment to develop a full pull-request.

## Dependencies

Users of Cute don't need to think about dependencies at all, since they are packaged up in the distribution of Cute, ready to use out-of-the-box. They are listed here for the curious.

Cute has very little dependencies, carefully chosen for their high quality. The first is the [SDL2 library](https://www.libsdl.org/), used for platform handling and GL context creation. The second is [libsodium](https://libsodium.gitbook.io/doc/), is used for security. The third is [glad](https://github.com/Dav1dde/glad) used to load OpenGL function pointers on the Windows platform.
