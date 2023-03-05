# cf_toi | [math](https://github.com/RandyGaul/cute_framework/blob/master/docs/math_readme.md) | [cute_math.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_math.h)

Computes the time of impact of two shapes.

```cpp
CF_ToiResult cf_toi(const void* A, CF_ShapeType typeA, const CF_Transform* ax_ptr, CF_V2 vA, const void* B, CF_ShapeType typeB, const CF_Transform* bx_ptr, CF_V2 vB, int use_radius);
```

Parameters | Description
--- | ---
A | The first shape.
typeA | The [CF_ShapeType](https://github.com/RandyGaul/cute_framework/blob/master/docs/math/cf_shapetype.md) of the first shape `A`.
ax_ptr | Can be `NULL` to represent an identity transform. An optional pointer to a [CF_Transform](https://github.com/RandyGaul/cute_framework/blob/master/docs/math/cf_transform.md) to transform `A`.
vA | The velocity of `A`.
B | The second shape.
typeA | The [CF_ShapeType](https://github.com/RandyGaul/cute_framework/blob/master/docs/math/cf_shapetype.md) of the second shape `B`.
bx_ptr | Can be `NULL` to represent an identity transform. An optional pointer to a [CF_Transform](https://github.com/RandyGaul/cute_framework/blob/master/docs/math/cf_transform.md) to transform `B`.
vB | The velocity of `B`.
use_radius | True if you want to use the radius of any [CF_Circle](https://github.com/RandyGaul/cute_framework/blob/master/docs/math/cf_circle.md) or [CF_Capsule](https://github.com/RandyGaul/cute_framework/blob/master/docs/math/cf_capsule.md) inputs, false to treat them as a point/line segment respectively (a radius of zero).

## Return Value

Returns a [CF_ToiResult](https://github.com/RandyGaul/cute_framework/blob/master/docs/math/cf_toiresult.md) containing information about the time of impact.

## Remarks

This is an advanced function, intended to be used by people who know what they're doing.

Computes the time of impact from shape A and shape B. The velocity of each shape is provided by `vA` and `vB` respectively. The shapes are
_not_ allowed to rotate over time. The velocity is assumed to represent the change in motion from time 0 to time 1, and so the return value
will be a number from 0 to 1. To move each shape to the colliding configuration, multiply `vA` and `vB` each by the return value.

IMPORTANT NOTE

The [cf_toi](https://github.com/RandyGaul/cute_framework/blob/master/docs/math/cf_toi.md) function can be used to implement a "swept character controller", but it can be difficult to do so. Say we compute a time
of impact with [cf_toi](https://github.com/RandyGaul/cute_framework/blob/master/docs/math/cf_toi.md) and move the shapes to the time of impact, and adjust the velocity by zeroing out the velocity along the surface
normal. If we then call [cf_toi](https://github.com/RandyGaul/cute_framework/blob/master/docs/math/cf_toi.md) again, it will fail since the shapes will be considered to start in a colliding configuration. There are
many styles of tricks to get around this problem, and all of them involve giving the next call to [cf_toi](https://github.com/RandyGaul/cute_framework/blob/master/docs/math/cf_toi.md) some breathing room. It is
recommended to use some variation of the following algorithm:

1. Call [cf_toi](https://github.com/RandyGaul/cute_framework/blob/master/docs/math/cf_toi.md).
2. Move the shapes to the TOI.
3. Slightly inflate the size of one, or both, of the shapes so they will be intersecting.
   The purpose is to make the shapes numerically intersecting, but not visually intersecting.
   Another option is to call [cf_toi](https://github.com/RandyGaul/cute_framework/blob/master/docs/math/cf_toi.md) with slightly deflated shapes.
   See the function [cf_inflate](https://github.com/RandyGaul/cute_framework/blob/master/docs/math/cf_inflate.md) for some more details.
4. Compute the collision manifold between the inflated shapes (for example, use [cf_poly_to_poly_manifold](https://github.com/RandyGaul/cute_framework/blob/master/docs/math/cf_poly_to_poly_manifold.md)).
5. Gently push the shapes apart. This will give the next call to [cf_toi](https://github.com/RandyGaul/cute_framework/blob/master/docs/math/cf_toi.md) some breathing room.

## Related Pages

[CF_ToiResult](https://github.com/RandyGaul/cute_framework/blob/master/docs/math/cf_toiresult.md)  
[CF_ShapeType](https://github.com/RandyGaul/cute_framework/blob/master/docs/math/cf_shapetype.md)  
