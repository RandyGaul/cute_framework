# server_update

Updates the server, allowing it to send and receive packets, and perform necessary logic involving handling incoming connection handshakes and validating connect tokens.

## Syntax

```cpp
void server_update(server_t* server, double dt, uint64_t current_time);
```

## Function Parameters

Parameter Name | Description
--- | ---
server | The server to update.
dt | The time elapsed, in seconds, since the last call to `client_update`.
current_time | A [unix timestamp](https://en.wikipedia.org/wiki/Unix_time) of the current time.

## Remarks

You should call this function once per game loop after calling [server_start](https://github.com/RandyGaul/cute_framework/blob/master/docs/networking/server/server_start.md).

## Related Functions

[server_create](https://github.com/RandyGaul/cute_framework/blob/master/docs/networking/server/server_create.md)  
[server_destroy](https://github.com/RandyGaul/cute_framework/blob/master/docs/networking/server/server_destroy.md)  
[server_start](https://github.com/RandyGaul/cute_framework/blob/master/docs/networking/server/server_start.md)  
[server_stop](https://github.com/RandyGaul/cute_framework/blob/master/docs/networking/server/server_stop.md)  
