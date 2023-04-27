[](../header.md ':include')

# cf_load_function

Category: [utility](/api_reference?id=utility)  
GitHub: [cute_symbol.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_symbol.h)  
---

Loads a function out of a shared library.

```cpp
void* cf_load_function(CF_SharedLibrary* library, const char* function_name);
```

Parameters | Description
--- | ---
library | A library of functions from `load_shared_library`.
function_name | The name of the function.

## Remarks

The function pointer is not valid after calling `unload_shared_library`. After obtaining the function pointer with `load_function`
you must typecast it yourself. Returns `NULL` in the case of errors.

## Related Pages

[cf_load_shared_library](/utility/cf_load_shared_library.md)  
[cf_unload_shared_library](/utility/cf_unload_shared_library.md)  
