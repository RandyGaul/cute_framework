# server_disconnect_client

Disconnects a specific client.

## Syntax

```cpp
void server_disconnect_client(server_t* server, int client_index, bool notify_client = true);
```

## Function Parameters

Parameter Name | Description
--- | ---
server | The server.
client_index | The index of the client to disconnect.
notify_client | If true the client is notified of the disconnect and will receive a disconnect packet.

## Related Functions

[server_pop_event](https://github.com/RandyGaul/cute_framework/blob/master/docs/networking/server/server_pop_event.md)  
[server_send](https://github.com/RandyGaul/cute_framework/blob/master/docs/networking/server/server_send.md)  
[server_send_to_all_clients](https://github.com/RandyGaul/cute_framework/blob/master/docs/networking/server/server_send_to_all_clients.md)  
[server_send_to_all_but_one_client](https://github.com/RandyGaul/cute_framework/blob/master/docs/networking/server/server_send_to_all_but_one_client.md)  
