# hitems | [hash](https://github.com/RandyGaul/cute_framework/blob/master/docs/hash/README.md) | [cute_hashtable.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_hashtable.h)

Get a pointer to the array of items.

```cpp
#define hitems(h) cf_hashtable_items(h)
```

Parameters | Description
--- | ---
h | The hashtable. Can be `NULL`. Needs to be a pointer to the type of items in the table.

## Code Example

> Loop over all {key, item} pairs of a table.

```cpp
htbl CF_V2 table = my_table();
const uint64_t keys = hkeys(table);
for (int i = 0; i < hcount(table); ++i) {
    uint64_t key = keys[i];
    CF_V2 item = table[i]; // Could also do `hitems(table)` here.
    // ...
}
```

## Remarks

This macro doesn't do much as `h` is already a valid pointer to the items.

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
[hclear](https://github.com/RandyGaul/cute_framework/blob/master/docs/hash/hclear.md)  
[hkeys](https://github.com/RandyGaul/cute_framework/blob/master/docs/hash/hkeys.md)  
[hfree](https://github.com/RandyGaul/cute_framework/blob/master/docs/hash/hfree.md)  
[hswap](https://github.com/RandyGaul/cute_framework/blob/master/docs/hash/hswap.md)  
[hsize](https://github.com/RandyGaul/cute_framework/blob/master/docs/hash/hsize.md)  
[hcount](https://github.com/RandyGaul/cute_framework/blob/master/docs/hash/hcount.md)  
