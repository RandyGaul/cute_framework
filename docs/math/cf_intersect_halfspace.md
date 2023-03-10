[](../header.md ':include')

# cf_intersect_halfspace

Category: [math](/api_reference?id=math)  
GitHub: [cute_math.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_math.h)  
---

Returns the intersection point of two points to a plane.

```cpp
CF_INLINE CF_V2 cf_intersect_halfspace(CF_V2 a, CF_V2 b, float da, float db)
```

## Remarks

The distance to the plane are provided as `da` and `db`. You can compute these with e.g. [cf_distance_hs](/math/cf_distance_hs.md), or instead
call the similar function [cf_intersect_halfspace2](/math/cf_intersect_halfspace2.md).

## Related Pages

[CF_Halfspace](/math/cf_halfspace.md)  
[cf_plane](/math/cf_plane.md)  
[cf_origin](/math/cf_origin.md)  
[cf_distance_hs](/math/cf_distance_hs.md)  
[cf_project](/math/cf_project.md)  
[cf_mul_tf_hs](/math/cf_mul_tf_hs.md)  
[cf_mulT_tf_hs](/math/cf_mult_tf_hs.md)  
