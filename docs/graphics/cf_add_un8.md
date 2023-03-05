# cf_add_un8 | [graphics](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics_readme.md) | [cute_color.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_color.h)

Returns the 8-bit unsigned addition of two input 8-bit numbers, intended for implementing 8-bit color blend operations.

```cpp
uint8_t cf_add_un8(int a, int b)
```

Parameters | Description
--- | ---
a | An 8-bit value promoted to an `int` upon calling.
b | An 8-bit value promoted to an `int` upon calling.

## Remarks

This is a helper-function intended for advanced users.
The core calculation is done with full integers to help avoid intermediate overflows on 8-bit values.

## Related Pages

[cf_mul_un8](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/cf_mul_un8.md)  
[cf_div_un8](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/cf_div_un8.md)  
[cf_sub_un8](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/cf_sub_un8.md)  
