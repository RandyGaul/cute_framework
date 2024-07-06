[](../header.md ':include')

<br>

Everything in the [Drawing API](https://randygaul.github.io/cute_framework/#/topics/drawing) is drawn relative to the current coordinate system. We can transform this coordinate system to adjust the position, scale, and rotation of anything drawn.

## Default System

In games typically the center of the screen is defined as position `(0, 0)`, with the positive y-axis going up the screen. This is the default setup in CF.

## Translating

We can move coordinate system by calling [`cf_draw_translate`](https://randygaul.github.io/cute_framework/#/draw/cf_draw_translate), which will shift the location of anything drawn thereafter. By default the coordinate system starts out at the origin `(0, 0)`. For example, call `cf_draw_translate(100, 0)` everything drawn thereafter will shift to the right by `100` units.

## Rotating

We can rotate the coordinate system by calling [`cf_draw_rotate`](https://randygaul.github.io/cute_framework/#/draw/cf_draw_rotate). A positive parameter will rotate everything drawn thereafter clockwise by a number of radians.

## Scaling

We can scale the coordinate system by calling [`cf_draw_scale`](https://randygaul.github.io/cute_framework/#/draw/cf_draw_scale). Numbers greater than 1 will zoom in, while numbers less than one will zoom out. Negative numbers will flip the screen on the x or y axis.

## Transform State

Whenever we adjust the coordinate system (by scaling, rotating, or translating) all subsequent draw commands will be drawn within the adjusted coordinate system. It's important to learn how to save coordinate systems restore them later. This allows you to draw something specific without affecting the coordinate system used by the rest of your code.

By using [`cf_draw_push`](https://randygaul.github.io/cute_framework/#/draw/cf_draw_push) and [`cf_draw_pop`](https://randygaul.github.io/cute_framework/#/draw/cf_draw_pop) you can save and restore coordinate systems. This technique lets you draw things locally without affecting anything else. Here's an example:

```cpp
cf_draw_push(); // Save the prior transform (coordinate system).
cf_draw_translate(100,0); // Shift everything drawn hereafter by 100 units.
cf_draw_sprite(sprite);
cf_draw_pop(); // Restore the previous transform.
```

## Transform Heirarchies

Whenever we adjust the coordinate system CF internally concatenates each prior adjustment, forming a series of translations/scales/rotations.

It's important to note that the order in which you call these functions matters. Take for example a rotation and a translation.

```cpp
cf_draw_rotate(CF_PI/4);
cf_draw_translate(50,0);
cf_draw_box(cf_make_aabb(cf_v2(-5,-5),cf_v2(5,5)),1,0.5);
```

In the above example the box will be drawn rotated about the origin by 45 degrees clockwise, or, towards the bottom-right corner of the screen. However, if we flip the order of translate/rotate like so:

```cpp
cf_draw_translate(50,0);
cf_draw_rotate(CF_PI/4);
cf_draw_box(cf_make_aabb(cf_v2(-5,-5),cf_v2(5,5)),1,0.5);
```

The box will be drawn at `(50,0)` but will itself be rotated locally by 45 degrees.
