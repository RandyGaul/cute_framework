# Drawing

Getting things on the screen with CF can be done in one of two ways: [Low Level Graphics](../topics/low_level_graphics.md), or the [Draw API Reference](../api_reference.md#draw). This page showcases the latter, the recommended option for getting shapes, sprites and text onto the screen. Text has it's own dedicated [Text API Reference](../api_reference.md#text), discussed here as well.

## Drawing Shapes

CF can render a variety of shape types:

- Circle
- Box/Aabb
- Triangle
- Line segment
- Polyline
- Polygon (fill)
- Capsule
- Bezier polyline

The shape renderer in CF has a few extra features that nearly all shapes take advantage of:

- Customizeable antialiasing
- Border stroke vs fill style
- Edge rounding (chubbiness)

For circles, use [`cf_draw_circle`](../draw/cf_draw_circle.md), for boxes/rectangles use [`cf_draw_quad`](../draw/cf_draw_quad.md), for lines use [`cf_draw_line`](../draw/cf_draw_line.md) or [`cf_draw_polyline`](../draw/cf_draw_polyline.md), and so on.

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
	draw_push_shape_aa(1.5f);
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
- shape antialias (0 = off, non-zero = on at that scale, default 1.5)
- layer
- chubbiness
- shader
- render state (blend modes/stencil)

Whenever a setting is pushed it will be used by subsequent drawing functions. For example, if we push a color with [`cf_draw_push_color`](../draw/cf_draw_push_color.md) it will get used until a new setting is pushed or popped. When we pop a setting the previously pushed state is restored. This is a great way to use your own settings locally, and then restore anything previous without messing up the settings for the rest of your code. You may nest push/pop pairs as many times as needed.

## Draw Layer

The layer controls the order things are drawn. You can set what layer to draw upon with [`cf_draw_push_layer`](../draw/cf_draw_push_layer.md). When done, restore the previously used layer with [`cf_draw_pop_layer`](../draw/cf_draw_pop_layer.md).

## Drawing Sprites

Sprites can be loaded with either .ase/.aseprite files or .png files. The recommended method is .ase files called [Aseprite](https://www.aseprite.org/) files. An aseprite file contains all the animation and image data necessary for a 2D frame based animations. If instead you want to support your own custom animation format, or any other format, you can load up your animation data and feed it into the [.png API](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_png_cache.h).

Some particular pages of interest are:

- [Sprite API Reference](../api_reference.md#sprite)
- [CF_Sprite](../sprite/cf_sprite.md)
- [cf_sprite_play](../sprite/cf_sprite_play.md)

CF comes with a convenience function called [`cf_make_demo_sprite`](../sprite/cf_make_demo_sprite.md). This sprite contains a small pixel art girl with a couple built-in animations. Here's a program to load her up and draw her on screen:

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

The sprite may have a local origin to offset itself whenever drawn. Set the `offset` member of any [CF_Sprite](../sprite/cf_sprite.md) struct. If the sprite has a [slice](https://www.aseprite.org/docs/slices/) on a particular frame with the `pivot` checkbox marked, the pivot will be recorded for that frame applied, in addition to the sprite's `offset`, to draw relative to that frame's pivot.

Be sure not to author your aseprite files with more than one slice on a given frame marked as pivot, otherwise the pivot data will overwrite one another when loading.

## Drawing Text

Text has it's own [Text API Reference](../api_reference.md#text). Call [`cf_make_font`](../text/cf_make_font.md) to load up a font file, then call [`cf_draw_text`](../text/cf_draw_text.md) to draw text. Text has a whole bunch of settings, such as:

- [`cf_push_text_wrap_width`](../text/cf_push_text_wrap_width.md)
- [`cf_push_font_size`](../text/cf_push_font_size.md)
- [`cf_push_font_blur`](../text/cf_push_font_blur.md)

> [!NOTE]
> Recall that each push function has associated peek and pop APIs! See the [Text API Reference](../api_reference.md#text) for a full list of text related pages.

Here's a [sample](https://github.com/RandyGaul/cute_framework/blob/master/samples/text_drawing.cpp) for drawing some text onto the screen.

<p align="center">
<img src=https://github.com/RandyGaul/cute_framework/blob/master/assets/text_drawing.gif?raw=true>
</p>

You can see the [Text Effect](../text/cf_text_effect_register.md) system in work. Text codes that look sort of like xml are supported for a variety of built-in effects. Click the previous link to see some documentation about built-in text effects, and how to contruct + register your own custom text effect codes.

> [!NOTE]
> The position of rendering text is the top-left corner of the text.

## Shaders

You can apply customizable shaders that work with the draw API by using functions like [cf_draw_push_shader](../draw/cf_draw_push_shader.md)
 and [cf_draw_pop_shader](../draw/cf_draw_pop_shader.md). These shaders are written in glsl version 450. By creating custom FX you can implement interesting visuals like the following wavelet example:

<p align="center">
<img src=https://github.com/RandyGaul/cute_framework/blob/master/assets/wavelets.gif?raw=true>
</p>

The draw API passes *all* geometry into an optional shader function, within the fragment shader, called `shader`. This function is the final step in the entire fragment shader, granting the opportunity to alter the final output pixel color. Let us look at the custom shader skeleton/stub (does no-op).

```glsl
vec4 shader(vec4 color, vec2 pos, vec2 screen_uv, vec4 params)
{
	return vec4(mix(color.rgb, params.rgb, params.a), color.a);
}
```

The `color` param is the color that would be rendered if you don't make any modifications to it within the `shader` function. If drawing a sprite this would be the color of a particular pixel from the sprite's texture, or, if drawing a shape the color of the shape itself.

`pos` was the rendering position used when calling an associated draw function like `cf_draw_sprite`.

For pixel art games it's important to sample using the function `smooth_uv`, something like so: `smooth_uv(v_uv, u_texture_size)` to generate a uv coordinate that will scale pixel art correctly.

`screen_uv` is a position relative to the screen, where (0,0) is the top-left, and (1,1) is the bottom-right of the screen.

`params` are four optional floats. They come from vertex attributes set by [cf_draw_push_vertex_attributes](../draw/cf_draw_push_vertex_attributes.md). Each different item drawn through CF's draw API will attach the previously pushed attributes onto their vertices. These four floats are general-purpose, and only used to pass into the `shader` function. Use them to pack information to implement your own custom visual FX, such as color tinting, or anything else on a per-object basis.

You may also add in uniforms and textures as-needed. The draw API has some functions for setting uniforms and textures via [cf_draw_set_uniform](../draw/cf_draw_set_uniform.md) and [cf_draw_set_texture](../draw/cf_draw_set_texture.md). These will get auto-magically hooked up and send values to your shader. When you add in your own uniforms just be sure to place them inside of a uniform block like in the below sample (see `shd_uniforms`, and don't change this name either! It must be called `shd_uniforms`).

Shaders have access to some "hidden" environment variables. In particular you have access to:

- `v_uv` if you're drawing a canvas with [cf_draw_canvas](../draw/cf_draw_canvas.md) it's often very helpful to sample from the canvas. You may do this via: `texture(u_image, v_uv)`.
- `u_image` if you're drawing a canvas with [cf_draw_canvas](../draw/cf_draw_canvas.md) can be quite useful if you need to, for any reason, sample the canvas. See the above point on `v_uv`.
- `u_texture_size` if you're drawing a canvas with [cf_draw_canvas](../draw/cf_draw_canvas.md) is sometimes useful for certain algorithms that need to calcualte texel sizes, or know the size of the texture they are sampling from.

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

vec4 shader(vec4 color, vec2 pos, vec2 screen_uv, vec4 params)
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

> [!NOTE]
> Custom shaders require a specific ordering for resource sets (the `set = N` part). Texture samplers `uniform sampler2D` must have `set = 2` and bindings start at index 1 (`binding = 1`), while the uniform block *must be named* `shd_uniforms` an have `set = 3, binding = 1`. Generally speaking you can just copy + paste this example and easily get away with incrementing the `binding = N` for textures. To add in more uniforms simply add more members to the `shd_uniforms` block.

The custom textures are `wavelets_tex`, `noise_tex` and `scene_tex`. The custom uniforms are `show_normals` and `show_noise`. In C++ it's quite easy to hook up your custom shader, textures, and uniforms (snippet from the [wavelets sample](https://github.com/RandyGaul/cute_framework/blob/master/samples/shallow_water.cpp)): If you want to learn about the fundamentals of writing shader code in CF take a look at the low-level graphics page here for an overview: [Low Level Graphics](../topics/low_level_graphics.md). This page assumes you know the basics of writing GLSL code to hook up to CF's draw API.

```cpp
draw_push_shader(shader);
draw_set_texture("wavelets_tex", canvas_get_target(offscreen));
draw_set_texture("noise_tex", noise_tex);
draw_set_texture("scene_tex", canvas_get_target(scene_canvas));
draw_set_uniform("show_noise", show_noise ? 1.0f : 0.0f);
draw_set_uniform("show_normals", show_normals ? 1.0f : 0.0f);
draw_push_shape_aa(0);
draw_box(V2(0,0), (float)W, (float)H);
```

The wavelets effects are drawn off-screen into render target textures. These are super easy to setup with either a `CF_Canvas`, or a more low-level option of creating the texture yourself with [cf_make_texture](../graphics/cf_make_texture.md).

Make a canvas like so:

```cpp
CF_Canvas offscreen = make_canvas(canvas_defaults(160, 120));
```

Then render to it like so (after calling draw functions to queue up sprites/shapes to draw):

```cpp
render_to(offscreen);
```

The canvas's internal texture can be sent to a shader as a uniform with [canvas_get_target](../graphics/cf_canvas_get_target.md), just as in one of the code snippets above detailing uniforms/textures.

## Loading Shaders

First you must call [cf_shader_directory](../graphics/cf_shader_directory.md) to tell the application where your shaders reside on disk. Then you may call [cf_make_draw_shader](../draw/cf_make_draw_shader.md) to create a shader compatible with [cf_draw_push_shader](../draw/cf_draw_push_shader.md). You may then optionally setup a callback via [cf_shader_on_changed](../graphics/cf_shader_on_changed.md) to receive notifications when shaders change on-disk, in order to support shader live-reloading during development.

Once done your custom shader will be able to apply itself to anything drawn through CF's draw API! A good example is the [metaballs sample](https://github.com/RandyGaul/cute_framework/blob/master/samples/metaballs.cpp)).

<p align="center">
<img src=https://github.com/RandyGaul/cute_framework/blob/master/assets/metaballs.gif?raw=true>
</p>

## Compiling Shaders

While the above is all you need to get started with shaders, to learn more about shader compilation, refer to [Shader Compilation](../topics/shader_compilation.md).
