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

See [CF_VertexFn](/draw/cf_vertexfn.md).

## Related Pages

[CF_Vertex](/draw/cf_vertex.md)  
[CF_VertexFn](/draw/cf_vertexfn.md)  
cf_draw_push_vertex_callback  
cf_draw_pop_vertex_callback  
