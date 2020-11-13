# batch_flush

All sprites and other geometry pushed onto the batch are converted to draw calls and submit to the underlying graphics API.

## Syntax

```cpp
void batch_flush(batch_t* b);
```

## Function Parameters

Parameter Name | Description
--- | ---
b | The batch.

## Remarks

All internal buffers storing renderable geometry and cleared to empty.

## Related Functions

[batch_make](https://github.com/RandyGaul/cute_framework/tree/master/doc/graphics/batch/batch_make)  
[batch_destroy](https://github.com/RandyGaul/cute_framework/tree/master/doc/graphics/batch/batch_destroy)  
