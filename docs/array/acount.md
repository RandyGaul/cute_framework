# acount

Category: [array](https://github.com/RandyGaul/cute_framework/blob/master/docs/api_reference?id=array)  
GitHub: [cute_array.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_array.h)  
---

Returns the number of elements in the array.

```cpp
#define acount(a) cf_array_count(a)
```

Parameters | Description
--- | ---
a | The array.

## Code Example

> Creating an array, getting the size of the array, then freeing it up afterwards.

```cpp
dyna int a = NULL;
apush(a, 5);
CUTE_ASSERT(acount(a) == 1);
afree(a);
```

## Remarks

`a` can be `NULL`.

## Related Pages

[dyna](https://github.com/RandyGaul/cute_framework/blob/master/docs/array/dyna.md)  
[asize](https://github.com/RandyGaul/cute_framework/blob/master/docs/array/asize.md)  
[afree](https://github.com/RandyGaul/cute_framework/blob/master/docs/array/afree.md)  
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
[astatic](https://github.com/RandyGaul/cute_framework/blob/master/docs/array/astatic.md)  
