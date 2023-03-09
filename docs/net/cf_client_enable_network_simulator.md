# cf_client_enable_network_simulator

Category: [net](https://github.com/RandyGaul/cute_framework/blob/master/docs/api_reference?id=net)  
GitHub: [cute_networking.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_networking.h)  
---

Turns on the network simulator for a client.

```cpp
void cf_client_enable_network_simulator(CF_Client* client, double latency, double jitter, double drop_chance, double duplicate_chance);
```

Parameters | Description
--- | ---
client | The client.
latency | A number of seconds of latency to add to the connection.
jitter | The variability of latency.
drop_chance | Number from [0,1]. 0 means drop no packets, 1 means drop all packets, 0.5f means 50% packet loss.
duplicate_chance | Number from [0,1] representing the chance to duplicate a packet, where 1 is 100% chance.

## Related Pages

[CF_Client](https://github.com/RandyGaul/cute_framework/blob/master/docs/net/cf_client.md)  
