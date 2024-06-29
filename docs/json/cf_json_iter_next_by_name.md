[](../header.md ':include')

# cf_json_iter_next_by_name

Category: [json](/api_reference?id=json)  
GitHub: [cute_json.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_json.h)  
---

Proceeds to the next element with a matching name.

```cpp
CF_JVal cf_json_iter_next_by_name(CF_JIter* iter, const char* key);
```

## Remarks

You should know the ordering of your key/val pairs before calling this function, as it only searches forwards. See [cf_json_iter](/json/cf_json_iter.md).

## Related Pages

[CF_JVal](/json/cf_jval.md)  
[cf_json_get](/json/cf_json_get.md)  
[cf_json_array_at](/json/cf_json_array_at.md)  
[cf_json_array_get](/json/cf_json_array_get.md)  
[cf_json_iter](/json/cf_json_iter.md)  
[cf_json_iter_remove](/json/cf_json_iter_remove.md)  
