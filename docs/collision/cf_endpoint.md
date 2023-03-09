# cf_endpoint

Category: [collision](https://github.com/RandyGaul/cute_framework/blob/master/docs/api_reference?id=collision)  
GitHub: [cute_math.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_math.h)  
---

Returns the endpoint of a ray.

```cpp
CF_V2 cf_endpoint(CF_Ray r)
```

## Remarks

Rays are defined to have an endpoint as an optimization. Usually infinite rays are not needed in games, and cause
unnecessarily large computations when doing raycasts.

## Related Pages

[CF_Ray](https://github.com/RandyGaul/cute_framework/blob/master/docs/math/cf_ray.md)  
[cf_impact](https://github.com/RandyGaul/cute_framework/blob/master/docs/collision/cf_impact.md)  
