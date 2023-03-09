# cf_server_pop_event

Category: [net](https://github.com/RandyGaul/cute_framework/blob/master/docs/api_reference?id=net)  
GitHub: [cute_networking.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_networking.h)  
---

Pops a [CF_ServerEvent](https://github.com/RandyGaul/cute_framework/blob/master/docs/net/cf_serverevent.md) off of the server, if available.

```cpp
bool cf_server_pop_event(CF_Server* server, CF_ServerEvent* event);
```

## Return Value

Returns true if an event was popped.

## Remarks

Server events notify of when a client connects/disconnects, or has sent a payload packet.
You must free the payload packets with [cf_server_free_packet](https://github.com/RandyGaul/cute_framework/blob/master/docs/net/cf_server_free_packet.md) when done.

## Related Pages

[CF_ServerEventType](https://github.com/RandyGaul/cute_framework/blob/master/docs/net/cf_servereventtype.md)  
[cf_server_event_type_to_string](https://github.com/RandyGaul/cute_framework/blob/master/docs/net/cf_server_event_type_to_string.md)  
[CF_ServerEvent](https://github.com/RandyGaul/cute_framework/blob/master/docs/net/cf_serverevent.md)  
[cf_server_send](https://github.com/RandyGaul/cute_framework/blob/master/docs/net/cf_server_send.md)  
[cf_server_update](https://github.com/RandyGaul/cute_framework/blob/master/docs/net/cf_server_update.md)  
