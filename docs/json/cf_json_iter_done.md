[](../header.md ':include')

# cf_json_iter_done

Category: [json](/api_reference?id=json)  
GitHub: [cute_json.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_json.h)  
---

Returns true if the [CF_JIter](/json/cf_jiter.md) has finished iterating over all elements.

```cpp
#define cf_json_iter_done(iter) ((iter).index >= (iter).count)
```

## Remarks

See [cf_json_iter](/json/cf_json_iter.md).

## Related Pages

[CF_JVal](/json/cf_jval.md)  
[cf_json_get](/json/cf_json_get.md)  
[cf_json_array_at](/json/cf_json_array_at.md)  
[cf_json_array_get](/json/cf_json_array_get.md)  
[cf_json_iter](/json/cf_json_iter.md)  
[cf_json_iter_remove](/json/cf_json_iter_remove.md)  
