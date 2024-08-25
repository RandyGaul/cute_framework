[](../header.md ':include')

<br>

Getting things on the screen with CF can be done in one of two ways: [Low Level Graphics](https://randygaul.github.io/cute_framework/#/topics/low_level_graphics), or the [Draw API Reference](https://randygaul.github.io/cute_framework/#/api_reference?id=draw). This page showcases the latter, the recommended option for getting shapes, sprites and text onto the screen. Text has it's own dedicated [Text API Reference](https://randygaul.github.io/cute_framework/#/api_reference?id=text), discussed here as well.

## Drawing Shapes

CF can render a variety of shape types:

- Circle
- Box/Aabb
- Triangle
- Line segment
- Polyline
- Capsule
- Bezier polyline

The shape renderer in CF has a few extra features that nearly all shapes take advantage of:

- Customizeable antialiasing
- Border stroke vs fill style
- Edge rounding (chubbiness)

For circles, use [`cf_draw_circle`](https://randygaul.github.io/cute_framework/#/draw/cf_draw_circle), for boxes/rectangles use [`cf_draw_quad`](https://randygaul.github.io/cute_framework/#/draw/cf_draw_quad), for lines use [`cf_draw_line`](https://randygaul.github.io/cute_framework/#/draw/cf_draw_line) or [`cf_draw_polyline`](https://randygaul.github.io/cute_framework/#/draw/cf_draw_polyline), and so on.

> Drawing some basic shapes, a pulsating circle and square.

```cpp
#include <cute.h>
using namespace Cute;

int main(int argc, char* argv[])
{
	Result result = make_app("Basic Shapes", 0, 0, 0, 640, 480, APP_OPTIONS_WINDOW_POS_CENTERED_BIT, argv[0]);
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

The draw API has some settings that can be pushed and popped. Pushing and popping settings is a great way to customize how to draw without affecting the settings of the rest of your code. Here are some of the customizeable settings:

- color
- antialias on/off
- antialias scale
- layer
- chubbiness

Whenever a setting is pushed it will be used by subsequent drawing functions. For example, if we push a color with [`cf_draw_push_color`](https://randygaul.github.io/cute_framework/#/draw/cf_draw_push_color) it will get used until a new setting is pushed or popped. When we pop a setting the previously pushed state is restored. This is a great way to use your own settings locally, and then restore anything previous without messing up the settings for the rest of your code. You may nest push/pop pairs as many times as needed.

## Draw Layer

The layer controls the order things are drawn. You can set what layer to draw upon with [`cf_draw_push_layer`](https://randygaul.github.io/cute_framework/#/draw/cf_draw_push_layer). When done, restore the previously used layer with [`cf_draw_pop_layer`](https://randygaul.github.io/cute_framework/#/draw/cf_draw_pop_layer).

## Drawing Sprites

Sprites can be loaded with either .ase/.aseprite files or .png files. The recommended method is .ase files called [Aseprite](https://www.aseprite.org/) files. An aseprite file contains all the animation and image data necessary for a 2D frame based animations. If instead you want to support your own custom animation format, or any other format, you can load up your animation data and feed it into the [.png API](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_png_cache.h).

Some particular pages of interest are:

- [Sprite API Reference](https://randygaul.github.io/cute_framework/#/api_reference?id=sprite)
- [CF_Sprite](https://randygaul.github.io/cute_framework/#/sprite/cf_sprite)
- [cf_sprite_play](https://randygaul.github.io/cute_framework/#/sprite/cf_sprite_play)

CF comes with a convenience function called [`cf_make_demo_sprite`](https://randygaul.github.io/cute_framework/#/draw/cf_make_demo_sprite). This sprite contains a small pixel art girl with a couple built-in animations. Here's a program to load her up and draw her on screen:

```cpp
#include <cute.h>
using namespace Cute;

int main(int argc, char* argv[])
{
	Result result = make_app("Basic Sprite", 0, 0, 0, 640, 480, APP_OPTIONS_WINDOW_POS_CENTERED_BIT, argv[0]);
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

The girl sprite in the above sample code contains a few different animations, such as "up" "side", "hold_side", "ladder", and "idle". Feel free to try them out!

---

Here's an example of drawing a more full looking scene with various sprites. Simply load up a bunch of sprite assets and draw them all! The sprite drawing API is designed to efficiently handle many thousands of different sprites on all platforms, all without the need to bake textures into atlases or do any kind of sprite packing yourself.

<p align="center">
<img src=https://github.com/RandyGaul/cute_framework/blob/master/assets/block_man.gif?raw=true>
</p>

### Sprite Origin

The sprite may have a local origin to offset itself whenever drawn. Set the `local_offset` member of any [CF_Sprite](https://randygaul.github.io/cute_framework/#/sprite/cf_sprite) struct. This field gets automatically populated if loading a sprite from a .ase file, if the .ase file contains a [slice](https://www.aseprite.org/docs/slices/) called `origin`.

<p align="center">
<img src=https://github.com/RandyGaul/cute_framework/blob/master/assets/origin_slice.png?raw=true>
</p>

## Drawing Text

Text has it's own [Text API Reference](https://randygaul.github.io/cute_framework/#/api_reference?id=text). Call [`cf_make_font`](https://randygaul.github.io/cute_framework/#/text/cf_make_font) to load up a font file, then call [`cf_draw_text`](https://randygaul.github.io/cute_framework/#/text/cf_draw_text) to draw text. Text has a whole bunch of settings, such as:

- [`cf_push_text_wrap_width`](https://randygaul.github.io/cute_framework/#/text/cf_push_text_wrap_width)
- [`cf_push_font_size`](https://randygaul.github.io/cute_framework/#/text/cf_push_font_size)
- [`cf_push_font_blur`](https://randygaul.github.io/cute_framework/#/text/cf_push_font_blur)
- [`cf_peek_text_clip_box`](https://randygaul.github.io/cute_framework/#/text/cf_peek_text_clip_box)

?> Recall that each push function has associated peek and pop APIs! See the [Text API Reference](https://randygaul.github.io/cute_framework/#/api_reference?id=text) for a full list of text related pages.

Here's a [sample](https://github.com/RandyGaul/cute_framework/blob/master/samples/text_drawing.cpp) for drawing some text onto the screen.

<p align="center">
<img src=https://github.com/RandyGaul/cute_framework/blob/master/assets/text_drawing.gif?raw=true>
</p>

You can see the [Text Effect](https://randygaul.github.io/cute_framework/#/text/cf_text_effect_register) system in work. Text codes that look sort of like xml are supported for a variety of built-in effects. Click the previous link to see some documentation about built-in text effects, and how to contruct + register your own custom text effect codes.

?> **Note** the position of rendering text is the top-left corner of the text.

## Shaders

You can apply customizable shaders that work with the draw API by using functions like [cf_render_settings_push_shader](https://randygaul.github.io/cute_framework/#/draw/cf_render_settings_push_shader)
 and [cf_render_settings_pop_shader](https://randygaul.github.io/cute_framework/#/draw/cf_render_settings_pop_shader). These shaders are written in glsl version 450. By creating custom FX you can implement interesting visuals like the following wavelet example:

<p align="center">
<img src=https://github.com/RandyGaul/cute_framework/blob/master/assets/wavelets.gif?raw=true>
</p>

The draw API passes *all* geometry into an optional shader function, within the fragment shader, called `shader`. This function is the final step in the entire fragment shader, granting the opportunity to alter the final output pixel color. Let us look at the custom shader skeleton/stub (does no-op).

```glsl
vec4 shader(vec4 color, vec2 pos, vec2 atlas_uv, vec2 screen_uv, vec4 params)
{
	return vec4(mix(color.rgb, params.rgb, params.a), color.a);
}
```

The `color` param is the color that would be rendered if you don't make any modifications to it within the `shader` function. If drawing a sprite this would be the color of a particular pixel from the sprite's texture, or, if drawing a shape the color of the shape itself.

`pos` was the rendering position used when calling an associated draw function like `cf_draw_sprite`.

`atlas_uv` is the uv corresponding to the texel within `u_image`, the atlas CF has generated behinds the scenes for any sprites to be drawn. This isn't a particularly useful thing to have access to, but is here just in case. You can freely access `u_image` and use `atlas_uv` for this particular fragment, such as for multisampling algorithms. For pixel art games it's important to sample using the function `smooth_uv`, something like so: `smooth_uv(v_uv, u_texture_size)` to generate a uv coordinate that will scale pixel art correctly.

`screen_uv` is a position relative to the screen, where (0,0) is the top-left, and (1,1) is the bottom-right of the screen.

`params` are four optional floats. They come from vertex attributes set by [cf_draw_push_vertex_attributes](https://randygaul.github.io/cute_framework/#/draw/cf_draw_push_vertex_attributes). Each different item drawn through CF's draw API will attach the previously pushed attributes onto their vertices. These four floats are general-purpose, and only used to pass into the `shader` function. Use them to pack information to implement your own custom visual FX, such as color tinting, or anything else on a per-object basis.

You may also add in uniforms and textures as-needed. The draw API has some functions for setting uniforms and textures via [cf_render_settings_push_uniform](https://randygaul.github.io/cute_framework/#/draw/cf_render_settings_push_uniform) and [cf_render_settings_push_texture](https://randygaul.github.io/cute_framework/#/draw/cf_render_settings_push_texture). These will get auto-magically hooked up and send values to your shader. When you add in your own uniforms just be sure to place them inside of a uniform block like in the below sample (see `shd_uniforms`, and don't change this name either! It must be called `shd_uniforms`).

Here's a full example shader from the wavelets (called [shallow water on github](https://github.com/RandyGaul/cute_framework/blob/master/samples/shallow_water.cpp)) demo:

```glsl
layout (set = 2, binding = 1) uniform sampler2D wavelets_tex;
layout (set = 2, binding = 2) uniform sampler2D noise_tex;
layout (set = 2, binding = 3) uniform sampler2D scene_tex;

layout (set = 3, binding = 1) uniform shd_uniforms {
	float show_normals;
	float show_noise;
};

vec2 normal_from_heightmap(sampler2D tex, vec2 uv)
{
	float ha = textureOffset(tex, uv, ivec2(-1, 1)).r;
	float hb = textureOffset(tex, uv, ivec2( 1, 1)).r;
	float hc = textureOffset(tex, uv, ivec2( 0,-1)).r;
	vec2 n = vec2(ha-hc, hb-hc);
	return n;
}

vec4 normal_to_color(vec2 n)
{
	return vec4(n * 0.5 + 0.5, 1.0, 1.0);
}

vec4 shader(vec4 color, vec2 pos, vec2 atlas_uv, vec2 screen_uv, vec4 params)
{
	vec2 uv = screen_uv;
	vec2 dim = vec2(1.0/160.0,1.0/120.0);
	vec2 n = normal_from_heightmap(noise_tex, uv);
	vec2 w = normal_from_heightmap(wavelets_tex, uv+n*dim*10.0);
	vec4 c = mix(normal_to_color(n), normal_to_color(w), 0.25);
	c = texture(scene_tex, uv+(n+w)*dim*10.0);
	c = mix(c, vec4(1), length(n+w) > 0.2 ? 0.1 : 0.0);

	c = show_normals > 0.0 ? mix(normal_to_color(n), normal_to_color(w), 0.25) : c;

	c = show_noise > 0.0 ? texture(noise_tex, uv) : c;

	return c;
}
```

?> **Note**  custom shaders require a specific ordering for resource sets (the `set = N` part). Texture samplers `uniform sampler2D` must have `set = 2` and bindings start at index 1 (`binding = 1`), while the uniform block *must be named* `shd_uniforms` an have `set = 3, binding = 1`. Generally speaking you can just copy + paste this example and easily get away with incrementing the `binding = N` for textures. To add in more uniforms simply add more members to the `shd_uniforms` block.

The custom textures are `wavelets_tex`, `noise_tex` and `scene_tex`. The custom uniforms are `show_normals` and `show_noise`. In C++ it's quite easy to hook up your custom shader, textures, and uniforms (snippet from the [wavelets sample](https://github.com/RandyGaul/cute_framework/blob/master/samples/shallow_water.cpp)): If you want to learn about the fundamentals of writing shader code in CF take a look at the low-level graphics page here for an overview: [Low Level Graphics](https://randygaul.github.io/cute_framework/#/topics/low_level_graphics). This page assumes you know the basics of writing GLSL code to hook up to CF's draw API.

```cpp
render_settings_push_shader(shader);
render_settings_push_texture("wavelets_tex", canvas_get_target(offscreen));
render_settings_push_texture("noise_tex", noise_tex);
render_settings_push_texture("scene_tex", canvas_get_target(scene_canvas));
render_settings_push_uniform("show_noise", show_noise ? 1.0f : 0.0f);
render_settings_push_uniform("show_normals", show_normals ? 1.0f : 0.0f);
draw_push_antialias(false);
draw_box(V2(0,0), (float)W, (float)H);
```

The wavelets effects are drawn off-screen into render target textures. These are super easy to setup with either a `CF_Canvas`, or a more low-level option of creating the texture yourself with [cf_make_texture](https://randygaul.github.io/cute_framework/#/graphics/cf_make_texture).

Make a canvas like so:

```cpp
CF_Canvas offscreen = make_canvas(canvas_defaults(160, 120));
```

Then render to it like so (after calling draw functions to queue up sprites/shapes to draw):

```cpp
render_to(offscreen);
```

The canvas's internal texture can be sent to a shader as a uniform with [canvas_get_target](https://randygaul.github.io/cute_framework/#/graphics/canvas_get_target), just as in one of the code snippets above detailing uniforms/textures.

## Loading Shaders

First you must call [cf_shader_directory](https://randygaul.github.io/cute_framework/#/graphics/cf_shader_directory) to tell the application where your shaders reside on disk. Then you may call [cf_make_draw_shader](https://randygaul.github.io/cute_framework/#/draw/cf_make_draw_shader) to create a shader compatible with [cf_render_settings_push_shader](https://randygaul.github.io/cute_framework/#/draw/cf_render_settings_push_shader). You may then optionally setup a callback via [cf_shader_on_changed](https://randygaul.github.io/cute_framework/#/graphics/cf_shader_on_changed) to receive notifications when shaders change on-disk, in order to support shader live-reloading during development.

Once done your custom shader will be able to apply itself to anything drawn through CF's draw API! A good example is the [metaballs sample](https://github.com/RandyGaul/cute_framework/blob/master/samples/metaballs.cpp))
