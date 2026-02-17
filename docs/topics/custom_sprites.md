# Custom Sprites

The [Custom Sprite API](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_custom_sprite.h) lets you build sprites from individual .png files with your own frame timings. This is useful when you have your own animation format or art pipeline and don't want to use .ase/.aseprite files.

If you just want to load sprites from Aseprite files, see [`cf_make_sprite`](../sprite/cf_make_sprite.md) instead.

## Overview

Building a custom sprite is a three-step process:

1. **Load PNGs** -- Load individual .png images into [`CF_Png`](../custom_sprite/cf_png.md) structs.
2. **Create Animations** -- Group PNGs into named [`CF_Animation`](../custom_sprite/cf_make_custom_sprite_animation.md) sequences with per-frame delays.
3. **Build the Sprite** -- Combine animations into an animation table and create a [`CF_Sprite`](../sprite/cf_sprite.md).

## Step 1: Load PNGs

Each frame of your animation is a separate .png file. Load them with [`cf_custom_sprite_load_png`](../custom_sprite/cf_custom_sprite_load_png.md):

```c
CF_Png frames[3];
cf_custom_sprite_load_png("/frame1.png", &frames[0]);
cf_custom_sprite_load_png("/frame2.png", &frames[1]);
cf_custom_sprite_load_png("/frame3.png", &frames[2]);
```

All frames within an animation must have the same dimensions. An assert will fire if they don't match.

Images are cached internally, so loading the same path twice is fast.

## Step 2: Create Animations

Group your PNGs into a named animation with per-frame delays (in seconds) using [`cf_make_custom_sprite_animation`](../custom_sprite/cf_make_custom_sprite_animation.md):

```c
float delays[] = { 0.1f, 0.1f, 0.1f };
const CF_Animation* walk = cf_make_custom_sprite_animation(
    "walk", frames, 3, delays, 3);
```

You can create as many animations as you need (walk, idle, attack, etc.), each from their own set of PNGs.

## Step 3: Build the Sprite

Combine your animations into an animation table using [`cf_make_custom_sprite_animation_table`](../custom_sprite/cf_make_custom_sprite_animation_table.md), then create the sprite with [`cf_make_custom_sprite`](../custom_sprite/cf_make_custom_sprite.md):

```c
const CF_Animation* anims[] = { walk, idle, attack };
const CF_AnimationTable* table = cf_make_custom_sprite_animation_table(
    "player", anims, 3);
CF_Sprite sprite = cf_make_custom_sprite("player", table);
```

The resulting `CF_Sprite` works exactly like any other sprite in CF. You can play animations, draw it, and update it with the normal sprite functions:

```c
cf_sprite_play(&sprite, "walk");
cf_sprite_update(&sprite);
cf_draw_sprite(&sprite);
```

## Non-Looping Animations

By default sprites loop their animation. Set `sprite.loop = false` for one-shot animations. When a non-looping animation reaches its last frame, the sprite's `finished` flag is set to `true`. This is useful for things like explosions or hit effects:

```c
cf_sprite_play(&sprite, "explosion");
sprite.loop = false;

// Later, in your update loop:
cf_sprite_update(&sprite);
if (sprite.finished) {
    // Animation is done, clean up.
}
```

Calling `cf_sprite_play` resets the `finished` flag.

## Full Example

Here is a complete example that loads two animations (bullet_pop and explosion) and spawns them randomly on screen. See the `customsprite` sample for the runnable version.

```c
#include <cute.h>

int main(int argc, char* argv[])
{
    cf_make_app("Custom Sprite", 0, 0, 0, 640, 480,
        CF_APP_OPTIONS_WINDOW_POS_CENTERED_BIT, argv[0]);

    // Load PNG frames.
    CF_Png bullet_pngs[3];
    cf_custom_sprite_load_png("/bullet_pop1.png", &bullet_pngs[0]);
    cf_custom_sprite_load_png("/bullet_pop2.png", &bullet_pngs[1]);
    cf_custom_sprite_load_png("/bullet_pop3.png", &bullet_pngs[2]);

    CF_Png explosion_pngs[5];
    for (int i = 0; i < 5; i++) {
        char path[32];
        snprintf(path, sizeof(path), "/explosion%d.png", i + 1);
        cf_custom_sprite_load_png(path, &explosion_pngs[i]);
    }

    // Create animations with per-frame delays (seconds).
    float bullet_delays[] = { 0.05f, 0.05f, 0.05f };
    float explosion_delays[] = { 0.05f, 0.05f, 0.05f, 0.05f, 0.05f };
    const CF_Animation* bullet_anim = cf_make_custom_sprite_animation(
        "bullet_pop", bullet_pngs, 3, bullet_delays, 3);
    const CF_Animation* explosion_anim = cf_make_custom_sprite_animation(
        "explosion", explosion_pngs, 5, explosion_delays, 5);

    // Build an animation table and create the sprite.
    const CF_Animation* anims[] = { bullet_anim, explosion_anim };
    const CF_AnimationTable* table = cf_make_custom_sprite_animation_table(
        "firework", anims, 2);
    CF_Sprite sprite = cf_make_custom_sprite("firework", table);

    // Play a one-shot animation.
    cf_sprite_play(&sprite, "explosion");
    sprite.loop = false;

    while (cf_app_is_running()) {
        cf_app_update(NULL);
        cf_sprite_update(&sprite);

        if (!sprite.finished) {
            cf_draw_sprite(&sprite);
        }

        cf_app_draw_onto_screen(true);
    }

    cf_destroy_app();
    return 0;
}
```

## API Reference

- [`CF_Png`](../custom_sprite/cf_png.md) -- A loaded PNG image.
- [`cf_png_defaults`](../custom_sprite/cf_png_defaults.md) -- Initialize an empty `CF_Png`.
- [`cf_custom_sprite_load_png`](../custom_sprite/cf_custom_sprite_load_png.md) -- Load a PNG from disk.
- [`cf_custom_sprite_load_png_from_memory`](../custom_sprite/cf_custom_sprite_load_png_from_memory.md) -- Load a PNG from memory.
- [`cf_custom_sprite_unload_png`](../custom_sprite/cf_custom_sprite_unload_png.md) -- Unload a PNG.
- [`cf_make_custom_sprite_animation`](../custom_sprite/cf_make_custom_sprite_animation.md) -- Create a named animation from PNGs.
- [`cf_make_custom_sprite_animation_table`](../custom_sprite/cf_make_custom_sprite_animation_table.md) -- Combine animations into a table.
- [`cf_make_custom_sprite`](../custom_sprite/cf_make_custom_sprite.md) -- Create a sprite from an animation table.
