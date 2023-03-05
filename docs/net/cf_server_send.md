# cf_server_send | [net](https://github.com/RandyGaul/cute_framework/blob/master/docs/net_readme.md) | [cute_networking.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_networking.h)

Sends a packet to a client.

```cpp
void cf_server_send(CF_Server* server, const void* packet, int size, int client_index, bool send_reliably);
```

Parameters | Description
--- | ---
server | The server.
packet | Data to send.
size | Size of `data` in bytes.
client_index | An index representing a particular client, from [CF_ServerEvent](https://github.com/RandyGaul/cute_framework/blob/master/docs/net/cf_serverevent.md).
send_reliably | If `true` the packet will be sent reliably and in order. If false the packet will be sent just once, and may
               arrive out of order or not at all.

## Related Pages

[cf_server_update](https://github.com/RandyGaul/cute_framework/blob/master/docs/net/cf_server_update.md)  
[CF_ServerEvent](https://github.com/RandyGaul/cute_framework/blob/master/docs/net/cf_serverevent.md)  
[cf_server_pop_event](https://github.com/RandyGaul/cute_framework/blob/master/docs/net/cf_server_pop_event.md)  
