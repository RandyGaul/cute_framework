# cf_fs_enumerate_directory | [file](https://github.com/RandyGaul/cute_framework/blob/master/docs/file/README.md) | [cute_file_system.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_file_system.h)

Returns a sorted list of files and directories in the given directory.

```cpp
const char** cf_fs_enumerate_directory(const char* virtual_path);
```

Parameters | Description
--- | ---
virtual_path | The virtual path to the directory.

## Return Value

Returns any errors as a [CF_Result](https://github.com/RandyGaul/cute_framework/blob/master/docs/utility/cf_result.md).

## Code Example

> Loop over a list of all files in a directory.

```cpp
const char list = cf_fs_enumerate_directory("/data");
for (const char i = list; i; ++i) {
    printf("Found %s\n", i);
}
cf_fs_free_enumerated_directory(list);
```

## Remarks

Results are collected by visiting the search path for all real directories mounted on `virtual_path`. No duplicate file
names will be reported. The list itself is sorted alphabetically, though you can further sort it however you like. Free
the list up with [cf_fs_free_enumerated_directory](https://github.com/RandyGaul/cute_framework/blob/master/docs/file/cf_fs_free_enumerated_directory.md) when done. The final element of the list is NULL. TODO_LINK_VFS_README.

## Related Pages

[cf_fs_remove_directory](https://github.com/RandyGaul/cute_framework/blob/master/docs/file/cf_fs_remove_directory.md)  
[cf_fs_create_directory](https://github.com/RandyGaul/cute_framework/blob/master/docs/file/cf_fs_create_directory.md)  
[cf_fs_free_enumerated_directory](https://github.com/RandyGaul/cute_framework/blob/master/docs/file/cf_fs_free_enumerated_directory.md)  
