[](../header.md ':include')

# CF_Sprite

Category: [sprite](/api_reference?id=sprite)  
GitHub: [cute_sprite.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_sprite.h)  
---

A sprite represents a drawable entity, made of 2D animations/images.

Struct Members | Description
--- | ---
`const char* name` | The name of the sprite.
`int w` | Width of the sprite in pixels.
`int h` | Height of the sprite in pixels.
`CF_V2 scale` | Scale factor for the sprite when drawing. Default of `(1, 1)`. See [cf_draw_sprite](/draw/cf_draw_sprite.md).
`CF_V2 local_offset` | A local offset/origin for the sprite when drawing. See [cf_draw_sprite](/draw/cf_draw_sprite.md).
`float opacity` | An opacity value for the entire sprite. Default of 1.0f. See [cf_draw_sprite](/draw/cf_draw_sprite.md).
`int frame_index` | The current frame within `animation` to display.
`int loop_count` | The number of times this sprite has completed an animation.
`float play_speed_multiplier` | A speed multiplier for updating frames. Default of 1.0f.
`bool paused` | Whether or not to pause updates to the animation.
`float t` | The current elapsed time within a frame of animation.
`uint64_t easy_sprite_id` | For internal use only.
`const CF_Animation* animation` | A pointer to the current animation to display, from within the set `animations`. See [CF_Animation](/sprite/cf_animation.md).
`htbl const CF_Animation** animations` | The set of named animations for this sprite. See [CF_Animation](/sprite/cf_animation.md) and [htbl](/hash/htbl.md).
`CF_Transform transform` | An optional transform for rendering within a particular space. See [CF_Transform](/math/cf_transform.md).

## Remarks

Sprites can be drawn by [cf_draw_sprite](/draw/cf_draw_sprite.md). Since sprites are [plain old data POD](https://stackoverflow.com/questions/146452/what-are-pod-types-in-c) you may create one on the stack anywhere
and freely copy it around. In C++ you may simply draw via `sprite.draw()`.

## Related Pages

[CF_Frame](/sprite/cf_frame.md)  
[CF_Animation](/sprite/cf_animation.md)  
[cf_make_sprite](/sprite/cf_make_sprite.md)  
cf_make_easy_sprite  
