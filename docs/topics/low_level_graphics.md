[](../header.md ':include')

<br>

This page talks about the low level graphics API in CF. If you want to draw shapes, text, or sprites you may instead by looking for [Drawing](https://randygaul.github.io/cute_framework/#/topics/drawing). This page covers things such as canvases, shaders, meshes, and similar. You will want to use this lower level graphics API whenever implementing custom rendering, or implementing advanced rendering techniques.

## Overall Picture

CF wraps [low level 3D rendering APIs](https://randygaul.github.io/cute_framework/#/api_reference?id=graphics). The backends supported are:

- Vulkan
- DirectX 11
- DirectX 12
- Metal
- ~~WebGPU~~ - This is disabled due to SDL_GPU not supporting OpenGL ES3, CF is currently seeking alternative solutions

The major primitives that make up the graphics layer of CF are:

- [`CF_Canvas`](https://randygaul.github.io/cute_framework/#/graphics/cf_canvas) - A texture that can be rendered to.
- [`CF_Texture`](https://randygaul.github.io/cute_framework/#/graphics/cf_texture) - Stores image data on the GPU. Can be drawn onto the screen.
- [`CF_Mesh`](https://randygaul.github.io/cute_framework/#/graphics/cf_mesh) - Stores vertex data (triangles) for the GPU to process and draw.
- [`CF_Shader`](https://randygaul.github.io/cute_framework/#/graphics/cf_shader) - A small program on the GPU to transform a mesh's vertices into pixels on the screen.
- [`CF_RenderState`](https://randygaul.github.io/cute_framework/#/graphics/cf_renderstate) - Settings for the rendering, such as stencil or blend state options.
- [`CF_Material`](https://randygaul.github.io/cute_framework/#/graphics/cf_material) - A collection of inputs to a shader, including uniforms and textures.

To draw a single frame the overall flow looks something like the following pseudocode:

```
for each canvas {
    cf_apply_canvas(canvas);
    for each mesh {
        cf_mesh_update_vertex_data(mesh, ...);
        cf_apply_mesh(mesh);
        for each material {
            cf_material_set_uniform_vs(material, ...);
            cf_material_set_uniform_fs(material, ...);
            for each shader {
                cf_apply_shader(shader, material);
                cf_draw_elements(...);
            }
        }
    }
    cf_commit();
}
```

The important functions are are the apply functions. Each apply function is used on a low level graphics primitive to compose our draw calls ([`cf_draw_elements`](https://randygaul.github.io/cute_framework/#/graphics/cf_draw_elements)).

- [`cf_apply_canvas`](https://randygaul.github.io/cute_framework/#/graphics/cf_apply_canvas)
- [`cf_apply_mesh`](https://randygaul.github.io/cute_framework/#/graphics/cf_apply_mesh)
- [`cf_apply_shader`](https://randygaul.github.io/cute_framework/#/graphics/cf_apply_shader)
- [`cf_draw_elements`](https://randygaul.github.io/cute_framework/#/graphics/cf_draw_elements)
- [`cf_commit`](https://randygaul.github.io/cute_framework/#/graphics/cf_commit)

## Meshes

The GPU wants to render triangles. A mesh contains triangle data and uploads them to the GPU. Typically a mesh is built on the CPU one-time, or updated with data each frame for animating geometry. The vertices of a mesh will be sent to a shader as inputs to the vertex shader.

To construct a mesh the layout of the vertices in memory must be described. We do this with an array of vertex attributes [`CF_VertexAttribute`](https://randygaul.github.io/cute_framework/#/graphics/cf_vertexattribute).

> Example code snippet for filling out an array of vertex attributes.

```cpp
// Mesh + vertex attributes.
draw->mesh = cf_make_mesh(CF_USAGE_TYPE_STREAM, CF_MB * 25, 0, 0);
CF_VertexAttribute attrs[4] = { };
attrs[0].name = "in_pos";
attrs[0].format = CF_VERTEX_FORMAT_FLOAT2;
attrs[0].offset = CF_OFFSET_OF(DrawVertex, position);
attrs[1].name = "in_uv";
attrs[1].format = CF_VERTEX_FORMAT_FLOAT2;
attrs[1].offset = CF_OFFSET_OF(DrawVertex, uv);
attrs[2].name = "in_col";
attrs[2].format = CF_VERTEX_FORMAT_UBYTE4_NORM;
attrs[2].offset = CF_OFFSET_OF(DrawVertex, color);
cf_mesh_set_attributes(draw->mesh, attrs, CF_ARRAY_SIZE(attrs), sizeof(DrawVertex), 0);
```

The `"name"` in the above snippet corresponds to the name of vertex attribute inputs for a vertex shader (more on shaders later). The vertex data will need to be set with [`cf_mesh_update_vertex_data`](https://randygaul.github.io/cute_framework/#/graphics/cf_mesh_update_vertex_data).

## Shaders

Shaders are small programs that run on the GPU and transform vertices into pixels on the screen. The vertices match `"name"` of vertex attributes from the previous section. Let's check out a full vertex and fragment shader file.

> A simplified version of CF's internal sprite shader.

```glsl
layout (location = 0) in vec2 in_pos;
layout (location = 1) in vec2 in_uv;
layout (location = 2) in vec4 in_col;

layout (location = 0) out vec2 v_uv;
layout (location = 1) out vec4 v_col;

void main()
{
	vec4 posH = vec4(in_pos, 0, 1);
	v_uv = in_uv;
	v_col = in_col;
	gl_Position = posH;
}
```

```glsl
layout (location = 0) in vec2 v_uv;
layout (location = 1) in vec4 v_col;

layout(location = 0) out vec4 result;

layout (set = 2, binding = 0) uniform sampler2D u_image;

layout (set = 3, binding = 0) uniform uniform_block {
	vec2 u_texture_size;
};

#include "smooth_uv.shd"

void main()
{
	vec4 c = de_gamma(texture(u_image, smooth_uv(v_uv, u_texture_size)));
	if (c.a == 0) discard;
	result = c;
}
```

In CF shaders are written in GLSL (OpenGL Shading Language) version 450. This does not necessarily mean the backend powering the shaders is OpenGL, as the shaders are cross-compiled for all available backends.

CF compiles shaders online making it very easy to write shaders once and have them _just work_ on all other platforms. As of now CF takes on some annoying dependencies to support runtime shader compilation. However, since CF is using SDL3 and SDL_GPU internally, eventually SDL will release a tool called SDL_Shader_Tools to entirely rememedy this dependency situation. For now though, a variety of open source tools for SPIRV cross-compilation get statically linked into cute. This does bloat library size by ~8mb in release builds, and does negatively affect initial compile times.

Okay! And with the bad news out of the way, let's focus on how to write your shaders and ship them with your game in a cross-platform way. You have a few options:

- Compile shaders from glsl source on-disk
- Compile shaders from glsl source from string (in-memory)
- Compile shaders from bytecode (SPIRV)
- Compile [draw-API compatible fragment shaders](https://randygaul.github.io/cute_framework/#/topics/drawing?id=shaders)

To compile shaders from glsl source code on-disk you must first call [cf_shader_directory](https://randygaul.github.io/cute_framework/#/graphics/cf_shader_directory). This tells the application where the shader folder is. You may then optionally setup a callback via [cf_shader_on_changed](https://randygaul.github.io/cute_framework/#/graphics/cf_shader_on_changed) to receive notifications when shaders change on-disk, in order to support shader live-reloading during development. Once done you may then call [cf_make_shader](https://randygaul.github.io/cute_framework/#/graphics/cf_make_shader)

To compile shaders from glsl source code from string (in-memory) simply call [cf_make_shader_from_source](https://randygaul.github.io/cute_framework/#/graphics/cf_make_shader_from_source).

To compile shaders from bytecode (SPIRV) you must first compile them to a bytecode blob. This is a good way to speed up shader compilation when you ship your game, as the bytecode blobs can be shipped alongside your game. Call [cf_compile_shader_to_bytecode](https://randygaul.github.io/cute_framework/#/graphics/cf_compile_shader_to_bytecode) to generate a bytecode blob. You can then create a finalized shader by calling [cf_make_shader_from_bytecode](https://randygaul.github.io/cute_framework/#/graphics/cf_make_shader_from_bytecode).

Shader compilation is explained in more details [here](/topics/shader_compilation.md).

Be aware that shaders must adhere to strict rules for resource sets. Here's the notes from CF's source:

```cpp
/**
 * For _VERTEX_ shaders:
 *  0: Sampled textures, followed by storage textures, followed by storage buffers
 *  1: Uniform buffers
 * For _FRAGMENT_ shaders:
 *  2: Sampled textures, followed by storage textures, followed by storage buffers
 *  3: Uniform buffers
 * 
 * Example _VERTEX shader:
 * layout (set = 0, binding = 0) uniform sampler2D u_image;
 * 
 * layout (set = 1, binding = 0) uniform uniform_block {
 *     vec2 u_texture_size;
 * };
 * 
 * Example _FRAGMENT_ shader:
 * 
 * layout (set = 2, binding = 0) uniform sampler2D u_image;
 * 
 * layout (set = 3, binding = 0) uniform uniform_block {
 *     vec2 u_texture_size;
 * };
 */
```

For uniforms you only have one uniform block available, and it *must* be named `uniform_block`. However, if your
shader is make from the draw api (`cf_make_draw_shader`) uniform blocks must be named `shd_uniforms`.

### Learning to Write Shaders

Learning to write shaders can be very challenging at first. That's mostly because writing a shader requires a lot of prerequisite knowledge. But, once the prereqs are filled out writing shaders becomes quite easy with a little time and practice. Here are some prerequisites that will make writing shaders much easier:

- Decent knowledge of linear algebra and vector math. Mainly coordinate space transforms, matrices, vectors, and the like.
- Good overview knowledge of the typical graphics pipeline (covered briefly below with recommended readings).
- Familiarity with colors and pixels.
- Practice setting shaders up on the CPU-side (gathering vertices into a mesh, creating a material, etc.).

A great way to learn all the fundamentals is to follow along the website [learnopengl.com](https://learnopengl.com/). Mainly the _Getting Started_ section, since it covers fundamental math and setup prereqs. However, we can do a short overview here as well as a refresher/primer!

### Graphics Pipeline Overview

A vertex is small chunk of data sent to the GPU. Usually they represent the points in a mesh, such as the points defining each triangle in a mesh. The vertex is an abstract concept and can actually contain any kind of data we like, not just positional data. Here's a common way to define a basic vertex in a 2D game.

```cpp
struct Vertex
{
	v2 position;    // The position of the vertex in 2D world space.
	v2 uv;          // A coordinate from [0,1] for accessing texture image data.
	CF_Color color; // An optional color component.
};
```

We can see here the size of a vertex would be 2 floats + 2 floats + 4 floats, or 32 bytes. A single triangle would comprise of three vertices, or 96 bytes. The code snippet from the [Meshes](https://randygaul.github.io/cute_framework/#/topics/low_level_graphics?id=Meshes) section above shows how we describe our vertex layout to the GPU. We must describe the vertex layout so the vertices can be uploaded and understood by shaders.

Each vertex, once uploaded, will be sent to a vertex shader as inputs. The vertex shader will be called three times per triangle, each time a single vertex is sent to the vertex shader. The individual members of `Vertex` from the above example (`position`, `uv` and `color`) are called _attributes_ in graphics parlance. We can see in the example sprite, shader from the [Shaders](https://randygaul.github.io/cute_framework/#/topics/low_level_graphics?id=Shaders) section of this document, the names of the attributes line up with names of glsl attributes in the vertex shader (specified with the `layout` and `in` keyword).

Once the vertex shader processes a vertex it outputs a color. The color is then processed by the fragment shader. Optionally, attributes are interpolated between all three vertices of a triangle for a single invocation of the fragment shader. The GPU can automatically perform this interpolation for us. Rephrased: the fragment shader will be invoked one time per pixel, while the vertex shader is invoked one time per vertex -- the inputs to the fragment shader are the interpolated outputs of all three vertex shaders mixed together for each pixel.

The fragment shader finally outputs the color for a single pixel within the triangle, once for each pixel. This produces the final image on the screen.

?> When we say "interpolated" it means the `varying` glsl keyword. We can see these interpolated values as outputs from the vertex shader in the [above sprite shader example](https://randygaul.github.io/cute_framework/#/topics/low_level_graphics?id=Shaders). The vertex shaders specifies it's interpolated outputs with `out`, and the fragment shader collects those inputs with the same `in` keyword.

We can visualize a set of steps a vertex will travel along to go from a sprite sitting on the CPU, to a pixel on the screen, for CF:

1. [Sprites](https://randygaul.github.io/cute_framework/#/topics/drawing?id=drawing-sprites) sit around on the CPU waiting to be drawn.
2. Once drawn, they are drawn relative to the [Camera](https://randygaul.github.io/cute_framework/#/topics/camera).
3. CF collects all of the sprites and generates a big array of vertices for each sprite.
4. All the vertices are uploaded to the GPU, invoking the vertex shader.
5. The vertex shader outputs are interpolated and sent to the fragment shader.
6. The fragment shader outputs pixel colors.

In your own implementation, using the low level graphics API, you will need to define your own graphics pipeline. A more abstract version of these steps may look something like so:

1. Meshes sit in model space.
2. Transform meshes to world space.
3. Transform meshes from world space to camera space.
4. Upload meshes to the GPU.
5. In the vertex shader vertices are transformed to view space.
6. The vertex shader outputs are interpolated and used to invoke the fragment shader.
7. The fragment shader clips vertices within the screen, and outputs pixels.

These steps are a little simplified, but give one option for a good (but abstract) graphics pipeline. A good way to learn more is to take the keywords and search them, especially the various spaces. [learnopengl.com](https://learnopengl.com/) has a great page about [Coordinate Systems](https://learnopengl.com/Getting-started/Coordinate-Systems) describing these various mathematical spaces. These abstracts steps line up roughly with the steps outlined for CF.

## Materials

[CF_Material](https://randygaul.github.io/cute_framework/#/graphics/cf_material) acts like a bag of inputs to shaders. Unlike meshes, materials don't hold vertices but instead hold uniforms and textures.

### Uniforms

A uniform is a global variable within either a vertex or fragment shader. The uniforms are defined in the shader itself using the `uniform` keyword. The sprite shader example from earlier specified some uniforms in the fragment shader like so:

```glsl
layout (set = 0, binding = 0) uniform sampler2D u_image;

layout (set = 1, binding = 0) uniform fs_params {
	vec2 u_texture_size;
};
```

For vertex shaders textures have `set = 0`, while the uniform block has `set = 1`. For fragment shaders textures use `set = 2`, while the uniform block must use `set = 3`.

In C++ we can set values for these uniforms by name using [`cf_material_set_uniform_vs`](https://randygaul.github.io/cute_framework/#/graphics/cf_material_set_uniform_vs) for vertex shaders, or [`cf_material_set_uniform_fs`](https://randygaul.github.io/cute_framework/#/graphics/cf_material_set_uniform_fs) for fragment shaders.

[CF_Material](https://randygaul.github.io/cute_framework/#/graphics/cf_material) has the cool feature to dynamically line up uniforms it stores with shaders. This means that a material can hold _many_ different uniforms, but only those that have a matching name will be sent to the shader. All other are simply ignored. This is great for making materials that can be applied to many different shaders, or making many different shaders that share a common material. Mix-and-matching is highly encouraged!

You can set textures on a material as well via [`cf_material_set_texture_vs`](https://randygaul.github.io/cute_framework/#/graphics/cf_material_set_texture_vs) for the vertex shader, or [`cf_material_set_texture_fs`](https://randygaul.github.io/cute_framework/#/graphics/cf_material_set_texture_fs) for the fragment shader. More on textures in the next section.

## Textures

Textures hold image data, as in pixels. Though in graphics we call them texels, not pixels. Actually, a texel can hold arbitrary data, but usually we just store one `vec4` (in glsl) or `CF_Color` (in C++) per pixel. Texture data is fetched from a shader using what's called uv-coordinates.

UV-coordinates are two floats, each in the range from `[0,1]`. The coordinate (0, 0) is usually the top-left of a texture, while (1, 1) maps to the bottom-right of a texture. Each vertex typically has a unique uv coordinate used to fetch which pixel to draw.

## Canvases

[`CF_Cavas`](https://randygaul.github.io/cute_framework/#/graphics/cf_canvas) represents a texture the GPU can render within. Instead of rendering to the screen itself, sometimes advanced rendering techniques require first rendering to a texture. This is common for some techniques such as reflections, or shadows. Here's an example image showing some basic reflections, using a canvas and some [stenciling techniques](https://en.wikipedia.org/wiki/Stencil_buffer).

<p align="center">
<img src=https://github.com/RandyGaul/cute_framework/blob/master/assets/block_man_final.gif?raw=true>
</p>

### App's Default Render Canvas

The app window itself has a [default canvas](https://randygaul.github.io/cute_framework/#/topics/application_window?id=resizing-windows). This canvas is used for higher-level [`Drawing API`](https://randygaul.github.io/cute_framework/#/topics/drawing) to get things onto the screen. By default CF collects everything and automatically displays it onto the default canvas.

However, for custom rendering techniques you must fetch and render to the app's canvas to display rendered contents on the screen. Use [`cf_app_get_canvas`](https://randygaul.github.io/cute_framework/#/app/cf_app_get_canvas) along with [`cf_app_get_canvas_height`](https://randygaul.github.io/cute_framework/#/app/cf_app_get_canvas_height) and [`cf_app_get_canvas_width`](https://randygaul.github.io/cute_framework/#/app/cf_app_get_canvas_width). You may then render to this canvas (or any other canvas) with [`cf_render_to`](https://randygaul.github.io/cute_framework/#/draw/cf_render_to).
