[](../header.md ':include')

# cf_draw_pop_color

Category: [draw](/api_reference?id=draw)  
GitHub: [cute_draw.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_draw.h)  
---

Pops and returns the last draw color.

```cpp
CF_API CF_Color CF_CALL cf_draw_pop_color();
```

## Remarks

Various draw functions do not specify a color. In these cases, the last color pushed will be used.

## Related Pages

[cf_draw_push_color](/draw/cf_draw_push_color.md)  
[cf_draw_peek_color](/draw/cf_draw_peek_color.md)  
