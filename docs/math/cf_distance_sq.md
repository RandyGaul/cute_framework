# cf_distance_sq

Category: [math](https://github.com/RandyGaul/cute_framework/blob/master/docs/api_reference?id=math)  
GitHub: [cute_math.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_math.h)  
---

Returns the square distance of a point to a line segment.

```cpp
float cf_distance_sq(CF_V2 a, CF_V2 b, CF_V2 p)
```

Parameters | Description
--- | ---
a | The start point of a line segment.
b | The end point of a line segment.
p | The query point.

## Remarks

See [this article](https://randygaul.github.io/math/collision-detection/2014/07/01/Distance-Point-to-Line-Segment.html) for implementation details.

## Related Pages

[CF_V2](https://github.com/RandyGaul/cute_framework/blob/master/docs/math/cf_v2.md)  
[CF_Ray](https://github.com/RandyGaul/cute_framework/blob/master/docs/math/cf_ray.md)  
