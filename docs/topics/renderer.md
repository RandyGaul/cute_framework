# A Tour of Cute Framework's Renderer

The renderer in CF (Cute Framework) has grown into something quite unique since its inception. Let us go through a tour from the ground up and cover the design and implementation of a novel, full-featured, high-performance, cross-platform 2D renderer. Yes, many buzzwords. Let's go.

# Sprites

The [original idea](https://github.com/RandyGaul/tinycavestory) of CF's renderer was to provide a way to push sprite structs into a buffer and have them get "drawn" in as few draw calls as possible; a pretty typical concept.

<p align="center">
<img src=https://github.com/RandyGaul/tinycavestory/raw/master/screenshots/tinycavestory_slide.gif?raw=true>
</p>

I really wanted to push this concept farther, and really embrace *truly* representing sprites as POD (Plain Old Data) structs, even to the point where the on-disk representation simply doesn't matter, at least in terms of run-time performance of the renderer. I wanted to throw any sprite at the API and have it "just work", enabling whatever kind of wacky feature you could possibly come up with.

<p align="center">
<img src=https://github.com/RandyGaul/tinycavestory/raw/master/screenshots/tinycavestory_live_edit.gif?raw=true>
</p>

# Don't Reinvent the Wheel

Don't reinvent the wheel! Seriously, what are you thinking wasting time drawing sprites? That's a solved problem.

Is it though? Truly a solved problem? [DRY principle](https://en.wikipedia.org/wiki/Don%27t_repeat_yourself) is another fun one. It turns out some psychology can be fun, and go a long way. If we take a look at the movie Pinocchio we find Gepetto, Pinocchio's father, sitting in a whale attempting to fish for food.

<p align="center">
<img src=https://github.com/RandyGaul/cute_framework/blob/master/assets/gepetto.png?raw=true>
</p>

This scene lays out a metaphor for the psychological story of how an up-and-coming son must save his father to actualize himself through life. The father represents order, structure, and the past. He knows how to make puppets, and fish (apparently), and uses the skills and knowledge of the past to navigate present tense. However, over time this crystalized structure of knowledge can become outdated, no longer sufficient to deal with the landscape evolving underneath it. And so, Geppetto doesn't even acknowledge the whale or his situation, simply living on in delusion without addressing the problem at-hand.

We can be like Pinocchio. We can recast the old traditions into a present day context, learn something in the process, and perhaps a new solution will unfold. Pinnocchio does eventually make his way into the whale to save his father. More on that later.

# Rethinking Sprites

If we consider the traditional method of efficiently drawing sprites you will find endless examples of sprite texture packing, also called texture atlases. Here's a pretty nice [blog post](https://www.ohsat.com/tutorial/flixel/using-texture-atlas/index.php) going over the idea in the context of [Flixel](https://haxeflixel.com). Love2D (a highly recommended game framework, by the way!) has their own [spritebatch API](https://love2d.org/wiki/SpriteBatch) centered around rendering glyphs from a single atlas. Noel Berry from Celeste also likes using a very similar `Batcher` API seen [here on GitHub](https://github.com/FosterFramework/Foster/blob/main/Framework/Graphics/Batcher.cs) in his Foster C# framework.

The purpose of the atlases and batchers is to reduce *draw calls*, an expensive operation where sprites are drawn on the GPU. When submitting a [draw call](https://toncijukic.medium.com/draw-calls-in-a-nutshell-597330a85381) quite a lot of work goes into the operation, making it a fairly high-latency and expensive thing to do. However, as a developer we can pack many sprites into a single draw call, minimizing the high constant cost of each draw call. The number of sprites a draw call is limited to drawing is primarily restricted by the number of textures a draw call can reference (typically implemented by referencing just one texture at a time for simplicity).

However, constructing atlases as an offline or preprocessing step has proven a popular paradigm, even today. This was done in the past as an optimization when computers were much less efficient than they are by today's standards, and had much less memory. Today, especially for 2D games, many games can easily get away with storing their textures in RAM and simply construct atlases on-the-fly based on what is actually being rendered at the time. There is no practical reason why atlases must be packed on disk or defined up-front and fed through our drawing API.

Instead, a novel idea of utilizing higher RAM capabilities of today lets us hide texture atlases entirely from the user. We can simply let the user push sprites into a buffer, and upon drawing scan the buffer for new sprites, inject them into atlases, and voila - draw call counts can be reduced to single digits no matter what.

So what would a full program for drawing sprites look like? Just make the sprite, and draw it. Done.

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

# Online Sprite Compiler

Let us take a moment to sketch out the API for sprites real quick.

```cpp
struct CF_Sprite
{
	int w, h;
	float x, y;
	float angle;
	uint64_t id;
};

void draw_sprite(CF_Sprite sprite);
```

Pretty cool, eh? Let us go over some pro/con analysis of the online sprite compiler design:

1. Nobody needs to really think much about atlases. Artists can export their images however they like. When drawing sprites you can just... Draw the sprite. No need to consider a batch, or atlas, UV, or any other associated plumbing. Just draw the sprite.
2. The online atlas API can compile sprites together based on what you're actually rendering at the time. This is really cool over the development of a game as you don't need to spend any energy thinking about what sprites will be drawn and when, trying to manually pack them into the same atlas to save a few draw calls.
3. The atlas compiler can decay sprites out of atlases automatically when they cease to be drawn for long enough.
4. Some fascinating benefits with text rendering, to be covered later.

And here are some cons of the design:

1. More RAM consumption. To efficiently recompile atlases on-the-fly textures sit in RAM until they're needed. The user may want to also prefetch sprites into RAM to avoid hitting the disk mid-game (though, less of a concern today with how prolific SSD's are).
2. Rendering very large images can be a bit annoying, especially if they don't fit within the atlas compilers internal dimensions. It may be best to just render high resolution images in a custom way, as opposed to fitting them into CF's renderer (we're talking like 2000x2000 pixels or higher). If you have a lot of these high res images it may be best to also make your own streaming/caching mechanism to deal with RAM limitations.
3. Opening many individual files on some OS's (looking at you, Windows) can be really slow due to slowly implemented security checks. This is totally out of our direct control, but, can be easily avoidable by using a tool to open invidual files out of an archive, like a .zip file.

You can find the guts of the atlas compiler here, in a [single-file C header called cute_spritebatch.h](https://github.com/RandyGaul/cute_framework/blob/master/libraries/cute/cute_spritebatch.h). It's managing the rolling atlas cache itself, and firing a variety of callbacks back to the user to fetch pixels, make textures, or report batches.

As promised, we can be more like Pinocchio, and escape the whale one sprite at a time.

<p align="center">
<img src=https://github.com/RandyGaul/cute_framework/blob/master/assets/whale_escape.png?raw=true>
<em><a href="https://www.mouseplanet.com/gallery/v/PersonalContributions/cbarry/Pinocchio+Behind+Scenes.jpg.html">image source</a></em>
</p>

Conclusion? If you're still packing sprites into atlases offline then you're getting swallowed by the whale. Just kidding! It's a totally valid solution to prebake atlases and render sprites this way; it's just there are other interesting things to try out as well if one were so inclined :)

# Animating Sprites

Upgrading sprites to deal with flipbook style animations is a broad topic with many solutions. For CF I've chosen to natively support [Aseprite](https://www.aseprite.org). In the indie dev scene (and generally the 2D gamedev scene) Aseprite has more or less become the de facto tool of choice (besides perhaps some Photoshop lovers). Aseprite has its own binary format. CF just [parses these files directly](https://github.com/RandyGaul/cute_framework/blob/master/libraries/cute/cute_aseprite.h) and fills in animation tables that sprites can point to. Credits to Noel Berry from Celeste for the original idea and implementation of the .ase file loader.

Of course, other formats can be supported, as CF provides some lower level APIs to manually construct sprites by setting up [arrays of .png files](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_png_cache.h). This lets users who already have their own custom art pipeline or flow integrate with CF in their own way.

# Shapes

Drawing shapes is not easy. Just try thinking about how to antialias (aa) the edges of a circle and enjoy tearing your hair out. Your aa needs to also work regardless of camera zoom, window dimensions, and not subject itself to weird alias acne artifacts.

Peeking into other open source projects reveals a plethora of hand-rolled triangulations. The typical strategy is to perform a bunch of math to figure out how to make many tiny feather triangles around the rim of shapes and draw them at half-opacity to produce antialiased pixels. However, this is super error-prone, tedious, produces a lot of code, and most important produces a lot of geometry to submit to the GPU.

Instead, a novel and effective approach is to steal all of [Inigo Quilez's work](https://iquilezles.org/articles/distfunctions2d) and draw shapes using SDF's (signed distance function, NOT signed distance field). These SDF's are cool because you can get fragment shaders to just power through them without a hitch. These SDF's work by defining a function to, given a pixel, compute the distance to that pixel as a signed distance value. This tells you how far from the shapes surface you're in, perfect for rendering shapes with perfect antialiased edges.

If you crawl through [CF's internal shaders](https://github.com/RandyGaul/cute_framework/blob/master/src/cute_shader/builtin_shaders.h) you will find a whole bunch of similar SDF functions and many ternary operators. It turns out compilers these days just simply evaluate both sides of simple ternary operators and branchlessly select the correct result, with these cmov or select style instructions. Or in other words, it go fast, like Sonic fast.

In practice this kind of SDF rendering will be limited by [pixel overdraw](https://forum.playcanvas.com/t/pixel-overdraw/29442) as the first bottleneck, assuming your draw call counts are low and you aren't flooding the upload limit to the GPU. So, when dealing with SDF rendering the big thing is to try and only submit quads to the GPU that tightly wrap around the shape to draw. Here's a [little program](https://gist.github.com/RandyGaul/c8abb1793d9d93e767b812ec9e636ea9) I made to prototype wrapping an oriented triangle in a tightly fitting box, while also giving some optional padding pixels.

```c
void bounding_box_of_triangle(v2 a, v2 b, v2 c, float radius, v2* out)
{
	v2 ab = b - a;
	v2 bc = c - b;
	v2 ca = a - c;
	float d0 = dot(ab, ab);
	float d1 = dot(bc, bc);
	float d2 = dot(ca, ca);
	auto build_box = [](float d, v2 a, v2 b, v2 c, float inflate, v2* out) {
		float w = sqrtf(d);
		v2 u = (b - a) / w;
		v2 v = skew(u);
		float h = dot(v, c) - dot(v, a);
		if (h < 0) {
			h = -h;
			v = -v;
		}
		out[0] = a - u * inflate - v * inflate;
		out[1] = b + u * inflate - v * inflate;
		out[2] = b + u * inflate + v * (inflate + h);
		out[3] = a - u * inflate + v * (inflate + h);
	};
	if (d0 >= d1 && d0 >= d2) {
		build_box(d0, a, b, c, radius, out);
	} else if (d1 >= d0 && d1 >= d2) {
		build_box(d1, b, c, a, radius, out);
	} else {
		build_box(d2, c, a, b, radius, out);
	}
}
```

<p align="center">
<img src=https://user-images.githubusercontent.com/1919825/211174135-3c6932ca-85ad-4a02-96a5-acbbf0c91174.png>
</p>

As long as we can wrap whatever shape we're rendering in a tight oriented box, we can get pretty good performance by optimizing against the likely bottleneck of pixel overdraw.

# Shapes on the GPU

Getting the shapes onto the GPU to actually call into SDF functions involves packing shapes in world space into vertex attributes for our given quad. Recall the quad itself tightly wraps whatever shape we're rendering (like a circle, capsule, triangle, box, etc.), so that means 2 triangles or 6 vertices. The reason we're wrapping the rendered shape in a quad is it's just a simple way to make sure the fragment shader runs for each pixel of the shape in question. The fragment shader evaluates how far each pixel is from the actual shape, and fills in the color depending on if it's inside or outside of the shape, by using the SDF function we mentioned earlier.

Each vertex uploaded to the GPU should look something like this:

```c
struct Vertex
{
	float x, y;
	int shape_type;
	float ax, ay;
	float bx, by;
	float cx, cy;
	// ...
};
```

The vectors `ax, ay` and `bx, by` etc. are used to pack the rendered shape itself into the vertex attributes, and are duplicated across all 6 quad vertices. For example, a sphere could pack its position in `ax, ay` while the radius could be packed into `bx`, while all the other attributes may simply remain unused for sphere rendering (but used for other more complex shapes like polylines). This lets us use the same vertex layout for all shapes, allowing us to compress shapes into a single draw call. Actually, we render sprites/text with the same vertex layout, compressing everything into a single draw call.

The fragment shader needs to look at `shape_type` and do an *actual branch* depending on what shape is to be drawn. Here's the actual source for CF's fragment shader SDF dispatch:

```glsl
void main()
{
	bool is_sprite  = v_type >= 0.0 && v_type < 0.5;
	bool is_text    = v_type >  0.5 && v_type < 1.5;
	bool is_box     = v_type >  1.5 && v_type < 2.5;
	bool is_seg     = v_type >  2.5 && v_type < 3.5;
	bool is_tri     = v_type >  3.5 && v_type < 4.5;
	bool is_tri_sdf = v_type >  4.5 && v_type < 5.5;
	bool is_poly    = v_type >  5.5 && v_type < 6.5;

	// Traditional sprite/text/tri cases.
	vec4 c = vec4(0);
	c = !(is_sprite && is_text) ? de_gamma(texture(u_image, smooth_uv(v_uv, u_texture_size))) : c;
	c = is_sprite ? gamma(c) : c;
	c = is_text ? v_col * c.a : c;
	c = is_tri ? v_col : c;

	// SDF cases.
	float d = 0;
	if (is_box) {
		d = distance_box(v_pos, v_ab.xy, v_ab.zw, v_cd.xy);
	} else if (is_seg) {
		d = distance_segment(v_pos, v_ab.xy, v_ab.zw);
		d = min(d, distance_segment(v_pos, v_ab.zw, v_cd.xy));
	} else if (is_tri_sdf) {
		d = distance_triangle(v_pos, v_ab.xy, v_ab.zw, v_cd.xy);
	} else if (is_poly) {
		pts[0] = v_ab.xy;
		pts[1] = v_ab.zw;
		pts[2] = v_cd.xy;
		pts[3] = v_cd.zw;
		pts[4] = v_ef.xy;
		pts[5] = v_ef.zw;
		pts[6] = v_gh.xy;
		pts[7] = v_gh.zw;
		d = distance_polygon(v_pos, pts, v_n);
	}
	c = (!is_sprite && !is_text && !is_tri) ? sdf(c, v_col, d - v_radius) : c;

	c *= v_alpha;
	vec2 screen_uv = (v_posH + vec2(1,-1)) * 0.5 * vec2(1,-1);
	c = shader(c, v_pos, screen_uv, v_user);
	if (u_alpha_discard != 0 && c.a == 0) discard;
	result = c;
}
```

WAIT A MINUTE -- Won't this blow up the GPU? If-statements are slow!!!

Are if-statements truly slow? When was the last time you wrote and if-statement in a fragment shader and then actually observed the performance? When writing CF I realized my own personal answer was "never". And so, in cowboy fashion, a fragment shader was whipped up from the dust, along with some bonafide hip-firing, and SDFs were dispatched naively from a big if-else chain.

GPUs these days can be understood as stamping out blocks of pixels in typical 32 or 64 size. As long as block of pixels associated with one of these hardware stamps is all executing the same branch-path you will get good performance. It turns out, in practice, games tend to draw a lot of the same shape in the same area, such as particles or sprites. Conclusion: worrying about warp/wavefront convergence is a total non-issue in terms of perf cost (in this context).

# Shape AA

By drawing with an SDF we know exactly how far a pixel is from the shape's surface. To AA all we then need to do is call something like `smoothstep` with some tune'd parameters to get perfect AA. The best part? You can write a single function to perfectly AA *any shape*, and also add in options for fill and stroke (annularize).

```glsl
// Given two colors a and b, and a distance to the isosurface of a shape,
// apply antialiasing, fill, and surface stroke FX.
vec4 sdf(vec4 a, vec4 b, float d)
{
	float wire_d = sdf_stroke(d);
	vec4 stroke_aa = mix(b, a, smoothstep(0.0, v_aa, wire_d));
	vec4 stroke_no_aa = wire_d <= 0.0 ? b : a;

	vec4 fill_aa = mix(b, a, smoothstep(0.0, v_aa, d));
	vec4 fill_no_aa = clamp(d, -1.0, 1.0) <= 0.0 ? b : a;

	vec4 stroke = mix(stroke_aa, stroke_aa, v_aa > 0.0 ? 1.0 : 0.0);
	vec4 fill = mix(fill_no_aa, fill_aa, v_aa > 0.0 ? 1.0 : 0.0);

	result = mix(stroke, fill, v_fill);
	return result;
}
```

Simply call your SDF, feed the distance into this here fancy (and badly named) `sdf` function to apply AA/stroke/fill options.

To deal with camera zoom CF chose to pass in an `antialias_factor`, or dubiously named `aaf` to automagically scale the shader's tuning params based on user inputs and camera zoom, to ensure the correct number of pixels have AA applied on them.

```cpp
// Sets the anti-alias factor, the width of roughly one pixel scaled.
// This factor remains constant-size despite zooming in/out with the camera.
void CF_Draw::set_aaf()
{
	float inv_cam_scale = 1.0f / len(draw->cam_stack.last().m.y);
	float scale = draw->antialias.last();
	aaf = scale * inv_cam_scale;
}
```

# Shape Rounding, Stroke, and Fill

We are talking about shape rendering. Get your mind out of the gutter. Since the shapes are rendered with a signed-distance we can normalize an API to expose rounding, stroke vs fill rendering, and AA. All shapes can have rounding applied. All shapes can render fill vs stroke (line rendering), or have AA on/off. This is really a novel feature of CF. If you look into the details of pretty much any other open source 2D renderer you'll get sprites, you'll get shapes, you'll get some AA, but you won't get a unified set of these features on all shapes. Typically just boxes will have rounding, and perhaps for AA you'll get lines with local AA, and then have to turn on [MSAA or some other global option](https://developer.valvesoftware.com/wiki/Anti-aliasing#MSAA) for the rest of your AA needs. To list this out we have:

- Shape local AA
- Shape rounding
- Shape annular/stroke
- Shape fill

And that's for *all shape types*:

- Circle
- Capsule (line)
- Polyline
- Box, oriented or AABB
- Polygon
- Triangle

# Text Rendering

The text rendering works in CF by piggy-backing off of the sprite implentation. By using [stb_truetype.h](https://github.com/nothings/stb/blob/master/stb_truetype.h) individual text glyphs can be rasterized on-demand. This works perfectly for loading up sprite as-needed and feeding them into the atlas compiler. Perfect! This is actually a highly novel way to render text and leads to the most powerful style of text rendering I've ever seen for raster-glyphs in games.

Typically games will lay out what glyphs they can render up-front in order to build atlases as a precompilation step. This is horrible, especially when considering localization! Each time you write new text into your game you have to have a complex system to regenerate and verify you have all those glyphs available beforehand. This makes it extra difficult for anyone to rapidly prototype, and also lends itself to wasting space if you end up cutting certain glyphs later.

It's also really challenging to support user chat systems where they could type in all kinds of arbitrary characters. Instead, the atlas compiler simply treats glyphs as any other sprite, and batches them together based only on what you're currently using and rendering (or have prefetched).

If we look at one of the more advanced open source solutions for text, [fontstash](https://github.com/memononen/fontstash), you can see the beginnings of a good solution -- simply grow a *single* atlas downwards, on-demand, whenever glyphs are used. However, it doesn't decay glyphs out of the atlas ever, meaning for larger codepoint sets you can simply overflow hardware limits on resolution. For 2D text games this is actually pretty easy to do once you start bolding, italicizing, or blurring glyphs with individual rasters. CF doesn't have this problem, as the online compiler handles all of those cases by design.

Just try drawing all of this with one draw call and a dozen lines of code in another engine/framework:

<p align="center">
<img src=https://github.com/RandyGaul/cute_framework/blob/master/assets/text_drawing.gif?raw=true>
</p>

And if you get that in a single draw call, add in sprites and shape rendering, and *then see* if you're still at a single draw call.

Now the complexity of rendering text lies entirely in the layout of the glyphs on screen. Things like word-wrapping, vertical vs horizontal layout, measuring the width/height of a text blurb, applying custom markup effects on text (like shaking, or dynamically glowing) are the focus. CF provides APIs for all of these in it's [draw API](https://randygaul.github.io/cute_framework/topics/drawing). All the gory implementation details are in [this file here](https://github.com/RandyGaul/cute_framework/blob/master/src/cute_draw.cpp) (search for `s_draw_text`).

# SDL_Gpu

CF renders through `SDL_Gpu`(https://wiki.libsdl.org/SDL3/CategoryGPU), a well-written abstraction over DX11, DX12, Metal, and Vulkan. This gives us future-proof access to efficient hardware acceleration. Internally SDL_Gpu is super complex, but, exposes an API centered around shaders, buffers, and passes. Here's a [good tutorial/blog on learning SDL_Gpu basics](https://hamdy-elzanqali.medium.com/let-there-be-triangles-sdl-gpu-edition-bd82cf2ef615) if you're interested. CF wraps SDL_Gpu in it's own cross-platform [lowish level graphics API here](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_graphics.h).

```c
/**
 * This header wraps low-level 3D rendering APIs. You're probably looking for other headers in Cute
 * Framework, not this one. This header is for implementing your own custom rendering stuff, and is
 * intended only for advanced users.
 *
 * If you want to draw sprites, lines/shapes, or text, see: cute_draw.h
 *
 * Quick list of unsupported features. CF's focus is on the 2D use case, so most of these features are
 * omit since they aren't super useful for 2D.
 *
 *     - Blend color constant
 *     - Multiple render targets (aka color/texture attachments)
 *     - Cube map
 *     - 3D textures
 *     - Texture arrays
 *
 * The basic flow of rendering a frame looks something like this:
 *
 *     for each canvas {
 *         cf_apply_canvas(canvas);
 *         for each mesh {
 *             cf_mesh_update_vertex_data(mesh, ...);
 *             cf_apply_mesh(mesh);
 *             for each material {
 *                 cf_material_set_uniform_vs(material, ...);
 *                 cf_material_set_uniform_fs(material, ...);
 *                 for each shader {
 *                     cf_apply_shader(shader, material);
 *                     cf_draw_elements(...);
 *                 }
 *             }
 *         }
 *     }
 */
```

The API is largely centered around constructing shaders [CF_Shader](https://randygaul.github.io/cute_framework/graphics/cf_make_shader), canvases ([CF_Canvas](https://randygaul.github.io/cute_framework/graphics/cf_make_canvas)), meshes ([CF_Mesh](https://randygaul.github.io/cute_framework/graphics/cf_make_mesh), materials ([CF_Material](https://randygaul.github.io/cute_framework/graphics/cf_make_material)), and textures ([CF_Texture](https://randygaul.github.io/cute_framework/graphics/cf_make_texture)). The shader itself is constructed originally from input GLSL 450, and is cross-compiled to SPIRV internally, and then again compiled by backend vendors into the actual hardware shader. SDL comes with some helper tools to assist this process, namely [SDL_Shadercross](https://github.com/libsdl-org/SDL_shadercross).

The canvas holds a texture CF can render to. The material holds global variable inputs to the shader (uniforms), while the mesh holds vertex inputs to the shader (attributes). You load up materials by specifing key-value pairs. Typically values are floats, vectors, or matrices, while keys are names of uniforms (global variables) in the shader.

Canvas can be drawn to, blitted to one another, have shaders applied (or not) to them, etc. The API is largely canvas-centric.

One nice thing about SDL_Gpu is how Meshes and Textures are dealt with in terms of buffers. These buffers use *staging buffers*, which are blocks of memory we can allocate on the CPU that get sent in-flight to the GPU when needed, and later cycled back. However, on devices with a shared memory model, these can simply be a single spot in memory. The cycling itself is also quite interesting in SDL_Gpu, as any writeable resource (namely textures and mesh buffers) will automagically allocate a ring-buffer of resources under the hood as-needed to reduce inter-frame dependencies. This means we get some parallelism for "free" without having to implement our own double/triple buffering systems.

# Web Builds

SDL_Gpu doesn't have a web compatible backend. So, CF implements its own GLES3 backend to get web support. It simply implements another backend for all of the graphics API we covered in the prior topic. Here's the docs on [getting web builds going](https://randygaul.github.io/cute_framework/topics/emscripten), great for those itch.io uploads or random playtests by hosting on your own site.

GLES3 doesn't have a fancy cycling system like SDL_Gpu, so we implemented our own in CF. Whenever a mesh or texture is updated we simply create a new internal instance of the resource on a ring buffer as-needed, up to 3 resources (hardcoded). This implements on-demand triple buffering for whatever resource is getting updated. Pretty cool. Here's what some of the internal guts looks like for managing these ring buffers:

```cpp
static GLenum s_poll_fence(GLsync fence)
{
	if (!fence) return GL_ALREADY_SIGNALED;
	return glClientWaitSync(fence, GL_SYNC_FLUSH_COMMANDS_BIT, 0);
}

static CF_GL_Slot* s_acquire_slot(CF_GL_Ring* ring, uint32_t frame, int* out_index)
{
	int count = ring->count;
	for (int tries = 0; tries < count; ++tries) {
		int index = (ring->head + tries) % count;
		CF_GL_Slot& slot = ring->slots[index];
		GLenum status = s_poll_fence(slot.fence);
		if (status == GL_ALREADY_SIGNALED || status == GL_CONDITION_SATISFIED) {
			if (slot.fence) {
				glDeleteSync(slot.fence);
				slot.fence = 0;
			}
			ring->head = (index + 1) % count;
			slot.last_use_frame = frame;
			if (out_index) *out_index = index;
			return &slot;
		}
	}
	if (count < RING_BUFFER_CAPACITY) {
		int index = count;
		CF_GL_Slot& new_slot = ring->slots[index];
		new_slot = CF_GL_Slot();
		ring->count = count + 1;
		ring->head = (index + 1) % ring->count;
		new_slot.last_use_frame = frame;
		if (out_index) *out_index = index;
		return &new_slot;
	}
	return NULL;
}

static CF_GL_Slot* s_force_slot(CF_GL_Ring* ring, uint32_t frame, int* out_index)
{
	if (!ring->count) return NULL;
	int index = ring->head;
	CF_GL_Slot& slot = ring->slots[index];
	if (slot.fence) {
		// Block the CPU until the GPU is done with this slot.
		// If you're seeing this on the hot-path of a profile or flame-graph it means you're GPU bound.
#ifdef CF_EMSCRIPTEN
		while (slot.fence) {
			GLenum status = s_poll_fence(slot.fence);
			if (status == GL_ALREADY_SIGNALED || status == GL_CONDITION_SATISFIED) {
				glDeleteSync(slot.fence);
				slot.fence = 0;
				break;
			}
			if (status == GL_WAIT_FAILED) {
				glDeleteSync(slot.fence);
				slot.fence = 0;
				break;
			}
			// We can't call glClientWaitSync on WebGL, so we'll try and block to simulate sort what it
			// would otherwise achieve.
			cf_sleep(0);
		}
#else
		glClientWaitSync(slot.fence, GL_SYNC_FLUSH_COMMANDS_BIT, GL_TIMEOUT_IGNORED);
		glDeleteSync(slot.fence);
		slot.fence = 0;
#endif
	}
	slot.last_use_frame = frame;
	ring->head = (index + 1) % ring->count;
	if (out_index) *out_index = index;
	return &slot;
}
```

# Custom Shaders

To create a shader you can check out the [Shader Compilation](https://randygaul.github.io/cute_framework/topics/shader_compilation) page.

When using CF's draw API you can upload [Draw Shaders](https://randygaul.github.io/cute_framework/topics/drawing/#shaders) to the GPU. These are little fragment shaders to operate on shapes or canvases. It looks like this to start with as a pass-through shader (does no-op):

```glsl
vec4 shader(vec4 color, ShaderParams params)
{
    return vec4(mix(color.rgb, params.attributes.rgb, params.attributes.a), color.a);
}
```

Although, can get arbitrarily complex for whatever kind of fragment-shader-related thing you can think of, such as click-water ring effects:

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

vec4 shader(vec4 color, ShaderParams params)
{
    vec2 uv = params.screen_uv;
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

<p align="center">
<img src=https://github.com/RandyGaul/cute_framework/blob/master/assets/wavelets.gif?raw=true>
</p>

Internally this works by injecting shader strings into each before feeding them into CF's shader compiler.

```glsl
// snip ...
layout (set = 3, binding = 0) uniform uniform_block {
	vec2 u_texture_size;
	int u_alpha_discard;
};

#include "blend.shd"
#include "gamma.shd"
#include "smooth_uv.shd"
#include "distance.shd"
#include "shader_stub.shd"

// Used only for polygon SDF.
vec2 pts[8];

void main()
{
	bool is_sprite  = v_type >= 0.0 && v_type < 0.5;
	bool is_text    = v_type >  0.5 && v_type < 1.5;
// snip ...
```

The line `"shader_stub.shd"` is the one in question. By implementing shader include support we can inject a user shader into the `stub` spot. This gets called at the end of CF's fragment shader as a final opportunity for the user to alter the color/rendering of the fragment.

```glsl
	// snip ...
	c = (!is_sprite && !is_text && !is_tri) ? sdf(c, v_col, d - v_radius) : c;

	c *= v_alpha;
	vec2 screen_uv = (v_posH + vec2(1,-1)) * 0.5 * vec2(1,-1);
	c = shader(c, v_pos, screen_uv, v_user);
	if (u_alpha_discard != 0 && c.a == 0) discard;
	result = c;
}
```

If you're familiar with Unity's old surface shaders, this is very similar in concept (hijacking the fragment shader).

# Conclusion

And that's it! The CF renderer design offers a very high-performance batching solution, unifying rendering sprites, shapes, and text all together. It also offers shader customization that can be applied to renderable items (as well as canvases), making for a unique and effective renderer API.
