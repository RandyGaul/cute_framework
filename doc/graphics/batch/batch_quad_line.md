# batch_quad_line

Pushes a quad onto the batch to be drawn with lines.

## Syntax

```cpp
void batch_quad_line(batch_t* b, aabb_t bb, float thickness, color_t c);
void batch_quad_line(batch_t* b, v2 p0, v2 p1, v2 p2, v2 p3, float thickness, color_t c);
void batch_quad_line(batch_t* b, v2 p0, v2 p1, v2 p2, v2 p3, float thickness, color_t c0, color_t c1, color_t c2, color_t c3);
```

## Function Parameters

Parameter Name | Description
--- | ---
b | The batch.
bb | The bounding box (quad).
thickness | The thickness, in pixels, of lines to render the quad with.
c | Color to render with.
p0 through p3 | Four vertices to specify the quad.
c0 through c3 | Colors for each of the four vertices `p0` to `p3`.

## Related Functions
 
[batch_quad](https://github.com/RandyGaul/cute_framework/tree/master/doc/graphics/batch/batch_quad.md)  
