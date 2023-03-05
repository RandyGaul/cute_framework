# cf_unload_shared_library | [utility](https://github.com/RandyGaul/cute_framework/blob/master/docs/utility_readme.md) | [cute_symbol.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_symbol.h)

Unloads a shared library previously loaded with `load_shared_library`.

```cpp
void cf_unload_shared_library(CF_SharedLibrary* library);
```

Parameters | Description
--- | ---
library | A library of functions from `load_shared_library`.

## Related Pages

[cf_load_shared_library](https://github.com/RandyGaul/cute_framework/blob/master/docs/utility/cf_load_shared_library.md)  
[cf_load_function](https://github.com/RandyGaul/cute_framework/blob/master/docs/utility/cf_load_function.md)  
