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

#ifndef CUTE_H
#define CUTE_H

#include <cute_defines.h>

namespace cute
{

struct cute_t;

enum cute_options_t
{
	CUTE_OPTIONS_NO_GFX               = 0x001,
	CUTE_OPTIONS_NO_AUDIO             = 0x002,
	CUTE_OPTIONS_GFX_GL               = 0x004,
	CUTE_OPTIONS_GFX_GLES             = 0x008,
	CUTE_OPTIONS_GFX_D3D9             = 0x010,
	CUTE_OPTIONS_FULLSCREEN           = 0x020,
	CUTE_OPTIONS_RESIZABLE            = 0x040,
	CUTE_OPTIONS_WINDOW_POS_CENTERED  = 0x080,
};

extern CUTE_API cute_t* CUTE_CALL cute_make(const char* window_title, int x, int y, int w, int h, uint32_t options = 0, void* user_allocator_context = NULL);
extern CUTE_API void CUTE_CALL cute_destroy(cute_t* cute);

extern CUTE_API int CUTE_CALL is_running(cute_t* cute);
extern CUTE_API void CUTE_CALL stop_running(cute_t* cute);

extern CUTE_API void CUTE_CALL cute_update(cute_t* cute, float dt);
extern CUTE_API float CUTE_CALL calc_dt();

}

#include <cute_error.h>
#include <cute_clipboard.h>

#endif // CUTE_H
