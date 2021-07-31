# batch_line

Pushes a line onto the batch.

## Syntax

```cpp
void batch_line(batch_t* b, v2 p0, v2 p1, float thickness, color_t c);
void batch_line(batch_t* b, v2 p0, v2 p1, float thickness, color_t c0, color_t c1);
```

## Function Parameters

Parameter Name | Description
--- | ---
b | The batch.
p0 to p1 | Two vertices specifying a line.
thickness | Number of pixels to draw for the thickness of the line.
c | Color to render with.
c0 to c1 | Colors for `p0` and `p1`.
