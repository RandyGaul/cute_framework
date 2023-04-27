[](../header.md ':include')

# alen

Category: [array](/api_reference?id=array)  
GitHub: [cute_array.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_array.h)  
---

Returns the number of elements in the array.

```cpp
#define alen(a) cf_array_len(a)
```

Parameters | Description
--- | ---
x | The x position of the window.
y | The y position of the window.

## Code Example

> Creating an array, adding an element, then decrementing the count to zero before freeing the array.

```cpp
dyna int a = NULL;
apush(a, 5);
CF_ASSERT(alen(a) == 1);
alen(a)--;
CF_ASSERT(alen(a) == 0);
afree(a);
```

## Remarks

`a` must not by `NULL`. This function returns a proper l-value, so you can assign to it, i.e. increment/decrement can be quite useful.

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
[astatic](/array/astatic.md)  
[afree](/array/afree.md)  
