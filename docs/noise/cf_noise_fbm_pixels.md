[](../header.md ':include')

# cf_noise_fbm_pixels

Category: [noise](/api_reference?id=noise)  
GitHub: [cute_noise.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_noise.h)  
---

Creates an image from noise using fractal brownian motion.

```cpp
CF_Pixel* cf_noise_fbm_pixels(int w, int h, uint64_t seed, float scale, float lacunarity, int octaves, float falloff);
```

Parameters | Description
--- | ---
w | The width of the image.
h | The height of the image.
seed | Used to seed the sequence of numbers generated. Default 0.
scale | Scales up or down the noise in the image, like zooming in or out. Default 1.0f.
lacunarity | The difference in the period of the noise between each octave. Default 2.0f.
octaves | The number of octaves to sum together. Higher numbers has worse performance. Default 3.
falloff | How much contribution higher octaves have compared to lower ones. Default 0.5f.

## Return Value

Returns a generated image filled with noise.

## Remarks

If you want the image to animate over a loop, or tile seamlessly, then check out [cf_noise_fbm_pixels_wrapped](/noise/cf_noise_fbm_pixels_wrapped.md).

## Related Pages

[cf_noise_pixels](/noise/cf_noise_pixels.md)  
[cf_noise_pixels_wrapped](/noise/cf_noise_pixels_wrapped.md)  
[cf_noise_fbm_pixels_wrapped](/noise/cf_noise_fbm_pixels_wrapped.md)  
