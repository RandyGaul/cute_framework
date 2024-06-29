[](../header.md ':include')

# cf_load_shared_library

Category: [utility](/api_reference?id=utility)  
GitHub: [cute_symbol.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_symbol.h)  
---

Loads a shared library from disk and returns a pointer to the library.

```cpp
CF_SharedLibrary* cf_load_shared_library(const char* path);
```

Parameters | Description
--- | ---
path | Path to the shared library in platform-dependent notation.

## Return Value

Returns `NULL` in the case of errors, and can be unloaded by calling `unload_shared_library`.

## Remarks

Does not use the virtual file system. Once loaded, individual functions can be loaded from the shared
library be called [cf_load_function](/utility/cf_load_function.md). See [Virtual File System](https://randygaul.github.io/cute_framework/#/topics/virtual_file_system).

## Related Pages

[cf_load_function](/utility/cf_load_function.md)  
[cf_unload_shared_library](/utility/cf_unload_shared_library.md)  
