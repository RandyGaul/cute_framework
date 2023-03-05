# cf_client_connect | [net](https://github.com/RandyGaul/cute_framework/blob/master/docs/net_readme.md) | [cute_networking.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_networking.h)

Attempts to connect the [CF_Client](https://github.com/RandyGaul/cute_framework/blob/master/docs/net/cf_client.md) to a [CF_Server](https://github.com/RandyGaul/cute_framework/blob/master/docs/net/cf_server.md).

```cpp
CF_Result cf_client_connect(CF_Client* client, const uint8_t* connect_token);
```

## Return Value

Returns any errors as a [CF_Result](https://github.com/RandyGaul/cute_framework/blob/master/docs/utility/cf_result.md).

## Remarks

The client will make an attempt to connect to all servers listed in the connect token, one after
another. If no server can be connected to the client's state will be set to an error state. Call
[cf_client_state_get](https://github.com/RandyGaul/cute_framework/blob/master/docs/net/cf_client_state_get.md) to get the client's state. Once [cf_client_connect](https://github.com/RandyGaul/cute_framework/blob/master/docs/net/cf_client_connect.md) is called then successive calls to
[cf_client_update](https://github.com/RandyGaul/cute_framework/blob/master/docs/net/cf_client_update.md) is expected, where [cf_client_update](https://github.com/RandyGaul/cute_framework/blob/master/docs/net/cf_client_update.md) will perform the connection handshake and make
connection attempts to your servers.

## Related Pages

[CF_Client](https://github.com/RandyGaul/cute_framework/blob/master/docs/net/cf_client.md)  
[cf_make_client](https://github.com/RandyGaul/cute_framework/blob/master/docs/net/cf_make_client.md)  
[cf_destroy_client](https://github.com/RandyGaul/cute_framework/blob/master/docs/net/cf_destroy_client.md)  
[cf_client_update](https://github.com/RandyGaul/cute_framework/blob/master/docs/net/cf_client_update.md)  
[cf_client_disconnect](https://github.com/RandyGaul/cute_framework/blob/master/docs/net/cf_client_disconnect.md)  
