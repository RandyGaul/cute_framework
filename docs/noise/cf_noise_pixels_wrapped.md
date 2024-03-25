[](../header.md ':include')

# cf_noise_pixels_wrapped

Category: [noise](/api_reference?id=noise)  
GitHub: [cute_noise.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_noise.h)  
---

Creates an image from noise that can animate in a loop, and tiles seamlessly.

```cpp
CF_Pixel* cf_noise_pixels_wrapped(int w, int h, uint64_t seed, float scale, float time, float time_amplitude);
```

Parameters | Description
--- | ---
w | The width of the image.
h | The height of the image.
seed | Used to seed the sequence of numbers generated. Default 0.
scale | Scales up or down the noise in the image, like zooming in or out. Default 1.0f.
time | A time parameter for animation.
time_amplitude | Adjusts how much the animation evolves over the period. Default 1.0f.

## Return Value

Returns a generated image filled with noise.

## Remarks

The generated image can be animated over a loop, and tiles seamlessly in the x-y directions. To control the animation
pass in a float starting at 0, and incremented with [CF_DELTA_TIME](/time/cf_delta_time.md) each game tick. This will loop the animation
over a one-second period. You can divide your accumulated time by a frequency to set a number of seconds to loop over.

If you want the animation to move faster without adjusting the loop time, then adjust `time_amplitude`. This scales how
much the animation will evolve over time. Higher values will have faster and more volatile looking motions.

## Related Pages

[cf_noise_pixels](/noise/cf_noise_pixels.md)  
[cf_noise_fbm_pixels_wrapped](/noise/cf_noise_fbm_pixels_wrapped.md)  
[cf_noise_fbm_pixels](/noise/cf_noise_fbm_pixels.md)  
