[](../header.md ':include')

# cf_softlight

Category: [graphics](/api_reference?id=graphics)  
GitHub: [cute_color.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_color.h)  
---

Returns a softlight blend, where the colors are darkened or lightened depending on the `blend` color.

```cpp
CF_INLINE float cf_softlight(float base, float blend)
```

Parameters | Description
--- | ---
base | The original color.
blend | The blend color to apply a softlight effect with.

## Remarks

This function attempts to mimic the Softlight [Photoshop blend-layer](https://helpx.adobe.com/photoshop/using/blending-modes.html) mode.
The `blend` color is used to adjust colors in the `base`, while still preserving shadows and highlights of the `base`.

## Related Pages

[CF_Color](/graphics/cf_color.md)  
[cf_hue](/graphics/cf_hue.md)  
[cf_overlay_color](/graphics/cf_overlay_color.md)  
[cf_softlight_color](/graphics/cf_softlight_color.md)  
[cf_overlay](/graphics/cf_overlay.md)  
