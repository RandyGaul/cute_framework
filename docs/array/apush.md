[](../header.md ':include')

# apush

Category: [array](/api_reference?id=array)  
GitHub: [cute_array.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_array.h)  
---

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
CF_ASSERT(a[0] == 5);
CF_ASSERT(a[1] == 13);
CF_ASSERT(asize(a) == 2);
afree(a);
```

## Remarks

`a` is automatically re-assigned to a new pointer if the array was internally regrown. If `a` is `NULL` a new
dynamic array is allocated on-the-spot for you, and assigned back to `a`.

## Related Pages

[dyna](/array/dyna.md)  
[asize](/array/asize.md)  
[acount](/array/acount.md)  
[acap](/array/acap.md)  
[afit](/array/afit.md)  
[afree](/array/afree.md)  
[apop](/array/apop.md)  
[aend](/array/aend.md)  
[alast](/array/alast.md)  
[aclear](/array/aclear.md)  
[aset](/array/aset.md)  
[arev](/array/arev.md)  
[ahash](/array/ahash.md)  
[astatic](/array/astatic.md)  
