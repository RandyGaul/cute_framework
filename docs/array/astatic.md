[](../header.md ':include')

# astatic

Category: [array](/api_reference?id=array)  
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

[dyna](/array/dyna.md)  
[asize](/array/asize.md)  
[acount](/array/acount.md)  
[acap](/array/acap.md)  
[afit](/array/afit.md)  
[apush](/array/apush.md)  
[apop](/array/apop.md)  
[aend](/array/aend.md)  
[alast](/array/alast.md)  
[aclear](/array/aclear.md)  
[aset](/array/aset.md)  
[arev](/array/arev.md)  
[ahash](/array/ahash.md)  
[afree](/array/afree.md)  
