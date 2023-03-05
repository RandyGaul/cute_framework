# CF_Ray | [math](https://github.com/RandyGaul/cute_framework/blob/master/docs/math_readme.md) | [cute_math.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_math.h)

A ray.

Struct Members | Description
--- | ---
`CF_V2 p` | Position.
`CF_V2 d` | Direction (normalized).
`float t` | Distance along d from position p to find endpoint of ray.

## Remarks

A ray is a directional line segment. It starts at an endpoint and extends into another direction for a specified distance (defined by `t`).

## Related Pages

[cf_ray_to_poly](https://github.com/RandyGaul/cute_framework/blob/master/docs/math/cf_ray_to_poly.md)  
[cf_impact](https://github.com/RandyGaul/cute_framework/blob/master/docs/math/cf_impact.md)  
[cf_endpoint](https://github.com/RandyGaul/cute_framework/blob/master/docs/math/cf_endpoint.md)  
[cf_ray_to_halfpsace](https://github.com/RandyGaul/cute_framework/blob/master/docs/math/cf_ray_to_halfpsace.md)  
[cf_ray_to_circle](https://github.com/RandyGaul/cute_framework/blob/master/docs/math/cf_ray_to_circle.md)  
[cf_ray_to_aabb](https://github.com/RandyGaul/cute_framework/blob/master/docs/math/cf_ray_to_aabb.md)  
[cf_ray_to_capsule](https://github.com/RandyGaul/cute_framework/blob/master/docs/math/cf_ray_to_capsule.md)  
