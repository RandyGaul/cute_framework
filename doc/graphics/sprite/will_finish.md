# sprite_t::will_finish

Returns true if the animation will loop around and finish if `update` is called.

## Syntax

```cpp
bool will_finish(float dt);
```

## Function Parameters

Parameter Name | Description
--- | ---
dt | Delta-time for the last game tick, such as from [calc_dt](https://github.com/RandyGaul/cute_framework/blob/master/doc/time/calc_dt.md).

## Return Value

Returns true if the animation will loop around and finish if `update` is called.

## Remarks

This function is useful to see if you're currently on the last frame of animation, and will finish in the particular `dt` tick.

## Related Functions

[sprite_t::frame_count](https://github.com/RandyGaul/cute_framework/blob/master/doc/graphics/sprite/frame_count.md)  
[sprite_t::frame_delay](https://github.com/RandyGaul/cute_framework/blob/master/doc/graphics/sprite/frame_delay.md)  
[sprite_t::animation_delay](https://github.com/RandyGaul/cute_framework/blob/master/doc/graphics/sprite/animation_delay.md)  
[sprite_t::on_loop](https://github.com/RandyGaul/cute_framework/blob/master/doc/graphics/sprite/on_loop.md)  
