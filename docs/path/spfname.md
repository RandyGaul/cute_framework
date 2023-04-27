[](../header.md ':include')

# spfname

Category: [path](/api_reference?id=path)  
GitHub: [cute_file_system.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_file_system.h)  
---

Returns the filename portion of a path. Returns a new string.

```cpp
#define spfname(s) cf_path_get_filename(s)
```

Parameters | Description
--- | ---
s | The path string.

## Code Example

> Example fetching a filename from a path.

```cpp
const char filename = spfname("/data/collections/rare/big_gem.txt");
printf("%s\n", filename);
// Prints: big_gem.txt
```

## Remarks

Call [sfree](/string/sfree.md) on the return value when done. `sp` stands for "sting path".

## Related Pages

[spnorm](/path/spnorm.md)  
[spfname_no_ext](/path/spfname_no_ext.md)  
[spext](/path/spext.md)  
[spext_equ](/path/spext_equ.md)  
[sppop](/path/sppop.md)  
[sppopn](/path/sppopn.md)  
[spcompact](/path/spcompact.md)  
[spdir_of](/path/spdir_of.md)  
[sptop_of](/path/sptop_of.md)  
