# Cute Framework

The *cutest* framework out there for creating 2D games in C++!

Cute runs everywhere SDL2 can run.
- [x] Windows
- [x] MacOSX
- [x] Linux
- [ ] iOS
- [ ] Android
- [x] Web browsers
- [x] Nintendo Switch

The underlying idea of Cute draws inspiration from the SDL2 library. A lot of focus placed on documentation and examples makes development with Cute as painless as possible, without getting in the way.

## Download Cute

Click here to download the latest version of Cute!

#### Hello World

> Rendering a hello world string centered on the screen.
```cpp
#include <cute.h>
using namespace cute;

int main(int argc, const char** argv)
{
	int options = CUTE_APP_OPTIONS_WINDOW_POS_CENTERED | CUTE_APP_OPTIONS_RESIZABLE;
	app_t* app = app_make("Cute Hello World", 0, 0, 640, 480, options);

	gfx_init(app);
	gfx_matrix_t mvp = matrix_ortho_2d(320, 240, 0, 0);
	const font_t* font = font_get_default(app);
	float w = (float)font_text_width(font, "Hello world!");
	float h = (float)font_text_height(font, "Hello world!");

	while (app_is_running(app)) {
		float dt = calc_dt();
		app_update(app, dt);

		font_push_verts(app, font, "Hello world!", -w / 2, h / 2, 0);
		font_submit_draw_call(app, font, mvp);

		gfx_flush(app);
	}

	app_destroy(app);

	return 0;
}
```

#### Drawing a Sprite

> A cloud moves in a circle while holding space.
```cpp
#include <stdio.h>
#include <cute.h>
using namespace cute;

int main(int argc, const char** argv)
{
	int options = CUTE_APP_OPTIONS_WINDOW_POS_CENTERED | CUTE_APP_OPTIONS_RESIZABLE;
	app_t* app = app_make("Cute Cloud", 0, 0, 640, 480, options);

	gfx_init(app);
	spritebatch_t* sb = sprite_batch_easy_make(app, "data");

	sprite_t cloud;
	error_t err = sprite_batch_easy_sprite(sb, "data/cloud.png", &cloud);
	if (err.is_error()) {
		printf("%s\n", err.details);
		return -1;
	}
	float t = 0;

	while (app_is_running(app)) {
		float dt = calc_dt();
		app_update(app, dt);

		if (key_is_down(app, KEY_SPACE)) {
			t += dt * 1.5f;
		}
		cloud.transform.p.x = cos(t) * 20.0f;
		cloud.transform.p.y = sin(t) * 20.0f;
		sprite_batch_push(sb, cloud);

		sprite_batch_flush(sb);

		gfx_flush(app);
	}

	app_destroy(app);

	return 0;
}
```

#### Playing a Sound Effect

> Play a jump sound when pressing space.
```cpp
#include <cute.h>
using namespace cute;

int main(int argc, const char** argv)
{
	int options = CUTE_APP_OPTIONS_WINDOW_POS_CENTERED | CUTE_APP_OPTIONS_RESIZABLE;
	app_t* app = app_make("Cute Jump Sound", 0, 0, 640, 480, options);

	app_init_audio(app);
	audio_t* jump_audio = audio_load_wav("jump.wav");

	while (app_is_running(app)) {
		float dt = calc_dt();
		app_update(app, dt);

		if (key_was_pressed(app, KEY_SPACE)) {
			audio_play(app, jump_audio);
		}
	}

	app_destroy(app);

	return 0;
}
```

#### Playing some Music

> Loads up then plays some music.
```cpp
#include <cute.h>
using namespace cute;

int main(int argc, const char** argv)
{
	int options = CUTE_APP_OPTIONS_WINDOW_POS_CENTERED | CUTE_APP_OPTIONS_RESIZABLE;
	app_t* app = app_make("Cute Music", 0, 0, 640, 480, options);

	app_init_audio(app);
	audio_t* music = audio_load_ogg("music.ogg");
	music_play(app, music, 2);

	while (app_is_running(app)) {
		float dt = calc_dt();
		app_update(app, dt);
	}

	app_destroy(app);

	return 0;
}
```

#### Running Dear ImGui

> Initialize and use Dear ImGui, a great built-in UI library.
```cpp
#include <stdio.h>
#include <imgui/imgui.h>
#include <cute.h>
using namespace cute;

int main(int argc, const char** argv)
{
	int options = CUTE_APP_OPTIONS_WINDOW_POS_CENTERED | CUTE_APP_OPTIONS_RESIZABLE;
	app_t* app = app_make("Cute ImGui", 0, 0, 640, 480, options);

	gfx_init(app);

	ImGuiContext* imgui_context = app_init_imgui(app);
	if (!imgui_context) {
		printf("Unable to initialize ImGui.\n");
		return -1;
	}
	ImGui::SetCurrentContext(imgui_context);

	while (app_is_running(app)) {
		float dt = calc_dt();
		app_update(app, dt);

		static bool hello_open = true;
		if (hello_open) {
			ImGui::Begin("Hello", &hello_open);
			static bool push_me;
			ImGui::Checkbox("Push Me", &push_me);
			if (push_me) {
				ImGui::Separator();
				ImGui::Indent();
				ImGui::Text("This is a Dear ImGui Window!");
			}
			ImGui::End();
		}

		gfx_flush(app);
	}

	app_destroy(app);

	return 0;
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

* [app](https://github.com/RandyGaul/cute_framework/blob/master/doc/app.md)
* audio
* camera
* concurrency
* clipboard
* crypto
* data structures
* ecs
* file system
* font
* graphics
* image
* input
* serialization
* math
* collision detection
* sprite
* string
* time
* utf8 (for localization support)
* window

## Integrating Cute with your Project

Cute has a couple options for integration into your project. Which one your pick depends on your preferences. If you are unsure which to pick, go with option #1 since it is the most traditional.

1. Download prebuilt binaries and headers for your platform.
2. Download the single-file header format of Cute, with prebuilt dependencies.
3. Download the source code of Cute and run the build scripts yourself.

Options #1 and #2 are nearly identical, except #1 has multiple headers and prebuilt shared libraries, while #2 provides Cute as a single-file header along with libsodium as a prebuilt shared library (which must be linked agianst).

Option #3 should work out-of-the-box for Windows, Linux, Mac OSX, iOS, and Android. Other platforms will require manual tinkering for support.

## Support

Cute is actively developed and the author, Randy Gaul, uses it for his own games. Just open a [github issue](https://github.com/RandyGaul/cute_framework/issues/new) to ask any questions you might have :)

## Contributing

Contributions are welcome, so feel free to open a [pull-request](https://github.com/RandyGaul/cute_framework/pulls). To make it as easy as possible to accept new pull-requests it is a good idea to open up a discussion as a [github issue](https://github.com/RandyGaul/cute_framework/issues/new) to talk about changes before making the commitment to develop a full pull-request.

## Dependencies

Cute has a few external dependencies, linked as shared libraries.
* [SDL2](https://www.libsdl.org/), used for platform handling and GL context creation.
* [libsodium](https://libsodium.gitbook.io/doc/), used for encryption + authentication.

Cute has a few internal dependencies, built straight from source as apart of Cute.
* [glad](https://github.com/Dav1dde/glad), used to load OpenGL function pointers on the Windows platform.
* [PhysicsFS](https://icculus.org/physfs/), used for [virtual file system](https://www.randygaul.net/2019/03/20/virtual-file-systems-in-games/).
* [STB Vorbis](https://github.com/nothings/stb/blob/master/stb_vorbis.c), for parsing OGG files.
* [Cute Headers](https://github.com/RandyGaul/cute_headers), used to implement all kinds of things. These headers were implemented by the author of the Cute Framework, and are used to implement the majority of the features in the Cute Framework. These headers are built by embedding the source directly, via single-file header format.
* [sokol_gfx.h](https://github.com/floooh/sokol), used to implement all rendering and abstract away low level platform-specific hardware acceleration APIs.