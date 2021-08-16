# server_send_to_all_but_one_client

Sends a packet to aall but one client.

## Syntax

```cpp
void server_send_to_all_but_one_client(server_t* server, const void* packet, int size, int client_index, bool send_reliably);
```

## Function Parameters

Parameter Name | Description
--- | ---
server | The server.
packet | Pointer to the packet data.
size | Size of the data in bytes at the `packet` pointer.
client_index | The index of the client to not send to.
send_reliably | If true the packet will be sent reliably and in order. False means a typical UDP packet will be sent, and if lost nothing special is done - it will be lost forever, can arrive late, out of order, or even duplicated.

## Related Functions

[server_pop_event](https://github.com/RandyGaul/cute_framework/blob/master/docs/networking/server/server_pop_event.md)  
[server_send](https://github.com/RandyGaul/cute_framework/blob/master/docs/networking/server/server_send.md)  
[server_send_to_all_clients](https://github.com/RandyGaul/cute_framework/blob/master/docs/networking/server/server_send_to_all_clients.md)  
[server_is_client_connected](https://github.com/RandyGaul/cute_framework/blob/master/docs/networking/server/server_is_client_connected.md)  
