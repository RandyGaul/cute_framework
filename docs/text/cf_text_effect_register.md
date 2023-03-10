[](../header.md ':include')

# cf_text_effect_register

Category: [text](/api_reference?id=text)  
GitHub: [cute_draw.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_draw.h)  
---

Registers a custom text effect.

```cpp
CF_API void CF_CALL cf_text_effect_register(const char* name, CF_TextEffectFn* fn);
```

Parameters | Description
--- | ---
name | A unique name for your text effect.
fn | The [CF_TextEffectFn](/text/cf_texteffectfn.md) function you must implement to perform the custom effect.

## Code Example

> Internally the text shake effect is implemented something like this.

```cpp
// Given a string like this:
"Some <shake freq=50 x=2.5 y=1>shaking text</shake> drawing!"

static bool s_text_fx_shake(TextEffect effect)
{
    double freq = effect->get_number("freq", 35);
    int seed = (int)(effect->elapsed  freq);
    float x = (float)effect->get_number("x", 2);
    float y = (float)effect->get_number("y", 2);
    CF_Rnd rnd = cf_rnd_seed(seed);
    v2 offset = V2(rnd_next_range(rnd, -x, y), rnd_next_range(rnd, -x, y));
    effect->q0 += offset;
    effect->q1 += offset;
    return true;
}

// Register it like so:
cf_text_effect_register("shake", s_text_fx_shake);
```

## Remarks

The `name` of the text effect will be used within the string text codes. For example, for the "shake" effect in the above
example, the text code <shake> will be used.
```
+ color
     example : "Here's some <color=#2c5ee8>blue text</color>."
             : default (white) - The color to render text with.
+ shake
     example : "<shake freq=30 x=3 y=3>This text is all shaky.</shake>"
     example : "<shake y=20>Shake this text with default values, but override for a big height.</shake>"
     freq    : default (35)    - Number of times per second to shake.
     x       : default (2)     - Max +/- distance to shake on x-axis.
     y       : default (2)     - Max +/- distance to shake on y-axis.
+ fade
     example : "<fade speed=10 span=3>Fading some text like a ghost~</fade>"
     example : "<fade>Fading some text like a ghost~</fade>"
     speed   : default (2)     - Number of times per second to find in and then out.
     span    : default (5)     - Number of characters long for the fade to loop.
+ wave
     example : "<wave>Wobbly wave text.</wave>"
     speed   : default (5)     - Number of times per second to bob up and down.
     span    : default (10)    - Number of characters long for the wave to loop.
     height. : default (5)     - How many characters high the wave will go.
+ strike
     example : "<strike>Strikethrough</strike>"
     example : "<strike=10>Thick Strikethrough</strike>"
             : default (font_height / 20) - The thickness of the strike line.
```
When registering a custom text effect, any parameters in the string will be stored for you
automatically. You only need to fetch them with the appropriate cf_text_effect_get function.

## Related Pages

[CF_TextEffect](/text/cf_texteffect.md)  
[CF_TextEffectFn](/text/cf_texteffectfn.md)  
[cf_text_effect_get_string](/text/cf_text_effect_get_string.md)  
[cf_text_effect_on_start](/text/cf_text_effect_on_start.md)  
[cf_text_effect_on_finish](/text/cf_text_effect_on_finish.md)  
[cf_text_effect_get_number](/text/cf_text_effect_get_number.md)  
[cf_text_effect_get_color](/text/cf_text_effect_get_color.md)  
