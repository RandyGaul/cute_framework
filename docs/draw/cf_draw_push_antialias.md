# cf_draw_push_antialias | [draw](https://github.com/RandyGaul/cute_framework/blob/master/docs/draw/README.md) | [cute_draw.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_draw.h)

Pushes whether or not to apply antialiasing.

```cpp
void cf_draw_push_antialias(bool antialias);
```

Parameters | Description
--- | ---
antialias | True to antialias, false otherwise.

## Remarks

Various shape drawing functions can be drawn in antialiased mode, or in plain mode. Antialiasing is slightly slower,
but looks much smoother.

## Related Pages

[cf_draw_peek_antialias](https://github.com/RandyGaul/cute_framework/blob/master/docs/draw/cf_draw_peek_antialias.md)  
[cf_draw_pop_antialias](https://github.com/RandyGaul/cute_framework/blob/master/docs/draw/cf_draw_pop_antialias.md)  
