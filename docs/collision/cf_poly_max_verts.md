# CF_POLY_MAX_VERTS | [collision](https://github.com/RandyGaul/cute_framework/blob/master/docs/collision/README.md) | [cute_math.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_math.h)

The maximum number of vertices in a [CF_Poly](https://github.com/RandyGaul/cute_framework/blob/master/docs/collision/cf_poly.md).

```cpp
#define CF_POLY_MAX_VERTS 8
```

## Remarks

It's quite common to limit the number of verts on polygons to a low number. Feel free to adjust this number if needed,
but be warned: higher than 8 and shapes generally start to look more like circles/ovals; it becomes pointless beyond a certain point.

## Related Pages

[CF_Poly](https://github.com/RandyGaul/cute_framework/blob/master/docs/collision/cf_poly.md)  
