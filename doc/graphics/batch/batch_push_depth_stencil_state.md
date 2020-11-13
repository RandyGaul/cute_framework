# batch_push_depth_stencil_state

Pushes depth and stencil state onto the batch.

## Syntax

```cpp
void batch_push_depth_stencil_state(batch_t* b, const sg_depth_stencil_state& depth_stencil_state);
```

## Function Parameters

Parameter Name | Description
--- | ---
b | The batch.
depth_stencil_state | Depth and stencil states.

## Remarks

This function uses the sokol depth and stencil state struct `sg_depth_stencil_state`. In order to learn how to properly use sokol graphics take a peek at the [primer here](https://github.com/RandyGaul/cute_framework/blob/master/doc/graphics/sokol.md).

## Related Functions
 
[batch_push_depth_stencil_defaults](https://github.com/RandyGaul/cute_framework/tree/master/doc/graphics/batch/batch_push_depth_stencil_defaults.md)  
[batch_pop_depth_stencil_state](https://github.com/RandyGaul/cute_framework/tree/master/doc/graphics/batch/batch_pop_depth_stencil_state.md)  
