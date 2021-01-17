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

namespace cute
{

struct coroutine_t
{
	float dt = 0;
	bool waiting = false;
	float seconds_left = 0;
	mco_coro* mco;
	coroutine_fn* fn = NULL;
	void* udata = NULL;
};

static void s_co_fn(mco_coro* mco)
{
	coroutine_t* co = (coroutine_t*)mco_get_user_data(mco);
	co->fn(co);
}

coroutine_t* coroutine_make(coroutine_fn* fn, void* udata)
{
	mco_desc desc = mco_desc_init(s_co_fn, 0);
	coroutine_t* co = CUTE_NEW(coroutine_t, NULL);
	desc.user_data = (void*)co;
	mco_coro* mco;
	mco_result res = mco_create(&mco, &desc);
	CUTE_ASSERT(res == MCO_SUCCESS);
	co->mco = mco;
	co->fn = fn;
	co->udata = udata;
	return co;
}

void coroutine_destroy(coroutine_t* co)
{
	mco_state state = mco_status(co->mco);
	CUTE_ASSERT(state == MCO_DEAD);
	mco_result res = mco_destroy(co->mco);
	CUTE_ASSERT(res == MCO_SUCCESS);
	CUTE_FREE(co, NULL);
}

error_t coroutine_resume(coroutine_t* co, float dt)
{
	co->dt = dt;

	if (co->waiting) {
		co->seconds_left -= dt;
		if (co->seconds_left <= 0) {
			co->waiting = false;
			co->seconds_left = 0;
		} else {
			return error_success();
		}
	}

	mco_result res = mco_resume(co->mco);
	if (res != MCO_SUCCESS) {
		return error_failure(mco_result_description(res));
	} else {
		return error_success();
	}
}

float coroutine_yield(coroutine_t* co, error_t* err)
{
	mco_result res = mco_yield(co->mco);
	if (err) {
		if (res != MCO_SUCCESS) {
			*err = error_failure(mco_result_description(res));
		} else {
			*err = error_success();
		}
	}
	return co->dt;
}

error_t coroutine_wait(coroutine_t* co, float seconds)
{
	co->waiting = true;
	co->seconds_left = seconds;
	error_t err;
	coroutine_yield(co, &err);
	return err;
}

coroutine_state_t coroutine_state(coroutine_t* co)
{
	mco_state s = mco_status(co->mco);
	switch (s) {
	default:
		case MCO_DEAD: return COROUTINE_STATE_DEAD;
		case MCO_NORMAL: return COROUTINE_STATE_ACTIVE_BUT_RESUMED_ANOTHER;
		case MCO_RUNNING: return COROUTINE_STATE_ACTIVE_AND_RUNNING;
		case MCO_SUSPENDED: return COROUTINE_STATE_SUSPENDED;
	}
}

void* coroutine_get_udata(coroutine_t* co)
{
	return co->udata;
}

error_t coroutine_push(coroutine_t* co, const void* data, size_t size)
{
	mco_result res = mco_push(co->mco, data, size);
	if (res != MCO_SUCCESS) {
		return error_failure(mco_result_description(res));
	} else {
		return error_success();
	}
}

error_t coroutine_pop(coroutine_t* co, void* data, size_t size)
{
	mco_result res = mco_pop(co->mco, data, size);
	if (res != MCO_SUCCESS) {
		return error_failure(mco_result_description(res));
	} else {
		return error_success();
	}
}

size_t coroutine_bytes_pushed(coroutine_t* co)
{
	return mco_get_bytes_stored(co->mco);
}

size_t coroutine_space_remaining(coroutine_t* co)
{
	return mco_get_storage_size(co->mco) - mco_get_bytes_stored(co->mco);
}

coroutine_t* coroutine_currently_running()
{
	mco_coro* mco = mco_running();
	coroutine_t* co = (coroutine_t*)mco_get_user_data(mco);
	return co;
}

}
