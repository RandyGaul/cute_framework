[](../header.md ':include')

# hclear

Category: [hash](/api_reference?id=hash)  
GitHub: [cute_hashtable.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_hashtable.h)  
---

Clears the hashtable.

```cpp
#define hclear(h) cf_hashtable_clear(h)
```

Parameters | Description
--- | ---
h | The hashtable. Can be `NULL`. Needs to be a pointer to the type of items in the table.

## Remarks

The count of items will now be zero. Does not free any memory. Call [hfree](/hash/hfree.md) when you are done.

## Related Pages

[htbl](/hash/htbl.md)  
[hset](/hash/hset.md)  
[hadd](/hash/hadd.md)  
[hget](/hash/hget.md)  
[hfind](/hash/hfind.md)  
[hget_ptr](/hash/hget_ptr.md)  
[hfind_ptr](/hash/hfind_ptr.md)  
[hhas](/hash/hhas.md)  
[hdel](/hash/hdel.md)  
[hfree](/hash/hfree.md)  
[hkeys](/hash/hkeys.md)  
[hitems](/hash/hitems.md)  
[hswap](/hash/hswap.md)  
[hsize](/hash/hsize.md)  
[hcount](/hash/hcount.md)  
