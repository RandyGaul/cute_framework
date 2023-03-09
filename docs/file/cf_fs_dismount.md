# cf_fs_dismount

Category: [file](https://github.com/RandyGaul/cute_framework/blob/master/docs/api_reference?id=file)  
GitHub: [cute_file_system.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_file_system.h)  
---

Removes an archive from the path specified in platform-dependent notation.

```cpp
CF_Result cf_fs_dismount(const char* archive);
```

## Return Value

Returns any errors as a [CF_Result](https://github.com/RandyGaul/cute_framework/blob/master/docs/utility/cf_result.md).

## Remarks

This function does not remove a `mount_point` from the virtual file system, but only the actual archive that was previously mounted. TODO_LINK_VFS_README.

## Related Pages

[cf_fs_get_base_directory](https://github.com/RandyGaul/cute_framework/blob/master/docs/file/cf_fs_get_base_directory.md)  
[cf_fs_set_write_directory](https://github.com/RandyGaul/cute_framework/blob/master/docs/file/cf_fs_set_write_directory.md)  
[cf_fs_get_user_directory](https://github.com/RandyGaul/cute_framework/blob/master/docs/file/cf_fs_get_user_directory.md)  
[cf_fs_mount](https://github.com/RandyGaul/cute_framework/blob/master/docs/file/cf_fs_mount.md)  
