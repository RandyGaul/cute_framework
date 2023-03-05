# cf_draw_pop_antialias | [draw](https://github.com/RandyGaul/cute_framework/blob/master/docs/draw_readme.md) | [cute_draw.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_draw.h)

Pops and returns the last antialias state.

```cpp
bool cf_draw_pop_antialias();
```

## Remarks

Various shape drawing functions can be drawn in antialiased mode, or in plain mode. Antialiasing is slightly slower,
but looks much smoother.

## Related Pages

[cf_draw_push_antialias](https://github.com/RandyGaul/cute_framework/blob/master/docs/draw/cf_draw_push_antialias.md)  
[cf_draw_peek_antialias](https://github.com/RandyGaul/cute_framework/blob/master/docs/draw/cf_draw_peek_antialias.md)  
