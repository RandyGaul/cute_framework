[](../header.md ':include')

# cf_client_free_packet

Category: [net](/api_reference?id=net)  
GitHub: [cute_networking.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_networking.h)  
---

Free's a packet created by [cf_client_pop_packet](/net/cf_client_pop_packet.md).

```cpp
CF_API void CF_CALL cf_client_free_packet(CF_Client* client, void* packet);
```

## Related Pages

[CF_Client](/net/cf_client.md)  
[cf_client_pop_packet](/net/cf_client_pop_packet.md)  
[cf_client_send](/net/cf_client_send.md)  
