# batch_tri_line

Pushes a triangle onto the batch to be drawn with lines.

## Syntax

```cpp
void batch_tri_line(batch_t* b, v2 p0, v2 p1, v2 p2, color_t c);
void batch_tri_line(batch_t* b, v2 p0, v2 p1, v2 p2, color_t c0, color_t c1, color_t c2);
```

## Function Parameters

Parameter Name | Description
--- | ---
b | The batch.
p0 to p2 | Three vertices in counter-clockwise order specifying a triangle.
thickness | Number of pixels to draw for the thickness of the lines.
c | Color to render with.
c0 to c2 | Three colors, one each for the three verts `p0` to `p2`.

## Related Functions
 
[batch_tri](https://github.com/RandyGaul/cute_framework/tree/master/doc/graphics/batch/batch_tri.md)  
