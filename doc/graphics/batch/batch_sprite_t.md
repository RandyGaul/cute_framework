# batch_sprite_t

Represents a single image rendered as a quad.

## Data Fields

type | name | Description
--- | --- | ---
uint64_t | id | Unique identifier for this quad's image, as determined by you.
transform_t | transform_t | Position and location rotation of the quad.
int | w | Width in pixels of the source image.
int | h | Width in pixels of the source image.
float | scale_x | Scaling along the quad's local x-axis in pixels.
float | scale_y | Scaling along the quad's local y-axis in pixels.
float | alpha | Applies additional alpha to this quad (defaulted to 1).
int | sort_bits | User-defined bitfield (defaulted to 0) used for sorting the sprites internally within the batching algorithm.

## Code Example

> Minimum code necessary to get a sprite onto the screen.

```cpp
batch_sprite_t s;
s.id = id;
s.w = w;
s.h = h;
s.scale_x = (float)w;
s.scale_y = (float)h;

batch_push(batch, s);
batch_flush(batch);
```

## Related Functions

[batch_push](https://github.com/RandyGaul/cute_framework/tree/master/doc/graphics/batch/batch_push.md)  
[batch_flush](https://github.com/RandyGaul/cute_framework/tree/master/doc/graphics/batch/batch_flush.md)  
