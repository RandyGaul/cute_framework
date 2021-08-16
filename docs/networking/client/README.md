# client

The client can use a [Connect Token](https://github.com/RandyGaul/cute_framework/tree/master/docs/networking/) to connect to a game server. The connect token provides all the information necessary to make attempts at connecting to a list of game servers. It is up to you as the server owner to provide a way to get connect tokens to the client. This is generally done from a web service over a rest call, such as HTTPS. Read more about connect tokens in the [networking section](https://github.com/RandyGaul/cute_framework/tree/master/docs/networking/).

Once connected the client can send and receive packets from the server.

[client_make](https://github.com/RandyGaul/cute_framework/blob/master/docs/networking/client/client_make.md)
[client_destroy](https://github.com/RandyGaul/cute_framework/blob/master/docs/networking/client/client_destroy.md)
[client_connect](https://github.com/RandyGaul/cute_framework/blob/master/docs/networking/client/client_connect.md)
[client_disconnect](https://github.com/RandyGaul/cute_framework/blob/master/docs/networking/client/client_disconnect.md)
[client_update](https://github.com/RandyGaul/cute_framework/blob/master/docs/networking/client/client_update.md)
[client_pop_packet](https://github.com/RandyGaul/cute_framework/blob/master/docs/networking/client/client_pop_packet.md)
[client_send](https://github.com/RandyGaul/cute_framework/blob/master/docs/networking/client/client_send.md)
[client_state_get](https://github.com/RandyGaul/cute_framework/blob/master/docs/networking/client/client_state_get.md)
[client_state_string](https://github.com/RandyGaul/cute_framework/blob/master/docs/networking/client/client_state_string.md
[client_time_of_last_packet_recieved](https://github.com/RandyGaul/cute_framework/blob/master/docs/networking/client/client_time_of_last_packet_recieved_get.md)  )
[client_enable_network_simulator](https://github.com/RandyGaul/cute_framework/blob/master/docs/networking/client/client_enable_network_simulator.md)
