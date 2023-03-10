[](../header.md ':include')

# cf_inflate

Category: [collision](/api_reference?id=collision)  
GitHub: [cute_math.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_math.h)  
---

Inflates a shape.

```cpp
CF_API void CF_CALL cf_inflate(void* shape, CF_ShapeType type, float skin_factor);
```

Parameters | Description
--- | ---
shape | The shape.
type | The [CF_ShapeType](/collision/cf_shapetype.md) of `shape`.
skin_factor | The amount to inflate the shape by.

## Remarks

This is useful to numerically grow or shrink a shape. For example, when calling a time of impact function it can be good to use
a slightly smaller shape. Then, once both shapes are moved to the time of impact a collision manifold can be made from the
slightly larger (and now overlapping) shapes.

IMPORTANT NOTE

Inflating a shape with sharp corners can cause those corners to move dramatically. Deflating a shape can avoid this problem,
but deflating a very small shape can invert the planes and result in something that is no longer convex. Make sure to pick an
appropriately small skin factor, for example 1.0e-6f.

## Related Pages

[cf_gjk](/collision/cf_gjk.md)  
[cf_toi](/collision/cf_toi.md)  
[CF_ShapeType](/collision/cf_shapetype.md)  
