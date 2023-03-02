/*
	Cute Framework
	Copyright (C) 2023 Randy Gaul https://randygaul.github.io/

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

struct CF_Coroutine
{
	mco_coro* mco;
	CF_CoroutineFn* fn = NULL;
	void* udata = NULL;
};

static void s_co_fn(mco_coro* mco)
{
	CF_Coroutine* co = (CF_Coroutine*)mco_get_user_data(mco);
	co->fn(co);
}

CF_Coroutine* cf_make_coroutine(CF_CoroutineFn* fn, int stack_size, void* udata)
{
	mco_desc desc = mco_desc_init(s_co_fn, (size_t)stack_size);
	CF_Coroutine* co = CUTE_NEW(CF_Coroutine);
	desc.user_data = (void*)co;
	mco_coro* mco;
	mco_result res = mco_create(&mco, &desc);
	CUTE_ASSERT(res == MCO_SUCCESS);
	co->mco = mco;
	co->fn = fn;
	co->udata = udata;
	return co;
}

void cf_destroy_coroutine(CF_Coroutine* co)
{
	mco_state state = mco_status(co->mco);
	CUTE_ASSERT(state == MCO_DEAD || state == MCO_SUSPENDED);
	mco_result res = mco_destroy(co->mco);
	CUTE_ASSERT(res == MCO_SUCCESS);
	CUTE_FREE(co);
}

CF_Result cf_coroutine_resume(CF_Coroutine* co)
{
	mco_result res = mco_resume(co->mco);
	if (res != MCO_SUCCESS) {
		return cf_result_error(mco_result_description(res));
	} else {
		return cf_result_success();
	}
}

CF_Result cf_coroutine_yield(CF_Coroutine* co)
{
	mco_result res = mco_yield(co->mco);
	if (res != MCO_SUCCESS) {
		return cf_result_error(mco_result_description(res));
	} else {
		return cf_result_success();
	}
}

CF_CoroutineState cf_coroutine_state(CF_Coroutine* co)
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

void* cf_coroutine_get_udata(CF_Coroutine* co)
{
	return co->udata;
}

CF_Result cf_coroutine_push(CF_Coroutine* co, const void* data, size_t size)
{
	mco_result res = mco_push(co->mco, data, size);
	if (res != MCO_SUCCESS) {
		return cf_result_error(mco_result_description(res));
	} else {
		return cf_result_success();
	}
}

CF_Result cf_coroutine_pop(CF_Coroutine* co, void* data, size_t size)
{
	mco_result res = mco_pop(co->mco, data, size);
	if (res != MCO_SUCCESS) {
		return cf_result_error(mco_result_description(res));
	} else {
		return cf_result_success();
	}
}

size_t cf_coroutine_bytes_pushed(CF_Coroutine* co)
{
	return mco_get_bytes_stored(co->mco);
}

size_t cf_coroutine_space_remaining(CF_Coroutine* co)
{
	return mco_get_storage_size(co->mco) - mco_get_bytes_stored(co->mco);
}

CF_Coroutine* cf_coroutine_currently_running()
{
	mco_coro* mco = mco_running();
	CF_Coroutine* co = (CF_Coroutine*)mco_get_user_data(mco);
	return co;
}
