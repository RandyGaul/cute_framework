[](../header.md ':include')

# cf_destroy_app

Category: [app](/api_reference?id=app)  
GitHub: [cute_app.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_app.h)  
---

Cleans up all resources used by the application. Call [cf_app_signal_shutdown](/app/cf_app_signal_shutdown.md) first.

```cpp
CF_API void CF_CALL cf_destroy_app();
```

## Related Pages

[cf_make_app](/app/cf_make_app.md)  
[cf_app_is_running](/app/cf_app_is_running.md)  
[cf_app_signal_shutdown](/app/cf_app_signal_shutdown.md)  
