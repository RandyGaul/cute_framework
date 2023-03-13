[](../header.md ':include')

<br>

This page talks about the low level graphics API in CF. If you want to draw shapes, text, or sprites you may instead by looking for [Drawing](https://randygaul.github.io/cute_framework/#/topics/drawing). This page covers things such as canvases, shaders, meshes, and similar. You will want to use this lower level graphics API whenever implementing custom rendering, or implementing advanced rendering techniques.

## Overall Picture

CF wraps [low level 3D rendering APIs](https://randygaul.github.io/cute_framework/#/api_reference?id=graphics). The backends supported are:

- OpenGL 3.3 Core Profile
- OpenGL ES 2.0
- OpenGL ES 3.0
- DirectX 11
- Metal (not quite available just yet)
- WebGPU

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
}
```

The important functions are are the apply functions. Each apply function is used on a low level graphics primitive to compose our draw calls ([`cf_draw_elements`](https://randygaul.github.io/cute_framework/#/graphics/cf_draw_elements)).

- [`cf_apply_canvas`](https://randygaul.github.io/cute_framework/#/graphics/cf_apply_canvas)
- [`cf_apply_mesh`](https://randygaul.github.io/cute_framework/#/graphics/cf_apply_mesh)
- [`cf_apply_shader`](https://randygaul.github.io/cute_framework/#/graphics/cf_apply_shader)

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
attrs[2].format = CF_VERTEX_FORMAT_UBYTE4N;
attrs[2].offset = CF_OFFSET_OF(DrawVertex, color);
cf_mesh_set_attributes(draw->mesh, attrs, CF_ARRAY_SIZE(attrs), sizeof(DrawVertex), 0);
```

The `"name"` in the above snippet corresponds to the name of vertex attribute inputs for a vertex shader (more on shaders later). The vertex data will need to be set with [`cf_mesh_update_vertex_data`](https://randygaul.github.io/cute_framework/#/graphics/cf_mesh_update_vertex_data).

## Shaders

Shaders are small programs that run on the GPU and transform vertices into pixels on the screen. The vertices match `"name"` of vertex attributes from the previous section. Let's check out a full vertex and fragment shader file.

> A simplified version of CF's sprite shader.

```glsl
@module sprite

@ctype vec4 CF_Color
@ctype vec2 CF_V2

@include includes/smooth_uv.glsl

@vs vs
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
@end

@fs fs
	layout (location = 0) in vec2 v_uv;
	layout (location = 1) in vec4 v_col;

	out vec4 result;

	layout (binding = 0) uniform sampler2D u_image;

	layout (binding = 0) uniform fs_params {
		vec2 u_texture_size;
	};

	@include_block smooth_uv

	void main()
	{
		vec4 c = de_gamma(texture(u_image, smooth_uv(v_uv, u_texture_size)));
		if (c.a == 0) discard;
		result = c;
	}
@end

@program shd vs fs
```

In CF shaders are written in GLSL (OpenGL Shading Language). This does not necessarily mean the backend powering the shaders is OpenGL, as the shaders are cross-compiled for all available backends. CF uses a third-party tool called [sokol-shdc](https://github.com/floooh/sokol-tools/blob/master/docs/sokol-shdc.md), which stands for Sokol Shader Compiler. The shader compiler also outputs a header file in plain C code we use to automate all the details of setting up shaders.

?> Don't worry about fully understanding the above code snippet! Learning about low level graphics and shaders requires a lot of knowledge. More resources for learning are found below.

### Special sokol-shdc Features

sokol-shdc has a number of special features added to glsl we use, as seen in the above example. Let's cover the really important ones right now.

| Keyword | Description |
| --- | --- |
| @module | Name of the shader. |
| @ctype | Maps a glsl type to a C-type. These should match with CF types. |
| @include | Allows us to include (copy + paste) another .glsl file into a shader with @include_block. |
| @include_block | Includes a block of shader code from another file. Blocks are named sections of glsl code. See the [sokol-shdc](https://github.com/floooh/sokol-tools/blob/master/docs/sokol-shdc.md) docs for more details. |
| @vs | Starts the vertex shader block of code, and gives the vertex shader block a name. |
| @fs | Starts the fragment shader block of code, and gives the fragment shader block a name. |
| @program | Glues a vertex and fragment shader together to make the final named shader. |

It's highly recommended to copy this format when writing your own shaders, including the name chosen for @vs, @fs and @shd to make things simple. It's also highly recommended to name your glsl file the same name as the module. For example, if we chose `@module sprite` a good matching file name would be `sprite.glsl`. This will make a lot of sense later when we want to load up a shader from C/C++, which will look like so:

```cpp
#include "my_shader.h"
CF_Shader my_shd = CF_MAKE_SOKOL_SHADER(my_shader);
```

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
layout (binding = 0) uniform sampler2D u_image;

layout (binding = 0) uniform fs_params {
	vec2 u_texture_size;
};
```

In C++ we can set values for these uniforms by name using [`cf_material_set_uniform_vs`](https://randygaul.github.io/cute_framework/#/graphics/cf_material_set_uniform_vs) for vertex shaders, or [`cf_material_set_uniform_fs`](https://randygaul.github.io/cute_framework/#/graphics/cf_material_set_uniform_fs) for fragment shaders.

[CF_Material](https://randygaul.github.io/cute_framework/#/graphics/cf_material) has the cool feature to dynamically line up uniforms it stores with shaders. This means that a material can hold _many_ different uniforms, but only those that have a matching name will be sent to the shader. All other are simply ignored. This is great for making materials that can be applied to many different shaders, or making many different shaders that share a common material. Mix-and-matching is highly encouraged!

You can set textures on a material as well via [`cf_material_set_texture_vs`](https://randygaul.github.io/cute_framework/#/graphics/cf_material_set_texture_vs) for the vertex shader, or [`cf_material_set_texture_fs`](https://randygaul.github.io/cute_framework/#/graphics/cf_material_set_texture_fs) for the fragment shader. More on textures in the next section.

## Textures

Textures hold image data, as in pixels. Though in graphics we call them texels, not pixels. Actually, a texel can hold arbitrary data, but usually we just store one `vec4` (in glsl) or `CF_Color` (in C++) per pixel. Texture data is fetched from a shader using what's called uv-coordinates.

UV-coordinates are two floats, each in the range from `[0,1]`. The coordinate (0, 0) is usually the top-left of a texture, while (1, 1) maps to the bottom-right of a texture. Each vertex typically has a unique uv coordinate used to fetch which pixel to draw.

## Canvases

[`CF_Cavas`](https://randygaul.github.io/cute_framework/#/graphics/cf_canvas) represents a texture the GPU can render within. Instead of rendering to the screen itself, sometimes advanced rendering techniques require first rendering to a texture. This is common for some techniques such as reflections, or shadows.

### App's Default Render Canvas

The app window itself has a [default canvas](https://randygaul.github.io/cute_framework/#/topics/application_window?id=resizing-windows). This canvas is used for higher-level [`Drawing API`](https://randygaul.github.io/cute_framework/#/topics/drawing) to get things onto the screen. By default CF collects everything and automatically displays it onto the default canvas.

However, for custom rendering techniques you must fetch and render to the app's canvas to display rendered contents on the screen. Use [`cf_app_get_canvas`](https://randygaul.github.io/cute_framework/#/app/cf_app_get_canvas) along with [`cf_app_get_canvas_height`](https://randygaul.github.io/cute_framework/#/app/cf_app_get_canvas_height) and [`cf_app_get_canvas_width`](https://randygaul.github.io/cute_framework/#/app/cf_app_get_canvas_width). You may then render to this canvas (or any other canvas) with [`cf_render_to`](https://randygaul.github.io/cute_framework/#/draw/cf_render_to).

## Limitations

Here is a quick list of unsupported graphics features. These can be potentially added to CF later, but are not slated for v1.00 of CF's initial release. Most of these features make more sense for the 3D use-case as opposed to 2D games. The ones marked with stars are currently considered higher priority for adding in the future.

- Mipmaps *
- MSAA
- Blend color constant
- Multiple render targets (aka color/texture attachments) *
- Depth bias tunables
- uint16_t indices (only uint32_t supported) *
- Cube map
- 3D textures
- Texture arrays *
- Sampler types signed/unsigned int (only float supported)
- Other primitive types besides triangles
- UV wrap border colors
- Face winding order (defaults to CCW only)
- Anisotropy tunable
- Min/max LOD tunable
- No direct access to the underlying device *
