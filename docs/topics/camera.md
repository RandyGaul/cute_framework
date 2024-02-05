[](../header.md ':include')

<br>

Make sure to read the [Drawing](https://randygaul.github.io/cute_framework/#/topics/drawing) topic before reading about the camera. The various drawing functions are all drawn relative to a small camera API. The camera can look at a specific position, rotate itself, and also adjust it's view scale. Use the camera draw from different perspectives, such as following along as a player moves, or moving along paths during cut scenes.

## Default Camera

When your app window is first created a default camera is automatically setup for you. It's dimensions are the same as the resolution of the drawable area of the window. For example, if you follow along with the [Getting Started](https://randygaul.github.io/cute_framework/#/getting_started) page the window will have pixel dimensions of 640x480. The camera copies those dimensions and scales itself to a width of 640 and height of 480. You can adjust the camera's scale with [`cf_camera_dimensions`](https://randygaul.github.io/cute_framework/#/camera/cf_camera_dimensions).

```cpp
float screen_w = 640.0f;
float screen_h = 480.0f;

// Zoom the camera in by 2x
cf_camera_dimensions(screen_w * 0.5f, screen_h * 0.5f);
```

?> You can flip the screen on the x-axis by negating the camera width when calling [`cf_camera_dimensions`](https://randygaul.github.io/cute_framework/#/camera/cf_camera_dimensions). You can also flip the screen on the y-axis by negating the second parameter, the camera height.

## Coordinate System

In games typically the center of the screen is defined as position `(0, 0)`, with the positive y-axis going up the screen.

## Moving the Camera

We can move the camera by calling [`cf_camera_look_at`](https://randygaul.github.io/cute_framework/#/camera/cf_camera_look_at), which will shift the location of anything drawn thereafter. By default the camera starts out looking at the origin `(0, 0)`. For example, if we make the camera look at `(100, 0)` everything drawn thereafter will shift to the left by `100` units.

## Rotating the Camera

The camera itself can be rotated with [`cf_camera_rotate`](https://randygaul.github.io/cute_framework/#/camera/cf_camera_rotate). A positive parameter will rotate counter-clockwise by a number of radians. This means everything on the screen will appear to rotate counter-clockwise.

## Camera Push and Pop

The camera comes along with [`cf_camera_push`](https://randygaul.github.io/cute_framework/#/camera/cf_camera_push) and [`cf_camera_pop`](https://randygaul.github.io/cute_framework/#/camera/cf_camera_pop). Internally CF has a stack of cameras. Whenever you call push it places a copy of the camera onto the stack. This is great for looking at all the various objects in your game and drawing them at their respective locations. Each time you want to visit a new location you may push the old camera, draw your object, then pop the camera. When popping the previously pushed camera is restored.

This kind of push/pop control is ideal for drawing anything heirarchical, such as parented game objects, enemies around a level, bits of user-interface elements that should move all together, etc.

## Getting the Camera

You may peek at the current camera with [`cf_camera_peek_position`](https://randygaul.github.io/cute_framework/#/camera/cf_camera_peek_position), [`cf_camera_peek_dimensions`](https://randygaul.github.io/cute_framework/#/camera/cf_camera_peek_dimensions), or [`cf_camera_peek_rotation`](https://randygaul.github.io/cute_framework/#/camera/cf_camera_peek_rotation). The values returned are always from the last camera to have been setup by any other camera functions. If [`cf_camera_pop`](https://randygaul.github.io/cute_framework/#/camera/cf_camera_pop) is called the previously used camera will be restored, changing the return values of all the peek functions.

## Drawing Game Objects

Let's look at a quick example of using camera push/pop to draw all of our game objects. Say we have an array of trees and want to draw all of them. What we can do is have the camera look at the negated position of each tree, and then draw the tree sprite.

```cpp
for (int i = 0; i < trees.size(); ++i)
{
	Tree tree = trees[i];
	camera_look_at(-tree.position.x, -tree.position.y);
	draw_sprite(tree.sprite);
}
```

This works, however one small caveat is that the function [`cf_camera_look_at`](https://randygaul.github.io/cute_framework/#/camera/cf_camera_look_at) sets CF's global camera and overwrites the previous value. A different way to design your code could make use of a heirarchy of positions. Games that need complex characters or multi-piece animations can make use of this kind of heirarchy, also known as a skeletal heirarchy, with parents and children. We can use the push/pop functions to traverse the heirarchy as a stack.

```cpp
void DrawObject(Object* object)
{
	// Draw this object.
	draw_sprite(object->sprite);

	// Visit each child.
	v2 p = camera_peek_position();
	for (int i = 0; i < object->children.size(); ++i)
	{
		Object* child = object->children[i];

		// Push child position onto the camera stack.
		camera_push();
		camera_look_at(p.x - child->position.x, p.y - child->position.y);
		DrawObject(child);
		camera_pop();
	}
}
```

## Why Negative?

A common technique for rendering with a camera: negate the positions as we call [`cf_camera_look_at`](https://randygaul.github.io/cute_framework/#/camera/cf_camera_look_at).

Whenever we submit draw calls, such as [`cf_draw_sprite`](https://randygaul.github.io/cute_framework/#/draw/cf_draw_sprite), the current camera is used as the reference frame for drawing. This means whatever the camera is centered upon will be drawn at (0,0) on the screen. For example, let us start the camera at (0,0) and draw a sprite. The sprite will get rendered at the center of the screen.

What if we want to draw the sprite at (10,0), ten units to the right? If we call `cf_camera_look_at(10, 0)` this will move the camera ten units to the right, while the sprite is still at location (0,0). Since the camera is on the right of the sprite, the sprite gets drawn to the left of the camera. Whatever the camera looks at is always centered on the screen, so if we want to have the sprite show up ten units the right, we must call `cf_camera_look_at(-10, 0)`.
