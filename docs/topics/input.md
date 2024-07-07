[](../header.md ':include')

<br>

There are four main ways to gather input: keyboard, mouse, touch, input text, and IME (input method editor).

- Keyboard -- Detects individual keyboard strokes -- not quite the same as text input (see input text).
- Mouse.
- Touch.
- Input text -- Different than the keyboard. Stores characters coming from the Operating System about actual text the user is inputting. This can be multiple characters in a single frame, or characters created from multiple keystrokes.
- IME ([input method editor](https://learn.microsoft.com/en-us/windows/apps/design/input/input-method-editors)) -- An operating system feature to transform multiple keystrokes into an editable text composition. For example, this is used when converting pinyin (typed english characters) into Mandarin (actual Chinese glyphs).
- Joypad.

## Keyboard

The main two functions for gathering keyboard input are [`cf_key_down`](https://randygaul.github.io/cute_framework/#/input/cf_key_down) and [`cf_key_just_pressed`](https://randygaul.github.io/cute_framework/#/input/cf_key_just_pressed).

```cpp
// Continually true while the Z key is down.
if (cf_key_down(CF_KEY_Z)) {
	printf("Key Z is currently DOWN\n");
}
```

The just-pressed function behaves a little differentely; it returns true for a single frame after a key was pressed, but does not continually return true.

```cpp
// The X key was just pressed -- returns true for one frame.
if (cf_key_just_pressed(CF_KEY_X)) {
	printf("Key X was just pressed\n");
}
```

You may also check if any alt, shift, or control key was pressed (either left or right) with [`cf_key_alt`](https://randygaul.github.io/cute_framework/#/input/cf_key_alt), [`cf_key_shift`](https://randygaul.github.io/cute_framework/#/input/cf_key_shift), and [`cf_key_ctrl`](https://randygaul.github.io/cute_framework/#/input/cf_key_ctrl).

```cpp
// Detect ctrl + z just pressed.
if (cf_key_ctrl() && cf_key_just_pressed(KEY_Z)) {
}
```

## Mouse

Much like the keyboard, the mouse also has down/just pressed behavior with [`cf_mouse_down`](https://randygaul.github.io/cute_framework/#/input/cf_mouse_down) and [`cf_mouse_just_pressed)`](https://randygaul.github.io/cute_framework/#/input/cf_mouse_just_pressed).

```cpp
// Check if right click is currently down.
if (cf_mouse_down(CF_MOUSE_BUTTON_RIGHT)) {
	printf("Right button is DOWN\n");
}
```

And to check for just-pressed:

```cpp
if (cf_mouse_just_pressed(CF_MOUSE_BUTTON_LEFT)) {
	printf("Left click detected\n");
}
```

Double clicks (both held and just-pressed) are available with [`cf_mouse_double_click_held`](https://randygaul.github.io/cute_framework/#/input/cf_mouse_double_click_held) and [`cf_mouse_double_clicked`](https://randygaul.github.io/cute_framework/#/input/cf_mouse_double_clicked).

The mouse coordinates can be fetched with [`cf_mouse_x`](https://randygaul.github.io/cute_framework/#/input/cf_mouse_x) and [`cf_mouse_y`](https://randygaul.github.io/cute_framework/#/input/cf_mouse_y). Each function returns an integer component. (0, 0) is the top-left of the screen, with the y-axis pointing downwards. It may be a little annoying to get mouse coordinates like this, since the default graphics coordinate system has (0, 0) at the center of the screen with the y-axis pointing upwards.

## Touch

Touch inputs come in the form of touch events ([`CF_Touch`](https://randygaul.github.io/cute_framework/#/input/cf_touch)). Each touch event has a unique 64-bit identifier. Usually touch events are fairly short-lived, so it's up to you to note their unique ID's from one frame to another, and notice when a particular touch event dissappears.

```cpp
struct CF_Touch
{
	uint64_t id;    // A unique identifier for every touch event.
	float x;        // The x-position of the touch event, normalized.
	float y;        // The y-position of the touch event, normalized.
	float pressure; // A number from [0,1] representing the touch pressure.
};
```

?> Coordinates for touch events are normalized from [0,1], where [0,0] is the top-left corner.

To get list of all live touch events call [`cf_touch_get_all`](https://randygaul.github.io/cute_framework/#/input/cf_touch_get_all). This function fills in a pointer you can use to loop over all the touch events.

```cpp
CF_Touch* touches = NULL;
int touch_count = cf_touch_get_all(&touches);
for (int i = 0; i < touch_count; ++i) {
	do_something(touches[i]);
}
```

If you'd like to see if a particular touch event is live, you can try to fetch it with [`cf_touch_get`](https://randygaul.github.io/cute_framework/#/input/cf_touch_get).

## Input Text

You may at first assume the best way to get user input in the form of written text is to scan all the keystrokes as they come in with [`cf_key_just_pressed`](https://randygaul.github.io/cute_framework/#/input/cf_key_just_pressed). This can seem to work at first for languages like English, but doesn't actually work too well when considering more use-cases. For example, what if we want to type a capitalized letter? What if shift is held? What if we're typing in another language like Mandarin, Japanese, or Korean, where multiple keystrokes can compose a single (or sometimes multiple) glyphs?

The best way to gather text or written input is to use CF's input text functions.

- [`cf_input_text_add_utf8`](https://randygaul.github.io/cute_framework/#/input/cf_input_text_add_utf8)
- [`cf_input_text_clear`](https://randygaul.github.io/cute_framework/#/input/cf_input_text_clear)
- [`cf_input_text_has_data`](https://randygaul.github.io/cute_framework/#/input/cf_input_text_has_data)
- [`cf_input_text_pop_utf32`](https://randygaul.github.io/cute_framework/#/input/cf_input_text_pop_utf32)

These functions have an internal buffer of [UTF8 encoded](https://en.wikipedia.org/wiki/UTF-8) text. As users input text (in any number ways, including typing on the keyboard) the operating system will report text characters as they arrive. These are all stored by CF and exposed here. To get one character at a time, simply call [`cf_input_text_pop_utf32`](https://randygaul.github.io/cute_framework/#/input/cf_input_text_pop_utf32), which returns a single character in [UTF32 format](https://en.wikipedia.org/wiki/UTF-32) (which means as a simple integer).

## Input Method Editor

An IME ([input method editor](https://learn.microsoft.com/en-us/windows/apps/design/input/input-method-editors)) is a feature of the operating system to allow users to input text that isn't easily representing on a standard QWERTY keyboard. A great example would by typing in [pinyin](https://en.wikipedia.org/wiki/Pinyin) as it's automatically converted into Mandarin characters.

We have access to the operating system's IME through [`CF_ImeComposition`](https://randygaul.github.io/cute_framework/#/input/cf_imecomposition) and friends. First, IME support must be enabled with [`cf_input_enable_ime`](https://randygaul.github.io/cute_framework/#/input/cf_input_enable_ime). From there the IME text can be viewed with [`cf_input_get_ime_composition`](https://randygaul.github.io/cute_framework/#/input/cf_input_get_ime_composition).

You can check for keyboard support in the IME with [`cf_input_has_ime_keyboard_support`](https://randygaul.github.io/cute_framework/#/input/cf_input_has_ime_keyboard_support), and know if the operating system is currently showing an IME keyboard on screen with [`cf_input_is_ime_keyboard_shown`](https://randygaul.github.io/cute_framework/#/input/cf_input_is_ime_keyboard_shown).

To tell the operating system where the IME should actually be on screen (as in, where the input rect is located) you can call [`cf_input_set_ime_rect`](https://randygaul.github.io/cute_framework/#/input/cf_input_set_ime_rect).

## Joypad

TODO
