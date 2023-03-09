# hhas

Category: [hash](https://github.com/RandyGaul/cute_framework/blob/master/docs/api_reference?id=hash)  
GitHub: [cute_hashtable.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_hashtable.h)  
---

Check to see if an item exists in the table.

```cpp
#define hhas(h, k) cf_hashtable_has(h, k)
```

Parameters | Description
--- | ---
h | The hashtable. Can be `NULL`. Needs to be a pointer to the type of items in the table.
k | The key for lookups. Each {key, item} pair must be unique. Keys are always typecasted to `uint64_t` e.g. you can use pointers as keys.

## Return Value

Returns true if the item was found, false otherwise.

## Code Example

> Checks if an item exists in the table.

```cpp
htbl v2 table = NULL;
hadd(table, 10, V2(-1, 1));
CUTE_ASSERT(hhas(table, 10));
hfree(table);
```

## Related Pages

[htbl](https://github.com/RandyGaul/cute_framework/blob/master/docs/hash/htbl.md)  
[hset](https://github.com/RandyGaul/cute_framework/blob/master/docs/hash/hset.md)  
[hadd](https://github.com/RandyGaul/cute_framework/blob/master/docs/hash/hadd.md)  
[hget](https://github.com/RandyGaul/cute_framework/blob/master/docs/hash/hget.md)  
[hfind](https://github.com/RandyGaul/cute_framework/blob/master/docs/hash/hfind.md)  
[hget_ptr](https://github.com/RandyGaul/cute_framework/blob/master/docs/hash/hget_ptr.md)  
[hfind_ptr](https://github.com/RandyGaul/cute_framework/blob/master/docs/hash/hfind_ptr.md)  
[hfree](https://github.com/RandyGaul/cute_framework/blob/master/docs/hash/hfree.md)  
[hdel](https://github.com/RandyGaul/cute_framework/blob/master/docs/hash/hdel.md)  
[hclear](https://github.com/RandyGaul/cute_framework/blob/master/docs/hash/hclear.md)  
[hkeys](https://github.com/RandyGaul/cute_framework/blob/master/docs/hash/hkeys.md)  
[hitems](https://github.com/RandyGaul/cute_framework/blob/master/docs/hash/hitems.md)  
[hswap](https://github.com/RandyGaul/cute_framework/blob/master/docs/hash/hswap.md)  
[hsize](https://github.com/RandyGaul/cute_framework/blob/master/docs/hash/hsize.md)  
[hcount](https://github.com/RandyGaul/cute_framework/blob/master/docs/hash/hcount.md)  
