# Game Loop and Time

The [`Time API`](../api_reference.md#time) is all about updating the game and controlling the game's main loop. There are two styles: variable timestep (default) and fixed-timestemp. If you're a beginner then read just the next section on Variable Timestep (and the [Time Utilities](../topics/game_loop_and_time.md#time-utilities) section). The rest of the document covers more advanced use cases.

## Variable Timestep

For many games variable timestep is a good choice. This means the game-tick runs as fast as possible and updates once per visually rendered frame. Each game-tick runs for a slightly variable time duration, since the hardware state changes each frame, and the game itself changes each frame. A time-slice is used to update everything in the game, usually called `dt` or  _delta time_.

> [!NOTE]
> It's not necessary to actually pass the `dt` variable around to all your functions. Instead you can make use of the global variable [`CF_DELTA_TIME`](../time/cf_delta_time.md).

The time-slice is usually a small value, such as (1/60) or 0.0167. We can use this delta time to update positions, velocities, animations, or anything else in the game.

> [!NOTE]
> The delta time for a given frame can't actually be computed, since before we simulate the frame we won't know exactly how long it takes. As a workaround employed by basically all games in existence, we can use the previous frame's delta time for the current frame, assuming it will last roughly as long.

Usually you want to choose a variable timestep if you don't need any [_determinism_](../topics/game_loop_and_time.md#determinism) in your game, or if you aren't running any kind of _physics simulation_. In CF variable timestep is used by default when you call [`cf_app_update`](../app/cf_app_update.md) and pass `NULL` in as the parameter.


## Time Utilities

There are a few other extremely useful time-related utilities in CF to be aware of.

- [`cf_on_interval`](../time/cf_on_interval.md)
- [`cf_between_interval`](../time/cf_between_interval.md)
- [`cf_pause_for`](../time/cf_pause_for.md)

The first two are super useful "events", while the second [`cf_pause_for`](../time/cf_pause_for.md) actually pauses all global updates of time. The application loop will still execute, but any callbacks to [`CF_OnUpdateFn`](../time/cf_onupdatefn.md), as well as updates to [`CF_DELTA_TIME`](../time/cf_delta_time.md) and friends will be paused.

[`cf_on_interval`](../time/cf_on_interval.md) can be placed into an if-statement, and will return true once a certain amount of time elapses, once per interval of time. This is great for anything you want to happen on a frequency, perfect for implementing ai routines, spawners, etc.

```cpp
if (cf_on_interval(1.0f)) {
	// Every one seconds do something.
}
```

Similarly, [`cf_between_interval`](../time/cf_between_interval.md) will return true for the full interval, then false for one full interval, alternating.

```cpp
if (cf_between_interval(1.0f)) {
	// True for one second, then false for one second.
}
```

## Target Framerate

CF has a nifty function called [`cf_set_target_framerate`](../time/cf_set_target_framerate.md) to try and control how often the application renders. This is _NOT_ the same as the next section on *fixed timestep*. This option will only affect how often the game renders by applying sleeps under the hood. This is a great way to reduce energy consumption and prevent excessive draining of user batteries or electricity. This can effectively pick a target rendering framerate, sort of like vsync. A good number to start with is 60 frames per second.

## Fixed Timestep

Fixed timestep updates upon a _specific frequency_, as opposed to variable timestep at _some_ frequency. The main benefit is _determinism_, which means to get the same results given the same inputs. Determinism is essential for games that want to things like replays, certain kinds of network synchronization, and in general have predictable or repeatable behavior. For example many platformer games will make sure the game is deterministic so players can achieve very high levels of precision while practicing.

Fixed timestep can be turned on by calling [`cf_set_fixed_timestep`](../time/cf_set_fixed_timestep.md). You will need to also implement [`CF_OnUpdateFn`](../time/cf_onupdatefn.md) and hand it to [`cf_app_update`](../app/cf_app_update.md). It will get called once per fixed-timestep.

### Render vs Update Frequency

When fixed timestep is turned on (see previous section) there are two different update frequencies. One is the application rendering frequency, while the other is the fixed timestep frequency. The default behavior is to run the application as fast as possible, while performing occasional fixed updates. The fixed updates will be issued through [`CF_OnUpdateFn`](../time/cf_onupdatefn.md). It's entirely possible for multiple fixed updates to occur within a single frame, so be prepared!

The rendering frequency can also be limited with [`cf_set_target_framerate`](../time/cf_set_target_framerate.md). Please be aware that the rendering frequency, aka the *target framerate* is not the same as the fixed update. Fixed updates happen in discrete chunks, while the rendering speed will oscillate somewhat from frame to frame.

## Determinism

With fixed timesteps you can actually make your game entirely deterministic, so long as you don't accidentally introduce non-determinism into the simulation. Here are some common sources of non-determinism:

- Variable timestep -- of course!
- Floating point math. On different CPUs the results of floating point arithmetic can vary. However, given the same hardware on two different machines you should end up with deterministic results.
- Sorting based on pointer. For example, some custom allocators may have an if-statement or sorting predicate that looks at the value of a pointer.
- Thread scheduling. Any results that are order-dependent but are reported on a first come first serve basis from multiple threads will be at the mercy of the operating system's scheduling, creating non-determinism.

Some games can get away with these constraints. For example some RTS games take advantage of popular IEEE754 floating point adherence and gain determinism on common PC setups, or on consoles (with fixed sets of hardware) -- they likely avoid complex float routines like trig functions. Other games stick to integer arithmetic on the game/simulation, and only use floats sparingly or for "visuals only". Other games achieve "mostly deterministic" outputs, and then sync over the network constantly correcting small errors as they accumulate. The exact scheme chosen is up to the developer, and the specific needs of the game.

## Rendering Interpolant

A key feature of running the application (rendering) at a different frequency than the fixed updates is to smoothly interpolate visuals to match the game/simulation state. A common setup is to use fixed timestep updates for gameplay/physics, and render at a faster frequency. For this setup the game should store two states, one for the current frame N and the previous frame N-1. Any time delta for rendering that doesn't line up on a multiple of the fixed timestep can be used as an interpolant. You should interpolate the state from N and N-1 frames to produce the current visual frame.

Failure to interpolate states will cause an unnacceptable visual jittering, as physics/gameplay take discrete steps in time and the render happens at another frequency.

This means each object/entity in your game will need to store two transforms. One for the current frame N and one for the prevous frame N-1. It's recommended to setup a function to grab an interpolated state between N and N-1 given an interpolant. You can get this interpolant from [`CF_DELTA_TIME_INTERPOLANT`](../time/cf_delta_time_interpolant.md), which represents the in-between timeslices from the application/rendering frequency and the fixed timestep frequency.

```cpp
struct MyObject
{
	v2 prev_position;
	v2 position;
	// ...
};

v2 get_interpolated_position(MyObject* object)
{
	return cf_lerp_v2(object->prev_position, object->position, CF_DELTA_TIME_INTERPOLANT);
}
```
