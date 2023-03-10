[](../header.md ':include')

# cf_rnd_seed

Category: [random](/api_reference?id=random)  
GitHub: [cute_rnd.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_rnd.h)  
---

Returns an initialized [CF_Rnd](/random/cf_rnd.md) based on an initial `seed` value.

```cpp
static CF_INLINE CF_Rnd   CF_CALL cf_rnd_seed(uint64_t seed);
```

Parameters | Description
--- | ---
seed | The initial seed value for the random number generator.

## Remarks

The `seed` is used to control which set of random numbers get generated. The numbers are generated in a completely
deterministic way, so it's often important for many games to control or note which seed is used.

## Related Pages

[CF_Rnd](/random/cf_rnd.md)  
[cf_rnd_next](/random/cf_rnd_next.md)  
