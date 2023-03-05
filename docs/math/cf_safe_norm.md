# cf_safe_norm | [math](https://github.com/RandyGaul/cute_framework/blob/master/docs/math/README.md) | [cute_math.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_math.h)

Returns a normalized vector.

```cpp
CF_V2 cf_safe_norm(CF_V2 a)
```

## Remarks

Sets the vector to `{ 0, 0 }` if the length of the vector is zero. Unlike [cf_norm](https://github.com/RandyGaul/cute_framework/blob/master/docs/math/cf_norm.md), this function cannot fail for
the case of a zero vector.

## Related Pages

[CF_V2](https://github.com/RandyGaul/cute_framework/blob/master/docs/math/cf_v2.md)  
[cf_len](https://github.com/RandyGaul/cute_framework/blob/master/docs/math/cf_len.md)  
[cf_distance](https://github.com/RandyGaul/cute_framework/blob/master/docs/math/cf_distance.md)  
[cf_norm](https://github.com/RandyGaul/cute_framework/blob/master/docs/math/cf_norm.md)  
