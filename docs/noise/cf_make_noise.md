[](../header.md ':include')

# cf_make_noise

Category: [noise](/api_reference?id=noise)  
GitHub: [cute_noise.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_noise.h)  
---

Returns a [CF_Noise](/noise/cf_noise.md) for generating noise at specified points.

```cpp
CF_Noise cf_make_noise(uint64_t seed);
```

Parameters | Description
--- | ---
seed | Used to seed the sequence of numbers generated. Default 0.

## Remarks

You're probably looking for image generation functions such as [cf_noise_pixels](/noise/cf_noise_pixels.md) or [cf_noise_fbm_pixels](/noise/cf_noise_fbm_pixels.md). This
function is fairly low-level and intended for those who know what they're doing.

## Related Pages

[CF_Noise](/noise/cf_noise.md)  
[cf_noise4](/noise/cf_noise4.md)  
[cf_make_noise_fbm](/noise/cf_make_noise_fbm.md)  
[cf_destroy_noise](/noise/cf_destroy_noise.md)  
[cf_noise2](/noise/cf_noise2.md)  
[cf_noise3](/noise/cf_noise3.md)  
