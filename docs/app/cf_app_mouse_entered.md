[](../header.md ':include')

# cf_app_mouse_entered

Category: [app](/api_reference?id=app)  
GitHub: [cute_app.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_app.h)  
---

Returns true if the mouse's coordinates began hovering over the app last frame.

```cpp
bool cf_app_mouse_entered();
```

## Remarks

This function only deals with mouse coordinates, not focus (such as [cf_app_has_focus](/app/cf_app_has_focus.md)).

## Related Pages

[cf_app_mouse_exited](/app/cf_app_mouse_exited.md)  
[cf_app_mouse_inside](/app/cf_app_mouse_inside.md)  
