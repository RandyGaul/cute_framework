# cf_server_free_packet | [net](https://github.com/RandyGaul/cute_framework/blob/master/docs/net_readme.md) | [cute_networking.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_networking.h)

Frees a payload packet from a [CF_ServerEvent](https://github.com/RandyGaul/cute_framework/blob/master/docs/net/cf_serverevent.md).

```cpp
void cf_server_free_packet(CF_Server* server, int client_index, void* data);
```

## Related Pages

[CF_ServerEventType](https://github.com/RandyGaul/cute_framework/blob/master/docs/net/cf_servereventtype.md)  
[cf_server_event_type_to_string](https://github.com/RandyGaul/cute_framework/blob/master/docs/net/cf_server_event_type_to_string.md)  
[CF_ServerEvent](https://github.com/RandyGaul/cute_framework/blob/master/docs/net/cf_serverevent.md)  
[cf_server_pop_event](https://github.com/RandyGaul/cute_framework/blob/master/docs/net/cf_server_pop_event.md)  
