[](../header.md ':include')

<br>

This page talks about the low level graphics API in CF. If you want to draw shapes, text, or sprites you may instead by looking for [Drawing](https://randygaul.github.io/cute_framework/#/topics/drawing). This page covers things such as canvases, shaders, meshes, and similar. You will want to use this lower level graphics API whenever implementing custom rendering, or implementing advanced rendering techniques.

## Overall Picture

CF wraps low level 3D rendering APIs. The backends supported are:

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

## Meshes

The GPU wants to render triangles. A mesh contains triangle data and uploads the triangles to the GPU. Typically a mesh is built on the CPU one-time, or updated with data each frame for animating geometry. The vertices of a mesh will be sent to a shader as inputs to the vertex shader.

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
attrs[3].name = "in_params";
attrs[3].format = CF_VERTEX_FORMAT_UBYTE4N;
attrs[3].offset = CF_OFFSET_OF(DrawVertex, alpha);
cf_mesh_set_attributes(draw->mesh, attrs, CF_ARRAY_SIZE(attrs), sizeof(DrawVertex), 0);
```

The `"name"` in the above snippet corresponds to the name of vertex attribute inputs for a vertex shader (more on shaders later). The vertex data will need to be set with [`cf_mesh_update_vertex_data`](https://randygaul.github.io/cute_framework/#/graphics/cf_mesh_update_vertex_data).

## Shaders

## Materials

## Canvases
