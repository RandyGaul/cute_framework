[](../header.md ':include')

# cf_joypad_add_mapping

Category: [input](/api_reference?id=input)  
GitHub: [cute_joypad.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_joypad.h)  
---

Adds an SDL2 mapping to the joypad system.

```cpp
CF_API CF_Result CF_CALL cf_joypad_add_mapping(const char* mapping);
```

## Remarks

For each valid mapping string added, another kind of joypad is supported.
Cute Framework automatically initializes many mappings from the community organized mapping
database on GitHub (https://github.com/gabomdq/SDL_GameControllerDB), so you probably don't need
to ever call this function.

## Related Pages

[CF_Joypad](/input/cf_joypad.md)  
