
# image_depallete

Depalettes an indexed image, converting it into an `image_t` instance.

## Syntax

```cpp
image_t image_depallete(image_indexed_t* img);
```

## Function Parameters

Parameter Name | Description
--- | ---
img | The image to depalette.

## Return Value

Returns a depaletted `image_t` instance in RGBA format.

## Related Functions
  
[image_load_png_indexed](https://github.com/RandyGaul/cute_framework/blob/master/doc/graphics/image/image_load_png_indexed.md)  
[image_load_png_mem_indexed](https://github.com/RandyGaul/cute_framework/blob/master/doc/graphics/image/image_load_png_mem_indexed.md)  
