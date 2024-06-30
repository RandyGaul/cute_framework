[](../header.md ':include')

# cf_draw_set_vertex_callback

Category: [draw](/api_reference?id=draw)  
GitHub: [cute_draw.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_draw.h)  
---

An optional callback for modifying vertices before they are sent to the GPU.

```cpp
void cf_draw_set_vertex_callback(CF_VertexFn* vertex_fn);
```

## Remarks

Setup this callback to apply per-vertex modulations for implementing advanced graphical effects.
`Count` is always a multiple of three, as this function always processes large batched arrays of
triangles. Since all shapes are rendered with signed-distance functions, most shapes merely generate
a single quad, so you may find triangle counts lower than originally anticipated.

Call [cf_draw_set_vertex_callback](/draw/cf_draw_set_vertex_callback.md) to setup your callback.

There is no adjecancy info provided. If you need to know which triangles connect to others you
should probably redesign your feature to not require adjecancy information, or use your own custom
rendering solution. With a custom solution you may use low-level graphics in cute_graphics.h, where
any adjacency info can be controlled 100% by you a-priori.

## Related Pages

[CF_Vertex](/draw/cf_vertex.md)  
[CF_VertexFn](/draw/cf_vertexfn.md)  
cf_draw_push_vertex_callback  
cf_draw_pop_vertex_callback  
