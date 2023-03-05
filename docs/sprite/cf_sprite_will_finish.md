# cf_sprite_will_finish | [sprite](https://github.com/RandyGaul/cute_framework/blob/master/docs/sprite/README.md) | [cute_sprite.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_sprite.h)

Returns true if the animation will loop around and finish if [cf_sprite_update](https://github.com/RandyGaul/cute_framework/blob/master/docs/sprite/cf_sprite_update.md) is called.

```cpp
bool cf_sprite_will_finish(CF_Sprite* sprite)
```

Parameters | Description
--- | ---
sprite | The sprite.

## Remarks

This is useful to see if you're currently on the last frame of animation, and will finish in this particular update.

## Related Pages

[CF_Sprite](https://github.com/RandyGaul/cute_framework/blob/master/docs/sprite/cf_sprite.md)  
[cf_sprite_frame_count](https://github.com/RandyGaul/cute_framework/blob/master/docs/sprite/cf_sprite_frame_count.md)  
[cf_sprite_current_frame](https://github.com/RandyGaul/cute_framework/blob/master/docs/sprite/cf_sprite_current_frame.md)  
[cf_sprite_frame_delay](https://github.com/RandyGaul/cute_framework/blob/master/docs/sprite/cf_sprite_frame_delay.md)  
[cf_sprite_animation_delay](https://github.com/RandyGaul/cute_framework/blob/master/docs/sprite/cf_sprite_animation_delay.md)  
