[](../header.md ':include')

# spext

Category: [path](https://github.com/RandyGaul/cute_framework/blob/master/docs/api_reference?id=path)  
GitHub: [cute_file_system.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_file_system.h)  
---

Returns the extension of the file for the given path. Returns a new string.

```cpp
#define spext(s) cf_path_get_ext(s)
```

Parameters | Description
--- | ---
s | The path string.

## Code Example

> Example fetching a filename from a path without the extension attached.

```cpp
const char ext = spfname("/data/collections/rare/big_gem.txt");
printf("%s\n", ext);
// Prints: .txt
```

## Remarks

Call [sfree](https://github.com/RandyGaul/cute_framework/blob/master/docs/string/sfree.md) on the return value when done. `sp` stands for "sting path".

## Related Pages

[spfname](https://github.com/RandyGaul/cute_framework/blob/master/docs/path/spfname.md)  
[spfname_no_ext](https://github.com/RandyGaul/cute_framework/blob/master/docs/path/spfname_no_ext.md)  
[spnorm](https://github.com/RandyGaul/cute_framework/blob/master/docs/path/spnorm.md)  
[spext_equ](https://github.com/RandyGaul/cute_framework/blob/master/docs/path/spext_equ.md)  
[sppop](https://github.com/RandyGaul/cute_framework/blob/master/docs/path/sppop.md)  
[sppopn](https://github.com/RandyGaul/cute_framework/blob/master/docs/path/sppopn.md)  
[spcompact](https://github.com/RandyGaul/cute_framework/blob/master/docs/path/spcompact.md)  
[spdir_of](https://github.com/RandyGaul/cute_framework/blob/master/docs/path/spdir_of.md)  
[sptop_of](https://github.com/RandyGaul/cute_framework/blob/master/docs/path/sptop_of.md)  
