# astatic

Category: [array](https://github.com/RandyGaul/cute_framework/blob/master/docs/api_reference?id=array)  
GitHub: [cute_array.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_array.h)  
---

Creates an array with an initial static storage backing. Will grow onto the heap if the size becomes too large.

```cpp
#define astatic(a, buffer, buffer_size) cf_array_static(a, buffer, buffer_size)
```

Parameters | Description
--- | ---
a | The array. Can be `NULL`.
buffer | An initial buffer of memory to store the array within.
buffer_size | The size of `buffer` in bytes.

## Return Value

Returns a pointer to `a`. `a` will automatically be reassigned to any new pointer.

## Remarks

This macro is useful as an optimization to avoid any dynamic memory allocation in the common case for implementing
certain data structures (such as strings or stack vectors). As the array grows too large for the `buffer` it will
dynamically grow into the heap.

## Related Pages

[dyna](https://github.com/RandyGaul/cute_framework/blob/master/docs/array/dyna.md)  
[asize](https://github.com/RandyGaul/cute_framework/blob/master/docs/array/asize.md)  
[acount](https://github.com/RandyGaul/cute_framework/blob/master/docs/array/acount.md)  
[acap](https://github.com/RandyGaul/cute_framework/blob/master/docs/array/acap.md)  
[afit](https://github.com/RandyGaul/cute_framework/blob/master/docs/array/afit.md)  
[apush](https://github.com/RandyGaul/cute_framework/blob/master/docs/array/apush.md)  
[apop](https://github.com/RandyGaul/cute_framework/blob/master/docs/array/apop.md)  
[aend](https://github.com/RandyGaul/cute_framework/blob/master/docs/array/aend.md)  
[alast](https://github.com/RandyGaul/cute_framework/blob/master/docs/array/alast.md)  
[aclear](https://github.com/RandyGaul/cute_framework/blob/master/docs/array/aclear.md)  
[aset](https://github.com/RandyGaul/cute_framework/blob/master/docs/array/aset.md)  
[arev](https://github.com/RandyGaul/cute_framework/blob/master/docs/array/arev.md)  
[ahash](https://github.com/RandyGaul/cute_framework/blob/master/docs/array/ahash.md)  
[afree](https://github.com/RandyGaul/cute_framework/blob/master/docs/array/afree.md)  
