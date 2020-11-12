
# strpool_defrag

Repacks all internal strings into a tightly packed block of memory and rehashes them. All ids remain valid.

## Syntax

```cpp
void strpool_defrag(strpool_t* pool);
```

## Function Parameters

Parameter Name | Description
--- | ---
pool | The string pool to degfragment.

## Remarks

Over time as strings are added and removed empty space between strings causes memory fragmentation. This causes the string pool to take up more memory than necessary. It can be useful to periodically or strategically degragment the pool to avoid this inefficiency.

## Related Functions

[strpool_inject](https://github.com/RandyGaul/cute_framework/blob/master/doc/string/strpool/strpool_inject.md)  
[strpool_discard](https://github.com/RandyGaul/cute_framework/blob/master/doc/string/strpool/strpool_discard.md)  
