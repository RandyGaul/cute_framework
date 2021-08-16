# client_pop_packet

Returns a packet from the server if available.

## Syntax

```cpp
bool client_pop_packet(client_t* client, void** packet, int* size);
```

## Function Parameters

Parameter Name | Description
--- | ---
client | The client to get a packet from.
packet | Will point to the packet data one is returned.
size | The size of the buffer at `packet`.


## Return Value

Returns true if a packet was returned. If false, then `packet` and `size` should not be considered valid.

## Remarks

The packet needs to be cleaned up when you're done using it by calling [client_free_packet](https://github.com/RandyGaul/cute_framework/blob/master/docs/networking/client/client_free_packet.md).

## Related Functions

[client_free_packet](https://github.com/RandyGaul/cute_framework/blob/master/docs/networking/client/client_free_packet.md)  
