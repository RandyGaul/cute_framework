[](../header.md ':include')

# cf_noise3

Category: [noise](/api_reference?id=noise)  
GitHub: [cute_noise.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_noise.h)  
---

Generates a random value given a 3D coordinate.

```cpp
float cf_noise3(CF_Noise noise, float x, float y, float z);
```

Parameters | Description
--- | ---
noise | The noise settings.
x | Noise at this x-component.
y | Noise at this y-component.
z | Noise at this z-component.

## Return Value

Returns a random value at the specified point.

## Remarks

You're probably looking for image generation functions such as [cf_noise_pixels](/noise/cf_noise_pixels.md) or [cf_noise_fbm_pixels](/noise/cf_noise_fbm_pixels.md). This
function is fairly low-level and intended for those who know what they're doing.

## Related Pages

[CF_Noise](/noise/cf_noise.md)  
[cf_make_noise](/noise/cf_make_noise.md)  
[cf_destroy_noise](/noise/cf_destroy_noise.md)  
[cf_noise2](/noise/cf_noise2.md)  
[cf_noise4](/noise/cf_noise4.md)  
