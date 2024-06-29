[](../header.md ':include')

# cf_json_set_string_range

Category: [json](/api_reference?id=json)  
GitHub: [cute_json.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_json.h)  
---

Sets the [CF_JVal](/json/cf_jval.md) to a string.

```cpp
void cf_json_set_string_range(CF_JVal jval, const char* begin, const char* end);
```

## Remarks

The string must be retained in memory while the [CF_JDoc](/json/cf_jdoc.md) persists.

## Related Pages

[CF_JVal](/json/cf_jval.md)  
[cf_json_set_null](/json/cf_json_set_null.md)  
[cf_json_set_int](/json/cf_json_set_int.md)  
[cf_json_set_i64](/json/cf_json_set_i64.md)  
[cf_json_set_u64](/json/cf_json_set_u64.md)  
[cf_json_set_float](/json/cf_json_set_float.md)  
[cf_json_set_double](/json/cf_json_set_double.md)  
[cf_json_set_bool](/json/cf_json_set_bool.md)  
[cf_json_set_string](/json/cf_json_set_string.md)  
