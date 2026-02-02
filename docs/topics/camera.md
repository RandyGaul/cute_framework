# Camera

Everything in the [Drawing API](../topics/drawing.md) is drawn relative to the current coordinate system. We can transform this coordinate system to adjust the position, scale, and rotation of anything drawn.

## Coordinate Systems

CF uses two different coordinate systems: **world space** and **screen space**.

### World Space (Drawing)

This is the coordinate system used by all drawing functions.

- **Origin**: Center of the screen `(0, 0)`
- **X-axis**: Positive values go **right**
- **Y-axis**: Positive values go **up**

```
                 +Y
                  ^
                  |
                  |
       -X <-------+-------> +X
                  |
                  |
                  v
                 -Y
```

### Screen Space (Input)

This is the coordinate system used by mouse and touch input functions like [`cf_mouse_x`](../input/cf_mouse_x.md) and [`cf_mouse_y`](../input/cf_mouse_y.md).

- **Origin**: Top-left corner of the screen `(0, 0)`
- **X-axis**: Positive values go **right**
- **Y-axis**: Positive values go **down**

```
       (0,0)+--------------> +X
            |
            |
            |
            |
            v
           +Y
```

### Converting Between Coordinate Systems

To convert mouse coordinates to world space for game logic, use [`cf_screen_to_world`](../draw/cf_screen_to_world.md):

```cpp
CF_V2 mouse_world = cf_screen_to_world(cf_v2((float)cf_mouse_x(), (float)cf_mouse_y()));
```

To convert world coordinates to screen space, use [`cf_world_to_screen`](../draw/cf_world_to_screen.md):

```cpp
CF_V2 screen_pos = cf_world_to_screen(world_position);
```

You can also get the visible screen bounds in world space with [`cf_screen_bounds_to_world`](../draw/cf_screen_bounds_to_world.md).

## Translating

We can move coordinate system by calling [`cf_draw_translate`](../draw/cf_draw_translate.md), which will shift the location of anything drawn thereafter. By default the coordinate system starts out at the origin `(0, 0)`. For example, call `cf_draw_translate(100, 0)` everything drawn thereafter will shift to the right by `100` units.

## Rotating

We can rotate the coordinate system by calling [`cf_draw_rotate`](../draw/cf_draw_rotate.md). A positive parameter will rotate everything drawn thereafter clockwise by a number of radians.

## Scaling

We can scale the coordinate system by calling [`cf_draw_scale`](../draw/cf_draw_scale.md). Numbers greater than 1 will zoom in, while numbers less than one will zoom out. Negative numbers will flip the screen on the x or y axis.

## Transform State

Whenever we adjust the coordinate system (by scaling, rotating, or translating) all subsequent draw commands will be drawn within the adjusted coordinate system. It's important to learn how to save coordinate systems restore them later. This allows you to draw something specific without affecting the coordinate system used by the rest of your code.

By using [`cf_draw_push`](../draw/cf_draw_push.md) and [`cf_draw_pop`](../draw/cf_draw_pop.md) you can save and restore coordinate systems. This technique lets you draw things locally without affecting anything else. Here's an example:

```cpp
cf_draw_push(); // Save the prior transform (coordinate system).
cf_draw_translate(100,0); // Shift everything drawn hereafter by 100 units.
cf_draw_sprite(sprite);
cf_draw_pop(); // Restore the previous transform.
```

These push/pop pairs are the recommended pattern for drawing. You may also nest push/pop pairs within each other. Conceptually this forms a tree-like relationship between coordinate systems.

```cpp
cf_draw_push();
// Draw some things...
	cf_draw_push();
	// Draw things relative to the previous coordinate system.
	// ...
	cf_draw_pop();
cf_draw_pop();
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
