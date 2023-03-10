[](../header.md ':include')

# cf_circle_to_circle

Category: [collision](/api_reference?id=collision)  
GitHub: [cute_math.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_math.h)  
---

Returns true if two circles are intersecting.

```cpp
CF_API bool CF_CALL cf_circle_to_circle(CF_Circle A, CF_Circle B);
```

## Remarks

For information about _how_ two shapes are intersecting (and not just boolean result), see [cf_circle_to_circle_manifold](/collision/cf_circle_to_circle_manifold.md).

## Related Pages

[CF_Circle](/math/cf_circle.md)  
[cf_circle_to_circle_manifold](/collision/cf_circle_to_circle_manifold.md)  
