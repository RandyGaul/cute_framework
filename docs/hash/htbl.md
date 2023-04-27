[](../header.md ':include')

# htbl

Category: [hash](/api_reference?id=hash)  
GitHub: [cute_hashtable.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_hashtable.h)  
---

An empty macro used in the C API to markup hastables.

```cpp
#define htbl
```

## Code Example

> Showcase of base htbl features.

```cpp
htbl CF_V2 pts = NULL;
hset(pts, 0, cf_v2(3, 5)); // Contructs a new table on-the-spot. The table is hidden behind `pts`.
hset(pts, 10, cf_v2(-1, -1);
hset(pts, -2, cf_v2(0, 0));

// Use [hget](/hash/hget.md) to fetch values.
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

[hfree](/hash/hfree.md)  
[hset](/hash/hset.md)  
[hadd](/hash/hadd.md)  
[hget](/hash/hget.md)  
[hfind](/hash/hfind.md)  
[hget_ptr](/hash/hget_ptr.md)  
[hfind_ptr](/hash/hfind_ptr.md)  
[hhas](/hash/hhas.md)  
[hdel](/hash/hdel.md)  
[hclear](/hash/hclear.md)  
[hkeys](/hash/hkeys.md)  
[hitems](/hash/hitems.md)  
[hswap](/hash/hswap.md)  
[hsize](/hash/hsize.md)  
[hcount](/hash/hcount.md)  
