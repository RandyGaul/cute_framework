# hadd | [hash](https://github.com/RandyGaul/cute_framework/blob/master/docs/hash/README.md) | [cute_hashtable.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_hashtable.h)

Add's a {key, item} pair.

```cpp
#define hadd(h, k, ...) cf_hashtable_add(h, k, (__VA_ARGS__))
```

Parameters | Description
--- | ---
h | The hashtable. Can be `NULL`. Needs to be a pointer to the type of items in the table.
k | The key for lookups. Each {key, item} pair must be unique. Keys are always typecasted to `uint64_t` e.g. you can use pointers as keys.
... | An item to place in the table.

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

This function works the same as [hset](https://github.com/RandyGaul/cute_framework/blob/master/docs/hash/hset.md). If the item already exists in the table, it's simply updated to a new value.
The pointer returned is not stable. Internally the table can be resized, invalidating _all_ pointers to any elements
within the table. Therefor, no items may store pointers to themselves or other items. Indices however, are totally fine.

## Related Pages

[htbl](https://github.com/RandyGaul/cute_framework/blob/master/docs/hash/htbl.md)  
[hset](https://github.com/RandyGaul/cute_framework/blob/master/docs/hash/hset.md)  
[hfree](https://github.com/RandyGaul/cute_framework/blob/master/docs/hash/hfree.md)  
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
