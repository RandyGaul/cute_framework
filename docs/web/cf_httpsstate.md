[](../header.md ':include')

# CF_HttpsState

Category: [web](/api_reference?id=web)  
GitHub: [cute_https.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_https.h)  
---

The states of power for the application.

## Values

Enum | Description
--- | ---
HTTPS_STATE_PENDING | Keep calling [cf_https_process](/web/cf_https_process.md). See [CF_Https](/web/cf_https.md) for a full example.
HTTPS_STATE_COMPLETED | The response has been acquired, retrieve it with [cf_https_response](/web/cf_https_response.md).
HTTPS_STATE_FAILED | The request has failed, the only valid operation left is [cf_https_destroy](/web/cf_https_destroy.md).

## Related Pages

[CF_Https](/web/cf_https.md)  
[cf_https_get](/web/cf_https_get.md)  
[cf_https_post](/web/cf_https_post.md)  
[cf_https_destroy](/web/cf_https_destroy.md)  
[cf_https_process](/web/cf_https_process.md)  
