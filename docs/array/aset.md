[](../header.md ':include')

# aset

Category: [array](/api_reference?id=array)  
GitHub: [cute_array.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_array.h)  
---

Copies the array b into array a. Will automatically fit a if needed with [afit](/array/afit.md).

```cpp
#define aset(a, b) cf_array_set(a, b)
```

Parameters | Description
--- | ---
a | The array to copy into (destination).
b | The array to copy from (source).

## Return Value

Returns a pointer to `a`. `a` will automatically be reassigned to any new pointer.

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
[afree](/array/afree.md)  
[arev](/array/arev.md)  
[ahash](/array/ahash.md)  
[astatic](/array/astatic.md)  
