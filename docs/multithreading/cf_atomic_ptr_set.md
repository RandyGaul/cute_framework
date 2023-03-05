# cf_atomic_ptr_set | [multithreading](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading_readme.md) | [cute_multithreading.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_multithreading.h)

Atomically sets `atomic` to `value` and returns the old value from `atomic`.

```cpp
void* cf_atomic_ptr_set(void** atomic, void* value);
```

Parameters | Description
--- | ---
atomic | The pointer to atomically manipulate.
value | A value to atomically set to `atomic`.

## Remarks

Atomics are an advanced topic. You've been warned! Beej has a [good article on atomics](https://beej.us/guide/bgc/html/split/chapter-atomics.html).

## Related Pages

[cf_atomic_zero](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading/cf_atomic_zero.md)  
[cf_atomic_add](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading/cf_atomic_add.md)  
[cf_atomic_set](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading/cf_atomic_set.md)  
[cf_atomic_get](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading/cf_atomic_get.md)  
[cf_atomic_cas](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading/cf_atomic_cas.md)  
[cf_atomic_ptr_cas](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading/cf_atomic_ptr_cas.md)  
[cf_atomic_ptr_get](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading/cf_atomic_ptr_get.md)  
