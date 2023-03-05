# CF_Stat | [file](https://github.com/RandyGaul/cute_framework/blob/master/docs/file_readme.md) | [cute_file_system.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_file_system.h)

A structure containing information about a file.

Struct Members | Description
--- | ---
`CF_FileType type` | The type of the file.
`bool is_read_only` | True if the file is read-only, false otherwise.
`size_t size` | The size of the file in bytes.
`uint64_t last_modified_time` | The last time the file was written to.
`uint64_t created_time` | The time of file creation.
`uint64_t last_accessed_time` | The last time this file was accessed (not necessarily modified).

## Related Pages

[CF_File](https://github.com/RandyGaul/cute_framework/blob/master/docs/file/cf_file.md)  
[cf_fs_close](https://github.com/RandyGaul/cute_framework/blob/master/docs/file/cf_fs_close.md)  
[cf_fs_create_file](https://github.com/RandyGaul/cute_framework/blob/master/docs/file/cf_fs_create_file.md)  
[cf_fs_open_file_for_write](https://github.com/RandyGaul/cute_framework/blob/master/docs/file/cf_fs_open_file_for_write.md)  
[cf_fs_open_file_for_read](https://github.com/RandyGaul/cute_framework/blob/master/docs/file/cf_fs_open_file_for_read.md)  
