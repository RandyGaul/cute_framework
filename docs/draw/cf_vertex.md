[](../header.md ':include')

# CF_Vertex

Category: [draw](/api_reference?id=draw)  
GitHub: [cute_draw.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_draw.h)  
---

The full vertex layout CF uses just before sending verts to the GPU.

Struct Members | Description
--- | ---
`CF_V2 p` | World space position.
`CF_V2 posH` | "Homogenous" position transformed by the camera.
`CF_V2 a, b, c` | For internal use -- For signed-distance functions for rendering shapes.
`CF_V2 uv` | For internal use -- For sprite rendering.
`CF_Pixel color` | Color for rendering shapes (ignored for sprites).
`float radius` | For internal use -- For applying "chubbiness" factor for shapes, or radii on circle/capsule.
`float stroke` | For internal use -- For shape rendering for border style stroke rendering (no fill).
`float aa` | For internal use -- Factor for the size of antialiasing.
`uint8_t type` | For internal use -- The type of shape to be rendered, used by the signed-distance functions within CF's internal fragment shader.
`uint8_t alpha` | Used for the alpha-component (transparency).
`uint8_t fill` | For internal use -- Whether or not to render shapes as filled or strokedx.
`uint8_t not_used` | For internal use -- Reserved for a future purpose, simply fulfills byte alignment for now.
`CF_Color attributes` | Four general purpose floats passed into custom user shaders.

## Remarks

You may fill in vertices via callback by `cf_draw_push_vertex_callback`. See [CF_VertexFn](/draw/cf_vertexfn.md).
This is useful when you need to fill in unique `attributes` per-vertex, or modify any other
bits of the vertex before rendering. This could be used to implement features like dynamically
generated UV's for shape slicing, or complex lighting systems.

## Related Pages

[cf_set_vertex_callback](/draw/cf_set_vertex_callback.md)  
[CF_VertexFn](/draw/cf_vertexfn.md)  
