# sppop

Category: [path](https://github.com/RandyGaul/cute_framework/blob/master/docs/api_reference?id=path)  
GitHub: [cute_file_system.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_file_system.h)  
---

Removes the rightmost file or directory from the path.

```cpp
#define sppop(s) cf_path_pop(s)
```

Parameters | Description
--- | ---
s | The path string.

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
[spnorm](https://github.com/RandyGaul/cute_framework/blob/master/docs/path/spnorm.md)  
[sppopn](https://github.com/RandyGaul/cute_framework/blob/master/docs/path/sppopn.md)  
[spcompact](https://github.com/RandyGaul/cute_framework/blob/master/docs/path/spcompact.md)  
[spdir_of](https://github.com/RandyGaul/cute_framework/blob/master/docs/path/spdir_of.md)  
[sptop_of](https://github.com/RandyGaul/cute_framework/blob/master/docs/path/sptop_of.md)  
