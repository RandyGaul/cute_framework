# cf_unapply_canvas | [graphics](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/README.md) | [cute_graphics.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_graphics.h)

An optional function to end the current rendering pass.

```cpp
void cf_unapply_canvas();
```

## Remarks

This is only useful when a particular canvas needs to be destroyed, though it may be currently applied. For example, if the screen is
resized and you want to resize some of your canvases as well.

## Related Pages

[CF_Canvas](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/cf_canvas.md)  
[cf_apply_canvas](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/cf_apply_canvas.md)  
