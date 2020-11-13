# batch_tri

Pushes a triangle onto the batch.

## Syntax

```cpp
void batch_tri(batch_t* b, v2 p0, v2 p1, v2 p2, color_t c);
void batch_tri(batch_t* b, v2 p0, v2 p1, v2 p2, color_t c0, color_t c1, color_t c2);
```

## Function Parameters

Parameter Name | Description
--- | ---
b | The batch.
p0 to p2 | Three vertices in counter-clockwise order specifying a triangle.
c | Color to render with.
c0 to c2 | Three colors, one each for the three verts `p0` to `p2`.

## Related Functions
 
[batch_tri_line](https://github.com/RandyGaul/cute_framework/tree/master/doc/graphics/batch/batch_tri_line.md)  
