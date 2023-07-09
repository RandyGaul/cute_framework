[](../header.md ':include')

<br>

Getting things on the screen with CF can be done in one of two ways: [Low Level Graphics](https://randygaul.github.io/cute_framework/#/topics/low_level_graphics), or the [Draw API Reference](https://randygaul.github.io/cute_framework/#/api_reference?id=draw). This page showcases the latter, the recommended option for getting shapes, sprites and text onto the screen. Text has it's own dedicated [Text API Reference](https://randygaul.github.io/cute_framework/#/api_reference?id=text), discussed here as well.

## Drawing Shapes

Getting started with drawing shapes in CF is about calling an associated drawing function. For circles, use [`cf_draw_circle`](https://randygaul.github.io/cute_framework/#/draw/cf_draw_circle), for boxes/rectangles use [`cf_draw_quad`](https://randygaul.github.io/cute_framework/#/draw/cf_draw_quad), for lines use [`cf_draw_line`](https://randygaul.github.io/cute_framework/#/draw/cf_draw_line) or [`cf_draw_polyline`](https://randygaul.github.io/cute_framework/#/draw/cf_draw_polyline), and so on.

> Drawing some basic shapes, a pulsating circle and square.

```cpp
#include <cute.h>
using namespace Cute;

int main(int argc, char* argv[])
{
	int options = APP_OPTIONS_DEFAULT_GFX_CONTEXT | APP_OPTIONS_WINDOW_POS_CENTERED;
	Result result = make_app("Basic Shapes", 0, 0, 640, 480, options, argv[0]);
	if (is_error(result)) {
		printf("Error: %s\n", result.details);
		return -1;
	}

	draw_push_color(make_color(0xeba48bff));
	draw_push_antialias(true);
	float t = 0;

	while (app_is_running()) {
		app_update();
		t += DELTA_TIME;

		float radius = 100.0f;
		float motion = (sinf(t) + 1.0f) * 0.5f * 40.0f;
		draw_circle(V2(0,0), radius + motion, 1.0f + motion / 4);

		draw_push_color(color_purple());
		motion *= 3;
		draw_quad(make_aabb(V2(0,0), 30 + motion, 30 + motion), 5);
		draw_pop_color();

		app_draw_onto_screen();
	}

	destroy_app();

	return 0;
}
```

<p align="center">
<img src=https://github.com/RandyGaul/cute_framework/blob/master/assets/basic_shapes.gif?raw=true>
</p>

## Push and Pop Settings

Like we saw in the above example, the draw API has some settings that can be pushed and popped. The various settings include:

- color
- antialias
- antialias scale
- layer
- tint

Whenever a setting is pushed it will be used by subsequent drawing functions. For example, if we push a color with [`cf_draw_push_color`](https://randygaul.github.io/cute_framework/#/draw/cf_draw_push_color) it will get used until a new setting is pushed or popped. When we pop a setting the previously pushed state is restored. This is a great way to use your own settings locally, and then restore anything previous without messing up the settings for the rest of your code.

## Draw Layer

The layer controls the order things are drawn. You can set what layer to draw upon with [`cf_draw_push_layer`](https://randygaul.github.io/cute_framework/#/draw/cf_draw_push_layer). When done, restore the previously used layer with [`cf_draw_pop_layer`](https://randygaul.github.io/cute_framework/#/draw/cf_draw_pop_layer).

## Drawing Sprites

Sprites can be loaded with either .ase/.aseprite files or .png files. The recommended method is .ase files called [Aseprite](https://www.aseprite.org/) files. An aseprite file contains all the animation and image data necessary for a 2D frame based animations. If instead you want to support your own custom animation format, or any other format, you can load up your animation data and feed it into the [.png API](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_png_cache.h).

Some particular pages of interest are:

- [Sprite API Reference](https://randygaul.github.io/cute_framework/#/api_reference?id=sprite)
- [CF_Sprite](https://randygaul.github.io/cute_framework/#/sprite/cf_sprite)
- [cf_sprite_play](https://randygaul.github.io/cute_framework/#/sprite/cf_sprite_play)

!> **Important Note** You will need to mount your content folder before the following sample code will run. This lets CF know where to find your files for loading. See here from the previous page on [File I/O](https://randygaul.github.io/cute_framework/#/#/topics/file_io) to learn how.

You may download [girl.aseprite](https://github.com/RandyGaul/cute_framework/raw/master/test/test_data/girl.aseprite) for this example.

```cpp
#include <cute.h>
using namespace Cute;

int main(int argc, char* argv[])
{
	int options = APP_OPTIONS_DEFAULT_GFX_CONTEXT | APP_OPTIONS_WINDOW_POS_CENTERED;
	Result result = make_app("Basic Sprite", 0, 0, 640, 480, options, argv[0]);
	if (is_error(result)) return -1;

	Sprite girl_sprite = cf_make_demo_sprite();
	girl_sprite.play("idle");
	girl_sprite.scale = V2(4,4);

	while (app_is_running()) {
		app_update();

		girl_sprite.update();
		girl_sprite.draw();

		app_draw_onto_screen();
	}

	destroy_app();

	return 0;
}
```

<p align="center">
<img src=https://github.com/RandyGaul/cute_framework/blob/master/assets/basic_sprite.gif?raw=true>
</p>

The girl sprite in the above sample code contains a few different animations, such as "up" "side", "hold_side", "ladder", and "idle". Feel free to try them out! The above gif shows the girl sprite spinning -- this animation was removed to reduced the sprite's file size (so you unfortunately can't try spinning out).

---

Here's an example of drawing a more full looking scene with various sprites. Simply load up a whole bunch of sprite assets and draw them all! The sprite drawing API is designed to efficiently handle many thousands of different sprites on all platforms, all without the need to bake textures into atlases or do any kind of sprite packing yourself.

<p align="center">
<img src=https://github.com/RandyGaul/cute_framework/blob/master/assets/block_man.gif?raw=true>
</p>

## Drawing Text

Text has it's own [Text API Reference](https://randygaul.github.io/cute_framework/#/api_reference?id=text). Call [`cf_make_font`](https://randygaul.github.io/cute_framework/#/text/cf_make_font) to load up a font file, then call [`cf_draw_text`](https://randygaul.github.io/cute_framework/#/text/cf_draw_text) to draw text. Text has a whole bunch of settings, such as:

- [`cf_push_text_wrap_width`](https://randygaul.github.io/cute_framework/#/text/cf_push_text_wrap_width)
- [`cf_push_font_size`](https://randygaul.github.io/cute_framework/#/text/cf_push_font_size)
- [`cf_push_font_blur`](https://randygaul.github.io/cute_framework/#/text/cf_push_font_blur)
- [`cf_peek_text_clip_box`](https://randygaul.github.io/cute_framework/#/text/cf_peek_text_clip_box)

?> Recall that each push function has associated peek and pop APIs! See the [Text API Reference](https://randygaul.github.io/cute_framework/#/api_reference?id=text) for a full list of text related pages.

Here's an example sample for drawing some text onto the screen.

!> **Important Note** You will need to mount your content folder before the following sample code will run. This lets CF know where to find your files for loading. See here from the previous page on [File I/O](https://randygaul.github.io/cute_framework/#/#/topics/file_io) to learn how.

You may download [calibri.tff](https://github.com/RandyGaul/cute_framework/raw/master/samples/sample_data/calibri.ttf) to run this sample.

```cpp
#include <cute.h>
using namespace Cute;

int main(int argc, char* argv[])
{
	int options = APP_OPTIONS_DEFAULT_GFX_CONTEXT | APP_OPTIONS_WINDOW_POS_CENTERED | APP_OPTIONS_RESIZABLE;
	Result result = make_app("Text Drawing", 0, 0, 640, 480, options, argv[0]);
	if (is_error(result)) return -1;

	draw_push_antialias(true);
	make_font("ProggyClean.ttf", "ProggyClean");
	push_font("ProggyClean");
	make_font("calibri.ttf", "calibri");
	set_fixed_timestep();
	int draw_calls = 0;

	char* sample = fs_read_entire_file_to_memory_and_nul_terminate("sample.txt");
	CF_DEFER(cf_free(sample));

	while (app_is_running()) {
		app_update();

		static float t = 0;
		t += DELTA_TIME;

		// Clip text within a box.
		v2 o = V2(cosf(t),sinf(t)) * 25.0f;
		Aabb clip = make_aabb(V2(-75,-75) + o, V2(75,50) + o);
		draw_quad(clip, 0);
		push_text_clip_box(clip);
		push_font_size(13);
		draw_text("Clip this text within a box.", V2(-100,0));
		pop_font_size();
		pop_text_clip_box();

		// Draw text with a limited width.
		draw_quad(clip, 0);
		push_font_size(13);
		push_text_wrap_width(100.0f + cosf(t) * 75.0f);
		o = V2(-200, 150);
		cf_draw_line(V2(cosf(t) * 75.0f,0) + o, V2(cosf(t) * 75.0f,-75) + o, 0);
		draw_text("This text width is animating over time and wrapping the words dynamically.", V2(-100,0) + o);
		pop_text_wrap_width();
		pop_font_size();

		// Draw utf8 encoded text loaded from a text file.
		push_font("calibri");
		push_font_size(20);
		draw_text(sample, V2(-300,200));
		pop_font_size();
		pop_font();

		// Coloring text.
		push_font_size(26);
		draw_push_color(make_color(0x55b6f2ff));
		draw_text("Some bigger and blue text.", V2(-100,150));
		draw_pop_color();
		pop_font_size();
		
		// Using font blurring for a glowing effect.
		push_font_size(13 * 5);
		push_font_blur(10);
		draw_text("glowing~", V2(-200-10,-90+10));
		pop_font_blur();
		draw_text("<fade>glowing~</fade>", V2(-200,-90));
		pop_font_size();
		
		// Using font blurring for a shadow effect.
		push_font_size(13 * 5);
		push_font_blur(10);
		draw_push_color(color_black());
		draw_text("shadow", V2(-150-10-2.5f,-150+5));
		draw_pop_color();
		pop_font_blur();
		draw_text("shadow", V2(-150,-150));
		pop_font_size();

		// Drawing a formatted string.
		String draws;
		draws.fmt("Draw calls: %d", draw_calls);
		push_font_size(13);
		draw_text(draws.c_str(), V2(-640/2.0f + 10,-480/2.0f + 20));
		pop_font_size();

		// Text shake effect.
		draw_text("Some <shake freq=50 x=2.5 y=1>shaking</shake> text.", V2(100,100));

		draw_calls = app_draw_onto_screen();
	}

	destroy_app();

	return 0;
}
```

<p align="center">
<img src=https://github.com/RandyGaul/cute_framework/blob/master/assets/text_drawing.gif?raw=true>
</p>

You can see the [Text Effect](https://randygaul.github.io/cute_framework/#/text/cf_text_effect_register) system in work. Text codes that look sort of like xml are supported for a variety of built-in effects. Click the previous link to see some documentation about built-in text effects, and how to contruct + register your own custom text effect codes.

?> **Note** the position of rendering text is the top-left corner of the text.

## Camera

The drawing functions are all relative to the camera. The camera has it's own [Camera API Reference](https://randygaul.github.io/cute_framework/#/api_reference?id=camera) as well as a [Camera](https://randygaul.github.io/cute_framework/#/topics/camera) topic overview page.
