
# strpool_incref

Increments a counter associated with one string and returns the new value.

## Syntax

```cpp
int strpool_incref(strpool_t* pool, strpool_id id);
```

## Function Parameters

Parameter Name | Description
--- | ---
pool | The string pool.
id | The string to increment the reference count of.

## Remarks

This function is for implementing reference counting. The reference counting must be implemented yourself by calling [strpool_incref](https://github.com/RandyGaul/cute_framework/blob/master/doc/string/strpool/strpool_incref.md) and [strpool_decref](https://github.com/RandyGaul/cute_framework/blob/master/doc/string/strpool/strpool_decref.md) yourself. These functions can be useful to implement a C++ wrapping string class that performs automated reference counting. For example, this is what Cute's [string](https://github.com/RandyGaul/cute_framework/blob/master/doc/string/string) class does.

## Related Functions
  
[strpool_decref](https://github.com/RandyGaul/cute_framework/blob/master/doc/string/strpool/strpool_decref.md)  
[strpool_getref](https://github.com/RandyGaul/cute_framework/blob/master/doc/string/strpool/strpool_getref.md)  
