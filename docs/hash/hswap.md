[](../header.md ':include')

# hswap

Category: [hash](/api_reference?id=hash)  
GitHub: [cute_hashtable.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_hashtable.h)  
---

Swaps internal ordering of two {key, item} pairs without ruining the hashing.

```cpp
#define hswap(h, index_a, index_b) cf_hashtable_swap(h, index_a, index_b)
```

Parameters | Description
--- | ---
h | The hashtable. Can be `NULL`. Needs to be a pointer to the type of items in the table.
index_a | Index to the first item to swap.
index_b | Index to the second item to swap.

## Code Example

> Loop over all {key, item} pairs of a table.

```cpp
htbl CF_V2 table = my_table();
const uint64_t keys = hkeys(table);
for (int i = 0; i < hcount(table); ++i) {
    for (int j = 0; j < hcount(table); ++j) {
        if (my_need_swap(table, i, j)) {
            hswap(table, i, j);
        }
    }
}
```

## Remarks

Use this for e.g. implementing a priority queue on top of the hash table.

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
[hclear](/hash/hclear.md)  
[hkeys](/hash/hkeys.md)  
[hitems](/hash/hitems.md)  
[hfree](/hash/hfree.md)  
[hsize](/hash/hsize.md)  
[hcount](/hash/hcount.md)  
