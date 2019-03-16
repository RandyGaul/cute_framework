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
using namespace cute;

cute_t* cute = cute_make("Fancy Window Title", x, y, w, h);

while (is_running(cute))
{
	int font_x = 0, font_y = 0;
	font_print(cute, font_x, font_y, "Hello, world!");
	cute_update(cute);
}

```

#### Drawing a Sprite

> A ghost appears when pressing space (boo!).
```cpp
#include <cute.h>
using namespace cute;

cute_t* cute = cute_make("Ghost Goes Boo!", x, y, w, h);
sprite_t ghost = sprite_make("ghost.png");

while (is_running(cute))
{
	if (key_is_pressed(KEY_SPACE)) {
		sprite_push(ghost);
	}

	cute_update(cute);
}
```

#### Playing Audio

> An empty screen accompanied by some music.
```cpp
#include <cute.h>
using namespace cute;

cute_t* cute = cute_make("Did you hear something?", x, y, w, h);
audio_t* song = audio_load_ogg("rad_song.ogg");

music_play(song);

while (is_running(cute))
{
	cute_update(cute);
}
```

## Examples

Here are a few examples built with Cute. They also run in the browser, thanks to emscripten!

* Snake
* Blue Armor-Suit Man
* Cave Cards

## Documentation by Features

Cute covers all the low level guts required to build 2D games by implementing the "common slice" of what games typically need the most. This leaves the exciting game implementation up to you! You won't find high level game-specific features here; cute stays lean-and-mean by focusing on knocking out the toughest problems games run into before getting off the ground. Check out the documentation categories.

* [cute](https://github.com/RandyGaul/cute_framework/blob/master/doc/cute_t.md)
* audio
* event
* file system
* graphics
* input
* math
* collision detection
* physics
* concurrency
* time
* net
* utf8
* serialization
* data structures
* allocators
* ini
* camera
* logging
* entity
* hotload
* profile

## Support

Cute is actively developed and the author, Randy Gaul, uses it for his own games. Just open a [github issue](https://github.com/RandyGaul/cute_framework/issues/new) to ask any questions you might have :)

## Contributing

Contributions are welcome, so feel free to open a [pull-request](https://github.com/RandyGaul/cute_framework/pulls). To make it as easy as possible to accept new pull-requests it is a good idea to open up a discussion as a [github issue](https://github.com/RandyGaul/cute_framework/issues/new) to talk about changes before making a commitment to develop a full pull-request.
