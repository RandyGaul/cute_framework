[](../header.md ':include')

# cf_update_texture

Category: [graphics](/api_reference?id=graphics)  
GitHub: [cute_graphics.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_graphics.h)  
---

Updates the contents of a [CF_Texture](/graphics/cf_texture.md).

```cpp
CF_API void CF_CALL cf_update_texture(CF_Texture texture, void* data, int size);
```

Parameters | Description
--- | ---
texture | The texture.
data | The data to upload to the texture.
size | The size in bytes of `data`.

## Remarks

The texture must not have been created with `CF_USAGE_TYPE_IMMUTABLE`.

## Related Pages

[CF_TextureParams](/graphics/cf_textureparams.md)  
[CF_Texture](/graphics/cf_texture.md)  
[cf_make_texture](/graphics/cf_make_texture.md)  
[cf_destroy_texture](/graphics/cf_destroy_texture.md)  
