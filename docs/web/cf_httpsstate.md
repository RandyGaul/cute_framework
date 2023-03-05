# CF_HttpsState | [web](https://github.com/RandyGaul/cute_framework/blob/master/docs/web_readme.md) | [cute_https.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_https.h)

The states of power for the application.

## Values

Enum | Description
--- | ---
HTTPS_STATE_PENDING | Keep calling [cf_https_process](https://github.com/RandyGaul/cute_framework/blob/master/docs/web/cf_https_process.md). See [CF_Https](https://github.com/RandyGaul/cute_framework/blob/master/docs/web/cf_https.md) for a full example.
HTTPS_STATE_COMPLETED | The response has been acquired, retrieve it with [cf_https_response](https://github.com/RandyGaul/cute_framework/blob/master/docs/web/cf_https_response.md).
HTTPS_STATE_FAILED | The request has failed, the only valid operation left is [cf_https_destroy](https://github.com/RandyGaul/cute_framework/blob/master/docs/web/cf_https_destroy.md).

## Related Pages

[CF_Https](https://github.com/RandyGaul/cute_framework/blob/master/docs/web/cf_https.md)  
[cf_https_get](https://github.com/RandyGaul/cute_framework/blob/master/docs/web/cf_https_get.md)  
[cf_https_post](https://github.com/RandyGaul/cute_framework/blob/master/docs/web/cf_https_post.md)  
[cf_https_destroy](https://github.com/RandyGaul/cute_framework/blob/master/docs/web/cf_https_destroy.md)  
[cf_https_process](https://github.com/RandyGaul/cute_framework/blob/master/docs/web/cf_https_process.md)  
