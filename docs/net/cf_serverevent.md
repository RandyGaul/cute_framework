[](../header.md ':include')

# CF_ServerEvent

Category: [net](https://github.com/RandyGaul/cute_framework/blob/master/docs/api_reference?id=net)  
GitHub: [cute_networking.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_networking.h)  
---

An event from the server, likely a client payload packet.

Struct Members | Description
--- | ---
`CF_ServerEventType type` | The type of the server event. See [CF_ServerEventType](https://github.com/RandyGaul/cute_framework/blob/master/docs/net/cf_servereventtype.md).
`int client_index` | An index representing this particular client.
`uint64_t client_id` | A unique identifier for this particular client, as read from the connect token.
`CF_Address endpoint` | The address and port of the incoming connection.
`int client_index` | An index representing this particular client.
`int client_index` | An index representing this particular client.
`void* data` | Pointer to the packet's payload data. Send this back to cf_`server_free_packet` when done.
`int size` | Size of the packet at the data pointer.

## Related Pages

[cf_server_free_packet](https://github.com/RandyGaul/cute_framework/blob/master/docs/net/cf_server_free_packet.md)  
[cf_server_pop_event](https://github.com/RandyGaul/cute_framework/blob/master/docs/net/cf_server_pop_event.md)  
[cf_server_update](https://github.com/RandyGaul/cute_framework/blob/master/docs/net/cf_server_update.md)  
