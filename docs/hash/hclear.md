# hclear | [hash](https://github.com/RandyGaul/cute_framework/blob/master/docs/hash/README.md) | [cute_hashtable.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_hashtable.h)

Clears the hashtable.

```cpp
#define hclear(h) cf_hashtable_clear(h)
```

Parameters | Description
--- | ---
h | The hashtable. Can be `NULL`. Needs to be a pointer to the type of items in the table.

## Remarks

The count of items will now be zero. Does not free any memory. Call [hfree](https://github.com/RandyGaul/cute_framework/blob/master/docs/hash/hfree.md) when you are done.

## Related Pages

[htbl](https://github.com/RandyGaul/cute_framework/blob/master/docs/hash/htbl.md)  
[hset](https://github.com/RandyGaul/cute_framework/blob/master/docs/hash/hset.md)  
[hadd](https://github.com/RandyGaul/cute_framework/blob/master/docs/hash/hadd.md)  
[hget](https://github.com/RandyGaul/cute_framework/blob/master/docs/hash/hget.md)  
[hfind](https://github.com/RandyGaul/cute_framework/blob/master/docs/hash/hfind.md)  
[hget_ptr](https://github.com/RandyGaul/cute_framework/blob/master/docs/hash/hget_ptr.md)  
[hfind_ptr](https://github.com/RandyGaul/cute_framework/blob/master/docs/hash/hfind_ptr.md)  
[hhas](https://github.com/RandyGaul/cute_framework/blob/master/docs/hash/hhas.md)  
[hdel](https://github.com/RandyGaul/cute_framework/blob/master/docs/hash/hdel.md)  
[hfree](https://github.com/RandyGaul/cute_framework/blob/master/docs/hash/hfree.md)  
[hkeys](https://github.com/RandyGaul/cute_framework/blob/master/docs/hash/hkeys.md)  
[hitems](https://github.com/RandyGaul/cute_framework/blob/master/docs/hash/hitems.md)  
[hswap](https://github.com/RandyGaul/cute_framework/blob/master/docs/hash/hswap.md)  
[hsize](https://github.com/RandyGaul/cute_framework/blob/master/docs/hash/hsize.md)  
[hcount](https://github.com/RandyGaul/cute_framework/blob/master/docs/hash/hcount.md)  
