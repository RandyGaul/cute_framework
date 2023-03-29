[](../header.md ':include')

# hsort

Category: [hash](/api_reference?id=hash)  
GitHub: [cute_hashtable.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_hashtable.h)  
---

Sorts the {key, item} pairs in the table by keys.

```cpp
#define hsort(h) cf_hashtable_sort(h)
```

Parameters | Description
--- | ---
h | The hashtable. Can be `NULL`. Needs to be a pointer to the type of items in the table.

## Remarks

The keys and items returned by [hkeys](/hash/hkeys.md) and [hitems](/hash/hitems.md) will be sorted. Recall that all keys in the hashtable are treated
as `uint64_t`, so the sorting simply sorts the keys from least to greatest as `uint64_t`. This is _not_ a stable sort.

## Related Pages

[htbl](/hash/htbl.md)  
[hswap](/hash/hswap.md)  
[hsisort](/hash/hsisort.md)  
[hssort](/hash/hssort.md)  
