# sprite_t::draw

Pushes an instance of this sprite onto the `batch`, which will be drawn the next time [batch_flush](https://github.com/RandyGaul/cute_framework/edit/master/doc/TODO_fill_me_in) is called on `batch`.

## Syntax

```cpp
void draw(batch_t* batch, transform_t transform);
```

## Function Parameters

Parameter Name | Description
--- | ---
batch | The batch to push a sprite quad onto for drawing the next time `batch_flush` is called.
transform | A transform to draw the sprite relative to.

## Related Functions

[sprite_t::batch_sprite](https://github.com/RandyGaul/cute_framework/blob/master/doc/graphics/sprite/batch_sprite.md)  
[batch_push](https://github.com/RandyGaul/cute_framework/edit/master/doc/TODO_fill_me_in)  
[batch_flush](https://github.com/RandyGaul/cute_framework/edit/master/doc/TODO_fill_me_in)  
