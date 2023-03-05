# hfind_ptr | [hash](https://github.com/RandyGaul/cute_framework/blob/master/docs/hash/README.md) | [cute_hashtable.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_hashtable.h)

Fetches a pointer to the item that `k` maps to.

```cpp
#define hfind_ptr(h, k) cf_hashtable_find_ptr(h, k)
```

Parameters | Description
--- | ---
h | The hashtable. Can be `NULL`. Needs to be a pointer to the type of items in the table.
k | The key for lookups. Each {key, item} pair must be unique. Keys are always typecasted to `uint64_t` e.g. you can use pointers as keys.

## Return Value

Returns a pointer to an item. Returns `NULL` if not found.

## Code Example

> Set and get a few elements from a hashtable.

```cpp
htbl CF_V2 table = NULL;
hadd(table, 10, cf_v2(-1, 1));
CF_V2 v = hfind_ptr(table, 10);
CUTE_ASSERT(v);
CUTE_ASSERT(v->x == -1);
CUTE_ASSERT(v->y == 1);
hfree(table);
```

## Remarks

If you want to fetch an item by value, you can use [hget](https://github.com/RandyGaul/cute_framework/blob/master/docs/hash/hget.md) or [hfind](https://github.com/RandyGaul/cute_framework/blob/master/docs/hash/hfind.md). Does the same thing as [hget_ptr](https://github.com/RandyGaul/cute_framework/blob/master/docs/hash/hget_ptr.md).

## Related Pages

[htbl](https://github.com/RandyGaul/cute_framework/blob/master/docs/hash/htbl.md)  
[hset](https://github.com/RandyGaul/cute_framework/blob/master/docs/hash/hset.md)  
[hadd](https://github.com/RandyGaul/cute_framework/blob/master/docs/hash/hadd.md)  
[hget](https://github.com/RandyGaul/cute_framework/blob/master/docs/hash/hget.md)  
[hfind](https://github.com/RandyGaul/cute_framework/blob/master/docs/hash/hfind.md)  
[hget_ptr](https://github.com/RandyGaul/cute_framework/blob/master/docs/hash/hget_ptr.md)  
[hfree](https://github.com/RandyGaul/cute_framework/blob/master/docs/hash/hfree.md)  
[hhas](https://github.com/RandyGaul/cute_framework/blob/master/docs/hash/hhas.md)  
[hdel](https://github.com/RandyGaul/cute_framework/blob/master/docs/hash/hdel.md)  
[hclear](https://github.com/RandyGaul/cute_framework/blob/master/docs/hash/hclear.md)  
[hkeys](https://github.com/RandyGaul/cute_framework/blob/master/docs/hash/hkeys.md)  
[hitems](https://github.com/RandyGaul/cute_framework/blob/master/docs/hash/hitems.md)  
[hswap](https://github.com/RandyGaul/cute_framework/blob/master/docs/hash/hswap.md)  
[hsize](https://github.com/RandyGaul/cute_framework/blob/master/docs/hash/hsize.md)  
[hcount](https://github.com/RandyGaul/cute_framework/blob/master/docs/hash/hcount.md)  
