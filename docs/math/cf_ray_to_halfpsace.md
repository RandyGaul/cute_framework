# cf_ray_to_halfpsace | [math](https://github.com/RandyGaul/cute_framework/blob/master/docs/math_readme.md) | [cute_math.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_math.h)

Returns true if the ray hits a given plane.

```cpp
bool cf_ray_to_halfpsace(CF_Ray A, CF_Halfspace B, CF_Raycast* out)
```

Parameters | Description
--- | ---
A | The ray.
B | The plane.
out | Can be `NULL`. [CF_Raycast](https://github.com/RandyGaul/cute_framework/blob/master/docs/math/cf_raycast.md) results are placed here (contains normal + time of impact).

## Related Pages

[CF_Ray](https://github.com/RandyGaul/cute_framework/blob/master/docs/math/cf_ray.md)  
