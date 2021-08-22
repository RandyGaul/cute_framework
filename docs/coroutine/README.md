# Coroutines

A [coroutine](https://en.wikipedia.org/wiki/Coroutine) is a function that can be paused and resumed at a later time. When paused all local variables, along with the [stack frame + call stack](https://en.wikipedia.org/wiki/Call_stack), are preserved. For a very quick description of coroutines I highly recommend checking out the [Pico8 documentation](https://pico-8.fandom.com/wiki/Cocreate) if you are a complete beginner.

## API List

[coroutine_make](https://github.com/RandyGaul/cute_framework/blob/master/docs/coroutine/coroutine_make.md)  
[coroutine_destroy](https://github.com/RandyGaul/cute_framework/blob/master/docs/coroutine/coroutine_destroy.md)  
[coroutine_resume](https://github.com/RandyGaul/cute_framework/blob/master/docs/coroutine/coroutine_resume.md)  
[coroutine_yield](https://github.com/RandyGaul/cute_framework/blob/master/docs/coroutine/coroutine_yield.md)  
[coroutine_wait](https://github.com/RandyGaul/cute_framework/blob/master/docs/coroutine/coroutine_wait.md)  
[coroutine_state](https://github.com/RandyGaul/cute_framework/blob/master/docs/coroutine/coroutine_state.md)  
[coroutine_get_udata](https://github.com/RandyGaul/cute_framework/blob/master/docs/coroutine/coroutine_get_udata.md)  
[coroutine_push](https://github.com/RandyGaul/cute_framework/blob/master/docs/coroutine/coroutine_push.md)  
[coroutine_pop](https://github.com/RandyGaul/cute_framework/blob/master/docs/coroutine/coroutine_pop.md)  
[coroutine_bytes_pushed](https://github.com/RandyGaul/cute_framework/blob/master/docs/coroutine/coroutine_bytes_pushed.md)  
[coroutine_space_remaining](https://github.com/RandyGaul/cute_framework/blob/master/docs/coroutine/coroutine_space_remaining.md)  
[coroutine_currently_running](https://github.com/RandyGaul/cute_framework/blob/master/docs/coroutine/coroutine_currently_running.md)  

## Why use Coroutines?

Why do coroutines matter? Is it some kind of multi-threading, or networking thing?

The straightforward answer is: **Coroutines have nothing to do with multithreading or concurrency or networking**. Sure, it's possible to implement some interesting concurrent abstractions with coroutines, but that's not where the _real value lies_. The point of including coroutines in CF is for streamlining state machines. Traditional state machines have a few problems.

1. State machines are complicated, take a long time to develop, and are bug prone.
2. They break code flow by jumping around, like switches or if statements. Following along and reading the code gets difficult since relevant sections are far away from each other in the source file.
3. Storing persistant variables in the state machine is really tough.

With some time investment on the reader's part to learn some new concepts about coroutines, a different way to implement state machines might be right around the corner. The easier it is to create state machines the more features can get into a game project, the more one-off cutscenes, sequences, and animations can exist.

## Overview of this Article

The best way to get into the real meat of coroutines, after the basics are grasped (have you read the [Pico8 article](https://pico-8.fandom.com/wiki/Cocreate) on coroutines yet?), is to go through transforming a full example of a complex state machine written in a traditional manner to one written with a coroutine.

Instead of talking about a full example in the context of a game, which [Elias Daler has done wonderfully here](https://eliasdaler.github.io/how-to-implement-action-sequences-and-cutscenes/), a very tight example has been chosen of making a game editor tool. Editors are typically very complex and riddled with little bits of state machines everywhere. The lessons to learn from the rest of the document are the following points.

1. Reducing extreme code duplication for storing state outside the state machine, like ad-hoc stacks or persistant data.
2. Rewriting the code so it reads like a script from a play. The state machine should list actions in chronological order: Do task A, then task B, then task C, etc. A traditional state machine cuts up the tasks and jumps around with switches or if-else chains, breaking code flow.
3. Reduce the amount of code and boilerplate needed to added new state machines.

## State Machines and Stacks

In C the typical state machine comprises of a loop and some branching (if-statements or a switch). The state machine must be entered and exited once per game tick, and so all local variables of the state machine's functions are not meant to persist beyond the lifetime of one game tick. Let use an example problem to discuss state machines.

Imagine we are making a tool to construct collision geometry for a game. It can support a variety of shape types: circle, capsule, aabb, polygon, and polyline are all supported. Undo/redo support is required, along with copy + paste. Implementing this tool from scratch will require some kind of state machines. Here is how building a circle should work.

![screenshot 0](https://github.com/RandyGaul/cute_framework/blob/master/screenshots/cf_sge_0.gif?raw=true)

It works by recording a down-click, tracking the mouse movement, then recording when the button is released, thus defining the circle center and radius. How might this be implemented with a typical state machine?

```c
void build_circle(editor_t* e)
{
	switch (e->state) {
	case EDITOR_STATE_BUILD_CIRCLE_START:
	{
		push_cursor(CURSOR_CROSSHAIRS);
		e->build.circle.p = mouse_pos();
		e->build.circle.r = 0;
		e->state = EDITOR_STATE_BUILD_CIRCLE_DRAG;
	}	break;

	case EDITOR_STATE_BUILD_CIRCLE_DRAG:
	{
		if (mouse_was_released(MOUSE_BUTTON_LEFT)) {
			pop_cursor();
			editor.add_circle(e->build.circle);
			e->state = e->state_prev;
		} else {
			float d = len(e->build.circle.p - mouse_pos());
			e->build.circle.r = d;
		}
	}	break;
	}
}
```

Not too bad, right? Fairly readable. There are two states, one for each mouse-click action (down, and then up). However, one detail in this implementation slides along easily unnoticed. I'll mark the dubious lines in the next code snippet.

```c
void build_circle(editor_t* e)
{
	switch (e->state) { // SUSPICIOUS
	case EDITOR_STATE_BUILD_CIRCLE_START:
	{
		push_cursor(CURSOR_CROSSHAIRS); // SUSPICIOUS
		e->build.circle.p = mouse_pos();
		e->build.circle.r = 0;
		e->state = EDITOR_STATE_BUILD_CIRCLE_DRAG;
	}	break;

	case EDITOR_STATE_BUILD_CIRCLE_DRAG:
	{
		if (mouse_was_released(MOUSE_BUTTON_LEFT)) {
			pop_cursor(); // SUSPICIOUS
			editor.add_circle(e->build.circle);
			e->state = e->state_prev; // SUSPICIOUS
		} else {
			float d = len(e->build.circle.p - mouse_pos());
			e->build.circle.r = d;
		}
	}	break;
	}
}
```

The `build_circle` function contains no persistant local state, so all notion of state has to be stored elsewhere. The appearance of multiple different stack-like data structures in the snippet can not be explained by mere coincidence. The cursors type pops onto a stack in order to remember what cursor existed prior. The state of the editor itself must be restored, and so the prior state in `state_prev` sits around waiting to be resumed.

These little stacks are typically implemented ad-hoc whenever needed. The programmer burns their time continually reimplementing the same data structure over and over, endlessly "bit-twidling" along.

Not only programmer effort becomes wasted, but the state machine `build_circle` itself rises in complexity - it must refer to variables that live far away on the `editor_t` instance. It must remember what happened the last time it was called. The [execution pointer](https://en.wikipedia.org/wiki/Program_counter) must jump around with a switch statement, or if-else chain, to resume where it last ran.

## The Right Tool for the Job

Thinking of a coroutine a bit more like a stack data structure, the `build_circle` function can be reimplemented with the help of one special function: [coroutine_yield](https://github.com/RandyGaul/cute_framework/blob/master/docs/coroutine/coroutine_yield.md). `coroutine_yield` pauses the function for later resuming.

The dubious parts of the last section can be changed to use local variables, replacing ad-hoc stack implementations with local variables on the coroutine's local stack.

```c
editor_t* e;

void build_circle(coroutine_t* co)
{
	cursor_t prev_cursor = get_cursor();
	set_cursor(CURSOR_CROSSHAIRS);
	e->is_building = true;
	e->build.type = SHAPE_TYPE_CIRCLE;
	e->build.circle.p = mouse_pos();
	while (!mouse_was_released(MOUSE_BUTTON_LEFT)) {
		float d = len(e->build.circle.p - mouse_pos());
		e->build.circle.r = d;
		coroutine_yield(co);
	}
	editor.add_circle(e->build.circle);
	e->is_building = false;
	set_cursor(prev_cursor);
}
```

Now the function `build_circle` reads more like a script from a play. Starting from the top of the function we can easily read each line of code, one will happen after the next. Contrasted against the previous implementation with switch statements or if-else statements, the code flow is sliced up and complicated. With the coroutine we can see once `build_circle` is called the editor is now building a circle, and only once `build_circle` returns will the editor finish building. A flag is set `is_building` to allow other parts of the code to read this flag and draw the circle as it's constructed.

The state enumerations `EDITOR_STATE_BUILD_CIRCLE_START` and `EDITOR_STATE_BUILD_CIRCLE_DRAG` no longer need to exist. The function `build_circle` *is the state*, so state enums are not necessary.

## What about a Complicated Use-Case?

The circle building example lacks complexity. Let us implement a more complicated interaction with more states, and see what it might look like with coroutines. Here is a clip of a convex polygon builder and editing tools.

![screenshot 1](https://github.com/RandyGaul/cute_framework/blob/master/screenshots/cf_sge_1.gif?raw=true)

First, an example implementation for the polygon construction function.

```c
void build_poly(coroutine_t* co)
{
	e->is_building = true;
	e->build.type = SHAPE_TYPE_POLY;
	poly_t* poly = &sge.build.poly;
	poly->count = 2;
	poly->verts[0] = mouse_pos();
	cursor_t prev_cursor = get_cursor();
	set_cursor(CURSOR_CROSSHAIRS);
	while (1) {
		v2 mp = mouse_pos();
		poly->verts[poly->count - 1] = mp;
		
		// Stop cosntruction on right-click.
		if (mouse_was_pressed(MOUSE_BUTTON_RIGHT)) {
			poly->count = max(0, poly->count - 1);
			break;
		}
		
		// Left click to place a new vertex.
		if (mouse_was_pressed(MOUSE_BUTTON_LEFT)) {
			bool hit_vert = false;
			for (int i = 0; i < poly->count - 1; ++i) {
				if (len(poly->verts[i] - mp) < e->skin_factor) {
					hit_vert = true;
					break;
				}
			}
			
			// Only place a new vertex if it's far enough away from all the
			// other vertices.
			if (!hit_vert) {
				poly->count = min(CUTE_POLY_MAX_VERTS, poly->count + 1);
				poly->verts[poly->count - 1] = mp;
			}
		}
		coroutine_yield(co);
	}
	
	// Convexify the points as a convex hull.
	poly->count = hull(poly->verts, poly->count);
	if (poly->count) {
		norms(poly->verts, poly->norms, poly->count);
		sge_add_poly(sge.build.shape.shape.u.poly);
	}
	set_cursor(prev_cursor)
	e->is_building = false;
}
```

Once again we can read this function like a script. It lays down, in chronological order, the sequence of things that happen. Right click to end construction, left click to place a vert, the placed verts are handed to a convex hull function called `hull`. It's all right here. No other state regarding the construction of a polygon exists anywhere but within this single function. It's all localized. No switches needed, no storing unnecessary variables in external objects.

What about editing the polygon after it's fully constructed? There are a few interactions going on here.

1. Click + drag a vertex.
2. Click + drag a face.
3. Click + drag the entire polygon.

One way to implement these three interactions is with an if-statement branching on the initial left-click of the mouse. Did the click land on a vertex? Go to step 1. Did the click land on a face? Go to step 2. And so on.

Each step can be implemented with a single while loop utilizing `coroutine_yield`. For example, here's a snippet implementing the face dragging functionality.

```c
v2 mp_before = mouse_pos();
poly_t poly_before = e->shapes[shape_index].u.poly;
int face_index = closest_face(poly_before.verts, poly_before.count, mp);
if (face_index != -1) {
	while (!mouse_is_up(MOUSE_BUTTON_LEFT)) {
		int i = face_index;
		int j = i + 1 == poly_before.count ? 0 : i + 1;
		float expand_factor = dot(mp - mouse_pos(), poly_before.norms[i]);
		e->shapes[shape_index].u.poly = expand_face(poly_before, face_index, expand_factor);
		coroutine_yield(co);
	}
}
```

## Thinking Like a Coroutine

Coroutines are admittedly weird. Thinking of how to implement things with a coroutine feels a bit odd, though becomes much easier with some practice. Luckily there are two communities who have embraced coroutines. Each community has made quite a lot of online content, including tutorials, which can help get us up to speed on thinking with coroutines.

1. [Unity/C# with coroutines](https://docs.unity3d.com/Manual/Coroutines.html).
2. [Coroutines from the Lua programming language](https://www.lua.org/pil/9.1.html).

Elias Daler wrote a fantastic blog post on using [coroutines to implement cutscenes](https://eliasdaler.github.io/how-to-implement-action-sequences-and-cutscenes/). Check it out! He has some absolutely beautiful gifs.

Small functions can be reused many times in different coroutines to perform common tasks, such as waiting for a number of seconds, walking along a path, or playing a sequence of animations.

Here's a univeristy [guest lecture about Coroutines at DigiPen IT](https://github.com/RandyGaul/kk_slides) source code, where the entire slideshow is written with coroutines to implement timers, colors, slide functions, and typewriter effects.

## Coroutine Stack Size

Coroutines are rather lightweight and don't consume too many resources. Calling into a coroutine (or yielding from) is more expensive than calling a normal function, but much less expensive than a full-blown [context switch](https://en.wikipedia.org/wiki/Context_switch) for comparison. Each coroutine in CF is given a stack, as allocated on the heap, to store it's local variables and encompassing call stack. In CF the default size of each coroutine is roughly 64kb, but can be set much lower. This means a thousand coroutines will take up about 64mb of RAM on default settings.

If your game only has about 100 active entities at any given time, where each one makes use of a coroutine for state machines, it comes to about 6MB of memory total on default settings.

The more shallow the stack size means the risk of a [stack overflow](https://en.wikipedia.org/wiki/Stack_Overflow) is more likely. However, since the coroutine stack is on the heap the stack overflow will cause heap corruption, which can be quite a bit trickier to diagnose as opposed to a traditional stack overflow, where the usual debug mechanisms catch and report stack overflows reliably. You've been warned.
