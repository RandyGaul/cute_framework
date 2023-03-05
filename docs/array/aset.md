# aset | [array](https://github.com/RandyGaul/cute_framework/blob/master/docs/array_readme.md) | [cute_array.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_array.h)

Copies the array b into array a. Will automatically fit a if needed with [afit](https://github.com/RandyGaul/cute_framework/blob/master/docs/array/afit.md).

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
[afree](https://github.com/RandyGaul/cute_framework/blob/master/docs/array/afree.md)  
[arev](https://github.com/RandyGaul/cute_framework/blob/master/docs/array/arev.md)  
[ahash](https://github.com/RandyGaul/cute_framework/blob/master/docs/array/ahash.md)  
[astatic](https://github.com/RandyGaul/cute_framework/blob/master/docs/array/astatic.md)  
