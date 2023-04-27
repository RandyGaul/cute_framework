[](../header.md ':include')

# apop

Category: [array](/api_reference?id=array)  
GitHub: [cute_array.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_array.h)  
---

Pops and returns an element off the back of the array.

```cpp
#define apop(a) cf_array_pop(a)
```

Parameters | Description
--- | ---
a | The array. Can not be `NULL`.

## Return Value

Returns the popped element.

## Remarks

The last element of the array is fetched and will be returned. The size of the array is decremented by one.

## Related Pages

[dyna](/array/dyna.md)  
[asize](/array/asize.md)  
[acount](/array/acount.md)  
[acap](/array/acap.md)  
[afit](/array/afit.md)  
[apush](/array/apush.md)  
[afree](/array/afree.md)  
[aend](/array/aend.md)  
[alast](/array/alast.md)  
[aclear](/array/aclear.md)  
[aset](/array/aset.md)  
[arev](/array/arev.md)  
[ahash](/array/ahash.md)  
[astatic](/array/astatic.md)  
