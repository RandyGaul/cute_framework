[](../header.md ':include')

# sppopn

Category: [path](/api_reference?id=path)  
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
string is modified in-place. You must call [sfree](/string/sfree.md) if a new dynamic string is returned, when done.

## Remarks

`sp` stands for "sting path".

## Related Pages

[spfname](/path/spfname.md)  
[spfname_no_ext](/path/spfname_no_ext.md)  
[spext](/path/spext.md)  
[spext_equ](/path/spext_equ.md)  
[sppop](/path/sppop.md)  
[spnorm](/path/spnorm.md)  
[spcompact](/path/spcompact.md)  
[spdir_of](/path/spdir_of.md)  
[sptop_of](/path/sptop_of.md)  
