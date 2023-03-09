# CF_M3x2

Category: [math](https://github.com/RandyGaul/cute_framework/blob/master/docs/api_reference?id=math)  
GitHub: [cute_math.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_math.h)  
---

2d transformation.

Struct Members | Description
--- | ---
`CF_M2x2 m` | The top-left 2x2 matrix representing scale + rotation.
`CF_V2 p` | The position column of the matrix.

## Remarks

Mostly useful for graphics and not physics colliders, since it supports scale.

## Related Pages

[cf_invert](https://github.com/RandyGaul/cute_framework/blob/master/docs/math/cf_invert.md)  
[cf_mul_m32_v2](https://github.com/RandyGaul/cute_framework/blob/master/docs/math/cf_mul_m32_v2.md)  
[cf_mul_m32](https://github.com/RandyGaul/cute_framework/blob/master/docs/math/cf_mul_m32.md)  
[cf_make_identity](https://github.com/RandyGaul/cute_framework/blob/master/docs/math/cf_make_identity.md)  
[cf_make_translation](https://github.com/RandyGaul/cute_framework/blob/master/docs/math/cf_make_translation.md)  
[cf_make_scale](https://github.com/RandyGaul/cute_framework/blob/master/docs/math/cf_make_scale.md)  
[cf_make_rotation](https://github.com/RandyGaul/cute_framework/blob/master/docs/math/cf_make_rotation.md)  
[cf_make_transform_TSR](https://github.com/RandyGaul/cute_framework/blob/master/docs/math/cf_make_transform_tsr.md)  
