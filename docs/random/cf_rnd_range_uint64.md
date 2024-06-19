[](../header.md ':include')

# cf_rnd_range_uint64

Category: [random](/api_reference?id=random)  
GitHub: [cute_rnd.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_rnd.h)  
---

Returns a random `uint64_t` from the range `min` to `max` (inclusive).

```cpp
uint64_t cf_rnd_range_uint64(CF_RndState* rnd, uint64_t min, uint64_t max);
```

Parameters | Description
--- | ---
rnd | The random number generator state.

## Related Pages

[CF_RndState](/random/cf_rndstate.md)  
[cf_rnd_seed](/random/cf_rnd_seed.md)  
[cf_rnd](/random/cf_rnd.md)  
[cf_rnd_float](/random/cf_rnd_float.md)  
[cf_rnd_double](/random/cf_rnd_double.md)  
[cf_rnd_range_int](/random/cf_rnd_range_int.md)  
[cf_rnd_range_double](/random/cf_rnd_range_double.md)  
[cf_rnd_range_float](/random/cf_rnd_range_float.md)  
