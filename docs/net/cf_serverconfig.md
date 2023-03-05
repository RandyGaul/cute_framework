# CF_ServerConfig | [net](https://github.com/RandyGaul/cute_framework/blob/master/docs/net/README.md) | [cute_networking.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_networking.h)

Parameters for calling [cf_make_server](https://github.com/RandyGaul/cute_framework/blob/master/docs/net/cf_make_server.md).

Struct Members | Description
--- | ---
`uint64_t application_id` | A unique number to identify your game, can be whatever value you like. This must be the same number as in `client_make`.
`int max_incoming_bytes_per_second` | Not implemented yet.
`int max_outgoing_bytes_per_second` | Not implemented yet.
`int connection_timeout` | The number of seconds before consider a connection as timed out when not receiving any packets on the connection.
`double resend_rate` | The number of seconds to wait before resending a packet that has not been acknowledge as received by a client.
`CF_CryptoSignPublic public_key` | The public part of your public key cryptography used for connect tokens. This can be safely shared with your players publicly. See [CF_CryptoSignPublic](https://github.com/RandyGaul/cute_framework/blob/master/docs/net/cf_cryptosignpublic.md).
`CF_CryptoSignSecret secret_key` | The secret part of your public key cryptography used for connect tokens. This must never be shared publicly and remain a complete secret only know to your servers. See [CF_CryptoSignSecret](https://github.com/RandyGaul/cute_framework/blob/master/docs/net/cf_cryptosignsecret.md).

## Remarks

Call [cf_server_config_defaults](https://github.com/RandyGaul/cute_framework/blob/master/docs/net/cf_server_config_defaults.md) to get a good set of default parameters.

## Related Pages

[cf_make_server](https://github.com/RandyGaul/cute_framework/blob/master/docs/net/cf_make_server.md)  
[cf_server_config_defaults](https://github.com/RandyGaul/cute_framework/blob/master/docs/net/cf_server_config_defaults.md)  
