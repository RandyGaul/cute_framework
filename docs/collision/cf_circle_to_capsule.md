[](../header.md ':include')

# cf_circle_to_capsule

Category: [collision](/api_reference?id=collision)  
GitHub: [cute_math.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_math.h)  
---

Returns true if a circle is intersecting with an capsule.

```cpp
CF_API bool CF_CALL cf_circle_to_capsule(CF_Circle A, CF_Capsule B);
```

## Remarks

For information about _how_ two shapes are intersecting (and not just boolean result), see [cf_circle_to_capsule_manifold](/collision/cf_circle_to_capsule_manifold.md).

## Related Pages

[CF_Circle](/math/cf_circle.md)  
[CF_Capsule](/collision/cf_capsule.md)  
[cf_circle_to_capsule_manifold](/collision/cf_circle_to_capsule_manifold.md)  
