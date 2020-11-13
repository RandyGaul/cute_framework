# batch_quad

Pushes a quad onto the batch.

## Syntax

```cpp
void batch_quad(batch_t* b, aabb_t bb, color_t c);
void batch_quad(batch_t* b, v2 p0, v2 p1, v2 p2, v2 p3, color_t c);
void batch_quad(batch_t* b, v2 p0, v2 p1, v2 p2, v2 p3, color_t c0, color_t c1, color_t c2, color_t c3);
```

## Function Parameters

Parameter Name | Description
--- | ---
b | The batch.
bb | The bounding box (quad).
c | Color to render with.
p0 through p3 | Four vertices to specify the quad.
c0 through c3 | Colors for each of the four vertices `p0` to `p3`.

## Related Functions
 
[batch_quad_line](https://github.com/RandyGaul/cute_framework/tree/master/doc/graphics/batch/batch_quad_line.md)  
