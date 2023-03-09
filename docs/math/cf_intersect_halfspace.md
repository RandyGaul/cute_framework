[](../header.md ':include')

# cf_intersect_halfspace

Category: [math](https://github.com/RandyGaul/cute_framework/blob/master/docs/api_reference?id=math)  
GitHub: [cute_math.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_math.h)  
---

Returns the intersection point of two points to a plane.

```cpp
CF_V2 cf_intersect_halfspace(CF_V2 a, CF_V2 b, float da, float db)
```

## Remarks

The distance to the plane are provided as `da` and `db`. You can compute these with e.g. [cf_distance_hs](https://github.com/RandyGaul/cute_framework/blob/master/docs/math/cf_distance_hs.md), or instead
call the similar function [cf_intersect_halfspace2](https://github.com/RandyGaul/cute_framework/blob/master/docs/math/cf_intersect_halfspace2.md).

## Related Pages

[CF_Halfspace](https://github.com/RandyGaul/cute_framework/blob/master/docs/math/cf_halfspace.md)  
[cf_plane](https://github.com/RandyGaul/cute_framework/blob/master/docs/math/cf_plane.md)  
[cf_origin](https://github.com/RandyGaul/cute_framework/blob/master/docs/math/cf_origin.md)  
[cf_distance_hs](https://github.com/RandyGaul/cute_framework/blob/master/docs/math/cf_distance_hs.md)  
[cf_project](https://github.com/RandyGaul/cute_framework/blob/master/docs/math/cf_project.md)  
[cf_mul_tf_hs](https://github.com/RandyGaul/cute_framework/blob/master/docs/math/cf_mul_tf_hs.md)  
[cf_mulT_tf_hs](https://github.com/RandyGaul/cute_framework/blob/master/docs/math/cf_mult_tf_hs.md)  
