[](../header.md ':include')

# asize

Category: [array](/api_reference?id=array)  
GitHub: [cute_array.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_array.h)  
---

Returns the number of elements in the array.

```cpp
#define asize(a) cf_array_size(a)
```

Parameters | Description
--- | ---
a | The array.

## Code Example

> Creating an array, getting the size of the array, then freeing it up afterwards.

```cpp
dyna int a = NULL;
apush(a, 5);
CF_ASSERT(asize(a) == 1);
afree(a);
```

## Remarks

`a` can be `NULL`.

## Related Pages

[dyna](/array/dyna.md)  
[afree](/array/afree.md)  
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
[astatic](/array/astatic.md)  
