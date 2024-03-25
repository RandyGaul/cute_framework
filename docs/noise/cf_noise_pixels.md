[](../header.md ':include')

# cf_noise_pixels

Category: [noise](/api_reference?id=noise)  
GitHub: [cute_noise.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_noise.h)  
---

Creates an image from noise.

```cpp
CF_Pixel* cf_noise_pixels(int w, int h, uint64_t seed, float scale);
```

Parameters | Description
--- | ---
w | The width of the image.
h | The height of the image.
seed | Used to seed the sequence of numbers generated. Default 0.
scale | Scales up or down the noise in the image, like zooming in or out. Default 1.0f.

## Return Value

Returns a generated image filled with noise.

## Remarks

The generated noise is quite simple -- you're probably looking for the more advanced [cf_noise_fbm_pixels](/noise/cf_noise_fbm_pixels.md), or [cf_noise_fbm_pixels_wrapped](/noise/cf_noise_fbm_pixels_wrapped.md).

## Related Pages

[cf_noise_fbm_pixels_wrapped](/noise/cf_noise_fbm_pixels_wrapped.md)  
[cf_noise_pixels_wrapped](/noise/cf_noise_pixels_wrapped.md)  
[cf_noise_fbm_pixels](/noise/cf_noise_fbm_pixels.md)  
