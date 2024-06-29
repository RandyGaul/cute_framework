[](../header.md ':include')

# cf_json_array_from_double

Category: [json](/api_reference?id=json)  
GitHub: [cute_json.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_json.h)  
---

Creates a new json array from an array of 64-bit floats.

```cpp
CF_JVal cf_json_array_from_double(CF_JDoc doc, double* vals, int count);
```

## Remarks

The returned [CF_JVal](/json/cf_jval.md) can be attached to the document by [cf_json_array_add](/json/cf_json_array_add.md) or [cf_json_object_add](/json/cf_json_object_add.md).

## Related Pages

[CF_JVal](/json/cf_jval.md)  
[cf_json_array](/json/cf_json_array.md)  
[cf_json_array_from_int](/json/cf_json_array_from_int.md)  
[cf_json_array_from_i64](/json/cf_json_array_from_i64.md)  
[cf_json_array_from_u64](/json/cf_json_array_from_u64.md)  
[cf_json_array_from_float](/json/cf_json_array_from_float.md)  
[cf_json_object_add](/json/cf_json_object_add.md)  
[cf_json_array_from_bool](/json/cf_json_array_from_bool.md)  
[cf_json_array_from_string](/json/cf_json_array_from_string.md)  
[cf_json_array_add](/json/cf_json_array_add.md)  
