# spfname | [path](https://github.com/RandyGaul/cute_framework/blob/master/docs/path_readme.md) | [cute_file_system.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_file_system.h)

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

Call [sfree](https://github.com/RandyGaul/cute_framework/blob/master/docs/string/sfree.md) on the return value when done. `sp` stands for "sting path".

## Related Pages

[spnorm](https://github.com/RandyGaul/cute_framework/blob/master/docs/path/spnorm.md)  
[spfname_no_ext](https://github.com/RandyGaul/cute_framework/blob/master/docs/path/spfname_no_ext.md)  
[spext](https://github.com/RandyGaul/cute_framework/blob/master/docs/path/spext.md)  
[spext_equ](https://github.com/RandyGaul/cute_framework/blob/master/docs/path/spext_equ.md)  
[sppop](https://github.com/RandyGaul/cute_framework/blob/master/docs/path/sppop.md)  
[sppopn](https://github.com/RandyGaul/cute_framework/blob/master/docs/path/sppopn.md)  
[spcompact](https://github.com/RandyGaul/cute_framework/blob/master/docs/path/spcompact.md)  
[spdir_of](https://github.com/RandyGaul/cute_framework/blob/master/docs/path/spdir_of.md)  
[sptop_of](https://github.com/RandyGaul/cute_framework/blob/master/docs/path/sptop_of.md)  
