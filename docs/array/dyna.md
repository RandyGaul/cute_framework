[](../header.md ':include')

# dyna

Category: [array](https://github.com/RandyGaul/cute_framework/blob/master/docs/api_reference?id=array)  
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

[afree](https://github.com/RandyGaul/cute_framework/blob/master/docs/array/afree.md)  
[asize](https://github.com/RandyGaul/cute_framework/blob/master/docs/array/asize.md)  
[acount](https://github.com/RandyGaul/cute_framework/blob/master/docs/array/acount.md)  
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
