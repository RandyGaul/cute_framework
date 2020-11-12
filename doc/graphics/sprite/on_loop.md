# sprite_t::on_loop

Returns true if the animation will loop around and finish if `update` is called.

## Syntax

```cpp
bool on_loop();
```

## Return Value

Returns true if the animation will loop around and finish if `update` is called.

## Remarks

This is useful for polling on when the animation restarts itself, for example, polling within an if-statement each game tick.

## Related Functions

[sprite_t::frame_count](https://github.com/RandyGaul/cute_framework/blob/master/doc/graphics/sprite/frame_count.md)  
[sprite_t::frame_delay](https://github.com/RandyGaul/cute_framework/blob/master/doc/graphics/sprite/frame_delay.md)  
[sprite_t::animation_delay](https://github.com/RandyGaul/cute_framework/blob/master/doc/graphics/sprite/animation_delay.md)  
[sprite_t::will_finish](https://github.com/RandyGaul/cute_framework/blob/master/doc/graphics/sprite/will_finish.md)  
