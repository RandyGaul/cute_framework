[](../header.md ':include')

# cf_make_rw_lock

Category: [multithreading](https://github.com/RandyGaul/cute_framework/blob/master/docs/api_reference?id=multithreading)  
GitHub: [cute_multithreading.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_multithreading.h)  
---

Returns an unlocked [CF_ReadWriteLock](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading/cf_readwritelock.md) lock.

```cpp
CF_ReadWriteLock cf_make_rw_lock();
```

## Remarks

Call [cf_destroy_rw_lock](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading/cf_destroy_rw_lock.md) when done.

## Related Pages

[CF_ReadWriteLock](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading/cf_readwritelock.md)  
[cf_write_unlock](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading/cf_write_unlock.md)  
[cf_destroy_rw_lock](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading/cf_destroy_rw_lock.md)  
[cf_read_lock](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading/cf_read_lock.md)  
[cf_read_unlock](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading/cf_read_unlock.md)  
[cf_write_lock](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading/cf_write_lock.md)  
