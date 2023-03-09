[](../header.md ':include')

# cf_draw_peek_antialias

Category: [draw](https://github.com/RandyGaul/cute_framework/blob/master/docs/api_reference?id=draw)  
GitHub: [cute_draw.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_draw.h)  
---

Returns the last antialias state.

```cpp
bool cf_draw_peek_antialias();
```

## Remarks

Various shape drawing functions can be drawn in antialiased mode, or in plain mode. Antialiasing is slightly slower,
but looks much smoother.

## Related Pages

[cf_draw_push_antialias](https://github.com/RandyGaul/cute_framework/blob/master/docs/draw/cf_draw_push_antialias.md)  
[cf_draw_pop_antialias](https://github.com/RandyGaul/cute_framework/blob/master/docs/draw/cf_draw_pop_antialias.md)  
