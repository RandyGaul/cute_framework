[](../header.md ':include')

# cf_make_json

Category: [json](/api_reference?id=json)  
GitHub: [cute_json.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_json.h)  
---

Loads a json blob.

```cpp
CF_JDoc cf_make_json(const void* data, size_t size);
```

Parameters | Description
--- | ---
data | A pointer to the raw json blob data.
size | The number of bytes in the `data` pointer.

## Return Value

Returns a [CF_JDoc](/json/cf_jdoc.md).

## Remarks

You should call [cf_json_get_root](/json/cf_json_get_root.md) on this document to begin fetching values out of it.

## Related Pages

[CF_JDoc](/json/cf_jdoc.md)  
[cf_json_to_file](/json/cf_json_to_file.md)  
[cf_make_json_from_file](/json/cf_make_json_from_file.md)  
[cf_json_get_root](/json/cf_json_get_root.md)  
[cf_destroy_json](/json/cf_destroy_json.md)  
[cf_json_get_root](/json/cf_json_get_root.md)  
[cf_json_to_string](/json/cf_json_to_string.md)  
