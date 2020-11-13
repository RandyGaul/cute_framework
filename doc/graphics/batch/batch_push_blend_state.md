# batch_push_blend_state

Pushes depth and stencil state onto the batch.

## Syntax

```cpp
void batch_push_blend_state(batch_t* b, const sg_blend_state& blend_state);
```

## Function Parameters

Parameter Name | Description
--- | ---
b | The batch.
blend_state | Blend states.

## Remarks

This function uses the sokol blend state struct `sg_blend_state`. In order to learn how to properly use sokol graphics take a peek at the [primer here](https://github.com/RandyGaul/cute_framework/blob/master/doc/graphics/sokol.md).

## Related Functions
 
[batch_push_blend_defaults](https://github.com/RandyGaul/cute_framework/tree/master/doc/graphics/batch/batch_push_blend_defaults.md)  
[batch_pop_blend_state](https://github.com/RandyGaul/cute_framework/tree/master/doc/graphics/batch/batch_pop_blend_state.md)  
