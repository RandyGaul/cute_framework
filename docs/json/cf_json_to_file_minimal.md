[](../header.md ':include')

# cf_json_to_file_minimal

Category: [json](/api_reference?id=json)  
GitHub: [cute_json.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_json.h)  
---

Saves the json document to a file.

```cpp
CF_Result cf_json_to_file_minimal(CF_JDoc doc, const char* virtual_path);
```

Parameters | Description
--- | ---
doc | The json document to save.
virtual_path | A virtual path to the json file. Make sure to setup your write directory with [cf_fs_set_write_directory](/file/cf_fs_set_write_directory.md). See [Virtual File System](https://randygaul.github.io/cute_framework/#/topics/virtual_file_system).

## Related Pages

[CF_JDoc](/json/cf_jdoc.md)  
[cf_make_json](/json/cf_make_json.md)  
[cf_make_json_from_file](/json/cf_make_json_from_file.md)  
[cf_json_get_root](/json/cf_json_get_root.md)  
[cf_destroy_json](/json/cf_destroy_json.md)  
[cf_json_get_root](/json/cf_json_get_root.md)  
[cf_json_to_string](/json/cf_json_to_string.md)  
[cf_json_to_file](/json/cf_json_to_file.md)  
[cf_json_to_string_minimal](/json/cf_json_to_string_minimal.md)  
