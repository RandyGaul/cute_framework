# Sprite

`sprite_t` is for drawing 2D images onto the screen. The term sprite here just means a picture or animation along with some basic controls for the animation. Each animation is a set of frames where each frame is an image and a number of milliseconds.

Here's a quick code snippet of making and drawing a sprite from an aseprite file.

```cpp
sprite s;

void init(app_t* app)
{
    s = sprite_make(app, "data/sprite.ase");
}

void update(app_t* app, float dt)
{
    s.update(dt);
}

void draw(app_t* app)
{
    batch_t* b = sprite_get_batch(app);
    s.draw(b);
    batch_flush(app);
}
```

# Easy Sprite API

These functions are great for beginners or testing if you just want to load a single-frame sprite with no animations from a single .png file. If you want animations the preferred API is the Aseprite Sprite API (in the next section).

[easy_sprite_make](https://github.com/RandyGaul/cute_framework/blob/master/doc/graphics/sprite/sprite_make.md)  
[easy_sprite_unload](https://github.com/RandyGaul/cute_framework/blob/master/doc/graphics/sprite/sprite_unload.md)  
[easy_sprite_get_batch](https://github.com/RandyGaul/cute_framework/blob/master/doc/graphics/sprite/easy_sprite_get_batch.md)  

# Aseprite Sprite API

These functions are the preferred API for creating and destroying sprites by directloy loading from .ase or .aseprite files. These functions setup default values for an [aseprite cache](https://github.com/RandyGaul/cute_framework/tree/master/doc/graphics/aseprite_cache) and a [batch](https://github.com/RandyGaul/cute_framework/tree/master/doc/graphics/batch). This is great to get some sprites drawing on the screen quickly with minimal effort. For more advanced graphics usages custom caches or batches need to be created and managed yourself, but these three functions below are great to start with.

[sprite_make](https://github.com/RandyGaul/cute_framework/blob/master/doc/graphics/sprite/sprite_make.md)  
[sprite_unload](https://github.com/RandyGaul/cute_framework/blob/master/doc/graphics/sprite/sprite_unload.md)  
[sprite_get_batch](https://github.com/RandyGaul/cute_framework/blob/master/doc/graphics/sprite/sprite_get_batch.md)  

# Sprite Member Functions

Calling the member functions on sprites is recommended over directly accessing the member variables (documented below). These member functions are generally easy to use without causing unintended problems. If you know what you're doing you can directly access or alter the member variables (documented below).

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

Calling the member functions on sprites (documented above) is recommended over directly accessing these member variables. The member functions are generally easy to use without causing unintended problems. If you know what you're doing you can directly access or alter these member variables.

Please note it is always safe to *read* from a member variable, but writing to or changing the value of a member variable can cause problems if you are unsure of what you are doing.

Member Variable Name | Type | Description
--- | --- | ---
name | `const char*` | Name of the sprite, mostly useful for debugging purposes. Can safely be `NULL`.
w | `int` | Width of the sprite in pixels.
h | `int` | Height of the sprite in pixels.
scale | `v2` | Scaling factor for the sprite in the local x-y axes, default is `(1, 1)`.
local_offset | `v2` | An offset from the local origin of the sprite, default is `(0, 0)`.
opacity | `float` | Value from 0 to 1 for how opaque the sprite is. 0 means invisible, 1 is fully visible. Default is 1.
layer | `int` | Used for sorting sprites based on "depth" or their sprite "layer". Default is 0. Larger numbers are drawn last.
frame_index | `int` | The current frame to be drawn for the sprite's currently playing animation.
loop_count | `int` | Incremented each time the current animation of the sprite has reset after completing.
play_speed_multiplier | `float` | Used to multiply time passed to the [update](https://github.com/RandyGaul/cute_framework/blob/master/doc/graphics/sprite/update.md) member function of the sprite. Default value is 1. 0.5 means the sprite plays at half speed, while 2.0 means the sprite plays at double speed.
animation | `animation_t` | The currently playing animation.
paused | `bool` | Whether or not the sprite is currently paused. True means paused. Default is `false`.
t | `float` | The current time elapsed while playing an animation. This gets incremented when [update](https://github.com/RandyGaul/cute_framework/blob/master/doc/graphics/sprite/update.md) is called.
animations | `animation_table_t` | The table of all animations. These animations can be loaded from [sprite_make](https://github.com/RandyGaul/cute_framework/blob/master/doc/graphics/sprite/sprite_make.md), or loaded with more customizability with your own [aseprite cache](https://github.com/RandyGaul/cute_framework/tree/master/doc/graphics/aseprite_cache), or built up from scratch from various png files manually with [png cache](https://github.com/RandyGaul/cute_framework/tree/master/doc/graphics/png_cache).
transform | `transform_t` | The transform used to place the sprite in the world when drawn.
