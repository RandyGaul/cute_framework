[](../header.md ':include')

# cf_json_iter

Category: [json](/api_reference?id=json)  
GitHub: [cute_json.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_json.h)  
---

Creates an iterator for a given JSON value.

```cpp
CF_JIter cf_json_iter(CF_JVal val);
```

Parameters | Description
--- | ---
val | The JSON value to iterate upon.

## Return Value

Returns a [CF_JIter](/json/cf_jiter.md) for iterating.

## Code Example

> Traversing arrays/objects.

```cpp
      // Traverse an array of strings:
      for (CF_JIter i = cf_json_iter(v); !cf_json_iter_done(i); i = cf_json_iter_next(i)) {
          const char val = cf_json_get_string(cf_json_iter_val(i));
          printf("%s\n", val);
      }
      
      // Traverse key/val pairs on an objects:
      for (CF_JIter i = cf_json_iter(v); !cf_json_iter_done(i); iter = cf_json_iter_next(i)) {
          const char val = cf_json_get_string(cf_json_iter_val(i));
          printf("%s\n", val);
      }
```

## Remarks

The [CF_JIter](/json/cf_jiter.md) can be used in foor loops, and can traverse both JSON arrays and objects. When
traversing arrays do not call [cf_json_iter_key](/json/cf_json_iter_key.md).

## Related Pages

[CF_JVal](/json/cf_jval.md)  
[cf_json_get](/json/cf_json_get.md)  
[cf_json_array_at](/json/cf_json_array_at.md)  
[cf_json_array_get](/json/cf_json_array_get.md)  
[cf_json_iter_remove](/json/cf_json_iter_remove.md)  
