/*
	Cute Framework
	Copyright (C) 2019 Randy Gaul https://randygaul.net

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

#include <cute_coroutine.h>
#include <cute_c_runtime.h>

#include <internal/cute_app_internal.h>

#define MINICORO_IMPL
#include <edubart/minicoro.h>

struct cf_coroutine_t
{
	float dt = 0;
	bool waiting = false;
	float seconds_left = 0;
	mco_coro* mco;
	cf_coroutine_fn* fn = NULL;
	void* udata = NULL;
};

static void s_co_fn(mco_coro* mco)
{
	cf_coroutine_t* co = (cf_coroutine_t*)mco_get_user_data(mco);
	co->fn(co);
}

cf_coroutine_t* cf_make_coroutine(cf_coroutine_fn* fn, int stack_size, void* udata)
{
	mco_desc desc = mco_desc_init(s_co_fn, (size_t)stack_size);
	cf_coroutine_t* co = CUTE_NEW(cf_coroutine_t);
	desc.user_data = (void*)co;
	mco_coro* mco;
	mco_result res = mco_create(&mco, &desc);
	CUTE_ASSERT(res == MCO_SUCCESS);
	co->mco = mco;
	co->fn = fn;
	co->udata = udata;
	return co;
}

void cf_destroy_coroutine(cf_coroutine_t* co)
{
	mco_state state = mco_status(co->mco);
	CUTE_ASSERT(state == MCO_DEAD || state == MCO_SUSPENDED);
	mco_result res = mco_destroy(co->mco);
	CUTE_ASSERT(res == MCO_SUCCESS);
	CUTE_FREE(co);
}

cf_result_t cf_coroutine_resume(cf_coroutine_t* co, float dt)
{
	co->dt = dt;

	if (co->waiting) {
		co->seconds_left -= dt;
		if (co->seconds_left <= 0) {
			co->waiting = false;
			co->seconds_left = 0;
		} else {
			return cf_result_success();
		}
	}

	mco_result res = mco_resume(co->mco);
	if (res != MCO_SUCCESS) {
		return cf_result_error(mco_result_description(res));
	} else {
		return cf_result_success();
	}
}

float cf_coroutine_yield(cf_coroutine_t* co, cf_result_t* err)
{
	mco_result res = mco_yield(co->mco);
	if (err) {
		if (res != MCO_SUCCESS) {
			*err = cf_result_error(mco_result_description(res));
		} else {
			*err = cf_result_success();
		}
	}
	return co->dt;
}

cf_result_t cf_coroutine_wait(cf_coroutine_t* co, float seconds)
{
	co->waiting = true;
	co->seconds_left = seconds;
	cf_result_t err;
	cf_coroutine_yield(co, &err);
	return err;
}

cf_coroutine_state_t cf_coroutine_state(cf_coroutine_t* co)
{
	mco_state s = mco_status(co->mco);
	switch (s) {
	default:
		case MCO_DEAD: return CF_COROUTINE_STATE_DEAD;
		case MCO_NORMAL: return CF_COROUTINE_STATE_ACTIVE_BUT_RESUMED_ANOTHER;
		case MCO_RUNNING: return CF_COROUTINE_STATE_ACTIVE_AND_RUNNING;
		case MCO_SUSPENDED: return CF_COROUTINE_STATE_SUSPENDED;
	}
}

void* cf_coroutine_get_udata(cf_coroutine_t* co)
{
	return co->udata;
}

cf_result_t cf_coroutine_push(cf_coroutine_t* co, const void* data, size_t size)
{
	mco_result res = mco_push(co->mco, data, size);
	if (res != MCO_SUCCESS) {
		return cf_result_error(mco_result_description(res));
	} else {
		return cf_result_success();
	}
}

cf_result_t cf_coroutine_pop(cf_coroutine_t* co, void* data, size_t size)
{
	mco_result res = mco_pop(co->mco, data, size);
	if (res != MCO_SUCCESS) {
		return cf_result_error(mco_result_description(res));
	} else {
		return cf_result_success();
	}
}

size_t cf_coroutine_bytes_pushed(cf_coroutine_t* co)
{
	return mco_get_bytes_stored(co->mco);
}

size_t cf_coroutine_space_remaining(cf_coroutine_t* co)
{
	return mco_get_storage_size(co->mco) - mco_get_bytes_stored(co->mco);
}

cf_coroutine_t* cf_coroutine_currently_running()
{
	mco_coro* mco = mco_running();
	cf_coroutine_t* co = (cf_coroutine_t*)mco_get_user_data(mco);
	return co;
}
