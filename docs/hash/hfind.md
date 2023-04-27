[](../header.md ':include')

# hfind

Category: [hash](/api_reference?id=hash)  
GitHub: [cute_hashtable.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_hashtable.h)  
---

Fetches the item that `k` maps to.

```cpp
#define hfind(h, k) cf_hashtable_find(h, k)
```

Parameters | Description
--- | ---
h | The hashtable. Can be `NULL`. Needs to be a pointer to the type of items in the table.
k | The key for lookups. Each {key, item} pair must be unique. Keys are always typecasted to `uint64_t` e.g. you can use pointers as keys.

## Return Value

Returns a pointer to the item set into the table.

## Code Example

> Set and get a few elements from a hashtable.

```cpp
htbl int table = NULL;
hadd(table, 0, 5);
hadd(table, 1, 12);
CF_ASSERT(hfind(table, 0) == 5);
CF_ASSERT(hfind(table, 1) == 12);
hfree(table);
```

## Remarks

Items are returned by value, not pointer. If the item doesn't exist a zero'd out item is instead returned. If you want to get a pointer
(so you can see if it's `NULL` in case the item didn't exist, then use [hfind_ptr](/hash/hfind_ptr.md)). You can also call [hhas](/hash/hhas.md) for a bool. This function does
the same as [hget](/hash/hget.md).

## Related Pages

[htbl](/hash/htbl.md)  
[hset](/hash/hset.md)  
[hadd](/hash/hadd.md)  
[hget](/hash/hget.md)  
[hfree](/hash/hfree.md)  
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
