# cf_make_aabb_center_half_extents | [math](https://github.com/RandyGaul/cute_framework/blob/master/docs/math_readme.md) | [cute_math.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_math.h)

Returns an AABB (axis-aligned bounding box).

```cpp
CF_Aabb cf_make_aabb_center_half_extents(CF_V2 center, CF_V2 half_extents)
```

## Remarks

Half-extents refer to half-width and height-height: `half_extents = { half_width, half_height }`.

## Related Pages

[CF_Aabb](https://github.com/RandyGaul/cute_framework/blob/master/docs/math/cf_aabb.md)  
[cf_make_aabb](https://github.com/RandyGaul/cute_framework/blob/master/docs/math/cf_make_aabb.md)  
[cf_make_aabb_pos_w_h](https://github.com/RandyGaul/cute_framework/blob/master/docs/math/cf_make_aabb_pos_w_h.md)  
[cf_make_aabb_from_top_left](https://github.com/RandyGaul/cute_framework/blob/master/docs/math/cf_make_aabb_from_top_left.md)  
