# cf_fs_mount

Category: [file](https://github.com/RandyGaul/cute_framework/blob/master/docs/api_reference?id=file)  
GitHub: [cute_file_system.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_file_system.h)  
---

Adds a new archive/directory onto the search path.

```cpp
CF_Result cf_fs_mount(const char* archive, const char* mount_point, bool append_to_path);
```

Parameters | Description
--- | ---
archive | Platform-dependent notation. The archive or directory to mount.
mount_point | The new virtual path for `archive`.
append_to_path | If true `mount_point` is appended onto the end of the path. If false it will be prepended.

## Return Value

Returns any errors as a [CF_Result](https://github.com/RandyGaul/cute_framework/blob/master/docs/utility/cf_result.md).

## Remarks

Each individual archive can only be mounted once. Duplicate mount attempts will be ignored.

You can mount multiple archives onto a single mount point. This is a great way to support
modding or download patches, as duplicate entries will be searched for on the path as normal,
without the need to overwrite each other on the actual disk.

You can mount an actual directory or an archive file. If it's an archive the vitrual file
system will treat it like a normal directory for you. There are a variety of archive file
formats supported (see top of file).

By default CF mounts the base directory when you call [cf_make_app](https://github.com/RandyGaul/cute_framework/blob/master/docs/app/cf_make_app.md). This can be disabled by
passing the `CF_APP_OPTIONS_FILE_SYSTEM_DONT_DEFAULT_MOUNT` flag to [cf_make_app](https://github.com/RandyGaul/cute_framework/blob/master/docs/app/cf_make_app.md). TODO_LINK_VFS_README.

## Related Pages

[cf_fs_get_base_directory](https://github.com/RandyGaul/cute_framework/blob/master/docs/file/cf_fs_get_base_directory.md)  
[cf_fs_set_write_directory](https://github.com/RandyGaul/cute_framework/blob/master/docs/file/cf_fs_set_write_directory.md)  
[cf_fs_get_user_directory](https://github.com/RandyGaul/cute_framework/blob/master/docs/file/cf_fs_get_user_directory.md)  
[cf_fs_dismount](https://github.com/RandyGaul/cute_framework/blob/master/docs/file/cf_fs_dismount.md)  
