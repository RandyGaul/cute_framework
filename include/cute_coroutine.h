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

#ifndef CUTE_COROUTINE_H
#define CUTE_COROUTINE_H

#include "cute_defines.h"
#include "cute_error.h"

namespace cute
{
	struct coroutine_t;
	typedef void (coroutine_fn)(coroutine_t* co);

	CUTE_API coroutine_t* CUTE_CALL coroutine_make(coroutine_fn* fn, int stack_size = 0, void* udata = NULL);
	CUTE_API void CUTE_CALL coroutine_destroy(coroutine_t* co);

	enum coroutine_state_t
	{
		COROUTINE_STATE_DEAD,
		COROUTINE_STATE_ACTIVE_AND_RUNNING,
		COROUTINE_STATE_ACTIVE_BUT_RESUMED_ANOTHER,
		COROUTINE_STATE_SUSPENDED,
	};

	CUTE_API error_t CUTE_CALL coroutine_resume(coroutine_t* co, float dt = 0);
	CUTE_API float CUTE_CALL coroutine_yield(coroutine_t* co, error_t* err = NULL);
	CUTE_API error_t CUTE_CALL coroutine_wait(coroutine_t* co, float seconds);
	CUTE_API coroutine_state_t CUTE_CALL coroutine_state(coroutine_t* co);
	CUTE_API void* CUTE_CALL coroutine_get_udata(coroutine_t* co);

	CUTE_API error_t CUTE_CALL coroutine_push(coroutine_t* co, const void* data, size_t size);
	CUTE_API error_t CUTE_CALL coroutine_pop(coroutine_t* co, void* data, size_t size);
	CUTE_API size_t CUTE_CALL coroutine_bytes_pushed(coroutine_t* co);
	CUTE_API size_t CUTE_CALL coroutine_space_remaining(coroutine_t* co);

	CUTE_API coroutine_t* CUTE_CALL coroutine_currently_running();
}

#endif // CUTE_COROUTINE_H
