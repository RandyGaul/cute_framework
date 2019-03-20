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

Here are a few examples built with Cute. They also run in the browser, thanks to emscripten!

* Snake
* Blue Armor-Suit Man
* Cave Cards

## Documentation by Features

Cute covers all the low level guts required to build 2D games by implementing the "common slice" of what games typically need the most. This leaves the exciting game implementation up to you! You won't find high level game-specific features here; cute stays lean-and-mean by focusing on knocking out the toughest problems games run into before getting off the ground. Check out the documentation categories.

* [x] [cute](https://github.com/RandyGaul/cute_framework/blob/master/doc/cute_t.md)
* [x] audio
* [ ] event
* [x] file system (DONE)
* [ ] graphics
* [x] input
* [ ] math
* [ ] collision detection
* [x] concurrency (DONE)
* [x] time (DONE)
* [x] net
* [ ] utf8
* [ ] serialization
* [ ] data structures
* [x] allocators (DONE)
* [ ] ini
* [ ] camera
* [ ] logging
* [ ] entity
* [ ] profile

## Support

Cute is actively developed and the author, Randy Gaul, uses it for his own games. Just open a [github issue](https://github.com/RandyGaul/cute_framework/issues/new) to ask any questions you might have :)

## Contributing

Contributions are welcome, so feel free to open a [pull-request](https://github.com/RandyGaul/cute_framework/pulls). To make it as easy as possible to accept new pull-requests it is a good idea to open up a discussion as a [github issue](https://github.com/RandyGaul/cute_framework/issues/new) to talk about changes before making the commitment to develop a full pull-request.

## Dependencies

Users of Cute don't need to think about dependencies at all, since they are packaged up in the distribution of Cute, ready to use out-of-the-box. They are listed here for the curious.

Cute has very little dependencies, carefully chosen for their high quality. The first is the [SDL2 library](https://www.libsdl.org/), used for platform handling and GL context creation. The second is [libsodium](https://libsodium.gitbook.io/doc/), is used for security. The third is [glad](https://github.com/Dav1dde/glad) used to load OpenGL function pointers on the Windows platform.
