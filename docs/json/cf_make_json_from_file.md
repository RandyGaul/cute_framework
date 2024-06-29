[](../header.md ':include')

# cf_make_json_from_file

Category: [json](/api_reference?id=json)  
GitHub: [cute_json.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_json.h)  
---

Loads a json blob from a file.

```cpp
CF_JDoc cf_make_json_from_file(const char* virtual_path);
```

Parameters | Description
--- | ---
virtual_path | A virtual path to the json file. See [Virtual File System](https://randygaul.github.io/cute_framework/#/topics/virtual_file_system).

## Return Value

Returns a [CF_JDoc](/json/cf_jdoc.md).

## Remarks

You should call [cf_json_get_root](/json/cf_json_get_root.md) on this document to begin fetching values out of it.

## Related Pages

[CF_JDoc](/json/cf_jdoc.md)  
[cf_make_json](/json/cf_make_json.md)  
[cf_json_to_file](/json/cf_json_to_file.md)  
[cf_json_get_root](/json/cf_json_get_root.md)  
[cf_destroy_json](/json/cf_destroy_json.md)  
[cf_json_get_root](/json/cf_json_get_root.md)  
[cf_json_to_string](/json/cf_json_to_string.md)  
