# string_defrag_static_pool

Repacks all internal strings into a tightly packed block of memory and rehashes them. All ids remain valid. This string only operates on the default static string pool, see remarks for more details.

## Syntax

```cpp
void string_defrag_static_pool();
```

## Remarks

Over time as strings are added and removed empty space between strings causes memory fragmentation. This causes the string pool to take up more memory than necessary. It can be useful to periodically or strategically degragment the pool to avoid this inefficiency.

The `string_t` class uses a default static [string pool](https://github.com/RandyGaul/cute_framework/tree/master/doc/string/strpool). This function operates on that static pool.

## Related Functions

[string_nuke_static_pool](https://github.com/RandyGaul/cute_framework/blob/master/doc/string/strpool/string_nuke_static_pool.md)  
