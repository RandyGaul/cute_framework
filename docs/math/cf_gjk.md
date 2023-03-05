# cf_gjk | [math](https://github.com/RandyGaul/cute_framework/blob/master/docs/math/README.md) | [cute_math.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_math.h)

Returns the distance between two shapes, and computes the closest two points of the shapes.

```cpp
float cf_gjk(const void* A, CF_ShapeType typeA, const CF_Transform* ax_ptr, const void* B, CF_ShapeType typeB, const CF_Transform* bx_ptr, CF_V2* outA, CF_V2* outB, bool use_radius, int* iterations, CF_GjkCache* cache);
```

Parameters | Description
--- | ---
A | The first shape.
typeA | The [CF_ShapeType](https://github.com/RandyGaul/cute_framework/blob/master/docs/math/cf_shapetype.md) of the first shape `A`.
ax_ptr | Can be `NULL` to represent an identity transform. An optional pointer to a [CF_Transform](https://github.com/RandyGaul/cute_framework/blob/master/docs/math/cf_transform.md) to transform `A`.
B | The second shape.
typeA | The [CF_ShapeType](https://github.com/RandyGaul/cute_framework/blob/master/docs/math/cf_shapetype.md) of the second shape `B`.
bx_ptr | Can be `NULL` to represent an identity transform. An optional pointer to a [CF_Transform](https://github.com/RandyGaul/cute_framework/blob/master/docs/math/cf_transform.md) to transform `B`.
outA | The closest point on `A` to `B`. Not well defined if the two shapes are already intersecting.
outB | The closest point on `B` to `A`. Not well defined if the two shapes are already intersecting.
use_radius | True if you want to use the radius of any [CF_Circle](https://github.com/RandyGaul/cute_framework/blob/master/docs/math/cf_circle.md) or [CF_Capsule](https://github.com/RandyGaul/cute_framework/blob/master/docs/math/cf_capsule.md) inputs, false to treat them as a point/line segment respectively (a radius of zero).
iterations | Can be `NULL`. The number of internal GJK iterations that occurred. For debugging.
cache | Can be `NULL`. An optional cache to a previous call of this function. See [CF_GjkCache](https://github.com/RandyGaul/cute_framework/blob/master/docs/math/cf_gjkcache.md) for details.

## Return Value

Returns the distance between the two shapes.

## Remarks

This is an advanced function, intended to be used by people who know what they're doing.

The GJK function is sensitive to large shapes, since it internally will compute signed area values. [cf_gjk](https://github.com/RandyGaul/cute_framework/blob/master/docs/math/cf_gjk.md) is called throughout
this file in many ways, so try to make sure all of your collision shapes are not gigantic. For example, try to keep the volume of
all your shapes less than 100.0f. If you need large shapes, you should use tiny collision geometry for all function here, and simply
render the geometry larger on-screen by scaling it up.

## Related Pages

[CF_ShapeType](https://github.com/RandyGaul/cute_framework/blob/master/docs/math/cf_shapetype.md)  
[CF_GjkCache](https://github.com/RandyGaul/cute_framework/blob/master/docs/math/cf_gjkcache.md)  
