# server_destroy

Destroys a server instance.

## Syntax

```cpp
void server_destroy(server_t* server);
```

## Function Parameters

Parameter Name | Description
--- | ---
server | The server to destroy.

## Remarks

Any connected clients will not be sent disconnect notifications. It is recommended to call [server_stop](https://github.com/RandyGaul/cute_framework/blob/master/docs/networking/server/server_stop.md) on all clients to disconnect gracefully before calling `server_destroy`.

## Related Functions

[server_create](https://github.com/RandyGaul/cute_framework/blob/master/docs/networking/server/server_create.md)  
[server_start](https://github.com/RandyGaul/cute_framework/blob/master/docs/networking/server/server_start.md)  
[server_stop](https://github.com/RandyGaul/cute_framework/blob/master/docs/networking/server/server_stop.md)  
[server_update](https://github.com/RandyGaul/cute_framework/blob/master/docs/networking/server/server_update.md)  
