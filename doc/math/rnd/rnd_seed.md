# rnd_seed

Initializes a seeded psuedo-random number generator.

## Syntax

```cpp
rnd_t rnd_seed(uint64_t seed);
```

## Function Parameters

Parameter Name | Description
--- | ---
seed | The seed to initialize `rnd_t` with.

## Return Value

Returns a ready-to-go psuedo-random number generator.

## Remarks

`rnd_t` is returned by value. It's a very small struct of two `uint64_t` integers, so simply copying it around the stack is encouraged.

## Related Functions

[rnd_next](https://github.com/RandyGaul/cute_framework/blob/master/doc/math/rnd/rnd_next.md)  
[rnd_next_float](https://github.com/RandyGaul/cute_framework/blob/master/doc/math/rnd/rnd_next_float.md)  
[rnd_next_double](https://github.com/RandyGaul/cute_framework/blob/master/doc/math/rnd/rnd_next_double.md)  
[rnd_next_range](https://github.com/RandyGaul/cute_framework/blob/master/doc/math/rnd/rnd_next_range.md)  
