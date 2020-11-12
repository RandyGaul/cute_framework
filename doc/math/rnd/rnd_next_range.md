# rnd_next_range

Returns a random value within a specified range.

## Syntax

```cpp
static int      rnd_next_range(rnd_t* rnd, int min, int max);
static uint64_t rnd_next_range(rnd_t* rnd, uint64_t min, uint64_t max);
static float    rnd_next_range(rnd_t* rnd, float min, float max);
static double   rnd_next_range(rnd_t* rnd, double min, double max);

static int      rnd_next_range(rnd_t& rnd, int min, int max);
static uint64_t rnd_next_range(rnd_t& rnd, uint64_t min, uint64_t max);
static float    rnd_next_range(rnd_t& rnd, float min, float max);
static double   rnd_next_range(rnd_t& rnd, double min, double max);
```

## Function Parameters

Parameter Name | Description
--- | ---
rnd | The state struct used to generate random numbers.
min | The lower bound for picking a random number.
max | The upper bound for picking a random number.

## Return Value

Returns the next random value between `min` and `max` in the sequence.

## Related Functions

[rnd_seed](https://github.com/RandyGaul/cute_framework/blob/master/doc/math/rnd/rnd_seed.md)  
[rnd_next](https://github.com/RandyGaul/cute_framework/blob/master/doc/math/rnd/rnd_next.md)  
[rnd_next_float](https://github.com/RandyGaul/cute_framework/blob/master/doc/math/rnd/rnd_next_float.md)  
[rnd_next_double](https://github.com/RandyGaul/cute_framework/blob/master/doc/math/rnd/rnd_next_double.md)  
