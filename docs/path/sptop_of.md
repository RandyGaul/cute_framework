# sptop_of | [path](https://github.com/RandyGaul/cute_framework/blob/master/docs/path_readme.md) | [cute_file_system.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_file_system.h)

Returns the top-level directory of a given file or directory. Returns a new string.

```cpp
#define sptop_of(s) cf_path_top_directory(s)
```

Parameters | Description
--- | ---
s | The path string.

## Code Example

> Example fetching a the top-level directory a file sits within.

```cpp
const char filename = spfname("/data/collections/rare/big_gem.txt");
printf("%s\n", filename);
// Prints: /data
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
[spdir_of](https://github.com/RandyGaul/cute_framework/blob/master/docs/path/spdir_of.md)  
[spnorm](https://github.com/RandyGaul/cute_framework/blob/master/docs/path/spnorm.md)  
