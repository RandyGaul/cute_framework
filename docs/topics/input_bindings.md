# Input Bindings

Input bindings provide an abstraction layer over raw keyboard, mouse, and joypad inputs. Instead of polling individual keys or buttons, you create a binding that maps multiple physical inputs to a single logical action. Bindings support input buffering for forgiving press/release detection.

There are three binding types, each building on the previous:

- **Button Binding** -- Maps any combination of keys, mouse buttons, joypad buttons, and analog triggers to a single on/off button.
- **Axis Binding** -- Combines two button bindings (negative/positive) into a -1..1 axis with conflict resolution.
- **Stick Binding** -- Combines two axis bindings (X/Y) into a 2D stick.

## Button Bindings

A [`CF_ButtonBinding`](../binding/cf_buttonbinding.md) aggregates multiple physical inputs into a single logical button. Any of the bound inputs being active will activate the binding.

```c
// Create a "jump" button bound to spacebar, joypad A, and left trigger.
CF_ButtonBinding jump = cf_make_button_binding(0, 0.1f);
cf_button_binding_add_key(jump, CF_KEY_SPACE);
cf_button_binding_add_joypad_button(jump, CF_JOYPAD_BUTTON_A);
cf_button_binding_add_trigger(jump, CF_JOYPAD_AXIS_TRIGGERLEFT, 0.5f, true);

// In your game loop:
if (cf_binding_pressed(jump)) {
    // Jump was just pressed (with 0.1s buffer window).
}
if (cf_button_binding_down(jump)) {
    // Jump is currently held down.
}

// When done:
cf_destroy_binding(jump);
```

The `press_buffer` parameter to [`cf_make_button_binding`](../binding/cf_make_button_binding.md) controls how long a press/release event stays active, in seconds. This means that even if your gameplay code checks the binding a few frames late, the press still registers. Use [`cf_button_binding_consume_press`](../binding/cf_button_binding_consume_press.md) to consume a buffered event so it only triggers once.

## Axis Bindings

A [`CF_AxisBinding`](../binding/cf_axisbinding.md) combines a negative and positive button binding into a single axis that outputs values from -1 to 1.

```c
// Create a horizontal movement axis.
CF_AxisBinding move_x = cf_make_axis_binding(0);
cf_axis_binding_add_keys(move_x, CF_KEY_A, CF_KEY_D);
cf_axis_binding_add_left_stick_x(move_x, 0.15f);
cf_axis_binding_add_dpad_x(move_x);

float x = cf_binding_value(move_x); // -1 to 1
float sign = cf_binding_sign(move_x); // -1, 0, or 1
```

When both directions are pressed simultaneously, the conflict resolution mode determines the result. Set it with [`cf_axis_binding_set_conflict`](../binding/cf_axis_binding_set_conflict.md):

- `CF_AXIS_CONFLICT_NEWEST` (default) -- Use the most recently pressed direction.
- `CF_AXIS_CONFLICT_OLDEST` -- Use the first pressed direction.
- `CF_AXIS_CONFLICT_CANCEL` -- Return 0.

## Stick Bindings

A [`CF_StickBinding`](../binding/cf_stickbinding.md) combines two axis bindings (X and Y) into a 2D stick that returns a `CF_V2`.

```c
// Create a movement stick with WASD + left analog stick + d-pad.
CF_StickBinding move = cf_make_stick_binding(0);
cf_stick_binding_add_wasd(move);
cf_stick_binding_add_left_stick(move, 0.15f);
cf_stick_binding_add_dpad(move);

CF_V2 dir = cf_binding_value(move);   // Each component -1 to 1.
CF_V2 sign = cf_binding_sign(move);   // Each component -1, 0, or 1.
```

Convenience functions [`cf_stick_binding_add_wasd`](../binding/cf_stick_binding_add_wasd.md), [`cf_stick_binding_add_arrow_keys`](../binding/cf_stick_binding_add_arrow_keys.md), and [`cf_stick_binding_add_dpad`](../binding/cf_stick_binding_add_dpad.md) bind common key/button groups in a single call.

## Input Buffering

Input buffering is a technique for forgiving input timing. When creating a button binding, you specify a `press_buffer` duration in seconds. During this window, [`cf_binding_pressed`](../binding/cf_binding_pressed.md) and [`cf_binding_released`](../binding/cf_binding_released.md) will continue to return true, even if the physical input has already changed.

This is useful for platformers where the player presses jump slightly before landing. By buffering the press for 0.1 seconds, the jump will still register when the character touches the ground.

To prevent a buffered press from firing multiple times, use [`cf_binding_consume_press`](../binding/cf_binding_consume_press.md). Once consumed, the press will not report again until a new physical press occurs.

## Deadzone

Analog stick inputs use a global deadzone threshold to filter out stick drift. By default this is 0.15. Values below the deadzone are zeroed out, and the remaining range is remapped to 0..1.

Use [`cf_binding_get_deadzone`](../binding/cf_binding_get_deadzone.md) and [`cf_binding_set_deadzone`](../binding/cf_binding_set_deadzone.md) to read or adjust the threshold.

## Joypad Connection Tracking

Register a callback with [`cf_register_joypad_connect_callback`](../binding/cf_register_joypad_connect_callback.md) to be notified when joypads connect or disconnect. This is useful for showing controller UI prompts or adjusting player assignments.

```c
void on_joypad(int player_index, bool connected, void* udata) {
    if (connected) {
        printf("Joypad %d connected\n", player_index);
    } else {
        printf("Joypad %d disconnected\n", player_index);
    }
}

cf_register_joypad_connect_callback(on_joypad, NULL);
```

## Generic Dispatch

In C, the `cf_binding_pressed`, `cf_binding_released`, `cf_binding_value`, `cf_binding_sign`, `cf_binding_consume_press`, `cf_binding_consume_release`, and `cf_destroy_binding` macros use C11 `_Generic` to dispatch based on handle type. This lets you write uniform code regardless of whether you are working with a button, axis, or stick binding.

In C++, equivalent overloaded functions are available in `namespace Cute`.
