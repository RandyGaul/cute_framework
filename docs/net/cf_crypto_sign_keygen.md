# cf_crypto_sign_keygen

Category: [net](https://github.com/RandyGaul/cute_framework/blob/master/docs/api_reference?id=net)  
GitHub: [cute_networking.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_networking.h)  
---

Generates a cryptographically secure keypair, used for facilitating connect tokens.

```cpp
void cf_crypto_sign_keygen(CF_CryptoSignPublic* public_key, CF_CryptoSignSecret* secret_key);
```

Parameters | Description
--- | ---
public_key | The public key of the keypair. Freely share this publicy.
secret_key | The secret key of the keypair. Keep this safe and hidden within your servers.

## Related Pages

[CF_CryptoKey](https://github.com/RandyGaul/cute_framework/blob/master/docs/net/cf_cryptokey.md)  
[cf_crypto_generate_key](https://github.com/RandyGaul/cute_framework/blob/master/docs/net/cf_crypto_generate_key.md)  
[cf_generate_connect_token](https://github.com/RandyGaul/cute_framework/blob/master/docs/net/cf_generate_connect_token.md)  
