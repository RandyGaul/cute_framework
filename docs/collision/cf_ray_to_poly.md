[](../header.md ':include')

# cf_ray_to_poly

Category: [collision](/api_reference?id=collision)  
GitHub: [cute_math.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_math.h)  
---

Returns true if a ray hits a polygon.

```cpp
CF_API bool CF_CALL cf_ray_to_poly(CF_Ray A, const CF_Poly* B, const CF_Transform* bx_ptr, CF_Raycast* out);
```

Parameters | Description
--- | ---
A | The ray.
B | The polygon.
out | Can be `NULL`. [CF_Raycast](/math/cf_raycast.md) results are placed here (contains normal + time of impact).

## Related Pages

[CF_Ray](/math/cf_ray.md)  
[CF_Poly](/collision/cf_poly.md)  
[CF_Raycast](/math/cf_raycast.md)  
[cf_ray_to_circle](/collision/cf_ray_to_circle.md)  
[cf_ray_to_aabb](/collision/cf_ray_to_aabb.md)  
[cf_ray_to_capsule](/collision/cf_ray_to_capsule.md)  
