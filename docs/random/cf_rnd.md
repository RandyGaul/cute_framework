# CF_Rnd

Category: [random](https://github.com/RandyGaul/cute_framework/blob/master/docs/api_reference?id=random)  
GitHub: [cute_rnd.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_rnd.h)  
---

A random number generator.

Struct Members | Description
--- | ---
`uint64_t state[2]` | Just two `uint64_t`'s for the internal state. Very small! These are setup by [cf_rnd_seed](https://github.com/RandyGaul/cute_framework/blob/master/docs/random/cf_rnd_seed.md).

## Remarks

A random number generator of the type LFSR (linear feedback shift registers). This specific
implementation uses the XorShift+ variation, and returns 64-bit random numbers. More information
can be found on Wikipedia.
https://en.wikipedia.org/wiki/Xorshift

This implementation comes from Mattias Gustavsson's single-file header collection.
https://github.com/mattiasgustavsson/libs/blob/main/rnd.h

## Related Pages

[cf_rnd_next](https://github.com/RandyGaul/cute_framework/blob/master/docs/random/cf_rnd_next.md)  
[cf_rnd_seed](https://github.com/RandyGaul/cute_framework/blob/master/docs/random/cf_rnd_seed.md)  
