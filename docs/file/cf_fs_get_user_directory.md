[](../header.md ':include')

# cf_fs_get_user_directory

Category: [file](/api_reference?id=file)  
GitHub: [cute_file_system.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_file_system.h)  
---

Returns a path safe to store game-specific files, such as save data or profiles.

```cpp
const char* cf_fs_get_user_directory(const char* company_name, const char* game_name);
```

Parameters | Description
--- | ---
company_name | The name of your org or company.
game_name | The name of your game.

## Remarks

The path is in platform-dependent notation. The location of this folder varies depending on the OS. You
should probably pass this into [cf_fs_set_write_directory](/file/cf_fs_set_write_directory.md) as well as [cf_fs_mount](/file/cf_fs_mount.md). Windows example:
```
"C:\\Users\\OS_user_name\\AppData\\Roaming\\my_company\\my_game"
```
Linux example:
```
"/home/OS_user_name/.local/share/my_game"
```
MacOS X example:
```
"/Users/OS_user_name/Library/Application Support/my_game"
```
You should assume this directory is the only safe place to write files. [Virtual File System](https://randygaul.github.io/cute_framework/#/topics/virtual_file_system).

## Related Pages

[cf_fs_get_base_directory](/file/cf_fs_get_base_directory.md)  
[cf_fs_set_write_directory](/file/cf_fs_set_write_directory.md)  
[cf_fs_dismount](/file/cf_fs_dismount.md)  
[cf_fs_mount](/file/cf_fs_mount.md)  
