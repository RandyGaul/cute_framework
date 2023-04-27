[](../header.md ':include')

# cf_fs_enumerate_directory

Category: [file](/api_reference?id=file)  
GitHub: [cute_file_system.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_file_system.h)  
---

Returns a sorted list of files and directories in the given directory.

```cpp
const char** cf_fs_enumerate_directory(const char* virtual_path);
```

Parameters | Description
--- | ---
virtual_path | The virtual path to the directory.

## Return Value

Returns any errors as a [CF_Result](/utility/cf_result.md).

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
the list up with [cf_fs_free_enumerated_directory](/file/cf_fs_free_enumerated_directory.md) when done. The final element of the list is NULL. [Virtual File System](https://randygaul.github.io/cute_framework/#/topics/virtual_file_system).

## Related Pages

[cf_fs_remove_directory](/file/cf_fs_remove_directory.md)  
[cf_fs_create_directory](/file/cf_fs_create_directory.md)  
[cf_fs_free_enumerated_directory](/file/cf_fs_free_enumerated_directory.md)  
