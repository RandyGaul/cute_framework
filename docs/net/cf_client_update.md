# cf_client_update

Category: [net](https://github.com/RandyGaul/cute_framework/blob/master/docs/api_reference?id=net)  
GitHub: [cute_networking.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_networking.h)  
---

Updates the client.

```cpp
void cf_client_update(CF_Client* client, double dt, uint64_t current_time);
```

## Remarks

You should call this one per game loop after calling [cf_client_connect](https://github.com/RandyGaul/cute_framework/blob/master/docs/net/cf_client_connect.md).

## Related Pages

[CF_Client](https://github.com/RandyGaul/cute_framework/blob/master/docs/net/cf_client.md)  
[cf_make_client](https://github.com/RandyGaul/cute_framework/blob/master/docs/net/cf_make_client.md)  
[cf_destroy_client](https://github.com/RandyGaul/cute_framework/blob/master/docs/net/cf_destroy_client.md)  
[cf_client_connect](https://github.com/RandyGaul/cute_framework/blob/master/docs/net/cf_client_connect.md)  
[cf_client_disconnect](https://github.com/RandyGaul/cute_framework/blob/master/docs/net/cf_client_disconnect.md)  
