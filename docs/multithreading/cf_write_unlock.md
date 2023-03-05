# cf_write_unlock | [multithreading](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading/README.md) | [cute_multithreading.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_multithreading.h)

Unlocks for writing.

```cpp
void cf_write_unlock(CF_ReadWriteLock* rw);
```

Parameters | Description
--- | ---
rw | The read/write lock.

## Remarks

When locked for writing, only one writer can be present with no readers. The writer will sleep/wait for all other
readers and writers to unlock before acquiring exclusive access to the lock.

## Related Pages

[CF_ReadWriteLock](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading/cf_readwritelock.md)  
[cf_make_rw_lock](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading/cf_make_rw_lock.md)  
[cf_destroy_rw_lock](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading/cf_destroy_rw_lock.md)  
[cf_read_lock](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading/cf_read_lock.md)  
[cf_read_unlock](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading/cf_read_unlock.md)  
[cf_write_lock](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading/cf_write_lock.md)  
