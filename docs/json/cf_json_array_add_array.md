[](../header.md ':include')

# cf_json_array_add_array

Category: [json](/api_reference?id=json)  
GitHub: [cute_json.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_json.h)  
---

Appends an array to the end of a json array.

```cpp
CF_JVal cf_json_array_add_array(CF_JDoc doc, CF_JVal arr);
```

## Return Value

Returns the newly added array.

## Remarks

This function is not equivalent to [cf_json_array_add](/json/cf_json_array_add.md). Instead, this actually appends the values onto the new array.

## Related Pages

[CF_JVal](/json/cf_jval.md)  
[cf_json_array_add](/json/cf_json_array_add.md)  
[cf_json_object_add](/json/cf_json_object_add.md)  
