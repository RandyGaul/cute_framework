# cf_https_post | [web](https://github.com/RandyGaul/cute_framework/blob/master/docs/web/README.md) | [cute_https.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_https.h)

Initiates a GET request for the specified host (website address) and a given uri.

```cpp
CF_Https* cf_https_post(const char* host, const char* port, const char* uri, const void* data, size_t size, CF_Result* err, bool verify_cert);
```

Parameters | Description
--- | ---
host | The web address where we send the HTTPS GET request.
port | The port to use.
data | Pointer to data to send to the host.
size | Size of `data` in bytes.
uri | The URI on the host.
err | Can be `NULL`. Reports any errors.
bool | You should set this to `true`. `false` will disable the secure part of HTTPS. Please only do this for testing.

## Return Value

Returns a [CF_Https](https://github.com/RandyGaul/cute_framework/blob/master/docs/web/cf_https.md) pointer.

## Remarks

Initiates a POST request for the specified host (website address) and a given uri. The content of the post
is in `data`, which can be `NULL` if `size` is 0.

Any errors are optionally reported through the `err` parameter.

`verify_cert` will verify the server's x509 certificate, but can be disabled (dangerous).

Returns an [CF_Https](https://github.com/RandyGaul/cute_framework/blob/master/docs/web/cf_https.md) pointer which needs to be processed with [cf_https_process](https://github.com/RandyGaul/cute_framework/blob/master/docs/web/cf_https_process.md) and cleaned up by [cf_https_destroy](https://github.com/RandyGaul/cute_framework/blob/master/docs/web/cf_https_destroy.md).

`host` and `port` are unused when building with emscripten -- this is since an XMLHttpRequest is used
underneath, meaning only files from the server this code came from can be loaded, and as such the `uri`
should only be a relative path on the server.

## Related Pages

[CF_Https](https://github.com/RandyGaul/cute_framework/blob/master/docs/web/cf_https.md)  
[cf_https_get](https://github.com/RandyGaul/cute_framework/blob/master/docs/web/cf_https_get.md)  
[cf_https_process](https://github.com/RandyGaul/cute_framework/blob/master/docs/web/cf_https_process.md)  
[cf_https_destroy](https://github.com/RandyGaul/cute_framework/blob/master/docs/web/cf_https_destroy.md)  
