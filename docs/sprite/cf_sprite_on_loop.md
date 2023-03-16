[](../header.md ':include')

# cf_sprite_on_loop

Category: [sprite](/api_reference?id=sprite)  
GitHub: [cute_sprite.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_sprite.h)  
---

Returns true whenever at the very beginning of the animation sequence.

```cpp
bool cf_sprite_on_loop(CF_Sprite* sprite)
```

Parameters | Description
--- | ---
sprite | The sprite.

## Remarks

This is useful for polling on when the animation restarts itself, for example, polling within an if-statement each game tick.

## Related Pages

[CF_Sprite](/sprite/cf_sprite.md)  
[cf_sprite_frame_count](/sprite/cf_sprite_frame_count.md)  
[cf_sprite_current_frame](/sprite/cf_sprite_current_frame.md)  
[cf_sprite_frame_delay](/sprite/cf_sprite_frame_delay.md)  
[cf_sprite_animation_delay](/sprite/cf_sprite_animation_delay.md)  
[cf_sprite_will_finish](/sprite/cf_sprite_will_finish.md)  
