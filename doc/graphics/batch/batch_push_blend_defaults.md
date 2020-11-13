# batch_push_blend_defaults

Pushes default blend state onto the batch.

## Syntax

```cpp
void batch_push_blend_defaults(batch_t* b);
```

## Function Parameters

Parameter Name | Description
--- | ---
b | The batch.

## Remarks

This function uses the sokol blend state struct `sg_blend_state` internally by defaulting all of the struct fields to zero. In order to learn how to properly use sokol graphics take a peek at the [primer here](https://github.com/RandyGaul/cute_framework/blob/master/doc/graphics/sokol.md).

## Related Functions
 
[batch_push_blend_state](https://github.com/RandyGaul/cute_framework/tree/master/doc/graphics/batch/batch_push_blend_state.md)  
[batch_pop_depth_state](https://github.com/RandyGaul/cute_framework/tree/master/doc/graphics/batch/batch_pop_depth_state.md)  
