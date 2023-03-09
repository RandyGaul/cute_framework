# cf_draw_peek_layer

Category: [draw](https://github.com/RandyGaul/cute_framework/blob/master/docs/api_reference?id=draw)  
GitHub: [cute_draw.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_draw.h)  
---

Returns the last draw layer.

```cpp
int cf_draw_peek_layer();
```

## Remarks

Draw layers are sorted before rendering. Lower numbers are rendered fast, while larger numbers are rendered last.
This can be used to pick which sprites/shapes should draw on top of each other.

## Related Pages

[cf_draw_push_layer](https://github.com/RandyGaul/cute_framework/blob/master/docs/draw/cf_draw_push_layer.md)  
[cf_draw_pop_layer](https://github.com/RandyGaul/cute_framework/blob/master/docs/draw/cf_draw_pop_layer.md)  
