# server_send_to_all_but_one_client

Checks to see if a client at a particular index is connected.

## Syntax

```cpp
bool server_is_client_connected(server_t* server, int client_index);
```

## Function Parameters

Parameter Name | Description
--- | ---
server | The server.
client_index | The index of the client.

## Return Value

Returns true if the client is connected, false otherwise.

## Related Functions

[server_pop_event](https://github.com/RandyGaul/cute_framework/blob/master/docs/networking/server/server_pop_event.md)  
[server_disconnect_client](https://github.com/RandyGaul/cute_framework/blob/master/docs/networking/server/server_disconnect_client.md)  
