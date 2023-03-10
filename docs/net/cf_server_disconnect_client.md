[](../header.md ':include')

# cf_server_disconnect_client

Category: [net](/api_reference?id=net)  
GitHub: [cute_networking.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_networking.h)  
---

Disconnects a client from the server.

```cpp
CF_API void CF_CALL cf_server_disconnect_client(CF_Server* server, int client_index, bool notify_client /* = true */);
```

## Related Pages

[cf_server_update](/net/cf_server_update.md)  
[CF_ServerEvent](/net/cf_serverevent.md)  
[cf_server_pop_event](/net/cf_server_pop_event.md)  
[cf_server_send](/net/cf_server_send.md)  
