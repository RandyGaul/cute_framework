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

#define MINICORO_IMPL
#include <edubart/minicoro.h>

namespace cute
{

coroutine_t* coroutine_make(coroutine_fn* fn, void* udata, void* mem_ctx)
{
	mco_desc desc = mco_desc_init((void (*)(mco_coro*))fn, 0);
	mco_coro* co;
	mco_result res = mco_create(&co, &desc);
	CUTE_ASSERT(res == MCO_SUCCESS);
	return (coroutine_t*)co;
}

void coroutine_destroy(coroutine_t* _co)
{
	mco_coro* co = (mco_coro*)_co;
	mco_state state = mco_status(co);
	CUTE_ASSERT(state == MCO_DEAD);
	mco_result res = mco_destroy((mco_coro*)co);
	assert(res == MCO_SUCCESS);
}

error_t coroutine_resume(coroutine_t* _co)
{
	mco_coro* co = (mco_coro*)_co;
	mco_result res = mco_resume(co);
	if (res != MCO_SUCCESS) {
		return error_failure(mco_result_description(res));
	} else {
		return error_success();
	}
}

error_t coroutine_yield(coroutine_t* _co)
{
	mco_coro* co = (mco_coro*)_co;
	mco_result res = mco_yield(co);
	if (res != MCO_SUCCESS) {
		return error_failure(mco_result_description(res));
	}
	else {
		return error_success();
	}
}

coroutine_state_t coroutine_state(coroutine_t* _co)
{
	mco_coro* co = (mco_coro*)_co;
	mco_state s = mco_status(co);
	switch (s) {
	default:
		case MCO_DEAD: return COROUTINE_STATE_DEAD;
		case MCO_NORMAL: return COROUTINE_STATE_ACTIVE_BUT_RESUMED_ANOTHER;
		case MCO_RUNNING: return COROUTINE_STATE_ACTIVE_AND_RUNNING;
		case MCO_SUSPENDED: return COROUTINE_STATE_SUSPENDED;
	}
}

void* coroutine_get_udata(coroutine_t* _co)
{
	mco_coro* co = (mco_coro*)_co;
	return mco_get_user_data(co);
}

error_t coroutine_push(coroutine_t* _co, const void* data, size_t size)
{
	mco_coro* co = (mco_coro*)_co;
	mco_result res = mco_push(co, data, size);
	if (res != MCO_SUCCESS) {
		return error_failure(mco_result_description(res));
	}
	else {
		return error_success();
	}
}

error_t coroutine_pop(coroutine_t* _co, void* data, size_t size)
{
	mco_coro* co = (mco_coro*)_co;
	mco_result res = mco_pop(co, data, size);
	if (res != MCO_SUCCESS) {
		return error_failure(mco_result_description(res));
	}
	else {
		return error_success();
	}
}

size_t coroutine_bytes_pushed(coroutine_t* _co)
{
	mco_coro* co = (mco_coro*)_co;
	return mco_get_bytes_stored(co);
}

size_t coroutine_space_remaining(coroutine_t* _co)
{
	mco_coro* co = (mco_coro*)_co;
	return mco_get_storage_size(co) - mco_get_bytes_stored(co);
}

coroutine_t* coroutine_currently_running()
{
	return (coroutine_t*)mco_running();
}

}
