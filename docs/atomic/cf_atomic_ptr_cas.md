[](../header.md ':include')

# cf_atomic_ptr_cas

Category: [atomic](/api_reference?id=atomic)  
GitHub: [cute_multithreading.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_multithreading.h)  
---

Atomically sets `atomic` to `value` if `expected` equals `atomic`.

```cpp
CF_Result cf_atomic_ptr_cas(void** atomic, void* expected, void* value);
```

Parameters | Description
--- | ---
atomic | The pointer to atomically manipulate.
expected | Used to compare against `atomic`.
value | A value to atomically set to `atomic`.

## Return Value

Returns success if the value was set, error otherwise.

## Remarks

Atomics are an advanced topic. You've been warned! Beej has a [good article on atomics](https://beej.us/guide/bgc/html/split/chapter-atomics.html).

## Related Pages

[cf_atomic_zero](/atomic/cf_atomic_zero.md)  
[cf_atomic_add](/atomic/cf_atomic_add.md)  
[cf_atomic_set](/atomic/cf_atomic_set.md)  
[cf_atomic_get](/atomic/cf_atomic_get.md)  
[cf_atomic_cas](/atomic/cf_atomic_cas.md)  
[cf_atomic_ptr_set](/atomic/cf_atomic_ptr_set.md)  
[cf_atomic_ptr_get](/atomic/cf_atomic_ptr_get.md)  
