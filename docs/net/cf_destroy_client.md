[](../header.md ':include')

# cf_destroy_client

Category: [net](/api_reference?id=net)  
GitHub: [cute_networking.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_networking.h)  
---

Destroys a client created by [cf_make_client](/net/cf_make_client.md).

```cpp
CF_API void CF_CALL cf_destroy_client(CF_Client* client);
```

## Remarks

Does not send out any disconnect packets. Call [cf_client_disconnect](/net/cf_client_disconnect.md) first.

## Related Pages

[CF_Client](/net/cf_client.md)  
[cf_make_client](/net/cf_make_client.md)  
[cf_client_disconnect](/net/cf_client_disconnect.md)  
[cf_client_connect](/net/cf_client_connect.md)  
