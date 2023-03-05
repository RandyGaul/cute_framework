# cf_parallel | [math](https://github.com/RandyGaul/cute_framework/blob/master/docs/math_readme.md) | [cute_math.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_math.h)

Returns true if two vectors are parallel within a `tol` tolerance value.

```cpp
bool cf_parallel(CF_V2 a, CF_V2 b, float tol)
```

## Remarks

You should experiment to find a good `tol` value, such as commonly used values like 1.0e-3f, 1.0e-6f, or 1.0e-8f.
Different orders of magnitude are suitable for different tasks, so it may take some experience to figure out
what a good tolerance is for your situation.

## Related Pages

[CF_V2](https://github.com/RandyGaul/cute_framework/blob/master/docs/math/cf_v2.md)  
[cf_lesser_v2](https://github.com/RandyGaul/cute_framework/blob/master/docs/math/cf_lesser_v2.md)  
[cf_greater_v2](https://github.com/RandyGaul/cute_framework/blob/master/docs/math/cf_greater_v2.md)  
[cf_lesser_equal_v2](https://github.com/RandyGaul/cute_framework/blob/master/docs/math/cf_lesser_equal_v2.md)  
[cf_greater_equal_v2](https://github.com/RandyGaul/cute_framework/blob/master/docs/math/cf_greater_equal_v2.md)  
