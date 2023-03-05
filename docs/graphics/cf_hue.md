# cf_hue | [graphics](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/README.md) | [cute_color.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_color.h)

Returns a result color with the luminance and saturation of the base color and the hue of the blend color.

```cpp
CF_Color cf_hue(CF_Color base, CF_Color tint)
```

Parameters | Description
--- | ---
base | The original color.
tint | The blend color to apply a hue-tint effect with.

## Remarks

This function attempts to mimic the Hue [Photoshop blend-layer](https://helpx.adobe.com/photoshop/using/blending-modes.html) mode.

## Related Pages

[CF_Color](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/cf_color.md)  
[cf_softlight](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/cf_softlight.md)  
[cf_overlay_color](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/cf_overlay_color.md)  
[cf_softlight_color](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/cf_softlight_color.md)  
[cf_overlay](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/cf_overlay.md)  
