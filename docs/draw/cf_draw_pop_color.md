# cf_draw_pop_color | [draw](https://github.com/RandyGaul/cute_framework/blob/master/docs/draw_readme.md) | [cute_draw.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_draw.h)

Pops and returns the last draw color.

```cpp
CF_Color cf_draw_pop_color();
```

## Remarks

Various draw functions do not specify a color. In these cases, the last color pushed will be used.

## Related Pages

[cf_draw_push_color](https://github.com/RandyGaul/cute_framework/blob/master/docs/draw/cf_draw_push_color.md)  
[cf_draw_peek_color](https://github.com/RandyGaul/cute_framework/blob/master/docs/draw/cf_draw_peek_color.md)  
