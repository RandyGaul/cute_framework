[](../header.md ':include')

# cf_animation_add_frame

Category: [sprite](https://github.com/RandyGaul/cute_framework/blob/master/docs/api_reference?id=sprite)  
GitHub: [cute_sprite.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_sprite.h)  
---

Adds a frame to an animation.

```cpp
void cf_animation_add_frame(CF_Animation* animation, CF_Frame frame)
```

Parameters | Description
--- | ---
animation | The sprite.
frame | The frame.

## Remarks

You can use this function to build your own animations in a custom manner. It's recommend to just use [cf_make_sprite](https://github.com/RandyGaul/cute_framework/blob/master/docs/sprite/cf_make_sprite.md), which
loads a full sprite out of a .ase file. But, this function provides another low-level option if desired.

## Related Pages

[CF_Sprite](https://github.com/RandyGaul/cute_framework/blob/master/docs/sprite/cf_sprite.md)  
[CF_Animation](https://github.com/RandyGaul/cute_framework/blob/master/docs/sprite/cf_animation.md)  
[CF_Frame](https://github.com/RandyGaul/cute_framework/blob/master/docs/sprite/cf_frame.md)  
[dyna](https://github.com/RandyGaul/cute_framework/blob/master/docs/array/dyna.md)  
[htbl](https://github.com/RandyGaul/cute_framework/blob/master/docs/hash/htbl.md)  
