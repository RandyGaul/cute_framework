# Sprite

`sprite_t` is for drawing 2D images onto the screen. The term sprite here just means a picture or animation along with some basic controls for the animation. Each animation is a set of frames where each frame is an image and a number of milliseconds.

Here's a quick code snippet of making and drawing a sprite from an aseprite file.

```cpp
sprite s;

void init()
{
    s = sprite_make(app, "data/sprite.ase");
}

void update(fload dt)
{
    s.update(dt);
}

void draw()
{
    s.draw();
    flush_sprites(app);
}
```

# Sprite Functions

[sprite_make](https://github.com/RandyGaul/cute_framework/blob/master/doc/graphics/sprite/sprite_make.md)  
[sprite_unload](https://github.com/RandyGaul/cute_framework/blob/master/doc/graphics/sprite/sprite_unload.md)  
[flush_sprites](https://github.com/RandyGaul/cute_framework/blob/master/doc/graphics/sprite/flush_sprites.md)  

# Sprite Member Functions

[update](https://github.com/RandyGaul/cute_framework/blob/master/doc/graphics/sprite/update.md)  
[play](https://github.com/RandyGaul/cute_framework/blob/master/doc/graphics/sprite/play.md)  
[is_playing](https://github.com/RandyGaul/cute_framework/blob/master/doc/graphics/sprite/is_playing.md)  
[reset](https://github.com/RandyGaul/cute_framework/blob/master/doc/graphics/sprite/reset.md)  
[draw](https://github.com/RandyGaul/cute_framework/blob/master/doc/graphics/sprite/draw.md)  
[batch_sprite](https://github.com/RandyGaul/cute_framework/blob/master/doc/graphics/sprite/batch_sprite.md)  
[pause](https://github.com/RandyGaul/cute_framework/blob/master/doc/graphics/sprite/pause.md)  
[unpause](https://github.com/RandyGaul/cute_framework/blob/master/doc/graphics/sprite/unpause.md)  
[toggle_pause](https://github.com/RandyGaul/cute_framework/blob/master/doc/graphics/sprite/toggle_pause.md)  
[flip_x](https://github.com/RandyGaul/cute_framework/blob/master/doc/graphics/sprite/flip_x.md)  
[flip_y](https://github.com/RandyGaul/cute_framework/blob/master/doc/graphics/sprite/flip_y.md)  
[frame_count](https://github.com/RandyGaul/cute_framework/blob/master/doc/graphics/sprite/frame_count.md)  
[current_frame](https://github.com/RandyGaul/cute_framework/blob/master/doc/graphics/sprite/current_frame.md)  
[frame_delay](https://github.com/RandyGaul/cute_framework/blob/master/doc/graphics/sprite/frame_delay.md)  
[animation_delay](https://github.com/RandyGaul/cute_framework/blob/master/doc/graphics/sprite/animation_delay.md)  
[animation_interpolant](https://github.com/RandyGaul/cute_framework/blob/master/doc/graphics/sprite/animation_interpolant.md)  
[will_finish](https://github.com/RandyGaul/cute_framework/blob/master/doc/graphics/sprite/will_finish.md)  
[on_loop](https://github.com/RandyGaul/cute_framework/blob/master/doc/graphics/sprite/on_loop.md)  

# Sprite Member Variables

[on_loop](https://github.com/RandyGaul/cute_framework/blob/master/doc/graphics/sprite/on_loop.md)  
[name](https://github.com/RandyGaul/cute_framework/blob/master/doc/graphics/sprite/name.md)  
[on_loop](https://github.com/RandyGaul/cute_framework/blob/master/doc/graphics/sprite/on_loop.md)  
[w](https://github.com/RandyGaul/cute_framework/blob/master/doc/graphics/sprite/h.md)  
[h](https://github.com/RandyGaul/cute_framework/blob/master/doc/graphics/sprite/h.md)  
[scale](https://github.com/RandyGaul/cute_framework/blob/master/doc/graphics/sprite/scale.md)  
[local_offset](https://github.com/RandyGaul/cute_framework/blob/master/doc/graphics/sprite/local_offset.md)  
[opacity](https://github.com/RandyGaul/cute_framework/blob/master/doc/graphics/sprite/opacity.md)  
[layer](https://github.com/RandyGaul/cute_framework/blob/master/doc/graphics/sprite/layer.md)  
[frame_index](https://github.com/RandyGaul/cute_framework/blob/master/doc/graphics/sprite/frame_index.md)  
[loop_count](https://github.com/RandyGaul/cute_framework/blob/master/doc/graphics/sprite/loop_count.md)  
[play_speed_multiplier](https://github.com/RandyGaul/cute_framework/blob/master/doc/graphics/sprite/play_speed_multiplier.md)  
[batch](https://github.com/RandyGaul/cute_framework/blob/master/doc/graphics/sprite/batch.md)  
[transform](https://github.com/RandyGaul/cute_framework/blob/master/doc/graphics/sprite/transform.md)  
