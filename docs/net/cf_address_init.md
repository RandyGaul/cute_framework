[](../header.md ':include')

# cf_address_init

Category: [net](/api_reference?id=net)  
GitHub: [cute_networking.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_networking.h)  
---

Initialze a [CF_Address](/net/cf_address.md) from a C string.

```cpp
CF_API int CF_CALL cf_address_init(CF_Address* endpoint, const char* address_and_port_string);
```

## Return Value

Returns 0 on success, -1 on failure.

## Related Pages

[CF_Address](/net/cf_address.md)  
[cf_address_equals](/net/cf_address_equals.md)  
[cf_address_to_string](/net/cf_address_to_string.md)  
