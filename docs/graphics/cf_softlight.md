# cf_softlight | [graphics](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/README.md) | [cute_color.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_color.h)

Returns a softlight blend, where the colors are darkened or lightened depending on the `blend` color.

```cpp
float cf_softlight(float base, float blend)
```

Parameters | Description
--- | ---
base | The original color.
blend | The blend color to apply a softlight effect with.

## Remarks

This function attempts to mimic the Softlight [Photoshop blend-layer](https://helpx.adobe.com/photoshop/using/blending-modes.html) mode.
The `blend` color is used to adjust colors in the `base`, while still preserving shadows and highlights of the `base`.

## Related Pages

[CF_Color](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/cf_color.md)  
[cf_hue](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/cf_hue.md)  
[cf_overlay_color](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/cf_overlay_color.md)  
[cf_softlight_color](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/cf_softlight_color.md)  
[cf_overlay](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/cf_overlay.md)  
