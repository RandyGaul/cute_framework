# batch_push

Pushes a sprite onto the batch.

## Syntax

```cpp
void batch_push(batch_t* b, batch_sprite_t sprite);
```

## Function Parameters

Parameter Name | Description
--- | ---
b | The batch.
sprite | The sprite.

## Return Value

Returns a new batch.

## Remarks

To do the actual rendering, please see [batch_flush](https://github.com/RandyGaul/cute_framework/tree/master/doc/graphics/batch/batch_flush).

## Related Functions

[batch_destroy](https://github.com/RandyGaul/cute_framework/tree/master/doc/graphics/batch/batch_destroy)  
