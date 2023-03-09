[](../header.md ':include')

# cf_server_update

Category: [net](https://github.com/RandyGaul/cute_framework/blob/master/docs/api_reference?id=net)  
GitHub: [cute_networking.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_networking.h)  
---

Updates the server.

```cpp
void cf_server_update(CF_Server* server, double dt, uint64_t current_time);
```

## Remarks

Call this once per game tick.

## Related Pages

[cf_server_pop_event](https://github.com/RandyGaul/cute_framework/blob/master/docs/net/cf_server_pop_event.md)  
[CF_ServerEvent](https://github.com/RandyGaul/cute_framework/blob/master/docs/net/cf_serverevent.md)  
