# hget | [hash](https://github.com/RandyGaul/cute_framework/blob/master/docs/hash/README.md) | [cute_hashtable.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_hashtable.h)

Fetches the item that `k` maps to.

```cpp
#define hget(h, k) cf_hashtable_get(h, k)
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
CUTE_ASSERT(hget(table, 0) == 5);
CUTE_ASSERT(hget(table, 1) == 12);
hfree(table);
```

## Remarks

Items are returned by value, not pointer. If the item doesn't exist a zero'd out item is instead returned. If you want to get a pointer
(so you can see if it's `NULL` in case the item didn't exist, then use [hget_ptr](https://github.com/RandyGaul/cute_framework/blob/master/docs/hash/hget_ptr.md)). You can also call [hhas](https://github.com/RandyGaul/cute_framework/blob/master/docs/hash/hhas.md) for a bool. This function does
the same as [hfind](https://github.com/RandyGaul/cute_framework/blob/master/docs/hash/hfind.md).

## Related Pages

[htbl](https://github.com/RandyGaul/cute_framework/blob/master/docs/hash/htbl.md)  
[hset](https://github.com/RandyGaul/cute_framework/blob/master/docs/hash/hset.md)  
[hadd](https://github.com/RandyGaul/cute_framework/blob/master/docs/hash/hadd.md)  
[hfree](https://github.com/RandyGaul/cute_framework/blob/master/docs/hash/hfree.md)  
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
