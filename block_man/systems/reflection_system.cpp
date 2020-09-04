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

#include <systems/reflection_system.h>

#include <components/transform.h>
#include <components/animator.h>
#include <components/reflection.h>

ReflectionSystem reflection_system_instance;
ReflectionSystem* reflection_system = &reflection_system_instance;

void reflection_system_pre_update(app_t* app, float dt, void* udata)
{
	ReflectionSystem* reflection_system = (ReflectionSystem*)udata;

	sg_depth_stencil_state stencil;
	CUTE_MEMSET(&stencil, 0, sizeof(stencil));
	stencil.stencil_front.fail_op = SG_STENCILOP_KEEP;
	stencil.stencil_front.depth_fail_op = SG_STENCILOP_KEEP;
	stencil.stencil_front.pass_op = SG_STENCILOP_REPLACE;
	stencil.stencil_front.compare_func = SG_COMPAREFUNC_ALWAYS;
	stencil.stencil_back = stencil.stencil_front;
	stencil.stencil_enabled = true;
	stencil.stencil_read_mask = 0xFF;
	stencil.stencil_write_mask = 0xFF;
	stencil.stencil_ref = 0x1;

	sg_blend_state blend = { 0 };
	blend.color_write_mask = SG_COLORMASK_NONE;

	// Draw masks onto the stencil buffer.
	batch_set_depth_stencil_state(batch, stencil);
	batch_set_blend_state(batch, blend);
	for (int i = 0; i < reflection_system->masks.size(); ++i) {
		batch_push(batch, reflection_system->masks[i]);
	}
	reflection_system->masks.clear();
	batch_flush(batch);
}

void reflection_system_update(app_t* app, float dt, void* udata, Transform* transforms, Animator* animators, Reflection* reflections, int entity_count)
{
	ReflectionSystem* reflection_system = (ReflectionSystem*)udata;

	for (int i = 0; i < entity_count; ++i) {
		Transform* transform = transforms + i;
		Animator* animator = animators + i;
		Reflection* reflection = reflections + i;

		// Collect all sprites that will be reflected.
		reflection_system->quads.add(animator->sprite.quad(transform->transform));
	}
}

void reflection_system_post_update(app_t* app, float dt, void* udata)
{
	ReflectionSystem* reflection_system = (ReflectionSystem*)udata;

	sg_depth_stencil_state stencil;
	CUTE_MEMSET(&stencil, 0, sizeof(stencil));
	stencil.stencil_front.fail_op = SG_STENCILOP_KEEP;
	stencil.stencil_front.depth_fail_op = SG_STENCILOP_KEEP;
	stencil.stencil_front.pass_op = SG_STENCILOP_REPLACE;
	stencil.stencil_front.compare_func = SG_COMPAREFUNC_ALWAYS;
	stencil.stencil_back = stencil.stencil_front;
	stencil.stencil_enabled = true;
	stencil.stencil_read_mask = 0xFF;
	stencil.stencil_write_mask = 0xFF;
	stencil.stencil_ref = 0x1;

	sg_blend_state blend = { 0 };
	blend.color_write_mask = SG_COLORMASK_NONE;

	// Cutout the reflection sprite from the masks.
	stencil.stencil_front.compare_func = SG_COMPAREFUNC_EQUAL;
	stencil.stencil_front.pass_op = SG_STENCILOP_ZERO;
	stencil.stencil_back = stencil.stencil_front;
	batch_set_depth_stencil_state(batch, stencil);
	for (int i = 0; i < reflection_system->quads.count(); ++i) {
		batch_quad_t quad = reflection_system->quads[i];
		batch_push(batch, quad);
	}

	batch_flush(batch);

	// Draw reflection sprite slightly above itself.
	stencil.stencil_front.compare_func = SG_COMPAREFUNC_EQUAL;
	stencil.stencil_front.pass_op = SG_STENCILOP_KEEP;
	stencil.stencil_back = stencil.stencil_front;
	batch_set_depth_stencil_state(batch, stencil);
	batch_set_blend_defaults(batch);
	for (int i = 0; i < reflection_system->quads.count(); ++i) {
		batch_quad_t quad = reflection_system->quads[i];
		quad.transform.p += v2(0, 6);
		quad.alpha = 0.35f;
		batch_push(batch, quad);
	}

	batch_flush(batch);
	batch_set_depth_stencil_defaults(batch);
	reflection_system->quads.clear();
}
