[](../header.md ':include')

# cf_app_mouse_exited

Category: [app](/api_reference?id=app)  
GitHub: [cute_app.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_app.h)  
---

Returns true if the mouse's coordinates stopped hovering over the app last frame.

```cpp
CF_API bool CF_CALL cf_app_mouse_exited();
```

## Remarks

This function only deals with mouse coordinates, not focus (such as [cf_app_has_focus](/app/cf_app_has_focus.md)).

## Related Pages

[cf_app_mouse_entered](/app/cf_app_mouse_entered.md)  
[cf_app_mouse_inside](/app/cf_app_mouse_inside.md)  
