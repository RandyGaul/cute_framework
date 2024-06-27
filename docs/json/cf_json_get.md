[](../header.md ':include')

# cf_json_get

Category: [json](/api_reference?id=json)  
GitHub: [cute_json.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_json.h)  
---

Looks up a value for a given key.

```cpp
CF_JVal cf_json_get(CF_JVal val, const char* key);
```

Parameters | Description
--- | ---
val | The JSON value to search for `key` within.
key | The search key.

## Return Value

Returns the [CF_JVal](/json/cf_jval.md) associated with `key` on the object `val`.

## Related Pages

[CF_JVal](/json/cf_jval.md)  
[cf_json_iter](/json/cf_json_iter.md)  
[cf_json_array_at](/json/cf_json_array_at.md)  
[cf_json_array_get](/json/cf_json_array_get.md)  
