# spdir_of

Category: [path](https://github.com/RandyGaul/cute_framework/blob/master/docs/api_reference?id=path)  
GitHub: [cute_file_system.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_file_system.h)  
---

Returns the directory of a given file or directory. Returns a new string.

```cpp
#define spdir_of(s) cf_path_directory_of(s)
```

Parameters | Description
--- | ---
s | The path string.

## Code Example

> Example fetching a directory a file sits within.

```cpp
const char filename = spfname("/data/collections/rare/big_gem.txt");
printf("%s\n", filename);
// Prints: /rare
```

## Remarks

`sp` stands for "sting path". Call [sfree](https://github.com/RandyGaul/cute_framework/blob/master/docs/string/sfree.md) on the return value when done.

## Related Pages

[spfname](https://github.com/RandyGaul/cute_framework/blob/master/docs/path/spfname.md)  
[spfname_no_ext](https://github.com/RandyGaul/cute_framework/blob/master/docs/path/spfname_no_ext.md)  
[spext](https://github.com/RandyGaul/cute_framework/blob/master/docs/path/spext.md)  
[spext_equ](https://github.com/RandyGaul/cute_framework/blob/master/docs/path/spext_equ.md)  
[sppop](https://github.com/RandyGaul/cute_framework/blob/master/docs/path/sppop.md)  
[sppopn](https://github.com/RandyGaul/cute_framework/blob/master/docs/path/sppopn.md)  
[spcompact](https://github.com/RandyGaul/cute_framework/blob/master/docs/path/spcompact.md)  
[spnorm](https://github.com/RandyGaul/cute_framework/blob/master/docs/path/spnorm.md)  
[sptop_of](https://github.com/RandyGaul/cute_framework/blob/master/docs/path/sptop_of.md)  
