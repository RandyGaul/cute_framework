[](../header.md ':include')

# cf_app_update

Category: [app](/api_reference?id=app)  
GitHub: [cute_app.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_app.h)  
---

Updates the application. Must be called once per frame.

```cpp
void cf_app_update(CF_OnUpdateFn* on_update);
```

Parameters | Description
--- | ---
on_update | Called for each update tick.

## Code Example

> Running an app in Variable or Fixed Timestep.

```cpp
TODO
```

## Related Pages

[cf_make_app](/app/cf_make_app.md)  
[cf_app_is_running](/app/cf_app_is_running.md)  
[cf_app_signal_shutdown](/app/cf_app_signal_shutdown.md)  
[cf_destroy_app](/app/cf_destroy_app.md)  
