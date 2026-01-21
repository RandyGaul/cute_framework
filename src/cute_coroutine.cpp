/*
	Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

#include <cute_coroutine.h>
#include <cute_c_runtime.h>

#include <internal/cute_alloc_internal.h>
#include <internal/cute_app_internal.h>

#define MINICORO_IMPL
#define MCO_USE_VMEM_ALLOCATOR
#include <edubart/minicoro.h>

struct CF_CoroutineInternal
{
	mco_coro* mco;
	CF_CoroutineFn* fn = NULL;
	void* udata = NULL;
};

static void s_co_fn(mco_coro* mco)
{
	CF_CoroutineInternal* co = (CF_CoroutineInternal*)mco_get_user_data(mco);
	CF_Coroutine result;
	result.id = (uint64_t)co;
	co->fn(result);
}

CF_Coroutine cf_make_coroutine(CF_CoroutineFn* fn, int stack_size, void* udata)
{
	mco_desc desc = mco_desc_init(s_co_fn, (size_t)stack_size);
	CF_CoroutineInternal* co = CF_NEW(CF_CoroutineInternal);
	desc.user_data = (void*)co;
	mco_coro* mco;
	mco_result res = mco_create(&mco, &desc);
	CF_ASSERT(res == MCO_SUCCESS);
	co->mco = mco;
	co->fn = fn;
	co->udata = udata;
	CF_Coroutine result;
	result.id = (uint64_t)co;
	return result;
}

void cf_destroy_coroutine(CF_Coroutine co_handle)
{
	CF_CoroutineInternal* co = (CF_CoroutineInternal*)co_handle.id;
	if (!co) return;
	mco_state state = mco_status(co->mco);
	CF_ASSERT(state == MCO_DEAD || state == MCO_SUSPENDED);
	mco_result res = mco_destroy(co->mco);
	CF_ASSERT(res == MCO_SUCCESS);
	CF_FREE(co);
}

CF_Result cf_coroutine_resume(CF_Coroutine co_handle)
{
	CF_CoroutineInternal* co = (CF_CoroutineInternal*)co_handle.id;
	mco_result res = mco_resume(co->mco);
	if (res != MCO_SUCCESS) {
		return cf_result_error(mco_result_description(res));
	} else {
		return cf_result_success();
	}
}

CF_Result cf_coroutine_yield(CF_Coroutine co_handle)
{
	CF_CoroutineInternal* co = (CF_CoroutineInternal*)co_handle.id;
	mco_result res = mco_yield(co->mco);
	if (res != MCO_SUCCESS) {
		return cf_result_error(mco_result_description(res));
	} else {
		return cf_result_success();
	}
}

CF_CoroutineState cf_coroutine_state(CF_Coroutine co_handle)
{
	CF_CoroutineInternal* co = (CF_CoroutineInternal*)co_handle.id;
	mco_state s = mco_status(co->mco);
	switch (s) {
	default:
		case MCO_DEAD: return CF_COROUTINE_STATE_DEAD;
		case MCO_NORMAL: return CF_COROUTINE_STATE_ACTIVE_BUT_RESUMED_ANOTHER;
		case MCO_RUNNING: return CF_COROUTINE_STATE_ACTIVE_AND_RUNNING;
		case MCO_SUSPENDED: return CF_COROUTINE_STATE_SUSPENDED;
	}
}

void* cf_coroutine_get_udata(CF_Coroutine co_handle)
{
	CF_CoroutineInternal* co = (CF_CoroutineInternal*)co_handle.id;
	return co->udata;
}

CF_Result cf_coroutine_push(CF_Coroutine co_handle, const void* data, size_t size)
{
	CF_CoroutineInternal* co = (CF_CoroutineInternal*)co_handle.id;
	mco_result res = mco_push(co->mco, data, size);
	if (res != MCO_SUCCESS) {
		return cf_result_error(mco_result_description(res));
	} else {
		return cf_result_success();
	}
}

CF_Result cf_coroutine_pop(CF_Coroutine co_handle, void* data, size_t size)
{
	CF_CoroutineInternal* co = (CF_CoroutineInternal*)co_handle.id;
	mco_result res = mco_pop(co->mco, data, size);
	if (res != MCO_SUCCESS) {
		return cf_result_error(mco_result_description(res));
	} else {
		return cf_result_success();
	}
}

size_t cf_coroutine_bytes_pushed(CF_Coroutine co_handle)
{
	CF_CoroutineInternal* co = (CF_CoroutineInternal*)co_handle.id;
	return mco_get_bytes_stored(co->mco);
}

size_t cf_coroutine_space_remaining(CF_Coroutine co_handle)
{
	CF_CoroutineInternal* co = (CF_CoroutineInternal*)co_handle.id;
	return mco_get_storage_size(co->mco) - mco_get_bytes_stored(co->mco);
}

CF_Coroutine cf_coroutine_currently_running()
{
	mco_coro* mco = mco_running();
	CF_CoroutineInternal* co = (CF_CoroutineInternal*)mco_get_user_data(mco);
	CF_Coroutine result;
	result.id = (uint64_t)co;
	return result;
}
