/*
	Cute Framework
	Copyright (C) 2020 Randy Gaul https://randygaul.net

	This software is provided 'as-is', without any express or implied
	warranty.  In no event will the authors be held liable for any damages
	arising from the use of this software.

	Permission is granted to anyone to use this software for any purpose,
	including commercial applications, and to alter it and redistribute it
	freely, subject to the following restrictions:

	1. The origin of this software must not be misrepresented; you must not
	   claim that you wrote the original software. If you use this software
	   in a product, an acknowledgment in the product documentation would be
	   appreciated but is not required.
	2. Altered source versions must be plainly marked as such, and must not be
	   misrepresented as being the original software.
	3. This notice may not be removed or altered from any source distribution.
*/

#ifndef REFLECTION_SYSTEM_H
#define REFLECTION_SYSTEM_H

#include <cute.h>
using namespace cute;

struct Transform;
struct Animator;
struct Reflection;

struct ReflectionSystem
{
	array<batch_quad_t> masks;
	array<batch_quad_t> quads;
};

extern ReflectionSystem* reflection_system;

void reflection_system_pre_update(app_t* app, float dt, void* udata);
void reflection_system_update(app_t* app, float dt, void* udata, Transform* transforms, Animator* animators, Reflection* reflections, int entity_count);
void reflection_system_post_update(app_t* app, float dt, void* udata);

#endif // REFLECTION_SYSTEM_H
