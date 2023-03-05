# cf_fs_get_actual_path | [file](https://github.com/RandyGaul/cute_framework/blob/master/docs/file_readme.md) | [cute_file_system.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_file_system.h)

Converts a virtual path to an actual path.

```cpp
const char* cf_fs_get_actual_path(const char* virtual_path);
```

Parameters | Description
--- | ---
virtual_path | A path to the file.

## Remarks

This can be useful for editors, asset hotloading, or other similar development features. When shipping your game it's highly
recommended to not call this function at all, and only use it for development purposes. If the virtual path points to a completely
fake directory this will return the first archive found there. This function can return a directory, an archive, a file, or `NULL`
if nothing suitable was found at all. TODO_LINK_VFS_README.

## Related Pages

[cf_fs_read_entire_file_to_memory](https://github.com/RandyGaul/cute_framework/blob/master/docs/file/cf_fs_read_entire_file_to_memory.md)  
[cf_fs_read_entire_file_to_memory_and_nul_terminate](https://github.com/RandyGaul/cute_framework/blob/master/docs/file/cf_fs_read_entire_file_to_memory_and_nul_terminate.md)  
[cf_fs_write_entire_buffer_to_file](https://github.com/RandyGaul/cute_framework/blob/master/docs/file/cf_fs_write_entire_buffer_to_file.md)  
