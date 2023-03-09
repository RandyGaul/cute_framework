[](../header.md ':include')

# spnorm

Category: [path](https://github.com/RandyGaul/cute_framework/blob/master/docs/api_reference?id=path)  
GitHub: [cute_file_system.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_file_system.h)  
---

Normalizes a path as a new string.

```cpp
#define spnorm(s) cf_path_normalize(s)
```

Parameters | Description
--- | ---
s | The path string.

## Remarks

All '\\' are replaced with '/'. Any duplicate '////' are replaced with a single '/'. Trailing '/' are removed. Dot folders are resolved, e.g.
```
spnorm("/a/b/./c") -> "/a/b/c"
spnorm("/a/b/../c") -> "/a/c"
```
The first character is always '/', unless it's a windows drive, e.g.
```
spnorm("C:\\Users\\Randy\\Documents") -> "C:/Users/Randy/Documents"
```

## Related Pages

[spfname](https://github.com/RandyGaul/cute_framework/blob/master/docs/path/spfname.md)  
[spfname_no_ext](https://github.com/RandyGaul/cute_framework/blob/master/docs/path/spfname_no_ext.md)  
[spext](https://github.com/RandyGaul/cute_framework/blob/master/docs/path/spext.md)  
[spext_equ](https://github.com/RandyGaul/cute_framework/blob/master/docs/path/spext_equ.md)  
[sppop](https://github.com/RandyGaul/cute_framework/blob/master/docs/path/sppop.md)  
[sppopn](https://github.com/RandyGaul/cute_framework/blob/master/docs/path/sppopn.md)  
[spcompact](https://github.com/RandyGaul/cute_framework/blob/master/docs/path/spcompact.md)  
[spdir_of](https://github.com/RandyGaul/cute_framework/blob/master/docs/path/spdir_of.md)  
[sptop_of](https://github.com/RandyGaul/cute_framework/blob/master/docs/path/sptop_of.md)  
