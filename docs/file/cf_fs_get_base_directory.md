# cf_fs_get_base_directory | [file](https://github.com/RandyGaul/cute_framework/blob/master/docs/file/README.md) | [cute_file_system.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_file_system.h)

Returns the path of the base directory.

```cpp
const char* cf_fs_get_base_directory();
```

## Remarks

This is not a virtual path, but the actual OS-path where the executable was run from. This might not be the working directory,
but probably is. You should probably mount the base directory with [cf_fs_mount](https://github.com/RandyGaul/cute_framework/blob/master/docs/file/cf_fs_mount.md). See TODO_LINK_VFS_README for an overview.

## Related Pages

[cf_fs_dismount](https://github.com/RandyGaul/cute_framework/blob/master/docs/file/cf_fs_dismount.md)  
[cf_fs_set_write_directory](https://github.com/RandyGaul/cute_framework/blob/master/docs/file/cf_fs_set_write_directory.md)  
[cf_fs_get_user_directory](https://github.com/RandyGaul/cute_framework/blob/master/docs/file/cf_fs_get_user_directory.md)  
[cf_fs_mount](https://github.com/RandyGaul/cute_framework/blob/master/docs/file/cf_fs_mount.md)  
