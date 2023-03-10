[](../header.md ':include')

# cf_client_connect

Category: [net](/api_reference?id=net)  
GitHub: [cute_networking.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_networking.h)  
---

Attempts to connect the [CF_Client](/net/cf_client.md) to a [CF_Server](/net/cf_server.md).

```cpp
CF_API CF_Result CF_CALL cf_client_connect(CF_Client* client, const uint8_t* connect_token);
```

## Return Value

Returns any errors as a [CF_Result](/utility/cf_result.md).

## Remarks

The client will make an attempt to connect to all servers listed in the connect token, one after
another. If no server can be connected to the client's state will be set to an error state. Call
[cf_client_state_get](/net/cf_client_state_get.md) to get the client's state. Once [cf_client_connect](/net/cf_client_connect.md) is called then successive calls to
[cf_client_update](/net/cf_client_update.md) is expected, where [cf_client_update](/net/cf_client_update.md) will perform the connection handshake and make
connection attempts to your servers.

## Related Pages

[CF_Client](/net/cf_client.md)  
[cf_make_client](/net/cf_make_client.md)  
[cf_destroy_client](/net/cf_destroy_client.md)  
[cf_client_update](/net/cf_client_update.md)  
[cf_client_disconnect](/net/cf_client_disconnect.md)  
