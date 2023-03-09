# CF_ClientState

Category: [net](https://github.com/RandyGaul/cute_framework/blob/master/docs/api_reference?id=net)  
GitHub: [cute_networking.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_networking.h)  
---

The various states of a [CF_Client](https://github.com/RandyGaul/cute_framework/blob/master/docs/net/cf_client.md).

## Values

Enum | Description
--- | ---
CLIENT_STATE_CONNECT_TOKEN_EXPIRED | 
CLIENT_STATE_INVALID_CONNECT_TOKEN | 
CLIENT_STATE_CONNECTION_TIMED_OUT | 
CLIENT_STATE_CHALLENGE_RESPONSE_TIMED_OUT | 
CLIENT_STATE_CONNECTION_REQUEST_TIMED_OUT | 
CLIENT_STATE_CONNECTION_DENIED | 
CLIENT_STATE_DISCONNECTED | 
CLIENT_STATE_SENDING_CONNECTION_REQUEST | 
CLIENT_STATE_SENDING_CHALLENGE_RESPONSE | 
CLIENT_STATE_CONNECTED | 

## Remarks

Anything less than or equal to 0 is an error.

## Related Pages

[cf_client_state_get](https://github.com/RandyGaul/cute_framework/blob/master/docs/net/cf_client_state_get.md)  
[cf_client_state_to_string](https://github.com/RandyGaul/cute_framework/blob/master/docs/net/cf_client_state_to_string.md)  
