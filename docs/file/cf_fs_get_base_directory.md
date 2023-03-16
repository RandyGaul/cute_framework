[](../header.md ':include')

# cf_fs_get_base_directory

Category: [file](/api_reference?id=file)  
GitHub: [cute_file_system.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_file_system.h)  
---

Returns the path of the base directory.

```cpp
const char* cf_fs_get_base_directory();
```

## Remarks

This is not a virtual path, but the actual OS-path where the executable was run from. This might not be the working directory,
but probably is. You should probably mount the base directory with [cf_fs_mount](/file/cf_fs_mount.md). See [Virtual File System](https://randygaul.github.io/cute_framework/#/topics/virtual_file_system) for an overview.

## Related Pages

[cf_fs_dismount](/file/cf_fs_dismount.md)  
[cf_fs_set_write_directory](/file/cf_fs_set_write_directory.md)  
[cf_fs_get_user_directory](/file/cf_fs_get_user_directory.md)  
[cf_fs_mount](/file/cf_fs_mount.md)  
