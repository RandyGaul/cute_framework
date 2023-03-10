[](../header.md ':include')

# cf_max_v2

Category: [math](/api_reference?id=math)  
GitHub: [cute_math.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_math.h)  
---

Returns the component-wise maximum of two vectors.

```cpp
CF_INLINE CF_V2 cf_max_v2(CF_V2 a, CF_V2 b)
```

## Remarks

The vector returned has the value `{ cf_max(a.x, b.x), cf_max(a.y, b.y) }`. See [cf_max](/math/cf_max.md).

## Related Pages

[CF_V2](/math/cf_v2.md)  
[cf_min_v2](/math/cf_min_v2.md)  
[cf_hmax](/math/cf_hmax.md)  
[cf_clamp_v2](/math/cf_clamp_v2.md)  
[cf_clamp01_v2](/math/cf_clamp01_v2.md)  
[cf_abs_v2](/math/cf_abs_v2.md)  
[cf_hmin](/math/cf_hmin.md)  
