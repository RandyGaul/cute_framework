[](../header.md ':include')

# cf_draw_push_color

Category: [draw](/api_reference?id=draw)  
GitHub: [cute_draw.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_draw.h)  
---

Pushes a draw color.

```cpp
CF_API void CF_CALL cf_draw_push_color(CF_Color c);
```

Parameters | Description
--- | ---
c | The color.

## Remarks

Various draw functions do not specify a color. In these cases, the last color pushed will be used.

## Related Pages

[cf_draw_peek_color](/draw/cf_draw_peek_color.md)  
[cf_draw_pop_color](/draw/cf_draw_pop_color.md)  
