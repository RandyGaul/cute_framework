[](../header.md ':include')

# cf_bezier2

Category: [math](/api_reference?id=math)  
GitHub: [cute_math.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_math.h)  
---

Returns a point along a cubic bezier curve according to time `t`.

```cpp
CF_V2 cf_bezier2(CF_V2 a, CF_V2 c0, CF_V2 c1, CF_V2 b, float t)
```

Parameters | Description
--- | ---
a | The start point.
c0 | A control point.
c1 | A control point.
b | The end point.
t | A position along the curve.

## Related Pages

[CF_V2](/math/cf_v2.md)  
[cf_lerp_v2](/math/cf_lerp_v2.md)  
[cf_bezier](/math/cf_bezier.md)  
