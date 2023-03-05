# CF_TextEffectFn | [text](https://github.com/RandyGaul/cute_framework/blob/master/docs/text_readme.md) | [cute_draw.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_draw.h)

Implements a custom text effect, called once per glyph.

```cpp
typedef bool (CF_TextEffectFn)(CF_TextEffect* fx);
```

Parameters | Description
--- | ---
fx | The text effect state.

## Return Value

Return true to go to the next glyph. Return false to stop processing the string.

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
```

## Remarks

The text between your custom text-code will get passed to `fn` for you, and called one time per glyph in
the text just before it gets rendered. You have the chance to modify things such as the text color, size, scale,
position, visibility, etc. You should use [cf_text_effect_get_number](https://github.com/RandyGaul/cute_framework/blob/master/docs/text/cf_text_effect_get_number.md), [cf_text_effect_get_color](https://github.com/RandyGaul/cute_framework/blob/master/docs/text/cf_text_effect_get_color.md), or
[cf_text_effect_get_string](https://github.com/RandyGaul/cute_framework/blob/master/docs/text/cf_text_effect_get_string.md) to fetch values from your codes. As a convenience, you can see if the current
character is the first or last to render using [cf_text_effect_on_start](https://github.com/RandyGaul/cute_framework/blob/master/docs/text/cf_text_effect_on_start.md) or [cf_text_effect_on_finish](https://github.com/RandyGaul/cute_framework/blob/master/docs/text/cf_text_effect_on_finish.md) respectively.

## Related Pages

[CF_TextEffect](https://github.com/RandyGaul/cute_framework/blob/master/docs/text/cf_texteffect.md)  
[cf_text_effect_get_string](https://github.com/RandyGaul/cute_framework/blob/master/docs/text/cf_text_effect_get_string.md)  
[cf_text_effect_register](https://github.com/RandyGaul/cute_framework/blob/master/docs/text/cf_text_effect_register.md)  
[cf_text_effect_on_start](https://github.com/RandyGaul/cute_framework/blob/master/docs/text/cf_text_effect_on_start.md)  
[cf_text_effect_on_finish](https://github.com/RandyGaul/cute_framework/blob/master/docs/text/cf_text_effect_on_finish.md)  
[cf_text_effect_get_number](https://github.com/RandyGaul/cute_framework/blob/master/docs/text/cf_text_effect_get_number.md)  
[cf_text_effect_get_color](https://github.com/RandyGaul/cute_framework/blob/master/docs/text/cf_text_effect_get_color.md)  
