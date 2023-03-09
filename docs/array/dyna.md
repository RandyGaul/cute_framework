[](../header.md ':include')

# dyna

Category: [array](/api_reference?id=array)  
GitHub: [cute_array.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_array.h)  
---

An empty macro used in the C API to markup dynamic arrays.

```cpp
#define dyna
```

## Code Example

> Creating a dynamic array, pushing some elements into the array, and freeing it up afterwards.

```cpp
dyna int a = NULL;
apush(a, 5);
CUTE_ASSERT(alen(a) == 1);
alen(a)--;
CUTE_ASSERT(alen(a) == 0);
afree(a);
```

## Remarks

This is an optional and _completely_ empty macro. It's only purpose is to provide a bit of visual indication a type is a
dynamic array. One downside of the C-macro API is the opaque nature of the pointer type. Since the macros use polymorphism
on typed pointers, there's no actual array struct type. It can get really annoying to sometimes forget if a pointer is an
array, a hashtable, or just a pointer. This macro can be used to markup the type to make it much more clear for function
parameters or struct member definitions. It's saying "Hey, I'm a dynamic array!" to mitigate this downside.

## Related Pages

[afree](/array/afree.md)  
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
