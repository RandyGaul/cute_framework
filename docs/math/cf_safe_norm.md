[](../header.md ':include')

# cf_safe_norm

Category: [math](/api_reference?id=math)  
GitHub: [cute_math.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_math.h)  
---

Returns a normalized vector.

```cpp
CF_INLINE CF_V2 cf_safe_norm(CF_V2 a)
```

## Remarks

Sets the vector to `{ 0, 0 }` if the length of the vector is zero. Unlike [cf_norm](/math/cf_norm.md), this function cannot fail for
the case of a zero vector.

## Related Pages

[CF_V2](/math/cf_v2.md)  
[cf_len](/math/cf_len.md)  
[cf_distance](/math/cf_distance.md)  
[cf_norm](/math/cf_norm.md)  
