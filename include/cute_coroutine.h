/*
	Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

#ifndef CF_COROUTINE_H
#define CF_COROUTINE_H

#include "cute_defines.h"
#include "cute_result.h"

//--------------------------------------------------------------------------------------------------
// C API

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

/**
 * @struct   CF_Coroutine
 * @category coroutine
 * @brief    An opaque handle representing a coroutine.
 * @remarks  A coroutine is a function that can be paused and resumed. Coroutines are often used as an alternative
 *           way to implement state machines, or create gameplay-helper tools for control code logic/flow. Once a coroutine
 *           is created with `cf_make_coroutine` call `cf_coroutine_resume` to start running it. At any moment the coroutine
 *           can pause itself with `cf_coroutine_yield`. Then, later, someone else can call `cf_coroutine_resume`. The coroutine
 *           will then continue running just after the last call to `cf_coroutine_yield`. This makes a coroutine great for
 *           preserving state between yield/resume calls, for example to perform some complex action over multiple frames.
 * @related  CF_Coroutine CF_CoroutineFn CF_CoroutineState cf_make_coroutine cf_destroy_coroutine cf_coroutine_state_to_string cf_coroutine_resume cf_coroutine_yield cf_coroutine_state cf_coroutine_get_udata cf_coroutine_push cf_coroutine_pop cf_coroutine_bytes_pushed cf_coroutine_space_remaining cf_coroutine_currently_running
 */
typedef struct CF_Coroutine { uint64_t id; } CF_Coroutine;
// @end

/**
 * @function CF_CoroutineFn
 * @category coroutine
 * @brief    Entry point for a coroutine to start.
 * @param    co            The coroutine.
 * @related  CF_Coroutine CF_CoroutineFn CF_CoroutineState cf_make_coroutine cf_destroy_coroutine cf_coroutine_state_to_string cf_coroutine_resume cf_coroutine_yield cf_coroutine_state cf_coroutine_get_udata cf_coroutine_push cf_coroutine_pop cf_coroutine_bytes_pushed cf_coroutine_space_remaining cf_coroutine_currently_running
 */
typedef void (CF_CoroutineFn)(CF_Coroutine co);

/**
 * @function cf_make_coroutine
 * @category coroutine
 * @brief    Sets up the entry point for a coroutine to start.
 * @param    fn            The entry point (function) the coroutine runs.
 * @param    stack_size    The size of the coroutine's stack to call functions and make local variables, the default is about 57344 bytes.
 * @param    udata         Can be `NULL`. Gets handed back to you when `cf_coroutine_get_udata` is called.
 * @remarks  The coroutine starts in a `COROUTINE_STATE_SUSPENDED`, and won't run until `cf_coroutine_resume` is first called. Free up the
 *           coroutine with `cf_destroy_coroutine` when done. See `CF_Coroutine` for some more details. **IMPORTANT NOTE**: You should beef
 *           up the stack_size to 1 or 2 MB (you may use e.g. `CF_MB * 2`) if you wish to call into APIs such as DirectX. A variety of APIs
 *           and libraries out there have very deep or complex call stacks -- so the default size may cause stack overflows in such cases.
 * @related  CF_Coroutine CF_CoroutineFn CF_CoroutineState cf_make_coroutine cf_destroy_coroutine cf_coroutine_state_to_string cf_coroutine_resume cf_coroutine_yield cf_coroutine_state cf_coroutine_get_udata cf_coroutine_push cf_coroutine_pop cf_coroutine_bytes_pushed cf_coroutine_space_remaining cf_coroutine_currently_running
 */
CF_API CF_Coroutine CF_CALL cf_make_coroutine(CF_CoroutineFn* fn, int stack_size, void* udata);

/**
 * @function cf_destroy_coroutine
 * @category coroutine
 * @brief    Destroys a coroutine created by `cf_make_coroutine`.
 * @param    co            The coroutine.
 * @remarks  All objects on the coroutine's stack will get automically cleaned up.
 * @related  CF_Coroutine CF_CoroutineFn CF_CoroutineState cf_make_coroutine cf_destroy_coroutine cf_coroutine_state_to_string cf_coroutine_resume cf_coroutine_yield cf_coroutine_state cf_coroutine_get_udata cf_coroutine_push cf_coroutine_pop cf_coroutine_bytes_pushed cf_coroutine_space_remaining cf_coroutine_currently_running
 */
CF_API void CF_CALL cf_destroy_coroutine(CF_Coroutine co);

/**
 * @enum     CF_CoroutineState
 * @category coroutine
 * @brief    The states of a coroutine.
 * @related  CF_Coroutine CF_CoroutineFn CF_CoroutineState cf_make_coroutine cf_destroy_coroutine cf_coroutine_state_to_string cf_coroutine_resume cf_coroutine_yield cf_coroutine_state cf_coroutine_get_udata cf_coroutine_push cf_coroutine_pop cf_coroutine_bytes_pushed cf_coroutine_space_remaining cf_coroutine_currently_running
 * @related  cf_make_app cf_destroy_app
 */
#define CF_COROUTINE_STATE_DEFS \
	/* @entry The coroutine has stopped running entirely. */                                           \
	CF_ENUM(COROUTINE_STATE_DEAD, 0)                                                                   \
	/* @entry The coroutine is not dead, and currently running. */                                     \
	CF_ENUM(COROUTINE_STATE_ACTIVE_AND_RUNNING, 1)                                                     \
	/* @entry The coroutine is not dead, but has called `cf_coroutine_resume` on another coroutine. */ \
	CF_ENUM(COROUTINE_STATE_ACTIVE_BUT_RESUMED_ANOTHER, 2)                                             \
	/* @entry The coroutine is not dead, but is not active since it called `cf_coroutine_yield`. */    \
	CF_ENUM(COROUTINE_STATE_SUSPENDED, 3)                                                              \
// @end

typedef enum CF_CoroutineState
{
	#define CF_ENUM(K, V) CF_##K = V,
	CF_COROUTINE_STATE_DEFS
	#undef CF_ENUM
} CF_CoroutineState;

/**
 * @function cf_coroutine_state_to_string
 * @category coroutine
 * @brief    Converts a `CF_CoroutineState` to c-string.
 * @param    type          The state.
 * @related  CF_Coroutine CF_CoroutineFn CF_CoroutineState cf_make_coroutine cf_destroy_coroutine cf_coroutine_state_to_string cf_coroutine_resume cf_coroutine_yield cf_coroutine_state cf_coroutine_get_udata cf_coroutine_push cf_coroutine_pop cf_coroutine_bytes_pushed cf_coroutine_space_remaining cf_coroutine_currently_running
 */
CF_INLINE const char* cf_coroutine_state_to_string(CF_CoroutineState type)
{
	switch (type) {
	#define CF_ENUM(K, V) case CF_##K: return CF_STRINGIZE(CF_##K);
	CF_COROUTINE_STATE_DEFS
	#undef CF_ENUM
	default: return NULL;
	}
}

/**
 * @function cf_coroutine_resume
 * @category coroutine
 * @brief    Resumes the coroutine.
 * @param    co            The coroutine.
 * return    Returns info on any errors as `CF_Result`.
 * @remarks  Coroutines are functions that can be paused with `cf_coroutine_yield` and resumed with `cf_coroutine_resume`. See `CF_Coroutine`
 *           for more details.
 * @related  CF_Coroutine CF_CoroutineFn CF_CoroutineState cf_make_coroutine cf_destroy_coroutine cf_coroutine_state_to_string cf_coroutine_resume cf_coroutine_yield cf_coroutine_state cf_coroutine_get_udata cf_coroutine_push cf_coroutine_pop cf_coroutine_bytes_pushed cf_coroutine_space_remaining cf_coroutine_currently_running
 */
CF_API CF_Result CF_CALL cf_coroutine_resume(CF_Coroutine co);

/**
 * @function cf_coroutine_yield
 * @category coroutine
 * @brief    Yields the coroutine.
 * @param    co            The coroutine.
 * return    Returns info on any errors as `CF_Result`.
 * @remarks  Coroutines are functions that can be paused with `cf_coroutine_yield` and resumed with `cf_coroutine_resume`. See `CF_Coroutine`
 *           for more details.
 * @related  CF_Coroutine CF_CoroutineFn CF_CoroutineState cf_make_coroutine cf_destroy_coroutine cf_coroutine_state_to_string cf_coroutine_resume cf_coroutine_yield cf_coroutine_state cf_coroutine_get_udata cf_coroutine_push cf_coroutine_pop cf_coroutine_bytes_pushed cf_coroutine_space_remaining cf_coroutine_currently_running
 */
CF_API CF_Result CF_CALL cf_coroutine_yield(CF_Coroutine co);

/**
 * @function cf_coroutine_state
 * @category coroutine
 * @brief    Returns the current state of the coroutine.
 * @param    co            The coroutine.
 * @related  CF_Coroutine CF_CoroutineFn CF_CoroutineState cf_make_coroutine cf_destroy_coroutine cf_coroutine_state_to_string cf_coroutine_resume cf_coroutine_yield cf_coroutine_state cf_coroutine_get_udata cf_coroutine_push cf_coroutine_pop cf_coroutine_bytes_pushed cf_coroutine_space_remaining cf_coroutine_currently_running
 */
CF_API CF_CoroutineState CF_CALL cf_coroutine_state(CF_Coroutine co);

/**
 * @function cf_coroutine_get_udata
 * @category coroutine
 * @brief    Returns the `void* udata` from `cf_coroutine_create`.
 * @param    co            The coroutine.
 * @related  CF_Coroutine CF_CoroutineFn CF_CoroutineState cf_make_coroutine cf_destroy_coroutine cf_coroutine_state_to_string cf_coroutine_resume cf_coroutine_yield cf_coroutine_state cf_coroutine_get_udata cf_coroutine_push cf_coroutine_pop cf_coroutine_bytes_pushed cf_coroutine_space_remaining cf_coroutine_currently_running
 */
CF_API void* CF_CALL cf_coroutine_get_udata(CF_Coroutine co);

/**
 * @function cf_coroutine_push
 * @category coroutine
 * @brief    Pushes some bytes onto the coroutine's storage.
 * @param    co            The coroutine.
 * @param    data          The data to push into the storage.
 * @param    size          The size of `data` in bytes.
 * return    Returns info on any errors as `CF_Result`.
 * @remarks  Each coroutine has an internal storage of 1k (1024) bytes. The purpose is to allow a formal communication/parameter passing
 *           in/out of coroutines via FIFO ordering. These storage are totally optional, and here merely for convenience.
 * @related  CF_Coroutine CF_CoroutineFn CF_CoroutineState cf_make_coroutine cf_destroy_coroutine cf_coroutine_state_to_string cf_coroutine_resume cf_coroutine_yield cf_coroutine_state cf_coroutine_get_udata cf_coroutine_push cf_coroutine_pop cf_coroutine_bytes_pushed cf_coroutine_space_remaining cf_coroutine_currently_running
 */
CF_API CF_Result CF_CALL cf_coroutine_push(CF_Coroutine co, const void* data, size_t size);

/**
 * @function cf_coroutine_pop
 * @category coroutine
 * @brief    Pops some bytes off of the coroutine's storage.
 * @param    co            The coroutine.
 * @param    data          A buffer to store pop'd storage data within.
 * @param    size          The size of `data` in bytes.
 * return    Returns info on any errors as `CF_Result`.
 * @remarks  Each coroutine has an internal storage of 1k (1024) bytes. The purpose is to allow a formal communication/parameter passing
 *           in/out of coroutines via FIFO ordering. These storage are totally optional, and here merely for convenience.
 * @related  CF_Coroutine CF_CoroutineFn CF_CoroutineState cf_make_coroutine cf_destroy_coroutine cf_coroutine_state_to_string cf_coroutine_resume cf_coroutine_yield cf_coroutine_state cf_coroutine_get_udata cf_coroutine_push cf_coroutine_pop cf_coroutine_bytes_pushed cf_coroutine_space_remaining cf_coroutine_currently_running
 */
CF_API CF_Result CF_CALL cf_coroutine_pop(CF_Coroutine co, void* data, size_t size);

/**
 * @function cf_coroutine_bytes_pushed
 * @category coroutine
 * @brief    Returns the number of bytes currently used in the coroutine's storage.
 * @param    co            The coroutine.
 * @remarks  Each coroutine has an internal storage of 1k (1024) bytes. The purpose is to allow a formal communication/parameter passing
 *           in/out of coroutines via FIFO ordering. These storage are totally optional, and here merely for convenience.
 * @related  CF_Coroutine CF_CoroutineFn CF_CoroutineState cf_make_coroutine cf_destroy_coroutine cf_coroutine_state_to_string cf_coroutine_resume cf_coroutine_yield cf_coroutine_state cf_coroutine_get_udata cf_coroutine_push cf_coroutine_pop cf_coroutine_bytes_pushed cf_coroutine_space_remaining cf_coroutine_currently_running
 */
CF_API size_t CF_CALL cf_coroutine_bytes_pushed(CF_Coroutine co);

/**
 * @function cf_coroutine_space_remaining
 * @category coroutine
 * @brief    Returns the number of bytes currently available for use in the coroutine's storage.
 * @param    co            The coroutine.
 * @remarks  Each coroutine has an internal storage of 1k (1024) bytes. The purpose is to allow a formal communication/parameter passing
 *           in/out of coroutines via FIFO ordering. These storage are totally optional, and here merely for convenience.
 * @related  CF_Coroutine CF_CoroutineFn CF_CoroutineState cf_make_coroutine cf_destroy_coroutine cf_coroutine_state_to_string cf_coroutine_resume cf_coroutine_yield cf_coroutine_state cf_coroutine_get_udata cf_coroutine_push cf_coroutine_pop cf_coroutine_bytes_pushed cf_coroutine_space_remaining cf_coroutine_currently_running
 */
CF_API size_t CF_CALL cf_coroutine_space_remaining(CF_Coroutine co);

/**
 * @function cf_coroutine_currently_running
 * @category coroutine
 * @brief    Returns the opaque pointer to the coroutine currently running.
 * @remarks  Each coroutine has ther `co` pointer handed to them as the only parameter in `CF_CoroutineFn`, so you likely
 *           already have access to the coroutine pointer. However, this function is made available here for convenience.
 *           For example, your coroutines may call into other functions -- instead of passing around a `co` pointer everywhere,
 *           your helper functions can simply fetch the `CF_Coroutine` pointer themselves on an as-needed basis by calling
 *           this function.
 * @related  CF_Coroutine CF_CoroutineFn CF_CoroutineState cf_make_coroutine cf_destroy_coroutine cf_coroutine_state_to_string cf_coroutine_resume cf_coroutine_yield cf_coroutine_state cf_coroutine_get_udata cf_coroutine_push cf_coroutine_pop cf_coroutine_bytes_pushed cf_coroutine_space_remaining cf_coroutine_currently_running
 */
CF_API CF_Coroutine CF_CALL cf_coroutine_currently_running();

#ifdef __cplusplus
}
#endif // __cplusplus

//--------------------------------------------------------------------------------------------------
// C++ API

#ifdef CF_CPP

namespace Cute
{

using Coroutine = CF_Coroutine;
using CoroutineFn = CF_CoroutineFn;

using CoroutineState = CF_CoroutineState;
#define CF_ENUM(K, V) CF_INLINE constexpr CoroutineState K = CF_##K;
CF_COROUTINE_STATE_DEFS
#undef CF_ENUM

CF_INLINE const char* to_string(CoroutineState type)
{
	switch (type) {
	#define CF_ENUM(K, V) case CF_##K: return #K;
	CF_COROUTINE_STATE_DEFS
	#undef CF_ENUM
	default: return NULL;
	}
}

CF_INLINE Coroutine make_coroutine(CoroutineFn* fn, int stack_size = 0, void* udata = NULL) { return cf_make_coroutine(fn, stack_size, udata); }
CF_INLINE void destroy_coroutine(Coroutine co) { cf_destroy_coroutine(co); }
	 
CF_INLINE Result coroutine_resume(Coroutine co) { return cf_coroutine_resume(co); }
CF_INLINE Result coroutine_yield(Coroutine co) { return cf_coroutine_yield(co); }
CF_INLINE CoroutineState coroutine_state(Coroutine co) { return cf_coroutine_state(co); }
CF_INLINE void* coroutine_get_udata(Coroutine co) { return cf_coroutine_get_udata(co); }
	 
CF_INLINE Result coroutine_push(Coroutine co, const void* data, size_t size) { return cf_coroutine_push(co, data, size); }
CF_INLINE Result coroutine_pop(Coroutine co, void* data, size_t size) { return cf_coroutine_pop(co, data, size); }
CF_INLINE size_t coroutine_bytes_pushed(Coroutine co) { return cf_coroutine_bytes_pushed(co); }
CF_INLINE size_t coroutine_space_remaining(Coroutine co) { return cf_coroutine_space_remaining(co); }
	 
CF_INLINE Coroutine coroutine_currently_running() { return cf_coroutine_currently_running(); }

}

#endif // CF_CPP

#endif // CF_COROUTINE_H
