[](../header.md ':include')

# sppop

Category: [path](/api_reference?id=path)  
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
string is modified in-place. You must call [sfree](/string/sfree.md) if a new dynamic string is returned, when done.

## Remarks

`sp` stands for "sting path".

## Related Pages

[spfname](/path/spfname.md)  
[spfname_no_ext](/path/spfname_no_ext.md)  
[spext](/path/spext.md)  
[spext_equ](/path/spext_equ.md)  
[spnorm](/path/spnorm.md)  
[sppopn](/path/sppopn.md)  
[spcompact](/path/spcompact.md)  
[spdir_of](/path/spdir_of.md)  
[sptop_of](/path/sptop_of.md)  
