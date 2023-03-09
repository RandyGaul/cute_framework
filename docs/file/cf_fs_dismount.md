[](../header.md ':include')

# cf_fs_dismount

Category: [file](/api_reference?id=file)  
GitHub: [cute_file_system.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_file_system.h)  
---

Removes an archive from the path specified in platform-dependent notation.

```cpp
CF_Result cf_fs_dismount(const char* archive);
```

## Return Value

Returns any errors as a [CF_Result](/utility/cf_result.md).

## Remarks

This function does not remove a `mount_point` from the virtual file system, but only the actual archive that was previously mounted. TODO_LINK_VFS_README.

## Related Pages

[cf_fs_get_base_directory](/file/cf_fs_get_base_directory.md)  
[cf_fs_set_write_directory](/file/cf_fs_set_write_directory.md)  
[cf_fs_get_user_directory](/file/cf_fs_get_user_directory.md)  
[cf_fs_mount](/file/cf_fs_mount.md)  
