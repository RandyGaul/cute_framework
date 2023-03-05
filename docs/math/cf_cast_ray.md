# cf_cast_ray | [math](https://github.com/RandyGaul/cute_framework/blob/master/docs/math_readme.md) | [cute_math.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_math.h)

Casts a ray onto a shape.

```cpp
bool cf_cast_ray(CF_Ray A, const void* B, const CF_Transform* bx, CF_ShapeType typeB, CF_Raycast* out);
```

Parameters | Description
--- | ---
A | The ray.
B | The shape.
typeB | The [CF_ShapeType](https://github.com/RandyGaul/cute_framework/blob/master/docs/math/cf_shapetype.md) of the shape `B`.
bx_ptr | Can be `NULL` to represent an identity transform. An optional pointer to a [CF_Transform](https://github.com/RandyGaul/cute_framework/blob/master/docs/math/cf_transform.md) to transform `B`.
out | Can be `NULL`. [CF_Raycast](https://github.com/RandyGaul/cute_framework/blob/master/docs/math/cf_raycast.md) results are placed here (contains normal + time of impact).

## Return Value

Returns true if the ray hit the shape.

## Related Pages

[CF_Ray](https://github.com/RandyGaul/cute_framework/blob/master/docs/math/cf_ray.md)  
[CF_Raycast](https://github.com/RandyGaul/cute_framework/blob/master/docs/math/cf_raycast.md)  
[CF_Transform](https://github.com/RandyGaul/cute_framework/blob/master/docs/math/cf_transform.md)  
[CF_ShapeType](https://github.com/RandyGaul/cute_framework/blob/master/docs/math/cf_shapetype.md)  
