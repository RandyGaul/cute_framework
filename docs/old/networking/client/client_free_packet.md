# client_free_packet

Frees a packet that came from [client_pop_packet](https://github.com/RandyGaul/cute_framework/blob/master/docs/networking/client/client_pop_packet.md).

## Syntax

```cpp
void client_free_packet(client_t* client, void* packet);
```

## Function Parameters

Parameter Name | Description
--- | ---
client | The client used when calling [client_pop_packet](https://github.com/RandyGaul/cute_framework/blob/master/docs/networking/client/client_pop_packet.md).

## Related Functions

[client_pop_packet](https://github.com/RandyGaul/cute_framework/blob/master/docs/networking/client/client_pop_packet.md)  
