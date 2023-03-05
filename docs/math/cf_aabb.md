# CF_Aabb | [math](https://github.com/RandyGaul/cute_framework/blob/master/docs/math/README.md) | [cute_math.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_math.h)

Axis-aligned bounding box. A box that cannot rotate.

Struct Members | Description
--- | ---
`CF_V2 min` | Position.
`CF_V2 max` | Radius.

## Remarks

There are many ways to describ an AABB, such as a point + half-extents, or a point and width/height pair. However, of all
these options the min/max choice is the simplest when it comes to the majority of collision detection routine implementations.

## Related Pages

[cf_aabb_to_poly_manifold](https://github.com/RandyGaul/cute_framework/blob/master/docs/collision/cf_aabb_to_poly_manifold.md)  
[cf_make_aabb](https://github.com/RandyGaul/cute_framework/blob/master/docs/math/cf_make_aabb.md)  
[cf_width](https://github.com/RandyGaul/cute_framework/blob/master/docs/math/cf_width.md)  
[cf_height](https://github.com/RandyGaul/cute_framework/blob/master/docs/math/cf_height.md)  
[cf_center](https://github.com/RandyGaul/cute_framework/blob/master/docs/math/cf_center.md)  
[cf_top_left](https://github.com/RandyGaul/cute_framework/blob/master/docs/math/cf_top_left.md)  
[cf_top_right](https://github.com/RandyGaul/cute_framework/blob/master/docs/math/cf_top_right.md)  
[cf_bottom_left](https://github.com/RandyGaul/cute_framework/blob/master/docs/math/cf_bottom_left.md)  
[cf_bottom_right](https://github.com/RandyGaul/cute_framework/blob/master/docs/math/cf_bottom_right.md)  
[cf_contains_point](https://github.com/RandyGaul/cute_framework/blob/master/docs/math/cf_contains_point.md)  
[cf_overlaps](https://github.com/RandyGaul/cute_framework/blob/master/docs/math/cf_overlaps.md)  
[cf_make_aabb_verts](https://github.com/RandyGaul/cute_framework/blob/master/docs/math/cf_make_aabb_verts.md)  
[cf_aabb_verts](https://github.com/RandyGaul/cute_framework/blob/master/docs/math/cf_aabb_verts.md)  
[cf_circle_to_aabb](https://github.com/RandyGaul/cute_framework/blob/master/docs/collision/cf_circle_to_aabb.md)  
[cf_aabb_to_aabb](https://github.com/RandyGaul/cute_framework/blob/master/docs/collision/cf_aabb_to_aabb.md)  
[cf_aabb_to_capsule](https://github.com/RandyGaul/cute_framework/blob/master/docs/collision/cf_aabb_to_capsule.md)  
[cf_aabb_to_poly](https://github.com/RandyGaul/cute_framework/blob/master/docs/collision/cf_aabb_to_poly.md)  
[cf_ray_to_aabb](https://github.com/RandyGaul/cute_framework/blob/master/docs/collision/cf_ray_to_aabb.md)  
[cf_circle_to_aabb_manifold](https://github.com/RandyGaul/cute_framework/blob/master/docs/collision/cf_circle_to_aabb_manifold.md)  
[cf_aabb_to_aabb_manifold](https://github.com/RandyGaul/cute_framework/blob/master/docs/collision/cf_aabb_to_aabb_manifold.md)  
[cf_aabb_to_capsule_manifold](https://github.com/RandyGaul/cute_framework/blob/master/docs/collision/cf_aabb_to_capsule_manifold.md)  
