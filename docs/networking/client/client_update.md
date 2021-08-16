# client_update

Updates a client, allowing it to send and receive packets to/from the server.

## Syntax

```cpp
void client_update(client_t* client, double dt, uint64_t current_time);
```

## Function Parameters

Parameter Name | Description
--- | ---
client | The client to update.
dt | The time elapsed, in seconds, since the last call to `client_update`.
current_time | A [unix timestamp](https://en.wikipedia.org/wiki/Unix_time) of the current time.

## Remarks

You should call this function once per game loop after calling [client_connect](https://github.com/RandyGaul/cute_framework/blob/master/docs/networking/client/client_connect.md).

## Related Functions

[client_make](https://github.com/RandyGaul/cute_framework/blob/master/docs/networking/client/client_make.md)  
[client_destroy](https://github.com/RandyGaul/cute_framework/blob/master/docs/networking/client/client_destroy.md)  
[client_connect](https://github.com/RandyGaul/cute_framework/blob/master/docs/networking/client/client_connect.md)  
[client_disconnect](https://github.com/RandyGaul/cute_framework/blob/master/docs/networking/client/client_disconnect.md)  
[client_state_get](https://github.com/RandyGaul/cute_framework/blob/master/docs/networking/client/client_state_get.md)  
