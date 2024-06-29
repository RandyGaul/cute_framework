[](../header.md ':include')

# cf_json_to_string_minimal

Category: [json](/api_reference?id=json)  
GitHub: [cute_json.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_json.h)  
---

Saves the json document as a string.

```cpp
dyna char* cf_json_to_string_minimal(CF_JDoc doc);
```

## Return Value

Returns a dynamic string, free it with [sfree](/string/sfree.md) when done.

## Related Pages

[CF_JDoc](/json/cf_jdoc.md)  
[cf_make_json](/json/cf_make_json.md)  
[cf_make_json_from_file](/json/cf_make_json_from_file.md)  
[cf_json_get_root](/json/cf_json_get_root.md)  
[cf_destroy_json](/json/cf_destroy_json.md)  
[cf_json_get_root](/json/cf_json_get_root.md)  
[cf_json_to_string](/json/cf_json_to_string.md)  
[cf_json_to_file](/json/cf_json_to_file.md)  
