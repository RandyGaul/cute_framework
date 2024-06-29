[](../header.md ':include')

# cf_json_iter_key

Category: [json](/api_reference?id=json)  
GitHub: [cute_json.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_json.h)  
---

Returns the key currently referenced by the iterator.

```cpp
const char* cf_json_iter_key(CF_JIter iter);
```

## Remarks

You should not call this function when iterating over an array. See [cf_json_iter](/json/cf_json_iter.md).

## Related Pages

[CF_JVal](/json/cf_jval.md)  
[cf_json_get](/json/cf_json_get.md)  
[cf_json_array_at](/json/cf_json_array_at.md)  
[cf_json_array_get](/json/cf_json_array_get.md)  
[cf_json_iter](/json/cf_json_iter.md)  
[cf_json_iter_remove](/json/cf_json_iter_remove.md)  
