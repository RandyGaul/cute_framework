[](../header.md ':include')

# cf_json_from_string

Category: [json](/api_reference?id=json)  
GitHub: [cute_json.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_json.h)  
---

Creates and returns a new string json value.

```cpp
CF_JVal cf_json_from_string(CF_JDoc doc, const char* val);
```

## Remarks

The value can be attached to the document by [cf_json_array_add](/json/cf_json_array_add.md) or [cf_json_object_add](/json/cf_json_object_add.md).

## Related Pages

[CF_JVal](/json/cf_jval.md)  
[cf_json_from_null](/json/cf_json_from_null.md)  
[cf_json_from_int](/json/cf_json_from_int.md)  
[cf_json_from_float](/json/cf_json_from_float.md)  
[cf_json_from_bool](/json/cf_json_from_bool.md)  
[cf_json_object_add](/json/cf_json_object_add.md)  
[cf_json_from_string_range](/json/cf_json_from_string_range.md)  
[cf_json_array_add](/json/cf_json_array_add.md)  
