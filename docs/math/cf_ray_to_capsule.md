# cf_ray_to_capsule | [math](https://github.com/RandyGaul/cute_framework/blob/master/docs/math/README.md) | [cute_math.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_math.h)

Returns true if a ray hits a capsule.

```cpp
bool cf_ray_to_capsule(CF_Ray A, CF_Capsule B, CF_Raycast* out);
```

Parameters | Description
--- | ---
A | The ray.
B | The capsule.
out | Can be `NULL`. [CF_Raycast](https://github.com/RandyGaul/cute_framework/blob/master/docs/math/cf_raycast.md) results are placed here (contains normal + time of impact).

## Related Pages

[CF_Ray](https://github.com/RandyGaul/cute_framework/blob/master/docs/math/cf_ray.md)  
[CF_Capsule](https://github.com/RandyGaul/cute_framework/blob/master/docs/math/cf_capsule.md)  
[CF_Raycast](https://github.com/RandyGaul/cute_framework/blob/master/docs/math/cf_raycast.md)  
[cf_ray_to_circle](https://github.com/RandyGaul/cute_framework/blob/master/docs/math/cf_ray_to_circle.md)  
[cf_ray_to_aabb](https://github.com/RandyGaul/cute_framework/blob/master/docs/math/cf_ray_to_aabb.md)  
[cf_ray_to_poly](https://github.com/RandyGaul/cute_framework/blob/master/docs/math/cf_ray_to_poly.md)  
