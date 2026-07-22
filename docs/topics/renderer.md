# A Tour of Cute Framework's Renderer

The renderer in CF (Cute Framework) has grown into something quite unique since its inception. Let us go through a tour from the ground up and cover the design and implementation of a novel, full-featured, high-performance, cross-platform 2D renderer. Yes, many buzzwords. Let's go.

Here's a taste of where this all ends up. This is a night city built from a few *draw lists* -- thousands of buildings, tens of thousands of lit windows, five parallax layers -- recorded once and replayed each frame for about 0.001 milliseconds of CPU time, while a giant moon with a ring of orbiting debris renders live on top using additive, multiply, and screen blend modes. Everything in this frame is drawn by the renderer described on this page.

<p align="center">
<video src="https://raw.githubusercontent.com/RandyGaul/cute_framework/master/assets/city_night.mp4" autoplay loop muted playsinline width="960" style="max-width:100%"></video>
</p>

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

You can find the guts of the atlas compiler here, in a [single-file C header called cute_atlas_cache.h](https://github.com/RandyGaul/cute_framework/blob/master/libraries/cute/cute_atlas_cache.h). It's managing the rolling atlas cache itself, and firing a variety of callbacks back to the user to fetch pixels, make textures, or report batches.

As promised, we can be more like Pinocchio, and escape the whale one sprite at a time.

<p align="center">
<img src=https://github.com/RandyGaul/cute_framework/blob/master/assets/whale_escape.png?raw=true>
<em><a href="https://www.mouseplanet.com/gallery/v/PersonalContributions/cbarry/Pinocchio+Behind+Scenes.jpg.html">image source</a></em>
</p>

Conclusion? If you're still packing sprites into atlases offline then you're getting swallowed by the whale. Just kidding! It's a totally valid solution to prebake atlases and render sprites this way; it's just there are other interesting things to try out as well if one were so inclined :)

# Animating Sprites

Upgrading sprites to deal with flipbook style animations is a broad topic with many solutions. For CF I've chosen to natively support [Aseprite](https://www.aseprite.org). In the indie dev scene (and generally the 2D gamedev scene) Aseprite has more or less become the de facto tool of choice (besides perhaps some Photoshop lovers). Aseprite has its own binary format. CF just [parses these files directly](https://github.com/RandyGaul/cute_framework/blob/master/libraries/cute/cute_aseprite.h) and fills in animation tables that sprites can point to. Credits to Noel Berry from Celeste for the original idea and implementation of the .ase file loader.

Of course, other formats can be supported, as CF provides some lower level APIs to manually construct sprites from individual .png files. See the [Custom Sprites](custom_sprites.md) topic for details. This lets users who already have their own custom art pipeline or flow integrate with CF in their own way.

# Shapes

Drawing shapes is not easy. Just try thinking about how to antialias (aa) the edges of a circle and enjoy tearing your hair out. Your aa needs to also work regardless of camera zoom, window dimensions, and not subject itself to weird alias acne artifacts.

Peeking into other open source projects reveals a plethora of hand-rolled triangulations. The typical strategy is to perform a bunch of math to figure out how to make many tiny feather triangles around the rim of shapes and draw them at half-opacity to produce antialiased pixels. However, this is super error-prone, tedious, produces a lot of code, and most important produces a lot of geometry to submit to the GPU.

Instead, a novel and effective approach is to steal all of [Inigo Quilez's work](https://iquilezles.org/articles/distfunctions2d) and draw shapes using SDF's (signed distance function, NOT signed distance field). These SDF's are cool because you can get fragment shaders to just power through them without a hitch. These SDF's work by defining a function to, given a pixel, compute the distance to that pixel as a signed distance value. This tells you how far from the shapes surface you're in, perfect for rendering shapes with perfect antialiased edges.

If you crawl through [CF's internal shaders](https://github.com/RandyGaul/cute_framework/blob/master/tools/builtin_shaders.h) you will find a whole bunch of similar SDF functions and many ternary operators. It turns out compilers these days just simply evaluate both sides of simple ternary operators and branchlessly select the correct result, with these cmov or select style instructions. Or in other words, it go fast, like Sonic fast.

In practice this kind of SDF rendering will be limited by [pixel overdraw](https://forum.playcanvas.com/t/pixel-overdraw/29442) as the first bottleneck, assuming your draw call counts are low and you aren't flooding the upload limit to the GPU. So, when dealing with SDF rendering the big thing is to try and only run the fragment shader on pixels that tightly wrap around the shape to draw -- and, when overdraw stacks up anyway, to skip shading pixels that are hidden underneath opaque shapes. Both of those jobs now live on the GPU, which brings us to the command renderer.

# The Command Renderer

CF used to work the classic way: the CPU expanded every drawable into six fat pre-transformed vertices (position, uv, shape params, colors -- over a hundred bytes each) and streamed the whole vertex soup to the GPU every frame. That machinery is gone. Today the CPU records one compact *command* per drawable -- 80 bytes holding a bounding box, a shape type, packed colors, and a payload offset -- and uploads commands plus a small shared payload buffer (shape params, matrix palette). Everything downstream happens on the GPU, via one of two paths that share the exact same upload:

**The instanced path.** One instance per command. The vertex shader reads the command out of a storage buffer via `gl_InstanceIndex`, derives a tightly-fitting coverage quad from the shape parameters right there in the shader (this is where the old CPU-side oriented-bounding-box fitting moved), and the fragment shader dispatches on the command's type to evaluate the right SDF:

```glsl
bool is_box     = v_type >  1.5 && v_type < 2.5;
bool is_seg     = v_type >  2.5 && v_type < 3.5;
// ... triangle, polygon, polyline body, arrow, custom shapes, CSG groups ...

float d = 0;
if (is_box) {
	d = distance_box(v_pos, v_ab.xy, v_ab.zw, v_cd.xy);
} else if (is_seg) {
	d = distance_segment(v_pos, v_ab.xy, v_ab.zw);
} // ...
c = sdf(c, v_col, d - v_radius);
```

WAIT A MINUTE -- Won't this blow up the GPU? If-statements are slow!!!

Are if-statements truly slow? When was the last time you wrote an if-statement in a fragment shader and then actually observed the performance? GPUs stamp out blocks of pixels in groups of 32 or 64, and as long as a block executes the same branch-path you get full performance. In practice games draw a lot of the same shape in the same area, so worrying about warp/wavefront convergence here is a total non-issue.

**The tiled path.** The instanced path still pays for overdraw: a stack of ten opaque shapes shades every covered pixel ten times. So the renderer has a second consumer of the very same command buffer: the screen is cut into 16x16 pixel tiles, and a short chain of compute dispatches bins commands into per-tile lists -- evaluating each command's SDF at tile centers to reject tiles the shape never touches (long skinny diagonal lines bin beautifully this way). A fullscreen pass then walks each pixel's tile list back-to-front, evaluating SDFs and compositing *in registers*, writing the framebuffer exactly once per pixel. The binning pass also performs opaque-cover culling: the moment a tile is fully covered by an opaque shape, everything painted beneath it is dropped from the list without ever being shaded.

The binning itself is four tiny dispatches: zero the tile headers, count how many commands touch each tile (AABB only -- counting just reserves list space), prefix-scan the counts into offsets, then gather. The gather is the fun one: one thread per tile walks all commands in submission order and writes its own list segment, which means painter's order falls out for free -- no atomics, no sort. That same walk applies the opaque-cover cull as it goes, so a tile stops binning the instant something opaque covers it. And since a pathological batch (thousands of screen-covering commands) could demand an unbounded list buffer, there's a footprint budget: batches that blow past it just route to the instanced path instead, which is O(commands) no matter what. Correctness never depends on the tiled path existing at all -- it's purely a go-faster button.

Neither path is strictly better. The rasterizer wins at moderate overdraw (it shades only real coverage), while the tile walk wins decisively when opaque shapes stack up (its cull skips hidden work entirely -- roughly an order of magnitude on heavily-stacked opaque scenes). So the renderer routes automatically, per batch: a cheap pre-scan detects whether a large opaque cover exists, and picks the winning path. Both preserve paint order exactly and produce near-identical pixels (the test suite diffs them against each other).

Because everything is just commands, the whole feature set rides along in both paths: sprites, text glyphs, every shape type, [custom SDF shapes](https://randygaul.github.io/cute_framework/topics/drawing#custom-sdf-shapes) you register as shader snippets, and [boolean shape groups](https://randygaul.github.io/cute_framework/topics/drawing#shape-groups-boolean-ops) that union/subtract/intersect plain draw calls into a single composite shape -- one command, one distance field, outlineable as one continuous stroke.

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
	float inv_cam_scale = 1.0f / len(s_draw->cam_stack.last().m.y);
	float scale = s_draw->antialias.last();
	aaf = scale * inv_cam_scale / app->pixel_scale;
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
- Arrow (a single SDF -- shaft and head unioned, so translucency never double-blends)
- [Custom SDF shapes](https://randygaul.github.io/cute_framework/topics/drawing#custom-sdf-shapes) you define in shader code
- [Boolean compositions](https://randygaul.github.io/cute_framework/topics/drawing#shape-groups-boolean-ops) of any of the above
- Vector paths built from Bezier curves (covered below)

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

But raster glyphs have a ceiling. Zoom the camera in and your beautiful crisp text turns into bilinear soup, or you re-rasterize at the new size and churn the atlas forever. Which brings us to the fun part.

# Text From Curves

Remember those "fascinating benefits with text rendering" promised way back in the atlas compiler section? Here's the payoff. By default CF doesn't rasterize glyphs at all anymore -- it renders them *directly from their Bezier outlines*, per-pixel, on the GPU.

If you've heard of Eric Lengyel's [Slug](https://sluglibrary.com/) library, this is the same family of idea (and his patent has since been dedicated to the public domain -- thanks Eric!). A TrueType glyph is just a bag of quadratic Bezier curves. Given a pixel, cast a ray and count signed curve crossings -- that's your winding number, which tells you inside vs outside. The clever bit from Lengyel's paper is that deciding *which* quadratic roots count as crossings folds into a 2-bit lookup indexed by the sign pattern of the three control points (a single magic constant, `0x2E74`). No branchy special cases. For antialiasing, instead of counting hard +1/-1 crossings we accumulate *fractional* crossings within the pixel footprint, on two perpendicular rays -- analytic AA that's exact at literally any scale. Stroked text solves the exact closest-distance to each curve (a closed-form cubic, courtesy of the usual suspect [Inigo Quilez](https://iquilezles.org/articles/distfunctions2d/)) and feeds it through the very same `sdf()` function every shape uses.

And now for my favorite part. Where do the curves live? *In the atlas.* The atlas compiler has no idea it's holding anything unusual -- each glyph's control points get encoded as 16-bit fixed-point coordinates packed into RGBA8 texels, three texels per curve, and pushed through the exact same cache as any sprite. The fragment shader reads them back with `texelFetch`. It turns out the "online sprite compiler" was never really a sprite compiler at all; it's a GPU data cache with a dedup policy, and curve strips are just really tiny sprites that happen to encode math. One strip per glyph, *ever* -- no font size, camera zoom, or rotation can invalidate it, because there's nothing resolution-dependent to invalidate.

The raster path didn't go away -- it's still there as an automatic per-glyph fallback (blurred text is inherently a bitmap-space effect, so `push_font_blur` routes to raster), and you can force it with `cf_push_text_curves(false)` if you want the old behavior. Layout, kerning, word wrap, and all the markup effects are shared between both paths, byte-for-byte identical. Benchmarks with the screen packed *solid* with text put the curve path at roughly 6% over the raster path -- for text that never blurs, never churns the atlas, and stays perfectly crisp under any camera. Oh, and you get outlined text now:

```cpp
push_text_stroke(0.5f);
draw_text("Outlined text!", V2(0,0));
pop_text_stroke();
```

Try the `vector_text` sample to see raster and curve text side-by-side under a zooming camera, plus stroked text, `<underline>` markup, and text laid along a curved baseline (each glyph individually rotated -- which stays crisp, because of course it does).

# Vector Paths

Once glyphs render from curves, an obvious question appears: why should fonts get all the fun? A glyph is just a path. So CF exposes the same machinery as a public API -- build a path out of lines, quadratics, and cubics, bake it once, then fill or stroke it like any shape:

```cpp
draw_path_begin();
draw_path_move_to(V2(0, -70));
draw_path_cubic_to(V2(-110, 20), V2(-70, 90), V2(0, 40));
draw_path_cubic_to(V2(70, 90), V2(110, 20), V2(0, -70));
draw_path_close();
CF_DrawPath heart = draw_path_end();

draw_path_fill(heart);       // Filled, nonzero winding rule.
draw_path(heart, 3.0f);      // Or stroked, with exact curve distance.
```

Multiple contours work, and holes fall out of the nonzero winding rule for free -- wind an inner contour opposite the outer one and the winding numbers cancel. Baking encodes the curves into the same atlas-texel format as glyph strips (bigger paths span multiple texel rows), so paths get the whole treatment: resolution-independent fills, camera-perfect AA, true curved strokes, no tessellation step, and no "how many segments should I use?" question to ever answer. The `vector_paths` sample draws a heart from cubics, a star with a pentagonal hole, and a stroked spline.

# Blend Modes

Here's a classic 2D renderer annoyance: additive particles. Blending is pipeline state, so the traditional answer is to segregate your additive stuff into its own pass or its own batcher, and now *you* are manually managing draw order between "the additive things" and "the normal things". Gross.

Since every drawable in CF is just a command in a stream, blend modes can simply be recorded *per drawable*:

```cpp
draw_push_blend(DRAW_BLEND_ADD);
draw_circle_fill(V2(x, y), r);   // Glowy!
draw_pop_blend();
```

Normal, add, multiply, and screen -- interleaved however you like, in one paint-ordered stream. Under the hood the flush splits the stream into runs wherever the mode changes, and each run renders with its mode's exact fixed-function blend state. A neat consequence of CF using premultiplied alpha everywhere is that all four modes have *exact* fixed-function equivalents (multiply is `DST_COLOR/ONE_MINUS_SRC_ALPHA`, screen is `ONE/ONE_MINUS_SRC_COLOR`, and so on), so the instanced path needed zero shader changes. The tiled path composites in registers, remember, so it instead swaps its accumulate operator per run -- additive sums, multiply accumulates a per-channel gain, screen under-composites per channel -- and the test suite asserts both paths agree with the pen-and-paper math down to the least significant bit. One more detail: opaque-cover culling turns itself off for non-normal runs, since an "opaque" additive shape doesn't actually hide anything beneath it.

The `blend_modes` sample draws four identical panels where only the mode differs, which makes the behavior differences pop immediately.

# Draw Lists

CF's draw API is immediate-mode: every frame you call the draw functions again and the renderer re-records everything from scratch. That's a wonderful API to *use*, but it means a completely static tilemap pays full price every frame -- transform math, text layout, the works -- to produce the exact same commands it produced last frame.

Draw lists fix this the obvious way: record once, replay forever.

```cpp
CF_DrawList list = make_draw_list();
draw_list_begin(list);
// ... draw your entire static level: shapes, sprites, text, paths ...
draw_list_end();

// Every frame afterwards:
draw_list(list); // Replays under the current camera. That's it.
```

Recording happens in list-local space (the camera resets to identity for the duration), so a replay composes whatever the current camera is onto every stored command -- record your level once, then fly the camera around it forever. The implementation has one sneaky trick worth calling out: replaying does *not* copy the recorded geometry. The replayed commands just borrow a reference to the list's geometry arrays, and the renderer's existing collate pass -- which already copies every command's geometry exactly once into the flush stream -- composes the replay transform during that same copy. The numbers came out delightful: the city scene in the video up top is ~30k drawables across five parallax layers, and re-recording it immediate-mode costs about 1.75 milliseconds per frame while replaying it as draw lists costs about *0.001* milliseconds. The moon and its orbiting debris are drawn live on top every frame, mixing freely with the recorded content.

And how do sprites and text survive being recorded, when the atlas compiler is busy defragging and decaying textures underneath them? Easy -- a replay re-pushes the recorded atlas entries every frame, exactly like live drawing does, so UVs always resolve against the *current* atlas. The online atlas compiler keeps doing its thing, blissfully unaware that some of its customers are recordings. Blend modes and vector paths record into lists too, and lists can even replay inside another list's recording if you want to compose them.

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

The API is largely centered around constructing shaders [CF_Shader](https://randygaul.github.io/cute_framework/graphics/cf_make_shader), canvases ([CF_Canvas](https://randygaul.github.io/cute_framework/graphics/cf_make_canvas)), meshes ([CF_Mesh](https://randygaul.github.io/cute_framework/graphics/cf_make_mesh), materials ([CF_Material](https://randygaul.github.io/cute_framework/graphics/cf_make_material)), and textures ([CF_Texture](https://randygaul.github.io/cute_framework/graphics/cf_make_texture)). The shader itself is constructed originally from input GLSL 450, and is cross-compiled to SPIRV internally (more on that in the next section), and then again compiled by backend vendors into the actual hardware shader. SDL comes with some helper tools to assist the final backend translation, namely [SDL_Shadercross](https://github.com/libsdl-org/SDL_shadercross).

The canvas holds a texture CF can render to. The material holds global variable inputs to the shader (uniforms), while the mesh holds vertex inputs to the shader (attributes). You load up materials by specifing key-value pairs. Typically values are floats, vectors, or matrices, while keys are names of uniforms (global variables) in the shader.

Canvas can be drawn to, blitted to one another, have shaders applied (or not) to them, etc. The API is largely canvas-centric.

# Compiling Our Own Shaders

For a long time CF bundled [glslang](https://github.com/KhronosGroup/glslang) to turn GLSL into SPIRV. It works, but it's a monstrous C++ dependency -- slow to build, heavy to link, and shaped like a compiler toolchain rather than a library you casually embed. Runtime shader compilation is load-bearing for CF: custom [draw shaders](https://randygaul.github.io/cute_framework/topics/drawing/#shaders), [custom SDF shapes](https://randygaul.github.io/cute_framework/topics/drawing#custom-sdf-shapes) registered as shader snippets at runtime, shader hotloading during development -- all of it wants a compiler sitting in the loop, not an offline build step.

So CF grew its own: [cute_spirv](https://github.com/RandyGaul/cute_framework/blob/master/libraries/cute/cute_spirv.h), a single-file C header that compiles CF's GLSL dialect straight to SPIRV. No glslang, no exceptions, no CMake safari. The entire builtin shader set -- the instanced draw pair, the tile walk, four binning compute shaders, blit shaders -- compiles at app startup in milliseconds, which is fast enough that CF doesn't even bother shipping precompiled shader blobs anymore. The SPIRV then flows into SDL_Shadercross for the backend-specific translation (DXIL for D3D12, MSL for Metal, or passed through directly for Vulkan), and on the GLES3 backend CF transpiles to GLSL ES 300 instead.

Writing your own compiler also means when you hit a compiler limitation you just... fix the compiler. While building the curve-text machinery the 10-argument glyph evaluator tripped a "too many arguments" error that turned out to be an arbitrarily-sized internal buffer capping function calls at 9 args. One-line fix, back to work. Try doing that with a vendored copy of glslang.

One nice thing about SDL_Gpu is how Meshes and Textures are dealt with in terms of buffers. These buffers use *staging buffers*, which are blocks of memory we can allocate on the CPU that get sent in-flight to the GPU when needed, and later cycled back. However, on devices with a shared memory model, these can simply be a single spot in memory. The cycling itself is also quite interesting in SDL_Gpu, as any writeable resource (namely textures and mesh buffers) will automagically allocate a ring-buffer of resources under the hood as-needed to reduce inter-frame dependencies. This means we get some parallelism for "free" without having to implement our own double/triple buffering systems.

# Web Builds

SDL_Gpu doesn't have a web compatible backend. So, CF implements its own GLES3 backend to get web support. It simply implements another backend for all of the graphics API we covered in the prior topic. Here's the docs on [getting web builds going](https://randygaul.github.io/cute_framework/topics/emscripten), great for those itch.io uploads or random playtests by hosting on your own site.

The command renderer runs on GLES3/WebGL2 too, with the same architecture. GLES3 has no storage buffers, so the backend emulates them: commands and payload upload into RGBA32UI textures (one texel per vec4), and a texel-fetch flavor of the same draw shader reads them back out with `texelFetch`. Everything works there -- shapes, sprites, custom SDF shapes, boolean shape groups, blend modes, draw lists, and notably curve text and vector paths (their curve data reads via `texelFetch` from the atlas, which GLES3 is perfectly happy to do) -- at roughly a 25% cost over real storage buffers. The one exception is the tiled path, which needs compute shaders GLES3 doesn't have; web builds run the instanced path for every batch, which is the same path the automatic routing picks for most scenes anyway.

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
#include "blend.shd"
#include "gamma.shd"
#include "smooth_uv.shd"
#include "distance.shd"
#include "glyph.shd"
#include "custom_shapes.shd"
#include "csg.shd"

struct ShaderParams
{
	vec2 view_pos;
	vec2 uv;
	vec2 uv_min;
	vec2 uv_max;
	vec2 screen_uv;
	vec4 attributes;
};

#include "shader_stub.shd"
// snip ...
```

The line `"shader_stub.shd"` is the one in question. By implementing shader include support we can inject a user shader into the `stub` spot. This gets called at the end of CF's fragment shader as a final opportunity for the user to alter the color/rendering of the fragment. (The `custom_shapes.shd` include works the same way for [custom SDF shapes](https://randygaul.github.io/cute_framework/topics/drawing#custom-sdf-shapes) -- registered distance-function snippets get stitched in there and compiled into every shape pipeline, including the tiled path's binning compute shaders.)

```glsl
	// snip ...
	c = sdf(c, v_col, d - v_radius);

	c *= v_alpha;
	ShaderParams sp;
	sp.view_pos = v_pos;
	sp.uv = v_uv;
	// ... uv bounds, screen_uv, attributes ...
	c = shader(c, sp);
	if (u_alpha_discard != 0 && c.a == 0) discard;
	result = c;
}
```

If you're familiar with Unity's old surface shaders, this is very similar in concept (hijacking the fragment shader).

# Conclusion

And that's it! The CF renderer records everything -- sprites, SDF shapes, curve text, vector paths -- as compact commands in one paint-ordered stream, routes each batch to whichever GPU path wins (rasterizer coverage vs tiled walk with opaque-cover culling), blends per-drawable, and lets you snapshot whole scenes into draw lists that replay for free. All of it batches through the online atlas compiler, all of it runs everywhere from D3D12 to WebGL2, and all of it is compiled by CF's own shader compiler at startup in milliseconds. Plus shader customization on any renderable item (as well as canvases), making for a unique and effective renderer API. Not bad for a library that started out pushing sprite structs into a buffer.
