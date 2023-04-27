
# strpool_getref

Returns the a counter associated with one string.

## Syntax

```cpp
int strpool_getref(strpool_t* pool, strpool_id id);
```

## Function Parameters

Parameter Name | Description
--- | ---
pool | The string pool.
id | The string to lookup the reference count of.

## Remarks

This function is for implementing reference counting. The reference counting must be implemented yourself by calling [strpool_incref](https://github.com/RandyGaul/cute_framework/blob/master/docs/string/strpool/strpool_incref.md) and [strpool_decref](https://github.com/RandyGaul/cute_framework/blob/master/docs/string/strpool/strpool_decref.md) yourself. These functions can be useful to implement a C++ wrapping string class that performs automated reference counting. For example, this is what Cute's [string](https://github.com/RandyGaul/cute_framework/blob/master/docs/string/string) class does.

## Related Functions
  
[strpool_incref](https://github.com/RandyGaul/cute_framework/blob/master/docs/string/strpool/strpool_incref.md)  
[strpool_decref](https://github.com/RandyGaul/cute_framework/blob/master/docs/string/strpool/strpool_decref.md)  
