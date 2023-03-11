[](../header.md ':include')

# cf_fs_set_write_directory

Category: [file](/api_reference?id=file)  
GitHub: [cute_file_system.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_file_system.h)  
---

Sets a path safe to store game-specific files, such as save data or profiles.

```cpp
CF_API CF_Result CF_CALL cf_fs_set_write_directory(const char* platform_dependent_directory);
```

Parameters | Description
--- | ---
platform_dependent_directory | The write directory in platform-dependent notation (use [cf_fs_get_user_directory](/file/cf_fs_get_user_directory.md), see remarks for
  more details).

## Return Value

Returns any errors as a [CF_Result](/utility/cf_result.md).

## Remarks

The path is in platform-dependent notation. It's highly recommend to use [cf_fs_get_user_directory](/file/cf_fs_get_user_directory.md) and pass it into this function
when shipping your game. This function will fail if any files are from the write directory are currently open.
See [Virtual File System](https://randygaul.github.io/cute_framework/#/topics/virtual_file_system) for an overview.

## Related Pages

[cf_fs_get_base_directory](/file/cf_fs_get_base_directory.md)  
[cf_fs_dismount](/file/cf_fs_dismount.md)  
[cf_fs_get_user_directory](/file/cf_fs_get_user_directory.md)  
[cf_fs_mount](/file/cf_fs_mount.md)  
