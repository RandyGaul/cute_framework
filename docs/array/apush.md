# apush | [array](https://github.com/RandyGaul/cute_framework/blob/master/docs/array/README.md) | [cute_array.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_array.h)

Pushes an element onto the back of the array.

```cpp
#define apush(a, ...) cf_array_push(a, (__VA_ARGS__))
```

Parameters | Description
--- | ---
a | The array. Can be `NULL`.
... | The element to push.

## Return Value

Returns the pushed element.

## Code Example

> Pushing some elements onto an array and asserting their values.

```cpp
dyna int a = NULL;
apush(a, 5);
apush(a, 13);
CUTE_ASSERT(a[0] == 5);
CUTE_ASSERT(a[1] == 13);
CUTE_ASSERT(asize(a) == 2);
afree(a);
```

## Remarks

`a` is automatically re-assigned to a new pointer if the array was internally regrown. If `a` is `NULL` a new
dynamic array is allocated on-the-spot for you, and assigned back to `a`.

## Related Pages

[dyna](https://github.com/RandyGaul/cute_framework/blob/master/docs/array/dyna.md)  
[asize](https://github.com/RandyGaul/cute_framework/blob/master/docs/array/asize.md)  
[acount](https://github.com/RandyGaul/cute_framework/blob/master/docs/array/acount.md)  
[acap](https://github.com/RandyGaul/cute_framework/blob/master/docs/array/acap.md)  
[afit](https://github.com/RandyGaul/cute_framework/blob/master/docs/array/afit.md)  
[afree](https://github.com/RandyGaul/cute_framework/blob/master/docs/array/afree.md)  
[apop](https://github.com/RandyGaul/cute_framework/blob/master/docs/array/apop.md)  
[aend](https://github.com/RandyGaul/cute_framework/blob/master/docs/array/aend.md)  
[alast](https://github.com/RandyGaul/cute_framework/blob/master/docs/array/alast.md)  
[aclear](https://github.com/RandyGaul/cute_framework/blob/master/docs/array/aclear.md)  
[aset](https://github.com/RandyGaul/cute_framework/blob/master/docs/array/aset.md)  
[arev](https://github.com/RandyGaul/cute_framework/blob/master/docs/array/arev.md)  
[ahash](https://github.com/RandyGaul/cute_framework/blob/master/docs/array/ahash.md)  
[astatic](https://github.com/RandyGaul/cute_framework/blob/master/docs/array/astatic.md)  
