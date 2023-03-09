[](../header.md ':include')

# sppopn

Category: [path](https://github.com/RandyGaul/cute_framework/blob/master/docs/api_reference?id=path)  
GitHub: [cute_file_system.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_file_system.h)  
---

Removes the rightmost n files or directories from the path.

```cpp
#define sppopn(s, n) cf_path_pop_n(s, n)
```

Parameters | Description
--- | ---
s | The path string.
n | The number of files to pop from the directory path.

## Return Value

If the string is not a dynamic string from CF's string API, a new string is returned. Otherwise the
string is modified in-place. You must call [sfree](https://github.com/RandyGaul/cute_framework/blob/master/docs/string/sfree.md) if a new dynamic string is returned, when done.

## Remarks

`sp` stands for "sting path".

## Related Pages

[spfname](https://github.com/RandyGaul/cute_framework/blob/master/docs/path/spfname.md)  
[spfname_no_ext](https://github.com/RandyGaul/cute_framework/blob/master/docs/path/spfname_no_ext.md)  
[spext](https://github.com/RandyGaul/cute_framework/blob/master/docs/path/spext.md)  
[spext_equ](https://github.com/RandyGaul/cute_framework/blob/master/docs/path/spext_equ.md)  
[sppop](https://github.com/RandyGaul/cute_framework/blob/master/docs/path/sppop.md)  
[spnorm](https://github.com/RandyGaul/cute_framework/blob/master/docs/path/spnorm.md)  
[spcompact](https://github.com/RandyGaul/cute_framework/blob/master/docs/path/spcompact.md)  
[spdir_of](https://github.com/RandyGaul/cute_framework/blob/master/docs/path/spdir_of.md)  
[sptop_of](https://github.com/RandyGaul/cute_framework/blob/master/docs/path/sptop_of.md)  
