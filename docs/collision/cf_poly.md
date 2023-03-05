# CF_Poly | [collision](https://github.com/RandyGaul/cute_framework/blob/master/docs/collision/README.md) | [cute_math.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_math.h)

2D polygon.

Struct Members | Description
--- | ---
`int count` | The number of vertices in the polygon, capped at [CF_POLY_MAX_VERTS](https://github.com/RandyGaul/cute_framework/blob/master/docs/collision/cf_poly_max_verts.md).
`CF_V2 verts[CF_POLY_MAX_VERTS]` | The vertices of the polygon, capped at [CF_POLY_MAX_VERTS](https://github.com/RandyGaul/cute_framework/blob/master/docs/collision/cf_poly_max_verts.md).
`CF_V2 norms[CF_POLY_MAX_VERTS]` | The normals of the polygon, capped at [CF_POLY_MAX_VERTS](https://github.com/RandyGaul/cute_framework/blob/master/docs/collision/cf_poly_max_verts.md). Each normal is perpendicular along the poly's surface.

## Remarks

Verts are ordered in counter-clockwise order (CCW).

## Related Pages

[CF_POLY_MAX_VERTS](https://github.com/RandyGaul/cute_framework/blob/master/docs/collision/cf_poly_max_verts.md)  
[cf_poly_to_poly_manifold](https://github.com/RandyGaul/cute_framework/blob/master/docs/collision/cf_poly_to_poly_manifold.md)  
[cf_circle_to_poly](https://github.com/RandyGaul/cute_framework/blob/master/docs/collision/cf_circle_to_poly.md)  
[cf_aabb_to_poly](https://github.com/RandyGaul/cute_framework/blob/master/docs/collision/cf_aabb_to_poly.md)  
[cf_capsule_to_poly](https://github.com/RandyGaul/cute_framework/blob/master/docs/collision/cf_capsule_to_poly.md)  
[cf_poly_to_poly](https://github.com/RandyGaul/cute_framework/blob/master/docs/collision/cf_poly_to_poly.md)  
[cf_ray_to_poly](https://github.com/RandyGaul/cute_framework/blob/master/docs/collision/cf_ray_to_poly.md)  
[cf_circle_to_poly_manifold](https://github.com/RandyGaul/cute_framework/blob/master/docs/collision/cf_circle_to_poly_manifold.md)  
[cf_aabb_to_poly_manifold](https://github.com/RandyGaul/cute_framework/blob/master/docs/collision/cf_aabb_to_poly_manifold.md)  
[cf_capsule_to_poly_manifold](https://github.com/RandyGaul/cute_framework/blob/master/docs/collision/cf_capsule_to_poly_manifold.md)  
