# cf_sprite_animation_interpolant | [sprite](https://github.com/RandyGaul/cute_framework/blob/master/docs/sprite/README.md) | [cute_sprite.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_sprite.h)

Returns a value from 0 to 1 representing how far along the animation has played.

```cpp
float cf_sprite_animation_interpolant(CF_Sprite* sprite)
```

Parameters | Description
--- | ---
sprite | The sprite.

## Remarks

0 means just started, while 1 means finished.

## Related Pages

[CF_Sprite](https://github.com/RandyGaul/cute_framework/blob/master/docs/sprite/cf_sprite.md)  
[CF_Frame](https://github.com/RandyGaul/cute_framework/blob/master/docs/sprite/cf_frame.md)  
[CF_Animation](https://github.com/RandyGaul/cute_framework/blob/master/docs/sprite/cf_animation.md)  
[cf_sprite_frame_count](https://github.com/RandyGaul/cute_framework/blob/master/docs/sprite/cf_sprite_frame_count.md)  
[cf_sprite_current_frame](https://github.com/RandyGaul/cute_framework/blob/master/docs/sprite/cf_sprite_current_frame.md)  
[cf_sprite_frame_delay](https://github.com/RandyGaul/cute_framework/blob/master/docs/sprite/cf_sprite_frame_delay.md)  
[cf_sprite_animation_delay](https://github.com/RandyGaul/cute_framework/blob/master/docs/sprite/cf_sprite_animation_delay.md)  
