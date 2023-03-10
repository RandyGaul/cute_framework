[](../header.md ':include')

# cf_hue

Category: [graphics](/api_reference?id=graphics)  
GitHub: [cute_color.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_color.h)  
---

Returns a result color with the luminance and saturation of the base color and the hue of the blend color.

```cpp
CF_INLINE CF_Color cf_hue(CF_Color base, CF_Color tint)
```

Parameters | Description
--- | ---
base | The original color.
tint | The blend color to apply a hue-tint effect with.

## Remarks

This function attempts to mimic the Hue [Photoshop blend-layer](https://helpx.adobe.com/photoshop/using/blending-modes.html) mode.

## Related Pages

[CF_Color](/graphics/cf_color.md)  
[cf_softlight](/graphics/cf_softlight.md)  
[cf_overlay_color](/graphics/cf_overlay_color.md)  
[cf_softlight_color](/graphics/cf_softlight_color.md)  
[cf_overlay](/graphics/cf_overlay.md)  
