[](../header.md ':include')

# CF_JIter

Category: [json](/api_reference?id=json)  
GitHub: [cute_json.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_json.h)  
---

An iterator for looping over arrays or key-value pairs.

Struct Members | Description
--- | ---
`size_t index` | For internal use. Don't touch. Used for iterating arrays.
`size_t count` | For internal use. Don't touch.
`CF_JVal val` | For internal use. Don't touch. The current [CF_JVal](/json/cf_jval.md).
`CF_JVal prev` | For internal use. Don't touch.
`CF_JVal parent` | For internal use. Don't touch.

## Related Pages

[cf_json_iter_val](/json/cf_json_iter_val.md)  
[cf_json_iter](/json/cf_json_iter.md)  
[cf_json_iter_next](/json/cf_json_iter_next.md)  
[cf_json_iter_next_by_name](/json/cf_json_iter_next_by_name.md)  
[cf_json_iter_remove](/json/cf_json_iter_remove.md)  
[cf_json_iter_key](/json/cf_json_iter_key.md)  
