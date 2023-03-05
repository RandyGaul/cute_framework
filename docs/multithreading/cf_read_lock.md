# cf_read_lock | [multithreading](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading_readme.md) | [cute_multithreading.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_multithreading.h)

Locks for reading. Many simultaneous readers are allowed.

```cpp
void cf_read_lock(CF_ReadWriteLock* rw);
```

Parameters | Description
--- | ---
rw | The read/write lock.

## Related Pages

[CF_ReadWriteLock](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading/cf_readwritelock.md)  
[cf_make_rw_lock](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading/cf_make_rw_lock.md)  
[cf_destroy_rw_lock](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading/cf_destroy_rw_lock.md)  
[cf_write_unlock](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading/cf_write_unlock.md)  
[cf_read_unlock](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading/cf_read_unlock.md)  
[cf_write_lock](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading/cf_write_lock.md)  
