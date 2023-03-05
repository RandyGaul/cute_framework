# cf_fs_create_directory | [file](https://github.com/RandyGaul/cute_framework/blob/master/docs/file/README.md) | [cute_file_system.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_file_system.h)

Creates a directory at the path.

```cpp
CF_Result cf_fs_create_directory(const char* virtual_path);
```

Parameters | Description
--- | ---
virtual_path | The virtual path to the directory.

## Return Value

Returns any errors as a [CF_Result](https://github.com/RandyGaul/cute_framework/blob/master/docs/utility/cf_result.md).

## Remarks

All missing directories are also created. TODO_LINK_VFS_README.

## Related Pages

[cf_fs_remove_directory](https://github.com/RandyGaul/cute_framework/blob/master/docs/file/cf_fs_remove_directory.md)  
[cf_fs_free_enumerated_directory](https://github.com/RandyGaul/cute_framework/blob/master/docs/file/cf_fs_free_enumerated_directory.md)  
[cf_fs_enumerate_directory](https://github.com/RandyGaul/cute_framework/blob/master/docs/file/cf_fs_enumerate_directory.md)  
