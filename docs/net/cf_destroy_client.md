# cf_destroy_client | [net](https://github.com/RandyGaul/cute_framework/blob/master/docs/net_readme.md) | [cute_networking.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_networking.h)

Destroys a client created by [cf_make_client](https://github.com/RandyGaul/cute_framework/blob/master/docs/net/cf_make_client.md).

```cpp
void cf_destroy_client(CF_Client* client);
```

## Remarks

Does not send out any disconnect packets. Call [cf_client_disconnect](https://github.com/RandyGaul/cute_framework/blob/master/docs/net/cf_client_disconnect.md) first.

## Related Pages

[CF_Client](https://github.com/RandyGaul/cute_framework/blob/master/docs/net/cf_client.md)  
[cf_make_client](https://github.com/RandyGaul/cute_framework/blob/master/docs/net/cf_make_client.md)  
[cf_client_disconnect](https://github.com/RandyGaul/cute_framework/blob/master/docs/net/cf_client_disconnect.md)  
[cf_client_connect](https://github.com/RandyGaul/cute_framework/blob/master/docs/net/cf_client_connect.md)  
