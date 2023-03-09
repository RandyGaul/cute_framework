[](../header.md ':include')

# CF_BlendFactor

Category: [graphics](https://github.com/RandyGaul/cute_framework/blob/master/docs/api_reference?id=graphics)  
GitHub: [cute_graphics.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_graphics.h)  
---

Blend factors to compose a blend equation.

## Values

Enum | Description
--- | ---
BLENDFACTOR_ZERO | 0
BLENDFACTOR_ONE | 1
BLENDFACTOR_SRC_COLOR | S.color
BLENDFACTOR_ONE_MINUS_SRC_COLOR | (1 - S.rgb)
BLENDFACTOR_SRC_ALPHA | S.alpha
BLENDFACTOR_ONE_MINUS_SRC_ALPHA | (1 - S.alpha)
BLENDFACTOR_DST_COLOR | D.rgb
BLENDFACTOR_ONE_MINUS_DST_COLOR | (1 - D.rgb)
BLENDFACTOR_DST_ALPHA | D.alpha
BLENDFACTOR_ONE_MINUS_DST_ALPHA | (1 - D.alpha)
BLENDFACTOR_SRC_ALPHA_SATURATED | min(S.alpha, 1 - D.alpha)
BLENDFACTOR_BLEND_COLOR | C (constant color not currently supported)
BLENDFACTOR_ONE_MINUS_BLEND_COLOR | 1 - C.rgb (constant color not currently supported)
BLENDFACTOR_BLEND_ALPHA | C.alpha (constant color not currently supported)
BLENDFACTOR_ONE_MINUS_BLEND_ALPHA | (1 - C.alpha) (constant color not currently supported)

## Remarks

See [CF_BlendState](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/cf_blendstate.md) for an overview.

## Related Pages

[CF_BlendState](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/cf_blendstate.md)  
[cf_blend_factor_string](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/cf_blend_factor_string.md)  
