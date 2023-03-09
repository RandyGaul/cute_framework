[](../header.md ':include')

# spcompact

Category: [path](/api_reference?id=path)  
GitHub: [cute_file_system.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_file_system.h)  
---

Squishes the path to be less than or equal to n characters in length.

```cpp
#define spcompact(s, n) cf_path_compact(s, n)
```

Parameters | Description
--- | ---
s | The path string.
n | The number of files to pop from the directory path.

## Return Value

If the string is not a dynamic string from CF's string API, a new string is returned. Otherwise the
string is modified in-place. You must call [sfree](/string/sfree.md) if a new dynamic string is returned, when done.

## Remarks

This will insert ellipses "..." into the path as necessary. This function is useful for displaying paths
and visualizing them in small boxes or windows. n includes the nul-byte. Returns a new string.

## Related Pages

[spfname](/path/spfname.md)  
[spfname_no_ext](/path/spfname_no_ext.md)  
[spext](/path/spext.md)  
[spext_equ](/path/spext_equ.md)  
[sppop](/path/sppop.md)  
[sppopn](/path/sppopn.md)  
[spnorm](/path/spnorm.md)  
[spdir_of](/path/spdir_of.md)  
[sptop_of](/path/sptop_of.md)  
