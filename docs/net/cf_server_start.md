# cf_server_start

Category: [net](https://github.com/RandyGaul/cute_framework/blob/master/docs/api_reference?id=net)  
GitHub: [cute_networking.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_networking.h)  
---

Starts up the server connection, ready to receive new client connections.

```cpp
CF_Result cf_server_start(CF_Server* server, const char* address_and_port);
```

Parameters | Description
--- | ---
address_and_port | The address and port combo to start the server upon.

## Remarks

Please note that not all users will be able to access an ipv6 server address, so it might be good to also provide a way to connect through ipv4.

## Related Pages

[CF_ServerConfig](https://github.com/RandyGaul/cute_framework/blob/master/docs/net/cf_serverconfig.md)  
[cf_server_config_defaults](https://github.com/RandyGaul/cute_framework/blob/master/docs/net/cf_server_config_defaults.md)  
[cf_make_server](https://github.com/RandyGaul/cute_framework/blob/master/docs/net/cf_make_server.md)  
[cf_destroy_server](https://github.com/RandyGaul/cute_framework/blob/master/docs/net/cf_destroy_server.md)  
[cf_server_update](https://github.com/RandyGaul/cute_framework/blob/master/docs/net/cf_server_update.md)  
