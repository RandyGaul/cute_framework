# CF_Manifold | [collision](https://github.com/RandyGaul/cute_framework/blob/master/docs/collision/README.md) | [cute_math.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_math.h)

Contains all information necessary to resolve a collision.

Struct Members | Description
--- | ---
`int count` | The number of points in the manifold (0, 1 or 2).
`float depths[2]` | The collision depth of each point in the manifold.
`CF_V2 contact_points[2]` | Up to two points on the contact plane that sufficiently (and minimally) describe how two shapes are touching.
`CF_V2 n` | Always points from shape A to shape B.

## Remarks

This is the information needed to separate shapes that are colliding.

## Related Pages

[cf_poly_to_poly_manifold](https://github.com/RandyGaul/cute_framework/blob/master/docs/collision/cf_poly_to_poly_manifold.md)  
[cf_circle_to_circle_manifold](https://github.com/RandyGaul/cute_framework/blob/master/docs/collision/cf_circle_to_circle_manifold.md)  
[cf_circle_to_aabb_manifold](https://github.com/RandyGaul/cute_framework/blob/master/docs/collision/cf_circle_to_aabb_manifold.md)  
[cf_circle_to_capsule_manifold](https://github.com/RandyGaul/cute_framework/blob/master/docs/collision/cf_circle_to_capsule_manifold.md)  
[cf_aabb_to_aabb_manifold](https://github.com/RandyGaul/cute_framework/blob/master/docs/collision/cf_aabb_to_aabb_manifold.md)  
[cf_aabb_to_capsule_manifold](https://github.com/RandyGaul/cute_framework/blob/master/docs/collision/cf_aabb_to_capsule_manifold.md)  
[cf_capsule_to_capsule_manifold](https://github.com/RandyGaul/cute_framework/blob/master/docs/collision/cf_capsule_to_capsule_manifold.md)  
[cf_circle_to_poly_manifold](https://github.com/RandyGaul/cute_framework/blob/master/docs/collision/cf_circle_to_poly_manifold.md)  
[cf_aabb_to_poly_manifold](https://github.com/RandyGaul/cute_framework/blob/master/docs/collision/cf_aabb_to_poly_manifold.md)  
[cf_capsule_to_poly_manifold](https://github.com/RandyGaul/cute_framework/blob/master/docs/collision/cf_capsule_to_poly_manifold.md)  
