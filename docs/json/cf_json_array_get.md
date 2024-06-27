[](../header.md ':include')

# cf_json_array_get

Category: [json](/api_reference?id=json)  
GitHub: [cute_json.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_json.h)  
---

Fetches a value in the given array.

```cpp
CF_JVal cf_json_array_get(CF_JVal val, int index);
```

Parameters | Description
--- | ---
val | The JSON value to search for `key` within.
index | The index of the value to return.

## Return Value

Returns the [CF_JVal](/json/cf_jval.md) associated with `index` on the object `val`.

## Remarks

This function does the same thing as [cf_json_array_at](/json/cf_json_array_at.md).

## Related Pages

[CF_JVal](/json/cf_jval.md)  
[cf_json_get](/json/cf_json_get.md)  
[cf_json_array_at](/json/cf_json_array_at.md)  
[cf_json_iter](/json/cf_json_iter.md)  
