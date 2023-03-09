[](../header.md ':include')

# Transfer Encoding

Category: [web](/api_reference?id=web)  
GitHub: [cute_https.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_https.h)  
---

Flags for [CF_HttpsResponse](/web/cf_httpsresponse.md) transfer encoding settings.

## Values

Enum | Description
--- | ---
TRANSFER_ENCODING_FLAG_NONE | No transfer encoding settings -- just raw bytes.
TRANSFER_ENCODING_FLAG_CHUNKED | The content of the response is in chunked form.
TRANSFER_ENCODING_FLAG_GZIP | GZIP compression is used.
TRANSFER_ENCODING_FLAG_DEFLATE | DEFLATE compression is used.
TRANSFER_ENCODING_FLAG_DEPRECATED_COMPRESS | A deprecated compression method was used.

## Related Pages

[CF_Https](/web/cf_https.md)  
[cf_https_get](/web/cf_https_get.md)  
[cf_https_post](/web/cf_https_post.md)  
[cf_https_destroy](/web/cf_https_destroy.md)  
[cf_https_process](/web/cf_https_process.md)  
