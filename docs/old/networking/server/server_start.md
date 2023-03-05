# server_start

Starts up the server, ready to receive new client connections.

## Syntax

```cpp
error_t server_start(server_t* server, const char* address_and_port);
```

## Function Parameters

Parameter Name | Description
--- | ---
server | The server to start.
address_and_port | A string format of the address an port, in either ipv4 or ipv6 format. Examples: `"73.140.224.19:3000"`, `[2001:4860:4860::8888]:3000`.

## Return Value

Returns any errors upon failure.

## Remarks

Please note that not all users will be able to access an ipv6 server address, so it might be good to also provide a way to connect through ipv4.

## Related Functions

[server_create](https://github.com/RandyGaul/cute_framework/blob/master/docs/networking/server/server_create.md)  
[server_destroy](https://github.com/RandyGaul/cute_framework/blob/master/docs/networking/server/server_destroy.md)  
[server_stop](https://github.com/RandyGaul/cute_framework/blob/master/docs/networking/server/server_stop.md)  
[server_update](https://github.com/RandyGaul/cute_framework/blob/master/docs/networking/server/server_update.md)  
