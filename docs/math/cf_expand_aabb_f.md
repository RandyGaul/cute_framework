# cf_expand_aabb_f | [math](https://github.com/RandyGaul/cute_framework/blob/master/docs/math/README.md) | [cute_math.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_math.h)

Expands an AABB (axis-aligned bounding box).

```cpp
CF_Aabb cf_expand_aabb_f(CF_Aabb aabb, float v)
```

## Remarks

`v` is added to to `max.x` and `max.y` of `aabb`, and subtracted from `min.x` and `min.y` of `aabb`.

## Related Pages

[CF_Aabb](https://github.com/RandyGaul/cute_framework/blob/master/docs/math/cf_aabb.md)  
[cf_make_aabb](https://github.com/RandyGaul/cute_framework/blob/master/docs/math/cf_make_aabb.md)  
[cf_expand_aabb](https://github.com/RandyGaul/cute_framework/blob/master/docs/math/cf_expand_aabb.md)  
