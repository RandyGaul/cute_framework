# Collision

CF comes with a full-featured [`Collision API`](../api_reference.md#collision). Here's a quick list of the features available:

- Shapes (circle, capsule, Aabb, ray, poly)
- Boolean collision functions (simply yes/no hit tests)
- Manifold functions (tells _how_ shapes touch, more complex than a pure yes/no hit-test)
- Raycasting
- Closest points between two shapes
- Time of impact between two moving shapes
- Convex hull algorithm

Testing for collision between two shapes looks like so:

```cpp
CF_Circle c;
c.p = position;
c.r = radius;

CF_Capsule cap;
cap.a = first_endpoint;
cap.b = second_endpoint;
cap.r = radius;

bool hit = cf_circle_to_capsule(c, cap);
if (hit) {
	handle collision here...
}
```

## Shapes

Here are the various shapes available:

- [`CF_Circle`](../math/cf_circle.md)
- [`CF_Capsule`](../collision/cf_capsule.md)
- [`CF_Aabb`](../math/cf_aabb.md)
- [`CF_Ray`](../math/cf_ray.md)
- [`CF_Poly`](../collision/cf_poly.md)

## Polygons

The [`CF_Poly`](../collision/cf_poly.md) shape is a bit unique -- it must be a proper [convex hull](https://en.wikipedia.org/wiki/Convex_hull) with the vertices ordered in counter-clockwise format. Luckily you may call [`cf_hull`](../collision/cf_hull.md) to calculate a convex hull from a given set of un-ordered points for you.

One last gotcha: polygons may only have up to [`CF_POLY_MAX_VERTS`](../collision/cf_poly_max_verts.md). It’s quite common for games to limit the number of verts on polygons to a low number. Higher than 8 and shapes generally start to look more like circles/ovals; it becomes pointless beyond a certain point (haha, shape joke).

## Boolean Hit Testing

In the [Collision API](../api_reference.md#collision) we can see a whole bunch of functions. The ones that look like `cf_***_to_***` (where `***` is a shape name) are the boolean result tests. Simply pass in shapes. If the shapes are touching, true is returned.

## Generic Collision Function

You may call [`cf_collide`](../collision/cf_collide.md) to perform a generic boolean hit-test on any two shapes using `void*`'s. Simply pass in pointers to each shape along with the associated [`cf_shapetype`](../collision/cf_shapetype.md) for each shape. Internally this just runs some switch statements on each shape type and calls the correct `cf_***_to_***`.

Similarly you can call [`cf_collided`](../collision/cf_collided.md) for a generic manifold function.

You can of course create your own versions of these "generic" functions if you wish -- they are here merely for convenience.

## Manifold Testing

[`CF_Manifold`](../collision/cf_manifold.md) is a special struct containing all the information necessary to separate shapes that are touching. To generate a manifold call one fo the functions `cf_***_to_***_manifold`. The manifold will contain 1 or 2 vertices if the shapes intersected, and 0 if the shapes do not intersect.

```cpp
struct CF_Manifold
{
	int count;               // The number of points in the manifold (0, 1 or 2).
	float depths[2];         // The collision depth of each point in the manifold.
	CF_V2 contact_points[2]; // Up to two points on the contact plane that suff-
	                         // iciently (and minimally) describe how two shapes touch.
	CF_V2 n                  // Always points from shape A to shape B.;
};
```

## Raycasting

A mathemetical ray is like a line, starting at one point and extending in a single direction infinitely. In CF rays are not of infinite length. An infinite ray wastes computational resources when used in games. Instead, a ray is defined as a line segment with a start and end position.

```cpp
struct CF_Ray
{
	CF_V2 p; // Position.
	CF_V2 d; // Direction (normalized).
	float t; // Distance along d from position p to find endpoint of ray.
};
```

Rays are stored in parametric form. A start position defines the initial point the ray is cast from. A normalized direction vector and distance along that vector define the end point: `endpoint = ray.p + ray.d * t`. If needed, a helper function called [`cf_endpoint`](../collision/cf_endpoint.md) can be used to calculate this point.

!> It's extremely important to **normalize** your ray direction. If you fail to normalize your ray direction the internal math algorithms can numerically fail due to sensitivity to large vectors in certain cases.

To cast a ray simply call one of the `cf_ray_to_***` functions:

- [`cf_ray_to_aabb`](../collision/cf_ray_to_aabb.md)
- [`cf_ray_to_capsule`](../collision/cf_ray_to_capsule.md)
- [`cf_ray_to_circle`](../collision/cf_ray_to_circle.md)
- [`cf_ray_to_halfpsace`](../collision/cf_ray_to_halfspace.md)
- [`cf_ray_to_poly`](../collision/cf_ray_to_poly.md)

Each of these functions will return true if the ray hits the given shape, and also fill in the `out` parameter, a [`CF_Raycast`](../math/cf_raycast.md) struct. The raycast contains the results for a raycast.

```cpp
struct CF_Raycast
{
	float t; // Time of impact.
	CF_V2 n; // Normal of surface at impact (unit length).
};
```

We can easily calculate the position at the point of impact by using the parametric equation for the ray, and plugging in `t` from the raycast result: `impact = ray.p + ray.d * raycast.t`. If needed, a helper function called [`cf_impact`](../collision/cf_impact.md) can calculate the hit-spot for us.

The `n` member of the raycast represents the normal vector at point of impact. It points perfectly perpendicular to the surface of the shape. For example, this can be useful for knowing what sort of slope a player is standing on, or how to reflect bullets off of a surface.

A "generic" raycast function [`cf_cast_ray`](../collision/cf_cast_ray.md) can be used to cast a ray against a generic shape using an enum and `void*` style polymorphism. Simply pass in a pointer to your shape and the according [`cf_shapetype`](../collision/cf_shapetype.md). Internally it's just a small function with a switch statement to call the proper `cf_ray_to_***` function.

## Closest Points

Sometimes it's quite useful to calculate the closest points between two shapes. Sometimes this is needed to implement other algorithms that require a good distance or direction check.

Two calculate the closest points between two _non-intersecting_ shapes you may call [`cf_gjk`](../collision/cf_gjk.md), which stands for the algorithm creators Gilbert-Johnson-Keerthi. The algorithm only works on _non-intersecting_ shapes, if the two shapes are already touching you will get a false result, and the closest points will not be well defined.

This function is generic and requires the use of `void*` + [`cf_shapetype`](../collision/cf_shapetype.md) calling, much like the previously mentioned generic functions.

!> Large shapes will degrade the performance of [`cf_gjk`](../collision/cf_gjk.md) and can cause bugs. It's highly recommended to use smaller shapes (with a volume preferably < 1000 units. If you need big shapes you should instead store all of your physics shapes in small form and simply render them at a much larger size.

## Swept Collision

The purpose of swept collision is to prevent tunneling. Tunneling is when a shape moves so fast the collision check from one frame to another completely misses, and shapes can fly through each other as a result. One solution to tunneling is to use swept collision checks. All the other collision functions mentioned in this article (besides gjk) are called _discrete collision_.

You may calculate the time of impact between two _linearly moving_ shapes (as in, no rotation allowed) with [`cf_toi`](../collision/cf_toi.md). This is a pretty advanced function, so be careful about reading the documentation page on it ([same as last link](../collision/cf_toi.md))! By calculating the time of impact you can implement a swept collision algorithm, perhaps like the one described in the previous links.

## Broadphase

All of the previous functions mentioned operate on pairs of shapes. Often times the pairwise collision functions are called the _narrow phase_ of a collision engine. When we have N shapes in the game, testing all shapes against all other shapes results in quadratic time complexity, or N^2 collision checks. Beyond about 200 to 300 shapes this time complexity usually starts to become much too slow. Instead, a different algorithm, sometimes called _the broadphase_ can cut down the time complexity.

CF implements a dymamic Aabb tree, [`CF_AabbTree`](../collision/cf_aabbtree.md), ideal for many games as a broadphase. To use the tree wrap all your shapes up in an Axis Aligned Bounding Box (Aabb, it means a rectangle that doesn't rotate). Insert the Aabb's into the tree. The tree will then organize the 2D space all the Aabb's occupy, and use an accelerated algorithm that runs in `Log(N) * N` time complexity -- much faster and scales well to many thousands of shapes.

After all the shapes are placed into the tree, the tree can be queried to find all inserted Aabb's that overlap the query. By doing so, a list of potential pairs is generated. Each potential pair can then be tested with _the narrowphase_ (a function from earlier in this article, such as [`cf_collide`](../collision/cf_collide.md)).
