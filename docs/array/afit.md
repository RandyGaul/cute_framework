[](../header.md ':include')

# afit

Category: [array](https://github.com/RandyGaul/cute_framework/blob/master/docs/api_reference?id=array)  
GitHub: [cute_array.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_array.h)  
---

Ensures the capacity of the array is at least `n` elements large.

```cpp
#define afit(a, n) cf_array_fit(a, n)
```

Parameters | Description
--- | ---
a | The array.
n | The number of elements to resize the array's capacity to.

## Return Value

Returns a pointer to the array.

## Remarks

This function does not change the number of live elements, the count/size, of the array. Only the capacity.
This function is only useful as an optimization to avoid extra/unnecessary internal allocations. `a` is
automatically re-assigned to a new pointer if the array was internally regrown.

## Related Pages

[dyna](https://github.com/RandyGaul/cute_framework/blob/master/docs/array/dyna.md)  
[asize](https://github.com/RandyGaul/cute_framework/blob/master/docs/array/asize.md)  
[acount](https://github.com/RandyGaul/cute_framework/blob/master/docs/array/acount.md)  
[acap](https://github.com/RandyGaul/cute_framework/blob/master/docs/array/acap.md)  
[afree](https://github.com/RandyGaul/cute_framework/blob/master/docs/array/afree.md)  
[apush](https://github.com/RandyGaul/cute_framework/blob/master/docs/array/apush.md)  
[apop](https://github.com/RandyGaul/cute_framework/blob/master/docs/array/apop.md)  
[aend](https://github.com/RandyGaul/cute_framework/blob/master/docs/array/aend.md)  
[alast](https://github.com/RandyGaul/cute_framework/blob/master/docs/array/alast.md)  
[aclear](https://github.com/RandyGaul/cute_framework/blob/master/docs/array/aclear.md)  
[aset](https://github.com/RandyGaul/cute_framework/blob/master/docs/array/aset.md)  
[arev](https://github.com/RandyGaul/cute_framework/blob/master/docs/array/arev.md)  
[ahash](https://github.com/RandyGaul/cute_framework/blob/master/docs/array/ahash.md)  
[astatic](https://github.com/RandyGaul/cute_framework/blob/master/docs/array/astatic.md)  
