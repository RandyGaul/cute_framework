[](../header.md ':include')

# hdel

Category: [hash](/api_reference?id=hash)  
GitHub: [cute_hashtable.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_hashtable.h)  
---

Removes an item from the table.

```cpp
#define hdel(h, k) cf_hashtable_del(h, k)
```

Parameters | Description
--- | ---
h | The hashtable. Can be `NULL`. Needs to be a pointer to the type of items in the table.
k | The key for lookups. Each {key, item} pair must be unique. Keys are always typecasted to `uint64_t` e.g. you can use pointers as keys.

## Code Example

> Removes an item in the table.

```cpp
htbl CF_V2 table = NULL;
hadd(table, 10, cf_v2(-1, 1));
hdel(table, 10);
hfree(table);
```

## Remarks

Asserts if the item does not exist.

## Related Pages

[htbl](/hash/htbl.md)  
[hset](/hash/hset.md)  
[hadd](/hash/hadd.md)  
[hget](/hash/hget.md)  
[hfind](/hash/hfind.md)  
[hget_ptr](/hash/hget_ptr.md)  
[hfind_ptr](/hash/hfind_ptr.md)  
[hhas](/hash/hhas.md)  
[hfree](/hash/hfree.md)  
[hclear](/hash/hclear.md)  
[hkeys](/hash/hkeys.md)  
[hitems](/hash/hitems.md)  
[hswap](/hash/hswap.md)  
[hsize](/hash/hsize.md)  
[hcount](/hash/hcount.md)  
