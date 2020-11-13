# batch_circle_line

Pushes a circle onto the batch to be drawn with lines.

## Syntax

```cpp
void batch_circle_line(batch_t* b, v2 p, float r, int iters, float thickness, color_t c);
```

## Function Parameters

Parameter Name | Description
--- | ---
b | The batch.
r | Radius of the circle.
iters | Number of line segments to draw the circle with.
thickness | The thickness, in pixels, of lines to render the quad with.
c | Color to render with.

## Related Functions
 
[batch_circle](https://github.com/RandyGaul/cute_framework/tree/master/doc/graphics/batch/batch_circle.md)  
