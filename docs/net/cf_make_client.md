[](../header.md ':include')

# cf_make_client

Category: [net](https://github.com/RandyGaul/cute_framework/blob/master/docs/api_reference?id=net)  
GitHub: [cute_networking.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_networking.h)  
---

Returns a new client.

```cpp
CF_Client* cf_make_client(uint16_t port, uint64_t application_id, bool use_ipv6);
```

Parameters | Description
--- | ---
port | Port for opening a UDP socket.
application_id | A unique number to identify your game, can be whatever value you like. This must be the same number as in `cf_server_create`.
use_ipv6 | Whether or not the socket should turn on ipv6. Some users will not have ipv6 enabled, so consider setting to `false`.

## Related Pages

[CF_Client](https://github.com/RandyGaul/cute_framework/blob/master/docs/net/cf_client.md)  
[cf_generate_connect_token](https://github.com/RandyGaul/cute_framework/blob/master/docs/net/cf_generate_connect_token.md)  
[cf_destroy_client](https://github.com/RandyGaul/cute_framework/blob/master/docs/net/cf_destroy_client.md)  
[cf_client_connect](https://github.com/RandyGaul/cute_framework/blob/master/docs/net/cf_client_connect.md)  
