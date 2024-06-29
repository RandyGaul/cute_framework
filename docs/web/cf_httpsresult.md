[](../header.md ':include')

# CF_HttpsResult

Category: [web](/api_reference?id=web)  
GitHub: [cute_https.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_https.h)  
---

Status of a [CF_HttpsRequest](/web/cf_httpsrequest.md).

## Values

Enum | Description
--- | ---
HTTPS_RESULT_BAD_CERTIFICATE | The server has an invalid certificate.
HTTPS_RESULT_CERTIFICATE_EXPIRED | The server's certificate has expired.
HTTPS_RESULT_BAD_HOSTNAME | The name of the host is invalid.
HTTPS_RESULT_CANNOT_VERIFY_CA_CHAIN | Unable to verify the host's cert.
HTTPS_RESULT_NO_MATCHING_ENCRYPTION_ALGORITHMS | Unable to form a secure connection.
HTTPS_RESULT_SOCKET_ERROR | Socket on the local machine failed.
HTTPS_RESULT_FAILED | Unknown error.
HTTPS_RESULT_PENDING | Continue calling [cf_https_process](/web/cf_https_process.md).
HTTPS_RESULT_OK | The result has finished, you may stop calling [cf_https_process](/web/cf_https_process.md), and fetch the response via [cf_https_response](/web/cf_https_response.md).

## Remarks

Intended to be used in a loop, along with [cf_https_process](/web/cf_https_process.md). See [CF_HttpsRequest](/web/cf_httpsrequest.md).

## Related Pages

[CF_HttpsRequest](/web/cf_httpsrequest.md)  
[cf_https_process](/web/cf_https_process.md)  
[cf_https_result_to_string](/web/cf_https_result_to_string.md)  
