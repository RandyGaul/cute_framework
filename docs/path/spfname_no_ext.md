[](../header.md ':include')

# spfname_no_ext

Category: [path](/api_reference?id=path)  
GitHub: [cute_file_system.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_file_system.h)  
---

Returns the filename portion of a path without the file extension. Returns a new string.

```cpp
#define spfname_no_ext(s) cf_path_get_filename_no_ext(s)
```

Parameters | Description
--- | ---
s | The path string.

## Code Example

> Example fetching a filename from a path without the extension attached.

```cpp
const char filename = spfname("/data/collections/rare/big_gem.txt");
printf("%s\n", filename);
// Prints: big_gem
```

## Remarks

Call [sfree](/string/sfree.md) on the return value when done. `sp` stands for "sting path".

## Related Pages

[spfname](/path/spfname.md)  
[spnorm](/path/spnorm.md)  
[spext](/path/spext.md)  
[spext_equ](/path/spext_equ.md)  
[sppop](/path/sppop.md)  
[sppopn](/path/sppopn.md)  
[spcompact](/path/spcompact.md)  
[spdir_of](/path/spdir_of.md)  
[sptop_of](/path/sptop_of.md)  
