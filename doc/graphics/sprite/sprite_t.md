# sprite_t

`sprite_t` represents a set of drawable animations. Each animation is a collection of frames, where each frame is one image to display on screen. The frames themselves are stored elsewhere, and the sprite simply refers to them by read-only pointer.

Switching between animations can be done by calling the [play](https://github.com/RandyGaul/cute_framework/blob/master/doc/graphics/sprite/play.md) and passing the name of the animation to the [play](https://github.com/RandyGaul/cute_framework/blob/master/doc/graphics/sprite/play.md) method.
for this texture to display on screen during a sprite's animation.

## Remarks

Sprites are loaded from either an [aseprite cache](https://github.com/RandyGaul/cute_framework/edit/master/doc/TODO_fill_me_in) or a [png cache](https://github.com/RandyGaul/cute_framework/edit/master/doc/TODO_fill_me_in).
