[](../header.md ':include')

# cf_make_aabb_center_half_extents

Category: [math](/api_reference?id=math)  
GitHub: [cute_math.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_math.h)  
---

Returns an AABB (axis-aligned bounding box).

```cpp
CF_Aabb cf_make_aabb_center_half_extents(CF_V2 center, CF_V2 half_extents)
```

## Remarks

Half-extents refer to half-width and height-height: `half_extents = { half_width, half_height }`.

## Related Pages

[CF_Aabb](/math/cf_aabb.md)  
[cf_make_aabb](/math/cf_make_aabb.md)  
[cf_make_aabb_pos_w_h](/math/cf_make_aabb_pos_w_h.md)  
[cf_make_aabb_from_top_left](/math/cf_make_aabb_from_top_left.md)  
