# htbl | [hash](https://github.com/RandyGaul/cute_framework/blob/master/docs/hash_readme.md) | [cute_hashtable.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_hashtable.h)

An empty macro used in the C API to markup hastables.

```cpp
#define htbl
```

## Code Example

> Showcase of base htbl features.

```cpp
htbl CF_V2 pts = NULL;
hset(pts, 0, cf_v2(3, 5)); // Contructs a new table on-the-spot.
                        // The table is hidden behind `pts`.
hset(pts, 10, cf_v2(-1, -1);
hset(pts, -2, cf_v2(0, 0));

// Use [hget](https://github.com/RandyGaul/cute_framework/blob/master/docs/hash/hget.md) to fetch values.
CF_V2 a = hget(pts, 0);
CF_V2 b = hget(pts, 10);
CF_V2 c = hget(pts, -2);

// Loop over {key, item} pairs like so:
const uint64_t keys = hkeys(pts);
for (int i = 0; i < hcount(pts); ++i) {
    uint64_t key = keys[i];
    CF_V2 v = pts[i];
    // ...
}

hfree(pts);
```

## Remarks

This is an optional and _completely_ empty macro. It's only purpose is to provide a bit of visual indication a type is a
hashtable. One downside of the C-macro API is the opaque nature of the pointer type. Since the macros use polymorphism
on typed pointers, there's no actual hashtable struct type. It can get really annoying to sometimes forget if a pointer is an
array, a hashtable, or just a pointer. This macro can be used to markup the type to make it much more clear for function
parameters or struct member definitions. It's saying "Hey, I'm a hashtable!" to mitigate this downside.

## Related Pages

[hfree](https://github.com/RandyGaul/cute_framework/blob/master/docs/hash/hfree.md)  
[hset](https://github.com/RandyGaul/cute_framework/blob/master/docs/hash/hset.md)  
[hadd](https://github.com/RandyGaul/cute_framework/blob/master/docs/hash/hadd.md)  
[hget](https://github.com/RandyGaul/cute_framework/blob/master/docs/hash/hget.md)  
[hfind](https://github.com/RandyGaul/cute_framework/blob/master/docs/hash/hfind.md)  
[hget_ptr](https://github.com/RandyGaul/cute_framework/blob/master/docs/hash/hget_ptr.md)  
[hfind_ptr](https://github.com/RandyGaul/cute_framework/blob/master/docs/hash/hfind_ptr.md)  
[hhas](https://github.com/RandyGaul/cute_framework/blob/master/docs/hash/hhas.md)  
[hdel](https://github.com/RandyGaul/cute_framework/blob/master/docs/hash/hdel.md)  
[hclear](https://github.com/RandyGaul/cute_framework/blob/master/docs/hash/hclear.md)  
[hkeys](https://github.com/RandyGaul/cute_framework/blob/master/docs/hash/hkeys.md)  
[hitems](https://github.com/RandyGaul/cute_framework/blob/master/docs/hash/hitems.md)  
[hswap](https://github.com/RandyGaul/cute_framework/blob/master/docs/hash/hswap.md)  
[hsize](https://github.com/RandyGaul/cute_framework/blob/master/docs/hash/hsize.md)  
[hcount](https://github.com/RandyGaul/cute_framework/blob/master/docs/hash/hcount.md)  
