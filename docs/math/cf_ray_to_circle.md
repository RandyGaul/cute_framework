# cf_ray_to_circle | [math](https://github.com/RandyGaul/cute_framework/blob/master/docs/math/README.md) | [cute_math.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_math.h)

Returns true if a ray hits a circle.

```cpp
bool cf_ray_to_circle(CF_Ray A, CF_Circle B, CF_Raycast* out);
```

Parameters | Description
--- | ---
A | The ray.
B | The circle.
out | Can be `NULL`. [CF_Raycast](https://github.com/RandyGaul/cute_framework/blob/master/docs/math/cf_raycast.md) results are placed here (contains normal + time of impact).

## Related Pages

[CF_Ray](https://github.com/RandyGaul/cute_framework/blob/master/docs/math/cf_ray.md)  
[CF_Circle](https://github.com/RandyGaul/cute_framework/blob/master/docs/math/cf_circle.md)  
[CF_Raycast](https://github.com/RandyGaul/cute_framework/blob/master/docs/math/cf_raycast.md)  
[cf_ray_to_poly](https://github.com/RandyGaul/cute_framework/blob/master/docs/math/cf_ray_to_poly.md)  
[cf_ray_to_aabb](https://github.com/RandyGaul/cute_framework/blob/master/docs/math/cf_ray_to_aabb.md)  
[cf_ray_to_capsule](https://github.com/RandyGaul/cute_framework/blob/master/docs/math/cf_ray_to_capsule.md)  
