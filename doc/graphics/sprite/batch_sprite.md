# sprite_t::batch_sprite
A lower level utility function used within the `draw` method. This is useful to prepare the sprite's drawable quad in a specific way before handing it off to a `batch`, to implement custom graphics effects.

## Syntax

```cpp
batch_sprite_t batch_sprite(transform_t transform);
```

## Function Parameters

Parameter Name | Description
--- | ---
transform | A transform to draw the sprite relative to.

## Return Value

Returns a `batch_sprite_t` ready to go for [batch_push](https://github.com/RandyGaul/cute_framework/edit/master/doc/TODO_fill_me_in).

## Related Functions

[sprite_t::draw](https://github.com/RandyGaul/cute_framework/blob/master/doc/graphics/sprite/draw.md)  
[batch_push](https://github.com/RandyGaul/cute_framework/edit/master/doc/TODO_fill_me_in)  
[batch_flush](https://github.com/RandyGaul/cute_framework/edit/master/doc/TODO_fill_me_in)  
[batch_sprite_t](https://github.com/RandyGaul/cute_framework/edit/master/doc/TODO_fill_me_in)  
